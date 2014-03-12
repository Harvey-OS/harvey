/*
 * myricom 10g-pcie-8a 10 Gb ethernet driver
 * © 2007 erik quanstrom, coraid
 *
 * the card is big endian.
 * we use uvlong rather than uintptr to hold addresses so that
 * we don't get "warning: stupid shift" on 32-bit architectures.
 *
 * appears to have massively-bloated buffers.
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "../port/error.h"
#include "../port/netif.h"

#include "../pc/etherif.h"

#ifndef KiB
#define KiB		1024u			/* Kibi 0x0000000000000400 */
#define MiB		1048576u		/* Mebi 0x0000000000100000 */
#endif /* KiB */

#define	dprint(...)	if(debug) print(__VA_ARGS__)
#define	pcicapdbg(...)
#define malign(n)	mallocalign((n), 4*KiB, 0, 0)

#include "etherm10g2k.i"
#include "etherm10g4k.i"

static int 	debug		= 0;
static char	Etimeout[]	= "timeout";

enum {
	Epromsz	= 256,
	Maxslots= 1024,		/* rcv descriptors; wasteful: only 9 needed */
	Align	= 4096,
	Maxmtu	= 9000,		/* jumbos; bad idea */
	Noconf	= 0xffffffff,

	Fwoffset= 1*MiB,
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

typedef ulong Slot;
typedef struct {
	ushort	cksum;
	ushort	len;
} Slotparts;

enum {
	SFsmall	= 1,
	SFfirst	= 2,
	SFalign	= 4,
	SFnotso	= 16,
};

typedef struct {
	ulong	high;
	ulong	low;
	ushort	hdroff;
	ushort	len;
	uchar	pad;
	uchar	nrdma;
	uchar	chkoff;
	uchar	flags;
} Send;

typedef struct {
	QLock;
	Send	*lanai;		/* tx ring (cksum+len in lanai memory) */
	Send	*host;		/* tx ring (data in our memory) */
	Block	**bring;
//	uchar	*wcfifo;	/* what the heck is a w/c fifo? */
	int	size;		/* of buffers in the z8's memory */
	ulong	segsz;
	uint	n;		/* rxslots */
	uint	m;		/* mask; rxslots must be a power of two */
	uint	i;		/* number of segments (not frames) queued */
	uint	cnt;		/* number of segments sent by the card */

	ulong	npkt;
	vlong	nbytes;
} Tx;

typedef struct {
	Lock;
	Block	*head;
	uint	size;		/* buffer size of each block */
	uint	n;		/* n free buffers */
	uint	cnt;
} Bpool;

static Bpool	smpool 	= { .size = 128, };
static Bpool	bgpool	= { .size = Maxmtu, };

typedef struct {
	Bpool	*pool;		/* free buffers */
	ulong	*lanai;		/* rx ring; we have no permanent host shadow */
	Block	**host;		/* called "info" in myricom driver */
//	uchar	*wcfifo;	/* cmd submission fifo */
	uint	m;
	uint	n;		/* rxslots */
	uint	i;
	uint	cnt;		/* number of buffers allocated (lifetime) */
	uint	allocfail;
} Rx;

/* dma mapped.  unix network byte order. */
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
	uvlong	busaddr;
	uint	m;
	uint	n;
	uint	i;
} Done;

typedef struct Ctlr Ctlr;
typedef struct Ctlr {
	QLock;
	int	state;
	int	kprocs;
	uvlong	port;
	Pcidev*	pcidev;
	Ctlr*	next;
	int	active;
	int	id;		/* do we need this? */

	uchar	ra[Eaddrlen];

	int	ramsz;
	uchar	*ram;

	ulong	*irqack;
	ulong	*irqdeass;
	ulong	*coal;

	char	eprom[Epromsz];
	ulong	serial;		/* unit serial number */

	QLock	cmdl;
	Cmd	*cmd;		/* address of command return */
	uvlong	cprt;		/* bus address of command */

	uvlong	boot;		/* boot address */

	Done	done;
	Tx	tx;
	Rx	sm;
	Rx	bg;
	Stats	*stats;
	uvlong	statsprt;

	Rendez	rxrendez;
	Rendez	txrendez;

	int	msi;
	ulong	linkstat;
	ulong	nrdma;
} Ctlr;

static Ctlr 	*ctlrs;

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
	AercCCR	= 0x18,		/* control register */
};

enum {
	PcieCTL	= 8,
	PcieLCR	= 12,
	PcieMRD	= 0x7000,	/* maximum read size */
};

static int
pcicap(Pcidev *p, int cap)
{
	int i, c, off;

	pcicapdbg("pcicap: %x:%d\n", p->vid, p->did);
	off = 0x34;			/* 0x14 for cardbus */
	for(i = 48; i--; ){
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
static int
pciecap(Pcidev *p, int cap)
{
	uint off, i;

	off = 0x100;
	while(((i = pcicfgr32(p, off)) & 0xffff) != cap){
		off = i >> 20;
		print("m10g: pciecap offset = %ud",  off);
		if(off < 0x100 || off >= 4*KiB - 1)
			return 0;
	}
	print("m10g: pciecap found = %ud",  off);
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
	ulong cap;

	/* check the number of configured lanes. */
	off = pcicap(p, PciCapPCIe);
	if(off < 64)
		return -1;
	off += PcieLCR;
	cap = pcicfgr16(p, off);
	lanes = (cap>>4) & 0x3f;

	/* check AERC register.  we need it on.  */
	off = pciecap(p, PcieAERC);
	print("; offset %d returned\n", off);
	cap = 0;
	if(off != 0){
		off += AercCCR;
		cap = pcicfgr32(p, off);
		print("m10g: %lud cap\n", cap);
	}
	ecrc = (cap>>4) & 0xf;
	/* if we don't like the aerc, kick it here. */

	print("m10g: %d lanes; ecrc=%d; ", lanes, ecrc);
	if(s = getconf("myriforce")){
		i = atoi(s);
		if(i != 4*KiB || i != 2*KiB)
			i = 2*KiB;
		print("fw = %d [forced]\n", i);
		return i;
	}
	if(lanes <= 4)
		print("fw = 4096 [lanes]\n");
	else if(ecrc & 10)
		print("fw = 4096 [ecrc set]\n");
	else
		print("fw = 4096 [default]\n");
	return 4*KiB;
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
		}else if(strncmp(s+i, "SN=", 3) == 0){
			bits ^= 2;
			c->serial = atoi(s+i+3);
		}
		i += l;
	}
	if(bits)
		return -1;
	return 0;
}

static ushort
pbit16(ushort i)
{
	ushort j;
	uchar *p;

	p = (uchar*)&j;
	p[1] = i;
	p[0] = i>>8;
	return j;
}

static ushort
gbit16(uchar i[2])
{
	ushort j;

	j  = i[1];
	j |= i[0]<<8;
	return j;
}

static ulong
pbit32(ulong i)
{
	ulong j;
	uchar *p;

	p = (uchar*)&j;
	p[3] = i;
	p[2] = i>>8;
	p[1] = i>>16;
	p[0] = i>>24;
	return j;
}

static ulong
gbit32(uchar i[4])
{
	ulong j;

	j  = i[3];
	j |= i[2]<<8;
	j |= i[1]<<16;
	j |= i[0]<<24;
	return j;
}

static void
prepcmd(ulong *cmd, int i)
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

ulong
cmd(Ctlr *c, int type, uvlong data)
{
	ulong buf[16], i;
	Cmd *cmd;

	qlock(&c->cmdl);
	cmd = c->cmd;
	cmd->i[1] = Noconf;
	memset(buf, 0, sizeof buf);
	buf[0] = type;
	buf[1] = data;
	buf[2] = data >> 32;
	buf[4] = c->cprt >> 32;
	buf[5] = c->cprt;
	prepcmd(buf, 6);
	coherence();
	memmove(c->ram + Cmdoff, buf, sizeof buf);

	if(waserror()){
		qunlock(&c->cmdl);
		nexterror();
	}
	for(i = 0; i < 15; i++){
		if(cmd->i[1] != Noconf){
			poperror();
			i = gbit32(cmd->c);
			qunlock(&c->cmdl);
			if(cmd->i[1] != 0)
				dprint("[%lux]", i);
			return i;	/* normal return */
		}
		tsleep(&up->sleep, return0, 0, 1);
	}
	iprint("m10g: cmd timeout [%ux %ux] cmd=%d\n",
		cmd->i[0], cmd->i[1], type);
	error(Etimeout);
	return ~0;			/* silence! */
}

ulong
maccmd(Ctlr *c, int type, uchar *m)
{
	ulong buf[16], i;
	Cmd *cmd;

	qlock(&c->cmdl);
	cmd = c->cmd;
	cmd->i[1] = Noconf;
	memset(buf, 0, sizeof buf);
	buf[0] = type;
	buf[1] = m[0]<<24 | m[1]<<16 | m[2]<<8 | m[3];
	buf[2] = m[4]<< 8 | m[5];
	buf[4] = c->cprt >> 32;
	buf[5] = c->cprt;
	prepcmd(buf, 6);
	coherence();
	memmove(c->ram + Cmdoff, buf, sizeof buf);

	if(waserror()){
		qunlock(&c->cmdl);
		nexterror();
	}
	for(i = 0; i < 15; i++){
		if(cmd->i[1] != Noconf){
			poperror();
			i = gbit32(cmd->c);
			qunlock(&c->cmdl);
			if(cmd->i[1] != 0)
				dprint("[%lux]", i);
			return i;	/* normal return */
		}
		tsleep(&up->sleep, return0, 0, 1);
	}
	iprint("m10g: maccmd timeout [%ux %ux] cmd=%d\n",
		cmd->i[0], cmd->i[1], type);
	error(Etimeout);
	return ~0;			/* silence! */
}

/* remove this garbage after testing */
enum {
	DMAread	= 0x10000,
	DMAwrite= 0x1,
};

ulong
dmatestcmd(Ctlr *c, int type, uvlong addr, int len)
{
	ulong buf[16], i;

	memset(buf, 0, sizeof buf);
	memset(c->cmd, Noconf, sizeof *c->cmd);
	buf[0] = Cdmatest;
	buf[1] = addr;
	buf[2] = addr >> 32;
	buf[3] = len * type;
	buf[4] = c->cprt >> 32;
	buf[5] = c->cprt;
	prepcmd(buf, 6);
	coherence();
	memmove(c->ram + Cmdoff, buf, sizeof buf);

	for(i = 0; i < 15; i++){
		if(c->cmd->i[1] != Noconf){
			i = gbit32(c->cmd->c);
			if(i == 0)
				error(Eio);
			return i;	/* normal return */
		}
		tsleep(&up->sleep, return0, 0, 5);
	}
	error(Etimeout);
	return ~0;			/* silence! */
}

ulong
rdmacmd(Ctlr *c, int on)
{
	ulong buf[16], i;

	memset(buf, 0, sizeof buf);
	c->cmd->i[0] = 0;
	coherence();
	buf[0] = c->cprt >> 32;
	buf[1] = c->cprt;
	buf[2] = Noconf;
	buf[3] = c->cprt >> 32;
	buf[4] = c->cprt;
	buf[5] = on;
	prepcmd(buf, 6);
	memmove(c->ram + Rdmaoff, buf, sizeof buf);

	for(i = 0; i < 20; i++){
		if(c->cmd->i[0] == Noconf)
			return gbit32(c->cmd->c);	/* normal return */
		tsleep(&up->sleep, return0, 0, 1);
	}
	iprint("m10g: rdmacmd timeout\n");
	error(Etimeout);
	return ~0;			/* silence! */
}

static int
loadfw(Ctlr *c, int *align)
{
	ulong *f, *s, sz;
	int i;

	if((*align = whichfw(c->pcidev)) == 4*KiB){
		f = (ulong*)fw4k;
		sz = sizeof fw4k;
	}else{
		f = (ulong*)fw2k;
		sz = sizeof fw2k;
	}

	s = (ulong*)(c->ram + Fwoffset);
	for(i = 0; i < sz / 4; i++)
		s[i] = f[i];
	return sz & ~3;
}

static int
bootfw(Ctlr *c)
{
	int i, sz, align;
	ulong buf[16];
	Cmd* cmd;

	if((sz = loadfw(c, &align)) == 0)
		return 0;
	dprint("bootfw %d bytes ... ", sz);
	cmd = c->cmd;

	memset(buf, 0, sizeof buf);
	c->cmd->i[0] = 0;
	coherence();
	buf[0] = c->cprt >> 32;	/* upper dma target address */
	buf[1] = c->cprt;	/* lower */
	buf[2] = Noconf;	/* writeback */
	buf[3] = Fwoffset + 8,
	buf[4] = sz - 8;
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
	dprint("[%lux %lux]", gbit32(cmd->c), gbit32(cmd->c+4));
	if(i == 20){
		print("m10g: cannot load fw\n");
		return -1;
	}
	dprint("\n");
	c->tx.segsz = align;
	return 0;
}

static int
kickthebaby(Pcidev *p, Ctlr *c)
{
	/* don't kick the baby! */
	ulong code;

	pcicfgw8(p,  0x10 + c->boot, 0x3);
	pcicfgw32(p, 0x18 + c->boot, 0xfffffff0);
	code = pcicfgr32(p, 0x14 + c->boot);

	dprint("reboot status = %lux\n", code);
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

static char *
fwtype(ulong type)
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

static int
chkfw(Ctlr *c)
{
	ulong off, type;
	Fwhdr *h;

	off = gbit32(c->ram+0x3c);
	dprint("firmware %lux\n", off);
	if((off&3) || off + sizeof *h > c->ramsz){
		print("!m10g: bad firmware %lux\n", off);
		return -1;
	}
	h = (Fwhdr*)(c->ram + off);
	type = gbit32(h->type);
	dprint("\t" "type	%s\n", fwtype(type));
	dprint("\t" "vers	%s\n", h->version);
	dprint("\t" "ramsz	%lux\n", gbit32(h->ramsz));
	if(type != Teth){
		print("!m10g: bad card type %s\n", fwtype(type));
		return -1;
	}

	return bootfw(c) || rdmacmd(c, 0);
}

static int
reset(Ether *e, Ctlr *c)
{
	ulong i, sz;

	if(waserror()){
		print("m10g: reset error\n");
		nexterror();
		return -1;
	}

	chkfw(c);
	cmd(c, Creset, 0);

	cmd(c, CSintrqsz, c->done.n * sizeof *c->done.entry);
	cmd(c, CSintrqdma, c->done.busaddr);
	c->irqack =   (ulong*)(c->ram + cmd(c, CGirqackoff, 0));
	/* required only if we're not doing msi? */
	c->irqdeass = (ulong*)(c->ram + cmd(c, CGirqdeassoff, 0));
	/* this is the driver default, why fiddle with this? */
	c->coal = (ulong*)(c->ram + cmd(c, CGcoaloff, 0));
	*c->coal = pbit32(25);

	dprint("dma stats:\n");
	rdmacmd(c, 1);
	sz = c->tx.segsz;
	i = dmatestcmd(c, DMAread, c->done.busaddr, sz);
	print("m10g: read %lud MB/s;", ((i>>16)*sz*2) / (i&0xffff));
	i = dmatestcmd(c, DMAwrite, c->done.busaddr, sz);
	print(" write %lud MB/s;", ((i>>16)*sz*2) / (i&0xffff));
	i = dmatestcmd(c, DMAwrite|DMAread, c->done.busaddr, sz);
	print(" r/w %lud MB/s\n", ((i>>16)*sz*2*2) / (i&0xffff));
	memset(c->done.entry, 0, c->done.n * sizeof *c->done.entry);

	maccmd(c, CSmac, c->ra);
//	cmd(c, Cnopromisc, 0);
	cmd(c, Cenablefc, 0);
	e->maxmtu = Maxmtu;
	cmd(c, CSmtu, e->maxmtu);
	dprint("CSmtu %d...\n", e->maxmtu);

	poperror();
	return 0;
}

static void
ctlrfree(Ctlr *c)
{
	/* free up all the Block*s, too */
	free(c->tx.host);
	free(c->sm.host);
	free(c->bg.host);
	free(c->cmd);
	free(c->done.entry);
	free(c->stats);
	free(c);
}

static int
setmem(Pcidev *p, Ctlr *c)
{
	ulong i;
	uvlong raddr;
	Done *d;
	void *mem;

	c->tx.segsz = 2048;
	c->ramsz = 2*MiB - (2*48*KiB + 32*KiB) - 0x100;
	if(c->ramsz > p->mem[0].size)
		return -1;

	raddr = p->mem[0].bar & ~0x0F;
	mem = vmap(raddr, p->mem[0].size);
	if(mem == nil){
		print("m10g: can't map %8.8lux\n", p->mem[0].bar);
		return -1;
	}
	dprint("%llux <- vmap(mem[0].size = %ux)\n", raddr, p->mem[0].size);
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

static Block*
balloc(Rx* rx)
{
	Block *bp;

	ilock(rx->pool);
	if((bp = rx->pool->head) != nil){
		rx->pool->head = bp->next;
		bp->next = nil;
		_xinc(&bp->ref);	/* prevent bp from being freed */
		rx->pool->n--;
	}
	iunlock(rx->pool);
	return bp;
}

static void
rbfree(Block *b, Bpool *p)
{
	b->rp = b->wp = (uchar*)PGROUND((uintptr)b->base);
 	b->flag &= ~(Bipck | Budpck | Btcpck | Bpktck);

	ilock(p);
	b->next = p->head;
	p->head = b;
	p->n++;
	p->cnt++;
	iunlock(p);
}

static void
smbfree(Block *b)
{
	rbfree(b, &smpool);
}

static void
bgbfree(Block *b)
{
	rbfree(b, &bgpool);
}

static void
replenish(Rx *rx)
{
	ulong buf[16], i, idx, e;
	Bpool *p;
	Block *b;

	p = rx->pool;
	if(p->n < 8)
		return;
	memset(buf, 0, sizeof buf);
	e = (rx->i - rx->cnt) & ~7;
	e += rx->n;
	while(p->n >= 8 && e){
		idx = rx->cnt & rx->m;
		for(i = 0; i < 8; i++){
			b = balloc(rx);
			buf[i*2]   = pbit32((uvlong)PCIWADDR(b->wp) >> 32);
			buf[i*2+1] = pbit32(PCIWADDR(b->wp));
			rx->host[idx+i] = b;
			assert(b);
		}
		memmove(rx->lanai + 2*idx, buf, sizeof buf);
		coherence();
		rx->cnt += 8;
		e -= 8;
	}
	if(e && p->n > 7+1)
		print("m10g: should panic? pool->n = %d\n", p->n);
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

	for(i = 0; j > (1 << i); i++)
		;
	return 1 << i;
}

static void*
emalign(int sz)
{
	void *v;

	v = malign(sz);
	if(v == nil)
		error(Enomem);
	memset(v, 0, sz);
	return v;
}

static void
open0(Ether *e, Ctlr *c)
{
	Block *b;
	int i, sz, entries;

	entries = cmd(c, CGsendrgsz, 0) / sizeof *c->tx.lanai;
	c->tx.lanai = (Send*)(c->ram + cmd(c, CGsendoff, 0));
	c->tx.host  = emalign(entries * sizeof *c->tx.host);
	c->tx.bring = emalign(entries * sizeof *c->tx.bring);
	c->tx.n = entries;
	c->tx.m = entries-1;

	entries = cmd(c, CGrxrgsz, 0)/8;
	c->sm.pool = &smpool;
	cmd(c, CSsmallsz, c->sm.pool->size);
	c->sm.lanai = (ulong*)(c->ram + cmd(c, CGsmallrxoff, 0));
	c->sm.n = entries;
	c->sm.m = entries-1;
	c->sm.host = emalign(entries * sizeof *c->sm.host);

	c->bg.pool = &bgpool;
	c->bg.pool->size = nextpow(2 + e->maxmtu);  /* 2-byte alignment pad */
	cmd(c, CSbigsz, c->bg.pool->size);
	c->bg.lanai = (ulong*)(c->ram + cmd(c, CGbigrxoff, 0));
	c->bg.n = entries;
	c->bg.m = entries-1;
	c->bg.host = emalign(entries * sizeof *c->bg.host);

	sz = c->sm.pool->size + BY2PG;
	for(i = 0; i < c->sm.n; i++){
		if((b = allocb(sz)) == 0)
			break;
		b->free = smbfree;
		freeb(b);
	}
	sz = c->bg.pool->size + BY2PG;
	for(i = 0; i < c->bg.n; i++){
		if((b = allocb(sz)) == 0)
			break;
		b->free = bgbfree;
		freeb(b);
	}

	cmd(c, CSstatsdma, c->statsprt);
	c->linkstat = ~0;
	c->nrdma = 15;

	cmd(c, Cetherup, 0);
}

static Block*
nextblock(Ctlr *c)
{
	uint i;
	ushort l, k;
	Block *b;
	Done *d;
	Rx *rx;
	Slot *s;
	Slotparts *sp;

	d = &c->done;
	s = d->entry;
	i = d->i & d->m;
	sp = (Slotparts *)(s + i);
	l = sp->len;
	if(l == 0)
		return 0;
	k = sp->cksum;
	s[i] = 0;
	d->i++;
	l = gbit16((uchar*)&l);
//dprint("nextb: i=%d l=%d\n", d->i, l);
	rx = whichrx(c, l);
	if(rx->i >= rx->cnt){
		iprint("m10g: overrun\n");
		return 0;
	}
	i = rx->i & rx->m;
	b = rx->host[i];
	rx->host[i] = 0;
	if(b == 0){
		iprint("m10g: error rx to no block.  memory is hosed.\n");
		return 0;
	}
	rx->i++;

	b->flag |= Bipck|Btcpck|Budpck;
	b->checksum = k;
	b->rp += 2;
	b->wp += 2+l;
	b->lim = b->wp;			/* lie like a dog. */
	return b;
}

static int
rxcansleep(void *v)
{
	Ctlr *c;
	Slot *s;
	Slotparts *sp;
	Done *d;

	c = v;
	d = &c->done;
	s = c->done.entry;
	sp = (Slotparts *)(s + (d->i & d->m));
	if(sp->len != 0)
		return -1;
	c->irqack[0] = pbit32(3);
	return 0;
}

static void
m10rx(void *v)
{
	Ether *e;
	Ctlr *c;
	Block *b;

	e = v;
	c = e->ctlr;
	for(;;){
		replenish(&c->sm);
		replenish(&c->bg);
		sleep(&c->rxrendez, rxcansleep, c);
		while(b = nextblock(c))
			etheriq(e, b, 1);
	}
}

static void
txcleanup(Tx *tx, ulong n)
{
	Block *b;
	uint j, l, m;

	if(tx->npkt == n)
		return;
	l = 0;
	m = tx->m;
	/*
	 * if tx->cnt == tx->i, yet tx->npkt == n-1, we just
	 * caught ourselves and myricom card updating.
	 */
	for(;; tx->cnt++){
		j = tx->cnt & tx->m;
		if(b = tx->bring[j]){
			tx->bring[j] = 0;
			tx->nbytes += BLEN(b);
			freeb(b);
			if(++tx->npkt == n)
				return;
		}
		if(tx->cnt == tx->i)
			return;
		if(l++ == m){
			iprint("m10g: tx ovrun: %lud %lud\n", n, tx->npkt);
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

static void
txproc(void *v)
{
	Ether *e;
	Ctlr *c;
	Tx *tx;

	e = v;
	c = e->ctlr;
	tx = &c->tx;
	for(;;){
 		sleep(&c->txrendez, txcansleep, c);
		txcleanup(tx, gbit32(c->stats->txcnt));
	}
}

static void
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

static int
nsegments(Block *b, int segsz)
{
	uintptr bus, end, slen, len;
	int i;

	bus = PCIWADDR(b->rp);
	i = 0;
	for(len = BLEN(b); len; len -= slen){
		end = bus + segsz & ~(segsz-1);
		slen = end - bus;
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
	ushort slen;
	ulong i, cnt, rdma, nseg, count, end, bus, len, segsz;
	uchar flags;
	Block *b;
	Ctlr *c;
	Send *s, *s0, *s0m8;
	Tx *tx;

	c = e->ctlr;
	tx = &c->tx;
	segsz = tx->segsz;

	qlock(tx);
	count = 0;
	s = tx->host + (tx->i & tx->m);
	cnt = tx->cnt;
	s0 =   tx->host + (cnt & tx->m);
	s0m8 = tx->host + ((cnt - 8) & tx->m);
	i = tx->i;
	for(; s >= s0 || s < s0m8; i += nseg){
		if((b = qget(e->oq)) == nil)
			break;
		flags = SFfirst|SFnotso;
		if((len = BLEN(b)) < 1520)
			flags |= SFsmall;
		rdma = nseg = nsegments(b, segsz);
		bus = PCIWADDR(b->rp);
		for(; len; len -= slen){
			end = (bus + segsz) & ~(segsz-1);
			slen = end - bus;
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
		tx->bring[(i + nseg - 1) & tx->m] = b;
		if(1 || count > 0){
			submittx(tx, count);
			count = 0;
			cnt = tx->cnt;
			s0 =   tx->host + (cnt & tx->m);
			s0m8 = tx->host + ((cnt - 8) & tx->m);
		}
	}
	qunlock(tx);
}

static void
checkstats(Ether *e, Ctlr *c, Stats *s)
{
	ulong i;

	if(s->updated == 0)
		return;

	i = gbit32(s->linkstat);
	if(c->linkstat != i){
		e->link = i;
		if(c->linkstat = i)
			dprint("m10g: link up\n");
		else
			dprint("m10g: link down\n");
	}
	i = gbit32(s->nrdma);
	if(i != c->nrdma){
		dprint("m10g: rdma timeout %ld\n", i);
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
	if(waserror()){
		c->state = Detached;
		qunlock(c);
		nexterror();
	}
	reset(e, c);
	c->state = Attached;
	open0(e, c);
	if(c->kprocs == 0){
		c->kprocs++;
		snprint(name, sizeof name, "#l%drxproc", e->ctlrno);
		kproc(name, m10rx, e);
		snprint(name, sizeof name, "#l%dtxproc", e->ctlrno);
		kproc(name, txproc, e);
	}
	c->state = Runed;
	qunlock(c);
	poperror();
}

static int
m10gdetach(Ctlr *c)
{
	dprint("m10gdetach\n");
//	reset(e->ctlr);
	vunmap(c->ram, c->pcidev->mem[0].size);
	ctlrfree(c);		/* this is a bad idea: don't free c */
	return -1;
}

static int
lstcount(Block *b)
{
	int i;

	i = 0;
	for(; b; b = b->next)
		i++;
	return i;
}

static long
m10gifstat(Ether *e, void *v, long n, ulong off)
{
	char *p;
	Ctlr *c;
	Stats s;

	c = e->ctlr;
	p = malloc(READSTR+1);
	if(p == nil)
		error(Enomem);
	/* no point in locking this because this is done via dma. */
	memmove(&s, c->stats, sizeof s);

	snprint(p, READSTR,
		"txcnt = %lud\n"  "linkstat = %lud\n" 	"dlink = %lud\n"
		"derror = %lud\n" "drunt = %lud\n" 	"doverrun = %lud\n"
		"dnosm = %lud\n"  "dnobg = %lud\n"	"nrdma = %lud\n"
		"txstopped = %ud\n" "down = %ud\n" 	"updated = %ud\n"
		"valid = %ud\n\n"
		"tx pkt = %lud\n"  "tx bytes = %lld\n"
		"tx cnt = %ud\n"  "tx n = %ud\n"	"tx i = %ud\n"
		"sm cnt = %ud\n"  "sm i = %ud\n"	"sm n = %ud\n"
		"sm lst = %ud\n"
		"bg cnt = %ud\n"  "bg i = %ud\n"	"bg n = %ud\n"
		"bg lst = %ud\n"
		"segsz = %lud\n"   "coal = %lud\n",
		gbit32(s.txcnt),  gbit32(s.linkstat),	gbit32(s.dlink),
		gbit32(s.derror), gbit32(s.drunt),	gbit32(s.doverrun),
		gbit32(s.dnosm),  gbit32(s.dnobg),	gbit32(s.nrdma),
		s.txstopped,  s.down, s.updated, s.valid,
		c->tx.npkt, c->tx.nbytes,
		c->tx.cnt, c->tx.n, c->tx.i,
		c->sm.cnt, c->sm.i, c->sm.pool->n, lstcount(c->sm.pool->head),
		c->bg.cnt, c->bg.i, c->bg.pool->n, lstcount(c->bg.pool->head),
		c->tx.segsz, gbit32((uchar*)c->coal));

	n = readstr(off, v, n, p);
	free(p);
	return n;
}

//static void
//summary(Ether *e)
//{
//	char *buf;
//	int n, i, j;
//
//	if(e == 0)
//		return;
//	buf = malloc(n=250);
//	if(buf == 0)
//		return;
//
//	snprint(buf, n, "oq\n");
//	qsummary(e->oq, buf+3, n-3-1);
//	iprint("%s", buf);
//
//	if(e->f) for(i = 0; e->f[i]; i++){
//		j = snprint(buf, n, "f%d %d\n", i, e->f[i]->type);
//		qsummary(e->f[i]->in, buf+j, n-j-1);
//		print("%s", buf);
//	}
//
//	free(buf);
//}

static void
rxring(Ctlr *c)
{
	Done *d;
	Slot *s;
	Slotparts *sp;
	int i;

	d = &c->done;
	s = d->entry;
	for(i = 0; i < d->n; i++) {
		sp = (Slotparts *)(s + i);
		if(sp->len)
			iprint("s[%d] = %d\n", i, sp->len);
	}
}

enum {
	CMdebug,
	CMcoal,
	CMwakeup,
	CMtxwakeup,
	CMqsummary,
	CMrxring,
};

static Cmdtab ctab[] = {
	CMdebug,	"debug",	2,
	CMcoal,		"coal",		2,
	CMwakeup,	"wakeup",	1,
	CMtxwakeup,	"txwakeup",	1,
//	CMqsummary,	"q",		1,
	CMrxring,	"rxring",	1,
};

static long
m10gctl(Ether *e, void *v, long n)
{
	int i;
	Cmdbuf *c;
	Cmdtab *t;

	dprint("m10gctl\n");
	if(e->ctlr == nil)
		error(Enonexist);

	c = parsecmd(v, n);
	if(waserror()){
		free(c);
		nexterror();
	}
	t = lookupcmd(c, ctab, nelem(ctab));
	switch(t->index){
	case CMdebug:
		debug = (strcmp(c->f[1], "on") == 0);
		break;
	case CMcoal:
		i = atoi(c->f[1]);
		if(i < 0 || i > 1000)
			error(Ebadarg);
		*((Ctlr*)e->ctlr)->coal = pbit32(i);
		break;
	case CMwakeup:
		wakeup(&((Ctlr*)e->ctlr)->rxrendez); /* you're kidding, right? */
		break;
	case CMtxwakeup:
		wakeup(&((Ctlr*)e->ctlr)->txrendez); /* you're kidding, right? */
		break;
//	case CMqsummary:
//		summary(e);
//		break;
	case CMrxring:
		rxring(e->ctlr);
		break;
	default:
		error(Ebadarg);
	}
	free(c);
	poperror();
	return n;
}

static void
m10gshutdown(Ether *e)
{
	dprint("m10gshutdown\n");
	m10gdetach(e->ctlr);
}

static void
m10gpromiscuous(void *v, int on)
{
	Ether *e;
	int i;

	dprint("m10gpromiscuous\n");
	e = v;
	if(on)
		i = Cpromisc;
	else
		i = Cnopromisc;
	cmd(e->ctlr, i, 0);
}

static int	mcctab[]  = { CSleavemc, CSjoinmc };
static char	*mcntab[] = { "leave", "join" };

static void
m10gmulticast(void *v, uchar *ea, int on)
{
	Ether *e;
	int i;

	dprint("m10gmulticast\n");
	e = v;
	if((i = maccmd(e->ctlr, mcctab[on], ea)) != 0)
		print("m10g: can't %s %E: %d\n", mcntab[on], ea, i);
}

static void
m10gpci(void)
{
	Pcidev *p;
	Ctlr *t, *c;

	t = 0;
	for(p = 0; p = pcimatch(p, Vmyricom, 0); ){
		switch(p->did){
		case 0x8:		/* 8a */
			break;
		case 0x9:		/* 8a with msi-x fw */
		case 0xa:		/* 8b */
		case 0xb:		/* 8b2 */
		case 0xc:		/* 2-8b2 */
			/* untested */
			break;
		default:
			print("etherm10g: unknown myricom did %#ux\n", p->did);
			continue;
		}
		c = malloc(sizeof *c);
		if(c == nil)
			break;
		c->pcidev = p;
		c->id = p->did<<16 | p->vid;
		c->boot = pcicap(p, PciCapVND);
//		kickthebaby(p, c);
		pcisetbme(p);
		if(setmem(p, c) == -1){
			print("m10g: setmem failed\n");
			free(c);
			/* cleanup */
			continue;
		}
		if(t)
			t->next = c;
		else
			ctlrs = c;
		t = c;
	}
}

static int
m10gpnp(Ether *e)
{
	Ctlr *c;

	if(ctlrs == nil)
		m10gpci();

	for(c = ctlrs; c != nil; c = c->next)
		if(c->active)
			continue;
		else if(e->port == 0 || e->port == c->port)
			break;
	if(c == nil)
		return -1;
	c->active = 1;

	e->ctlr = c;
	e->port = c->port;
	e->irq = c->pcidev->intl;
	e->tbdf = c->pcidev->tbdf;
	e->mbps = 10000;
	memmove(e->ea, c->ra, Eaddrlen);

	e->attach = m10gattach;
	e->detach = m10gshutdown;
	e->transmit = m10gtransmit;
	e->interrupt = m10ginterrupt;
	e->ifstat = m10gifstat;
	e->ctl = m10gctl;
	e->shutdown = m10gshutdown;

	e->arg = e;
	e->promiscuous = m10gpromiscuous;
	e->multicast = m10gmulticast;
	return 0;
}

void
etherm10glink(void)
{
	addethercard("m10g", m10gpnp);
}
