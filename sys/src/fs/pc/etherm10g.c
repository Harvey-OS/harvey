/*
 * myricom 10 Gb ethernet (file server driver)
 * © 2007 erik quanstrom, coraid
 */
#include "all.h"
#include "io.h"
#include "../ip/ip.h"
#include "etherif.h"
#include "portfns.h"
#include "mem.h"

#undef	MB
#define	K	* 1024
#define	MB	* 1024 K

#define	dprint(...)	if(debug) print(__VA_ARGS__)
#define	pcicapdbg(...)
#define malign(n)	ialloc(n, 4 K)
#define PCIWADDR(x)	PADDR(x)+0

extern ulong upamalloc(ulong, int, int);

#include "etherm10g2k.i"
#include "etherm10g4k.i"

static int 	debug		= 0;
static char	Etimeout[]	= "timeout";
static char	Enomem[]	= "no memory";
static char	Enonexist[]	= "controller lost";
static char	Ebadarg[]	= "bad argument";

enum {
	Epromsz	= 256,
	Maxslots= 1024,
	Align	= 4096,
	Maxmtu	= 9000,
	Noconf	= 0xffffffff,

	Fwoffset= 1 MB,
	Cmdoff	= 0xf80000,	/* command port offset */
	Fwsubmt	= 0xfc0000,	/* firmware submission command port offset */
	Rdmaoff	= 0xfc01c0,	/* rdma command port offset */
};

enum {
	CZero,
	Creset,
	Cversion,

	CSintrqdma,	/* issue these before Cetherup */
	CSbigsz,	/* in bytes bigsize = 2^n */
	CSsmallsz,

	CGsendoff,
	CGsmallrxoff,
	CGbigrxoff,
	CGirqackoff,
	CGirqdeassoff,
	CGsendrgsz,
	CGrxrgsz,

	CSintrqsz,	/* 2^n */
	Cetherup,	/* above parameters + mtu/mac addr must be set first. */
	Cetherdn,

	CSmtu,		/* below may be issued live */
	CGcoaloff,	/* in µs */
	CSstatsrate,	/* in µs */
	CSstatsdma,

	Cpromisc,
	Cnopromisc,
	CSmac,

	Cenablefc,
	Cdisablefc,

	Cdmatest,	/* address in d[0-1], d[2]=length */

	Cenableallmc,
	Cdisableallmc,

	CSjoinmc,
	CSleavemc,
	Cleaveallmc,

	CSstatsdma2,	/* adds (unused) multicast stats */
};

typedef union {
	uint	i[2];
	uchar	c[8];
} Cmd;

typedef struct {
	u16int	cksum;
	u16int	len;
} Slot;

enum {
	SFsmall	= 1,
	SFfirst	= 2,
	SFalign	= 4,
	SFnotso	= 16,
};

typedef struct {
	u32int	high;
	u32int	low;
	u16int	hdroff;
	u16int	len;
	uchar	pad;
	uchar	nrdma;
	uchar	chkoff;
	uchar	flags;
} Send;

typedef struct {
	QLock;
	Send	*lanai;		/* tx ring (cksum+len in lanai memory) */
	Send	*host;		/* tx ring (data in our memory) */
	Msgbuf	**bring;
//	uchar	*wcfifo;	/* what the heck is a w/c fifo? */
	int	size;		/* of buffers in the z8's memory */
	u32int	segsz;
	uint	n;		/* rxslots */
	uint	m;		/* mask; rxslots must be a power of two */
	uint	i;		/* number of segments (not frames) queued */
	uint	cnt;		/* number of segments sent by the card */

	ulong	npkt;
	vlong	nbytes;
} Tx;

typedef struct {
	Lock;
	Msgbuf	*head;
	uint	size;		/* buffer size of each block */
	uint	n;		/* n free buffers */
	uint	cnt;
} Bpool;

Bpool	smpool 	= { .size = 128, };
Bpool	bgpool	= { .size = Maxmtu, };

typedef struct {
	Bpool	*pool;		/* free buffers */
	u32int	*lanai;		/* rx ring; we have no permanent host shadow */
	Msgbuf	**host;		/* called "info" in myricom driver */
//	uchar	*wcfifo;	/* cmd submission fifo */
	uint	m;
	uint	n;		/* rxslots */
	uint	i;
	uint	cnt;		/* number of buffers allocated (lifetime) */
	uint	allocfail;
} Rx;

/* dma mapped.  internet network byte order. */
typedef struct {
	uchar	txcnt[4];
	uchar	linkstat[4];
	uchar	dlink[4];
	uchar	derror[4];
	uchar	drunt[4];
	uchar	doverrun[4];
	uchar	dnosm[4];
	uchar	dnobg[4];
	uchar	nrdma[4];
	uchar	txstopped;
	uchar	down;
	uchar	updated;
	uchar	valid;
} Stats;

enum {
	Detached,
	Attached,
	Runed,
};

typedef struct {
	Slot 	*entry;
	uintptr	busaddr;
	uint	m;
	uint	n;
	uint	i;
} Done;

typedef struct Ctlr Ctlr;
typedef struct Ctlr {
	QLock;
	int	state;
	int	kprocs;
	uintptr	port;
	Pcidev*	pcidev;
	Ctlr*	next;
	int	active;
	int	id;		/* do we need this? */

	uchar	ra[Easize];

	int	ramsz;
	uchar	*ram;

	u32int	*irqack;
	u32int	*irqdeass;
	u32int	*coal;

	char	eprom[Epromsz];
	ulong	serial;		/* unit serial number */

	QLock	cmdl;
	Cmd	*cmd;		/* address of command return */
	uintptr	cprt;		/* bus address of command */

	uintptr	boot;		/* boot address */

	Done	done;
	Tx	tx;
	Rx	sm;
	Rx	bg;
	Stats	*stats;
	uintptr	statsprt;

	Rendez	rxrendez;
	Rendez	txrendez;

	int	msi;
	u32int	linkstat;
	u32int	nrdma;
} Ctlr;

enum {
	PciCapPMG	 = 0x01,	/* power management */
	PciCapAGP	 = 0x02,
	PciCapVPD	 = 0x03,	/* vital product data */
	PciCapSID	 = 0x04,	/* slot id */
	PciCapMSI	 = 0x05,
	PciCapCHS	 = 0x06,	/* compact pci hot swap */
	PciCapPCIX	 = 0x07,
	PciCapHTC	 = 0x08,	/* hypertransport irq conf */
	PciCapVND	 = 0x09,	/* vendor specific information */
	PciCapHSW	 = 0x0C,	/* hot swap */
	PciCapPCIe	 = 0x10,
	PciCapMSIX	 = 0x11,
};

enum {
	PcieAERC = 1,
	PcieVC,
	PcieSNC,
	PciePBC,
};

enum {
	AercCCR	= 0x18,			/* control register */
};

enum {
	PcieCTL	= 8,
	PcieLCR	= 12,
	PcieMRD	= 0x7000,		/* maximum read size */
};

int
pcicap(Pcidev *p, int cap)
{
	int i, c, off;

	pcicapdbg("pcicap: %x:%d\n", p->vid, p->did);
	off = 0x34;			/* 0x14 for cardbus */
	for(i = 48; i--;){
		pcicapdbg("\t" "loop %x\n", off);
		off = pcicfgr8(p, off);
		pcicapdbg("\t" "pcicfgr8 %x\n", off);
		if(off < 0x40)
			break;
		off &= ~3;
		c = pcicfgr8(p, off);
		pcicapdbg("\t" "pcicfgr8 %x\n", c);
		if(c == 0xff)
			break;
		if(c == cap)
			return off;
		off++;
	}
	return 0;
}

/*
 * this function doesn't work because pcicgr32 doesn't have access
 * to the pcie extended configuration space.
 */
int
pciecap(Pcidev *p, int cap)
{
	uint off, i;

	off = 0x100;
	while(((i = pcicfgr32(p, off)) & 0xffff) != cap){
		off = i >> 20;
		print("pciecap offset = %ud\n",  off);
		if(off < 0x100 || off >= 4 K - 1)
			return 0;
	}
	print("pciecap found = %ud\n", off);
	return off;
}

static int
setpcie(Pcidev *p)
{
	int off;

	/* set 4k writes */
	off = pcicap(p, PciCapPCIe);
	if(off < 64)
		return -1;
	off += PcieCTL;
	pcicfgw16(p, off, (pcicfgr16(p, off) & ~PcieMRD) | 5<<12);
	return 0;
}

static int
whichfw(Pcidev *p)
{
	char *s;
	int i, off, lanes, ecrc;
	u32int cap;

	/* check the number of configured lanes. */
	off = pcicap(p, PciCapPCIe);
	if(off < 64)
		return -1;
	off += PcieLCR;
	cap = pcicfgr16(p, off);
	lanes = (cap>>4)&0x3f;

	/* check AERC register.  we need it on. */
	off = pciecap(p, PcieAERC);
	print("%d offset\n", off);
	cap = 0;
	if(off != 0){
		off += AercCCR;
		cap = pcicfgr32(p, off);
		print("%ud cap\n", cap);
	}
	ecrc = (cap>>4)&0xf;
	/* if we don't like the aerc, kick it here. */

	print("m10g %d lanes; ecrc=%d; ", lanes, ecrc);
	if(s = getconf("myriforce")){
		i = strtoul(s, 0, 0);
		if(i != 4 K || i != 2 K)
			i = 2 K;
		print("fw=%d [forced]\n", i);
		return i;
	}
	if(lanes <= 4){
		print("fw = 4096 [lanes]\n");
		return 4 K;
	}
	if(ecrc & 10){
		print("fw = 4096 [ecrc set]\n");
		return 4K;
	}
	print("fw = 4096 [default]\n");
	return 4 K;
}

static int
parseeprom(Ctlr *c)
{
	int i, j, k, l, bits;
	char *s;

	dprint("m10g eprom:\n");
	s = c->eprom;
	bits = 3;
	for(i = 0; s[i] && i < Epromsz; i++){
		l = strlen(s+i);
		dprint("\t%s\n", s+i);
		if(strncmp(s+i, "MAC=", 4) == 0 && l == 4+12+5){
			bits ^= 1;
			j = i + 4;
			for(k = 0; k < 6; k++)
				c->ra[k] = strtoul(s+j+3*k, 0, 16);
		} else if(strncmp(s+i, "SN=", 3) == 0){
			bits ^= 2;
			c->serial = strtoul(s+i+3, 0, 0);
		}
		i += l;
	}
	if(bits)
		return -1;
	return 0;
}

u16int
pbit16(u16int i)
{
	u16int j;
	uchar *p;

	p = (uchar*)&j;
	p[1] = i;
	p[0] = i>>8;
	return j;
}

u16int
gbit16(uchar i[2])
{
	u16int j;

	j  = i[1];
	j |= i[0]<<8;
	return j;
}

u32int
pbit32(u32int i)
{
	u32int j;
	uchar *p;

	p = (uchar*)&j;
	p[3] = i;
	p[2] = i>>8;
	p[1] = i>>16;
	p[0] = i>>24;
	return j;
}

static u32int
gbit32(uchar i[4])
{
	u32int j;

	j  = i[3];
	j |= i[2]<<8;
	j |= i[1]<<16;
	j |= i[0]<<24;
	return j;
}

static void
prepcmd(uint *cmd, int i)
{
	while(i-- > 0)
		cmd[i] = pbit32(cmd[i]);
}

/*
 * the command looks like this (int 32bit integers)
 * cmd type
 * addr (low)
 * addr (high)
 * pad (used for dma testing)
 * response (high)
 * response (low)
 * 40 byte = 5 int pad.
 */

static Rendez cmdr;

static int
return0(void *)
{
	return 0;
}

u32int
cmd(Ctlr *c, int type, u32int data)
{
	u32int buf[16], i;
	Cmd *cmd;

	qlock(&c->cmdl);
	cmd = c->cmd;
	cmd->i[1] = Noconf;
	memset(buf, 0, sizeof buf);
	buf[0] = type;
	buf[1] = data;
	buf[5] = c->cprt;
	prepcmd(buf, 6);
	coherence();
	memmove(c->ram+Cmdoff, buf, sizeof buf);

	for(i = 0; i < 15; i++){
		if(cmd->i[1] != Noconf){
			i = gbit32(cmd->c);
			qunlock(&c->cmdl);
			if(cmd->i[1] != 0)
				dprint("[%ux]", i);
			return i;
		}
		delay(1);
	}
	qunlock(&c->cmdl);
	panic("m10g: cmd timeout [%ux %ux] cmd=%d\n",
		cmd->i[0], cmd->i[1], type);
	return ~0;				/* silence! */
}

u32int
maccmd(Ctlr *c, int type, uchar *m)
{
	u32int buf[16], i;
	Cmd *cmd;

	qlock(&c->cmdl);
	cmd = c->cmd;
	cmd->i[1] = Noconf;
	memset(buf, 0, sizeof buf);
	buf[0] = type;
	buf[1] = m[0]<<24 | m[1]<<16 | m[2]<<8 | m[3];
	buf[2] = m[4]<< 8 | m[5];
	buf[5] = c->cprt;
	prepcmd(buf, 6);
	coherence();
	memmove(c->ram+Cmdoff, buf, sizeof buf);

	for(i = 0; i < 15; i++){
		if(cmd->i[1] != Noconf){
			i = gbit32(cmd->c);
			qunlock(&c->cmdl);
			if(cmd->i[1] != 0)
				dprint("[%ux]", i);
			return i;
		}
		delay(1);
	}
	qunlock(&c->cmdl);
	print("m10g: maccmd timeout [%ux %ux] cmd=%d\n",
		cmd->i[0], cmd->i[1], type);
	panic(Etimeout);
	return ~0;				/* silence! */
}

/* remove this garbage after testing */
enum {
	DMAread	= 0x10000,
	DMAwrite= 0x1,
};

u32int
dmatestcmd(Ctlr *c, int type, u32int addr, int len)
{
	u32int buf[16], i;

	memset(buf, 0, sizeof buf);
	memset(c->cmd, Noconf, sizeof *c->cmd);
	buf[0] = Cdmatest;
	buf[1] = addr;
	buf[3] = len*type;
	buf[5] = c->cprt;
	prepcmd(buf, 6);
	coherence();
	memmove(c->ram+Cmdoff, buf, sizeof buf);

	for(i = 0; i < 15; i++){
		if(c->cmd->i[1] != Noconf){
			i = gbit32(c->cmd->c);
			if(i == 0)
				return 0;
			return i;
		}
		delay(5);
	}
	panic(Etimeout);
	return ~0;			/* silence! */
}

u32int
rdmacmd(Ctlr *c, int on)
{
	u32int buf[16], i;

	memset(buf, 0, sizeof buf);
	c->cmd->i[0] = 0;
	coherence();
	buf[1] = c->cprt;
	buf[2] = Noconf;
	buf[4] = c->cprt;
	buf[5] = on;
	prepcmd(buf, 6);
	memmove(c->ram+Rdmaoff, buf, sizeof buf);

	for(i = 0; i < 20; i++){
		if(c->cmd->i[0] == Noconf)
			return gbit32(c->cmd->c);
		delay(1);
	}
	panic(Etimeout);
	return ~0;			/* silence! */
}

static int
loadfw(Ctlr *c, int *align)
{
	uint *f, *s, sz;
	int i;

	if((*align = whichfw(c->pcidev)) == 4 K){
		f = (u32int*)fw4k;
		sz = sizeof fw4k;
	} else {
		f = (u32int*)fw2k;
		sz = sizeof fw2k;
	}

	s = (u32int*)(c->ram + Fwoffset);
	for(i = 0; i < sz / 4; i++)
		s[i] = f[i];
	return sz & ~3;
}

static int
bootfw(Ctlr *c)
{
	int i, sz, align;
	uint buf[16];
	Cmd* cmd;

	if((sz = loadfw(c, &align)) == 0)
		return 0;
	dprint("bootfw %d bytes ... ", sz);
	cmd = c->cmd;

	memset(buf, 0, sizeof buf);
	c->cmd->i[0] = 0;
	coherence();
	buf[0] = 0;		/* upper 32 bits of dma target address */
	buf[1] = c->cprt;	/* lower */
	buf[2] = Noconf;	/* writeback */
	buf[3] = Fwoffset+8,
	buf[4] = sz-8;
	buf[5] = 8;
	buf[6] = 0;
	prepcmd(buf, 7);
	coherence();
	memmove(c->ram + Fwsubmt, buf, sizeof buf);

	for(i = 0; i < 20; i++){
		if(cmd->i[0] == Noconf)
			break;
		delay(1);
	}
	dprint("[%ux %ux]", gbit32(cmd->c), gbit32(cmd->c+4));
	if(i == 20){
		print("m10g: cannot load fw\n");
		return -1;
	}
	dprint("\n");
	c->tx.segsz = align;
	return 0;
}

int
kickthebaby(Pcidev *p, Ctlr *c)
{
	/* don't kick the baby! */
	u32int code;

	pcicfgw8(p,  0x10+c->boot, 0x3);
	pcicfgw32(p, 0x18+c->boot, 0xfffffff0);
	code = pcicfgr32(p, 0x14+c->boot);

	dprint("reboot status = %ux\n", code);
	if(code != 0xfffffff0)
		return -1;
	return 0;
}

typedef struct {
	uchar	len[4];
	uchar	type[4];
	char	version[128];
	uchar	globals[4];
	uchar	ramsz[4];
	uchar	specs[4];
	uchar	specssz[4];
} Fwhdr;

enum {
	Tmx	= 0x4d582020,
	Tpcie	= 0x70636965,
	Teth	= 0x45544820,
	Tmcp0	= 0x4d435030,
};

char*
fwtype(u32int type)
{
	switch(type){
	case Tmx:
		return "mx";
	case Tpcie:
		return "PCIe";
	case Teth:
		return "eth";
	case Tmcp0:
		return "mcp0";
	}
	return "*GOK*";
}

int
chkfw(Ctlr *c)
{
	uintptr off;
	Fwhdr *h;
	u32int type;

	off = gbit32(c->ram+0x3c);
	dprint("firmware %ulx\n", off);
	if((off&3) || off + sizeof *h > c->ramsz){
		print("!m10g: bad firmware %ulx\n", off);
		return -1;
	}
	h = (Fwhdr*)(c->ram + off);
	type = gbit32(h->type);
	dprint("\t" "type	%s\n", fwtype(type));
	dprint("\t" "vers	%s\n", h->version);
	dprint("\t" "ramsz	%ux\n", gbit32(h->ramsz));
	if(type != Teth){
		print("!m10g: bad card type %s\n", fwtype(type));
		return -1;
	}

	return bootfw(c) || rdmacmd(c, 0);
}

static int
reset(Ether *, Ctlr *c)
{
	u32int i, sz;

	chkfw(c);
	cmd(c, Creset, 0);

	cmd(c, CSintrqsz, c->done.n * sizeof *c->done.entry);
	cmd(c, CSintrqdma, c->done.busaddr);
	c->irqack =   (u32int*)(c->ram + cmd(c, CGirqackoff, 0));
	/* required only if we're not doing msi? */
	c->irqdeass = (u32int*)(c->ram + cmd(c, CGirqdeassoff, 0));
	/* this is the driver default, why fiddle with this? */
	c->coal = (u32int*)(c->ram + cmd(c, CGcoaloff, 0));
	*c->coal = pbit32(25);

	dprint("dma stats:\n");
	rdmacmd(c, 1);
	sz = c->tx.segsz;
	i = dmatestcmd(c, DMAread, c->done.busaddr, sz);
	print("\t" "read: %ud MB/s\n", ((i>>16)*sz*2)/(i&0xffff));
	i = dmatestcmd(c, DMAwrite, c->done.busaddr, sz);
	print("\t" "write: %ud MB/s\n", ((i>>16)*sz*2)/(i&0xffff));
	i = dmatestcmd(c, DMAwrite|DMAread, c->done.busaddr, sz);
	print("\t" "r/w: %ud MB/s\n", ((i>>16)*sz*2*2)/(i&0xffff));
	memset(c->done.entry, 0, c->done.n * sizeof *c->done.entry);

	maccmd(c, CSmac, c->ra);
//	cmd(c, Cnopromisc, 0);
	cmd(c, Cenablefc, 0);
	cmd(c, CSmtu, Maxmtu);
	dprint("CSmtu %d...\n", Maxmtu);

	return 0;
}

static int
setmem(Pcidev *p, Ctlr *c)
{
	u32int i, raddr;
	Done *d;
	void *mem;

	c->tx.segsz = 2048;
	c->ramsz = 2 MB - (2*48 K + 32 K) - 0x100;
	if(c->ramsz > p->mem[0].size)
		return -1;

	raddr = p->mem[0].bar & ~0x0F;
	mem = (void*)upamalloc(raddr, p->mem[0].size, 0);
	if(mem == nil){
		print("m10g: can't map %8.8lux\n", p->mem[0].bar);
		return -1;
	}
	dprint("%ux <- vmap(mem[0].size = %ux)\n", raddr, p->mem[0].size);
	c->port = raddr;
	c->ram = mem;
	c->cmd = malign(sizeof *c->cmd);
	c->cprt = PCIWADDR(c->cmd);

	d = &c->done;
	d->n = Maxslots;
	d->m = d->n - 1;
	i = d->n * sizeof *d->entry;
	d->entry = malign(i);
	memset(d->entry, 0, i);
	d->busaddr = PCIWADDR(d->entry);

	c->stats = malign(sizeof *c->stats);
	memset(c->stats, 0, sizeof *c->stats);
	c->statsprt = PCIWADDR(c->stats);

	memmove(c->eprom, c->ram + c->ramsz - Epromsz, Epromsz-2);
	return setpcie(p) || parseeprom(c);
}

static Rx*
whichrx(Ctlr *c, int sz)
{
	if(sz <= smpool.size)
		return &c->sm;
	return &c->bg;
}

static Msgbuf*
m10balloc(Rx* rx)
{
	Msgbuf *m;

	ilock(rx->pool);
	if((m = rx->pool->head) != nil){
		rx->pool->head = m->next;
		m->next = nil;
		rx->pool->n--;
	}
	iunlock(rx->pool);
	return m;
}

static void
smbfree(Msgbuf *m)
{
	Bpool *p;

	m->data = (uchar*)PGROUND((uintptr)m->xdata);
	m->count = 0;
	p = &smpool;
	ilock(p);
	m->next = p->head;
	p->head = m;
	p->n++;
	p->cnt++;
	iunlock(p);
}

static void
bgbfree(Msgbuf *m)
{
	Bpool *p;

	m->data = (uchar*)PGROUND((uintptr)m->xdata);
	m->count = 0;
	p = &bgpool;
	ilock(p);
	m->next = p->head;
	p->head = m;
	p->n++;
	p->cnt++;
	iunlock(p);
}

static void
replenish(Rx *rx)
{
	u32int buf[16], i, idx, e;
	Bpool *p;
	Msgbuf *m;

	p = rx->pool;
	if(p->n < 8)
		return;
	memset(buf, 0, sizeof buf);
	e = (rx->i - rx->cnt) & ~7;
	e += rx->n;
	while(p->n >= 8 && e){
		idx = rx->cnt & rx->m;
		for(i = 0; i < 8; i++){
			m = m10balloc(rx);
			buf[i*2+1] = pbit32(PCIWADDR(m->data));
			rx->host[idx+i] = m;
		}
		memmove(rx->lanai + 2*idx, buf, sizeof buf);
		coherence();
		rx->cnt += 8;
		e -= 8;
	}
	if(e && p->n > 7+1)
		print("should panic? pool->n = %d\n", p->n);
}

/*
 * future:
 * if (c->mtrr >= 0) {
 *	c->tx.wcfifo = c->ram+0x200000;
 *	c->sm.wcfifo = c->ram+0x300000;
 *	c->bg.wcfifo = c->ram+0x340000;
 * }
 */

static int
nextpow(int j)
{
	int i;

	for(i = 0; j > (1<<i); i++)
		;
	return 1 << i;
}

static void*
emalign(int sz)
{
	void *v;

	v = malign(sz);
	if(v == nil)
		panic(Enomem);
	memset(v, 0, sz);
	return v;
}

#define Mbeth10gbebg Mbeth4
#define Mbeth10gbesm Mbeth4

static void
open0(Ether *, Ctlr *c)
{
	Msgbuf *m;
	int i, sz, entries;

	entries = cmd(c, CGsendrgsz, 0) / sizeof *c->tx.lanai;
	c->tx.lanai = (Send*)(c->ram + cmd(c, CGsendoff, 0));
	c->tx.host =  emalign(entries * sizeof *c->tx.host);
	c->tx.bring = emalign(entries * sizeof *c->tx.bring);
	c->tx.n = entries;
	c->tx.m = entries-1;

	entries = cmd(c, CGrxrgsz, 0)/8;
	c->sm.pool = &smpool;
	cmd(c, CSsmallsz, c->sm.pool->size);
	c->sm.lanai = (u32int*)(c->ram + cmd(c, CGsmallrxoff, 0));
	c->sm.n = entries;
	c->sm.m = entries-1;
	c->sm.host = emalign(entries * sizeof *c->sm.host);

	c->bg.pool = &bgpool;
	c->bg.pool->size = nextpow(2 + Maxmtu);	/* 2-byte alignment pad */
	cmd(c, CSbigsz, c->bg.pool->size);
	c->bg.lanai = (u32int*)(c->ram + cmd(c, CGbigrxoff, 0));
	c->bg.n = entries;
	c->bg.m = entries-1;
	c->bg.host = emalign(entries * sizeof *c->bg.host);

	sz = c->sm.pool->size + BY2PG;
	for(i = 0; i < c->sm.n; i++){
		m = mballoc(sz, 0, Mbeth10gbesm);
		m->free = smbfree;
		mbfree(m);
	}
	/* allocate our own buffers.  leak the ones we're given. */
//	sz = c->bg.pool->size + BY2PG;
	for(i = 0; i < c->bg.n; i++){
//		m = mballoc(sz, 0, Mbeth10gbebg);
		m = mballoc( 1, 0, Mbeth10gbebg);
		m->xdata = emalign(c->bg.pool->size);
		m->free = bgbfree;
		mbfree(m);
	}

	cmd(c, CSstatsdma, c->statsprt);
	c->linkstat = ~0;
	c->nrdma = 15;

	cmd(c, Cetherup, 0);
}

static Msgbuf*
nextbuf(Ctlr *c)
{
	uint i;
	u16int l;
	Done *d;
	Msgbuf *m;
	Rx *rx;
	Slot *s;

	d = &c->done;
	s = d->entry;
	i = d->i & d->m;
	l = s[i].len;
	if(l == 0)
		return nil;
	*(u32int*)(s+i) = 0;
	d->i++;
	l = gbit16((uchar*)&l);
	rx = whichrx(c, l);
	if(rx->i >= rx->cnt){
		print("m10g: overrun\n");
		return nil;
	}
	i = rx->i & rx->m;
	m = rx->host[i];
	rx->host[i] = 0;
	if(m == 0){
		print("m10g: error rx to no block.  memory is hosed.\n");
		return nil;
	}
	rx->i++;
	m->data += 2;
	m->count = l;
	return m;
}

static int
rxcansleep(void *v)
{
	Ctlr *c;
	Slot *s;
	Done *d;

	c = v;
	d = &c->done;
	s = c->done.entry;
	if(s[d->i & d->m].len != 0)
		return -1;
	c->irqack[0] = pbit32(3);
	return 0;
}

static void
m10rx(void)
{
	Ether *e;
	Ctlr *c;
	Msgbuf *m;

	e = u->arg;
	c = e->ctlr;
	for(;;){
		replenish(&c->sm);
		replenish(&c->bg);
		sleep(&c->rxrendez, rxcansleep, c);
		while(m = nextbuf(c))
			etheriq(e, m);
	}
}

void
txcleanup(Tx *tx, u32int n)
{
	Msgbuf *mb;
	uint j, l, m;

	if(tx->npkt == n)
		return;
	l = 0;
	m = tx->m;
	/* if tx->cnt == tx->i, yet tx->npkt == n-1 we just */
	/* caught ourselves and myricom card updating. */
	for(;; tx->cnt++){
		j = tx->cnt & tx->m;
		if(mb = tx->bring[j]){
			tx->bring[j] = 0;
			tx->nbytes += mb->count;
			mbfree(mb);
			if(++tx->npkt == n)
				return;
		}
		if(tx->cnt == tx->i)
			return;
		if(l++ == m){
			print("tx ovrun: %ud %uld\n", n, tx->npkt);
			return;
		}
	}
}

static int
txcansleep(void *v)
{
	Ctlr *c;

	c = v;
	if(c->tx.cnt != c->tx.i && c->tx.npkt != gbit32(c->stats->txcnt))
		return -1;
	return 0;
}

void
txproc(void)
{
	Ether *e;
	Ctlr *c;
	Tx *tx;

	e = u->arg;
	c = e->ctlr;
	tx = &c->tx;
	for(;;){
 		sleep(&c->txrendez, txcansleep, c);
		txcleanup(tx, gbit32(c->stats->txcnt));
	}
}

void
submittx(Tx *tx, int n)
{
	Send *l, *h;
	int i0, i, m;

	m = tx->m;
	i0 = tx->i & m;
	l = tx->lanai;
	h = tx->host;
	for(i = n-1; i >= 0; i--)
		memmove(l+(i + i0 & m), h+(i + i0 & m), sizeof *h);
	tx->i += n;
//	coherence();
}

int
nsegments(Msgbuf *m, int segsz)
{
	uintptr bus, end, slen, len;
	int i;

	bus = PCIWADDR(m->data);
	i = 0;
	for(len = m->count; len; len -= slen){
		end = bus + segsz & ~(segsz-1);
		slen = end-bus;
		if(slen > len)
			slen = len;
		bus += slen;
		i++;
	}
	return i;
}

static void
m10gtransmit(Ether *e)
{
	u16int slen;
	u32int i, cnt, rdma, nseg, count, end, bus, len, segsz;
	uchar flags;
	Ctlr *c;
	Msgbuf *m;
	Send *s, *s0, *s0m8;
	Tx *tx;

	c = e->ctlr;
	tx = &c->tx;
	segsz = tx->segsz;

	qlock(tx);
	count = 0;
	s =    tx->host + (tx->i & tx->m);
	cnt = tx->cnt;
	s0 =   tx->host + (cnt & tx->m);
	s0m8 = tx->host + (cnt - 8 & tx->m);
	i = tx->i;
	for(; s >= s0 || s < s0m8; i += nseg){
		if((m = etheroq(e)) == nil)
			break;
		flags = SFfirst|SFnotso;
		if((len = m->count) < 1520)
			flags |= SFsmall;
		rdma = nseg = nsegments(m, segsz);
		bus = PCIWADDR(m->data);
		for(; len; len -= slen){
			end = bus + segsz & ~(segsz-1);
			slen = end-bus;
			if(slen > len)
				slen = len;
			s->low = pbit32(bus);
			s->len = pbit16(slen);
			s->nrdma = rdma;
			s->flags = flags;

			bus += slen;
			if(++s ==  tx->host + tx->n)
				s = tx->host;
			count++;
			flags &= ~SFfirst;
			rdma = 1;
		}
		tx->bring[i + nseg - 1 & tx->m] = m;
		submittx(tx, count);
		count = 0;
		cnt = tx->cnt;
		s0 =   tx->host + (cnt & tx->m);
		s0m8 = tx->host + (cnt - 8 & tx->m);
	}
	qunlock(tx);
}

static void
checkstats(Ether *, Ctlr *c, Stats *s)
{
	u32int i;

	if(s->updated == 0)
		return;

	i = gbit32(s->linkstat);
	if(c->linkstat != i)
		if(c->linkstat = i)
			dprint("m10g: link up\n");
		else
			dprint("m10g: link down\n");
	i = gbit32(s->nrdma);
	if(i != c->nrdma){
		dprint("m10g: rdma timeout %d\n", i);
		c->nrdma = i;
	}
}

static void
waitintx(Ctlr *c)
{
	int i;

	for(i = 0; i < 1024*1024; i++){
		if(c->stats->valid == 0)
			break;
		coherence();
	}
}

static void
m10ginterrupt(Ureg *, void *v)
{
	Ether *e;
	Ctlr *c;

	e = v;
	c = e->ctlr;

	if(c->state != Runed || c->stats->valid == 0)	/* not ready for us? */
		return;

	if(c->stats->valid & 1)
		wakeup(&c->rxrendez);
	if(gbit32(c->stats->txcnt) != c->tx.npkt)
		wakeup(&c->txrendez);
	if(c->msi == 0)
		*c->irqdeass = 0;
	else
		c->stats->valid = 0;
	waitintx(c);
	checkstats(e, c, c->stats);
	c->irqack[1] = pbit32(3);
}

static void
m10gattach(Ether *e)
{
	Ctlr *c;
	char name[12];

	dprint("m10gattach\n");

	qlock(e->ctlr);
	c = e->ctlr;
	if(c->state != Detached){
		qunlock(c);
		return;
	}
	reset(e, c);
	c->state = Attached;
	open0(e, c);
	if(c->kprocs == 0){
		c->kprocs++;
		snprint(name, sizeof name, "#l%drxproc", e->ctlrno);
		userinit(m10rx, e, name);
		snprint(name, sizeof name, "#l%dtxproc", e->ctlrno);
		userinit(txproc, e, name);
	}
	c->state = Runed;
	qunlock(c);
}

static int
lstcount(Msgbuf *m)
{
	int i;

	i = 0;
	for(; m; m = m->next)
		i++;
	return i;
}

static char ifstatbuf[2 K];

static void
cifstat(Ctlr *c, int, char **)
{
	Stats s;

	/* no point in locking this because this is done via dma. */
	memmove(&s, c->stats, sizeof s);
	snprint(ifstatbuf, sizeof ifstatbuf,
		"txcnt = %ud\n"   "linkstat = %ud\n" 	"dlink = %ud\n"
		"derror = %ud\n"  "drunt = %ud\n" 	"doverrun = %ud\n"
		"dnosm = %ud\n"   "dnobg = %ud\n" 	"nrdma = %ud\n"
		"txstopped = %ud\n" "down = %ud\n" 	"updated = %ud\n"
		"valid = %ud\n\n"
		"tx pkt = %uld\n" "tx bytes = %lld\n"
		"tx cnt = %ud\n"  "tx n = %ud\n"	"tx i = %ud\n"
		"sm cnt = %ud\n"  "sm i = %ud\n"	"sm n = %ud\n"	"sm lst = %ud\n"
		"bg cnt = %ud\n"  "bg i = %ud\n"	"bg n = %ud\n"	"bg lst = %ud\n"
		"segsz = %ud\n"	  "coal = %d\n",
		gbit32(s.txcnt),  gbit32(s.linkstat),	gbit32(s.dlink),
		gbit32(s.derror), gbit32(s.drunt),	gbit32(s.doverrun),
		gbit32(s.dnosm),  gbit32(s.dnobg),	gbit32(s.nrdma),
		s.txstopped,  s.down, s.updated, s.valid,
		c->tx.npkt, c->tx.nbytes,
		c->tx.cnt, c->tx.n, c->tx.i,
		c->sm.cnt, c->sm.i, c->sm.pool->n, lstcount(c->sm.pool->head),
		c->bg.cnt, c->bg.i, c->bg.pool->n, lstcount(c->bg.pool->head),
		c->tx.segsz, gbit32((uchar*)c->coal));
	print("%s", ifstatbuf);
}

static void
cdebug(Ctlr *, int, char**)
{
	debug ^= 1;
	print("debug %d\n", debug);
}

static void
ccoal(Ctlr *c, int n, char **v)
{
	int i;

	if(n == 2){
		i = strtoul(*v, 0, 0);
		*c->coal = pbit32(i);
		coherence();
	}
	print("%d\n", gbit32((uchar*)c->coal));
}

static void
chelp(Ctlr*, int, char **)
{
	print("coal ctlr n -- get/set interrupt colesing delay\n");
	print("debug -- toggle debug (all ctlrs)\n");
	print("ifstat ctlr -- print statistics\n");
}

static Ctlr 	ctlrs[3];
static int	nctlr;

typedef struct {
	void	(*f)(Ctlr *, int, char**);
	char*	name;
	int	minarg;
	int	maxarg;
} Cmdtab;

static void
docmd(Cmdtab *t, int n, int c, char **v)
{
	int i;

	for(i = 0; i < n; i++)
		if(strcmp(*v, t[n].name) == 0)
			break;
	c--;
	v++;
	t += i;
	if(i == n)
		print("unknown subcommand\n");
	else if(c < t->minarg)
		print("too few args, need %d\n", t->minarg);
	else if(c > t->maxarg)
		print("too many args, max %d\n", t->maxarg);
	else {
		i = 0;
		if(t->minarg > 0){
			i = strtoul(*v++, 0, 0);
			c--;
		}
		if(i < 0 || i == nctlr)
			print("bad controller %d\n", i);
		else
			t->f(ctlrs+i, c, v);
	}
}

static Cmdtab ctab[] = {
	cdebug,	"debug",	0,	0,
	ccoal,	"coal",		1,	2,
	cifstat,"ifstat",	1,	1,
	chelp,	"help",		0,	0,
};

static void
m10gctl(int c, char **v)
{
	docmd(ctab, nelem(ctab), c, v);
}

static void
m10gpci(void)
{
	Pcidev *p;
	Ctlr *c;

	for(p = 0; p = pcimatch(p, 0x14c1, 0x0008); ){
		c = ctlrs + nctlr;
		memset(c, 0, sizeof *c);
		c->pcidev = p;
		c->id = p->did<<16 | p->vid;
		c->boot = pcicap(p, PciCapVND);
//		kickthebaby(p, c);
		pcisetbme(p);
		if(setmem(p, c) == -1){
			print("m10g failed\n");
			continue;
		}
		if(++nctlr == nelem(ctlrs))
			break;
	}
}

int
m10gpnp(Ether *e)
{
	Ctlr *c;
	static int once, cmd;

	if(once++ == 0)
		m10gpci();
	for(c = ctlrs; c < ctlrs + nctlr; c++)
		if(c->active)
			continue;
		else if(e->port == 0 || e->port == c->port)
			break;
	if(c == ctlrs + nctlr)
		return -1;
	c->active = 1;
	e->ctlr = c;
	e->port = c->port;
	e->irq = c->pcidev->intl;
	e->tbdf = c->pcidev->tbdf;
	e->mbps = 10000;
	memmove(e->ea, c->ra, Easize);

	e->attach = m10gattach;
	e->transmit = m10gtransmit;
	e->interrupt = m10ginterrupt;
	if(cmd++)
		cmd_install("myrictl", "tweak myri parameters", m10gctl);

	return 0;
}
