/*
 * Support for 9am(8).
 */
#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"../port/error.h"
#include	"mp.h"
#include	"acpi.h"

typedef struct Regio Regio;

enum
{

	Qdir = 0,
	Qctl,
	Qtbl,
	Qio,

	CMinit = 0,	/* call hw init; everything has been configured */
	CMregion,	/* regio name spc base len accsz*/
	CMgpe,		/* gpe name id */
	CMpwr,		/* pwr state val */
	CMdev,		/* dev path name pnp uid addr */
	CMpush,		/* [ */
	CMpop,		/* ] */
	CMirq,		/* irq flags nb... */
	CMoirq,		/* oirq flags nb... */
	CMdma,		/* dma chans flags */
	CModma,
	CMio,		/* io min max align */
	CMoio,
	CMmem,		/* mem rw|ro min max align */
	CMomem,
	CMas,		/* as io|mem|bus flags mask min max len off attr */
	CMoas,
	CMpir,		/* pir dev pin link irq */
	CMbbn,		/* bbn n */
	CMdebug,	/* debug n */
	CMdump,		/* dump */

	/* ACPI tbdf as encoded in acpi region base addresses */
	Rpciregshift	= 0,
	Rpciregmask	= 0xFFFF,
	Rpcifunshift	= 16,
	Rpcifunmask	= 0xFFFF,
	Rpcidevshift	= 32,
	Rpcidevmask	= 0xFFFF,
	Rpcibusshift	= 48,
	Rpcibusmask	= 0xFFFF,

};

struct Regio
{
	void	*arg;
	u8int	(*get8)(uintptr, void*);
	void	(*set8)(uintptr, u8int, void*);
	u16int	(*get16)(uintptr, void*);
	void	(*set16)(uintptr, u16int, void*);
	u32int	(*get32)(uintptr, void*);
	void	(*set32)(uintptr, u32int, void*);
	u64int	(*get64)(uintptr, void*);
	void	(*set64)(uintptr, u64int, void*);
};

static Cmdtab ctls[] =
{
	{CMinit,	"init",		1},
	{CMregion,	"region",	6},
	{CMgpe,		"gpe",		3},
	{CMpwr,		"pwr",		4},
	{CMdev,		"dev",		6},
	{CMpush,	"[",		1},
	{CMpop,		"]",		1},
	{CMirq,		"irq",		0},
	{CMoirq,	"oirq",		0},
	{CMdma,		"dma",		3},
	{CModma,	"odma",		3},
	{CMio,		"io",		4},
	{CMoio,		"oio",		4},
	{CMmem,		"mem",		5},
	{CMomem,	"omem",		5},
	{CMas,		"as",		9},
	{CMoas,		"oas",		9},
	{CMpir,		"pir",		5},
	{CMbbn,		"bbn",		2},
	{CMdebug,	"debug",	2},
	{CMdump,		"dump",		1},
};

static Dirtab acpidir[]={
	".",		{Qdir, 0, QTDIR},	0,	DMDIR|0555,
	"acpictl",	{Qctl},			0,	0666,
	"acpitbl",	{Qtbl},			0,	DMEXCL|0444,
	"acpiregio",	{Qio},			0,	0666,
};

static char* regnames[] = {
	"mem", "io", "pcicfg", "embed",
	"smb", "cmos", "pcibar",
};

#define dprint		if(debug)print
#define Dprint		if(debug>1)print
#define dioprint	if(debugio)print

static Reg	*reg;		/* region used for I/O */
static int	debug = 2;
static int	debugio = 0;

static ACPIdev	*devstack[16];	/* device config stack */
static int	devdepth;	
static ACPIdev	**lastptr;
static ACPIdev	*lastdev;

ACPIdev *acpidevs;

static u8int
mget8(uintptr p, void*)
{
	u8int *cp = (u8int*)p;
	return *cp;
}

static void
mset8(uintptr p, u8int v, void*)
{
	u8int *cp = (u8int*)p;
	*cp = v;
}

static u16int
mget16(uintptr p, void*)
{
	u16int *cp = (u16int*)p;
	return *cp;
}

static void
mset16(uintptr p, u16int v, void*)
{
	u16int *cp = (u16int*)p;
	*cp = v;
}

static u32int
mget32(uintptr p, void*)
{
	u32int *cp = (u32int*)p;
	return *cp;
}

static void
mset32(uintptr p, u32int v, void*)
{
	u32int *cp = (u32int*)p;
	*cp = v;
}

static u64int
mget64(uintptr p, void*)
{
	u64int *cp = (u64int*)p;
	return *cp;
}

static void
mset64(uintptr p, u64int v, void*)
{
	u64int *cp = (u64int*)p;
	*cp = v;
}

static u8int
ioget8(uintptr p, void*)
{
	return inb(p);
}

static void
ioset8(uintptr p, u8int v, void*)
{
	outb(p, v);
}

static u16int
ioget16(uintptr p, void*)
{
	return ins(p);
}

static void
ioset16(uintptr p, u16int v, void*)
{
	outs(p, v);
}

static u32int
ioget32(uintptr p, void*)
{
	return inl(p);
}

static void
ioset32(uintptr p, u32int v, void*)
{
	outl(p, v);
}

static u8int
cfgget8(uintptr p, void* r)
{
	Reg *ro = r;
	Pcidev d;

	d.tbdf = ro->tbdf;
	return pcicfgr8(&d, p);
}

static void
cfgset8(uintptr p, u8int v, void* r)
{
	Reg *ro = r;
	Pcidev d;

	d.tbdf = ro->tbdf;
	pcicfgw8(&d, p, v);
}

static u16int
cfgget16(uintptr p, void* r)
{
	Reg *ro = r;
	Pcidev d;

	d.tbdf = ro->tbdf;
	return pcicfgr16(&d, p);
}

static void
cfgset16(uintptr p, u16int v, void* r)
{
	Reg *ro = r;
	Pcidev d;

	d.tbdf = ro->tbdf;
	pcicfgw16(&d, p, v);
}

static u32int
cfgget32(uintptr p, void* r)
{
	Reg *ro = r;
	Pcidev d;

	d.tbdf = ro->tbdf;
	return pcicfgr32(&d, p);
}

static void
cfgset32(uintptr p, u32int v, void* r)
{
	Reg *ro = r;
	Pcidev d;

	d.tbdf = ro->tbdf;
	pcicfgw32(&d, p, v);
}

static Regio memio = 
{
	nil,
	mget8, mset8, mget16, mset16,
	mget32, mset32, mget64, mset64
};

static Regio ioio = 
{
	nil,
	ioget8, ioset8, ioget16, ioset16,
	ioget32, ioset32, nil, nil
};

static Regio cfgio = 
{
	nil,
	cfgget8, cfgset8, cfgget16, cfgset16,
	cfgget32, cfgset32, nil, nil
};

/*
 * Copy memory, 1/2/4/8-bytes at a time, to/from a region.
 */
static long
regcpy(Regio *dio, uintptr da, Regio *sio, uintptr sa, long len, int align)
{
	int n, i;

	if(0)dioprint("regcpy %#ulx %#ulx %#ulx %#ux\n", da, sa, len, align);
	if((len%align) != 0)
		print("regcpy: bug: copy not aligned. truncated\n");
	n = len/align;
	for(i = 0; i < n; i++){
		switch(align){
		case 1:
			if(0)dioprint("cpy8 %#p %#p\n", da, sa);
			dio->set8(da, sio->get8(sa, sio->arg), dio->arg);
			break;
		case 2:
			if(0)dioprint("cpy16 %#p %#p\n", da, sa);
			dio->set16(da, sio->get16(sa, sio->arg), dio->arg);
			break;
		case 4:
			if(0)dioprint("cpy32 %#p %#p\n", da, sa);
			dio->set32(da, sio->get32(sa, sio->arg), dio->arg);
			break;
		case 8:
			if(0)dioprint("cpy64 %#p %#p\n", da, sa);
			dio->set64(da, sio->get64(sa, sio->arg), dio->arg);
			break;
		default:
			panic("regcpy: align bug");
		}
		da += align;
		sa += align;
	}
	return n*align;
}

static void
hdump(uchar *p, long len)
{
	int i;

	if(len <= 4)
		print(" ->");
	else
		print("\n\t");
	for(i = 0; i < len; i++){
		print("%02x ", p[i]);
		if((i % 16) == 0 && i > 0)
			print("\n");
	}
	print("\n");
}

/*
 * Perform I/O within region. All units in bytes.
 */
long
acpiregio(Reg *r, void *p, ulong len, uintptr off, int iswr)
{
	Regio rio;
	uintptr rp;

	if(r->accsz == 0){
		print("acpi bug: acpiregio: accsz is 0\n");
		r->accsz = 1;
	}
	if(debugio){
		print("acpi:%s %s %#p %#ulx %#lx sz=%d",
			iswr ? "out" : "in ", r->name, p, off, len, r->accsz);
		if(iswr)
			hdump(p, len);
	}
	rp = 0;
	if(off + len > r->len){
		print("\nregio: off %#p + len %#ulx > rlen %#ullx\n",
			off, len, r->len);
		len = r->len - off;
	}
	if(len <= 0){
		dioprint("\n");
		return 0;
	}
	switch(r->spc){
	case Rsysmem:
		if(r->p != nil && r->plen < off+len){
			vunmap(r->p, r->plen);
			r->p = nil;
		}
		if(r->p == nil){
			r->plen = off+len;
			r->p = vmap(r->base, r->plen);
		}
		rp = (uintptr)r->p + off;
		rio = memio;
		break;
	case Rsysio:
		rp = r->base + off;
		rio = ioio;
		break;
	case Rpcicfg:
		rp = r->base + off;
		rio = cfgio;
		rio.arg = r;
		break;
	case Rpcibar:
	case Rembed:
	case Rsmbus:
	case Rcmos:
	case Ripmi:
	case Rfixedhw:
		print("\nregio: reg %s not supported\n", acpiregstr(r->spc));
		error("region not supported");
	}
	if(iswr)
		regcpy(&rio, rp, &memio, (uintptr)p, len, r->accsz);
	else{
		regcpy(&memio, (uintptr)p, &rio, rp, len, r->accsz);
		if(debugio)
			hdump(p, len);
	}
	return len;
}

static char*
tabs(int n)
{
	static char s[10];

	memset(s, '\t', sizeof(s));
	if(n > 9)
		n = 9;
	s[n] = 0;
	return s;
}

static void dumpdevs(ACPIdev *, int);
static void
dumpdev(ACPIdev *d, int lvl)
{
	ACPIres *r;

	print("%s%s %#p %s %s", tabs(lvl), d->name, d, d->acpiname, d->pnpid);
	print(" %lld %#ullx\n", d->uid, d->addr);
	for(r = d->res; r != nil; r = r->next)
		 print("%s%R\n", tabs(lvl+1), r);
	for(r = d->other; r != nil; r = r->next)
		 print("%sother %R\n", tabs(lvl+1), r);
	
	if(d->child != nil)
		dumpdevs(d->child, lvl+1);
}

static void
dumpdevs(ACPIdev *nd, int lvl)
{
	if(nd == nil)
		print("no acpi devs\n");
	for(; nd != nil; nd = nd->cnext)
		dumpdev(nd, lvl);
}

static int
hwgen(Chan *c, char*, Dirtab *tab, int ntab, int i, Dir *dp)
{
	Qid qid;

	if(i == DEVDOTDOT){
		mkqid(&qid, Qdir, 0, QTDIR);
		devdir(c, qid, ".", 0, eve, 0555, dp);
		return 1;
	}
	i++; /* skip first element for . itself */
	if(tab==0 || i>=ntab)
		return -1;
	tab += i;
	qid = tab->qid;
	qid.path &= ~Qdir;
	qid.vers = 0;
	devdir(c, qid, tab->name, tab->length, eve, tab->perm, dp);
	return 1;
}

static Chan*
hwattach(char *spec)
{
	return devattach(L'9', spec);
}

static void
hwinit(void)
{
	acpistart();
}

static Walkqid*
hwwalk(Chan *c, Chan *nc, char **name, int nname)
{
	return devwalk(c, nc, name, nname, acpidir, nelem(acpidir), hwgen);
}

static int
hwstat(Chan *c, uchar *dp, int n)
{
	return devstat(c, dp, n, acpidir, nelem(acpidir), hwgen);
}

/*
 * Tables can be huge.
 * We make acpitbl DMEXCL and release the read buffer
 * upon clunks of acpitbl.
 * Tables should be read only during boot, later they are useless
 * unless we are debugging 9am.
 */
static char *ttext;

static Chan*
hwopen(Chan *c, int omode)
{
	long q;

	q = c->qid.path;
	if(q == Qtbl){
		if(ttext != nil)
			error(Einuse);
		ttext = smprinttables();
	}
	return devopen(c, omode, acpidir, nelem(acpidir), hwgen);
}

static void
hwclose(Chan *c)
{
	long q;

	q = c->qid.path;
	if(q == Qtbl){
		free(ttext);
		ttext = nil;
	}
}

static long
hwread(Chan *c, void *a, long n, vlong off)
{
	long q;

	q = c->qid.path;
	switch(q){
	case Qdir:
		return devdirread(c, a, n, acpidir, nelem(acpidir), hwgen);
	case Qtbl:
		if(ttext == nil)
			return 0;
		return readstr(off, a, n, ttext);
	case Qio:
		if(reg == nil)
			error("region not configured");
		return acpiregio(reg, a, n, off, 0);
	}
	error(Eperm);
	return -1;
}

extern void startdevs(void);

/*
 * Complete what once was done by main() to initialize
 * everything else.
 */
static void
initctl(Cmdbuf *)
{
	ulong s;
	static int alreadyinit;

	if(alreadyinit++ != 0)
		error("once is enough");
	if(debug)
		dumpdevs(acpidevs, 0);
	s = spllo();
	startdevs();
	splx(s);
}

char*
acpiregstr(int id)
{
	static char buf[20];	/* BUG */

	if(id >= 0 && id < nelem(regnames))
		return regnames[id];
	seprint(buf, buf+sizeof(buf), "spc:%#x", id);
	return buf;
}

static int
acpiregid(char *s)
{
	int i;

	for(i = 0; i < nelem(regnames); i++)
		if(strcmp(regnames[i], s) == 0)
			return i;
	return -1;
}

static void
regionctl(Cmdbuf *cb)
{
	Reg *r;
	uint rno, fun, dev, bus;

	r = reg;
	if(r == nil){
		r = smalloc(sizeof(Reg));
		r->name = nil;
	}
	if(r->p != nil)
		vunmap(r->p, r->plen);
	r->p = nil;
	r->plen = 0;
	kstrdup(&r->name, cb->f[1]);
	r->spc = acpiregid(cb->f[2]);
	if(r->spc < 0){
		free(r);
		reg = nil;
		error("bad region type");
	}
	if(r->spc == Rpcicfg || r->spc == Rpcibar){
		rno = r->base>>Rpciregshift & Rpciregmask;
		fun = r->base>>Rpcifunshift & Rpcifunmask;
		dev = r->base>>Rpcidevshift & Rpcidevmask;
		bus = r->base>>Rpcibusshift & Rpcibusmask;
		r->tbdf = MKBUS(BusPCI, bus, dev, fun);
		r->base = rno;	/* register ~ our base addr */
	}
	r->base = strtoull(cb->f[3], nil, 0);
	r->len = strtoull(cb->f[4], nil, 0);
	r->accsz = strtoul(cb->f[5], nil, 0);
	if(r->accsz < 1 || r->accsz > 4){
		free(r);
		reg = nil;
		error("bad region access size");
	}
	reg = r;
	dioprint("acpi: region %s %s %llux %llux sz %d\n",
		r->name, acpiregstr(r->spc), r->base, r->len, r->accsz);
}

static void
gpectl(Cmdbuf *cb)
{
	int i;

	i = strtoul(cb->f[1], nil, 0);
	acpigpe(i, cb->f[2]);
}

static void
pwrctl(Cmdbuf *cb)
{
	int i, pm1a, pm1b;

	i = strtoul(cb->f[1], nil, 0);
	pm1a = strtoul(cb->f[2], nil, 0);
	pm1b = strtoul(cb->f[3], nil, 0);
	acpipwr(i, pm1a, pm1b);
}

/*
 * devices are never deallocated; not yet.
 */
static ACPIdev *
newdev(void)
{
	ACPIdev *d;
	static ACPIdev *last;

	d = smalloc(sizeof(ACPIdev));
	if(last != nil)
		last->next = d;
	last = d;
	return d;
}

static void
devctl(Cmdbuf *cb)
{
	ACPIdev *d;

	if(lastptr == nil)
		lastptr = &acpidevs;

	lastdev = d = newdev();
	kstrdup(&d->acpiname, cb->f[1]);
	kstrdup(&d->name, cb->f[2]);
	kstrdup(&d->pnpid, cb->f[3]);
	d->uid = strtoul(cb->f[4], nil, 0);
	d->addr = strtoull(cb->f[5], nil, 0);
	*lastptr = d;
	lastptr = &d->cnext;
	Dprint("acpidev %#p %s %s %s %ulld %#ullx\n",
		d, d->acpiname, d->name, d->pnpid, d->uid, d->addr);
}

static void
pushctl(Cmdbuf *)
{
	if(lastdev == nil)
		error("no device");
	if(lastdev->child != nil)
		error("device has chilren");
	if(devdepth == nelem(devstack)){
		print("p9am bug: device depth\n");
		error("bug: device depth exceeded");
	}
	devstack[devdepth++] = lastdev;
	lastptr = &lastdev->child;
	lastdev = nil;
	Dprint("push\n");
}

static void
popctl(Cmdbuf *)
{
	ACPIdev *d;

	if(devdepth == 0)
		error("no device in stack");
	d = devstack[--devdepth];
	lastptr = &d->cnext;
	assert(*lastptr == nil);
	lastdev = nil;
	Dprint("pop\n");
}

static ACPIres*
newres(int type, int excl, int other)
{
	ACPIres *r, **l;

	if(lastdev == nil)
		error("no device");

	l = &lastdev->res;
	if(other)
		l = &lastdev->other;
	for(; *l != nil; l = &(*l)->next)
		if(excl != 0 && (*l)->type == type)
			error("resource already declared"); /* will happen? */
	*l = r = smalloc(sizeof(ACPIres));
	r->type = type;
	return r;
}

static int
strtointr(char *s)
{
	int r;

	if(strlen(s) < 2)
		error("short interrupt flags");
	if(strcmp(s, "none") == 0)
		return -1;
	switch(s[0]){
	case 'l':
		r = PcmpLEVEL;
		break;
	case 'e':
		r = PcmpEDGE;
		break;
	default:
		r = 0;
	}
	switch(s[1]){
	case 'l':
		r |= PcmpLOW;
		break;
	case 'h':
		r |= PcmpHIGH;
		break;
	}
	return r;	
}

static void
irqctl(Cmdbuf *cb)
{
	ACPIres *r;
	int i;

	if(cb->nf < 3)
		error("usage: irq flags nb...");
	r = newres(Rirq, 1, *cb->f[0] == 'o');
	r->irq.flags = strtointr(cb->f[1]);
	for(i = 2; i < cb->nf && i-2 < nelem(r->irq.nb); i++)
		r->irq.nb[i-2] = strtoul(cb->f[i], nil, 0);
	if(i-2 < nelem(r->irq.nb))
		r->irq.nb[i-2] = -1;
	if(i < cb->nf)
		print("p9am: bug: too many irqs for device\n");
	Dprint("dev %s %R\n", lastdev->name, r);
}

static void
dmactl(Cmdbuf *cb)
{
	ACPIres *r;

	r = newres(Rdma, 1, *cb->f[0] == 'o');
	r->dma.chans = strtoul(cb->f[1], nil, 0);
	r->irq.flags = strtoul(cb->f[2], nil, 0);
	Dprint("dev %s %R\n", lastdev->name, r);
}

static void
ioctl(Cmdbuf *cb)
{
	ACPIres *r;

	r = newres(Rio, 0, *cb->f[0] == 'o');
	r->io.min = strtoull(cb->f[1], nil, 0);
	r->io.max = strtoull(cb->f[2], nil, 0);
	r->io.align = strtoul(cb->f[3], nil, 0);
	r->io.isrw = 1;
	Dprint("dev %s %R\n", lastdev->name, r);

}

static void
memctl(Cmdbuf *cb)
{
	ACPIres *r;

	r = newres(Rmem, 0, *cb->f[0] == 'o');
	r->mem.isrw = strcmp(cb->f[1], "rw") == 0;
	r->mem.min = strtoull(cb->f[2], nil, 0);
	r->mem.max = strtoull(cb->f[3], nil, 0);
	r->mem.align = strtoul(cb->f[4], nil, 0);
	Dprint("dev %s %R\n", lastdev->name, r);
}

static void
asctl(Cmdbuf *cb)
{
	ACPIres *r;
	int type;

	type = -1;
	if(strcmp(cb->f[1], "io") == 0)
		type = Rioas;
	else if(strcmp(cb->f[1], "mem") == 0)
		type = Rmemas;
	else if(strcmp(cb->f[1], "bus") == 0)
		type = Rbusas;
	else
		error("unknown as type");
	r = newres(type, 0, *cb->f[0] == 'o');
	r->as.flags = strtoul(cb->f[2], nil, 0);
	r->as.mask = strtoull(cb->f[3], nil, 0);
	r->as.min = strtoull(cb->f[4], nil, 0);
	r->as.max = strtoull(cb->f[5], nil, 0);
	r->as.len = strtoull(cb->f[6], nil, 0);
	r->as.off = strtol(cb->f[7], nil, 0);
	r->as.attr = strtoull(cb->f[8], nil, 0);
	Dprint("dev %s %R\n", lastdev->name, r);
}

static void
pirctl(Cmdbuf *cb)
{
	ACPIres *r;

	r = newres(Rpir, 0, *cb->f[0] == 'o');
	r->pir.dno = strtoul(cb->f[1], nil, 0);
	r->pir.pin = strtoul(cb->f[2], nil, 0);
	r->pir.link = strtoul(cb->f[3], nil, 0);
	r->pir.airq = strtoul(cb->f[4], nil, 0);
	Dprint("dev %s %R\n", lastdev->name, r);
}

static void
bbnctl(Cmdbuf *cb)
{
	ACPIres *r;

	r = newres(Rbbn, 0, *cb->f[0] == 'o');
	r->bbn = strtoul(cb->f[1], nil, 0);
	Dprint("dev %s %R\n", lastdev->name, r);
}

static void
debugctl(Cmdbuf *cb)
{
	if(strcmp(cb->f[1], "on") == 0)
		debug = 1;
	else
		debug = strtol(cb->f[1], nil, 0);
}

static void
dumpctl(Cmdbuf *)
{
	int odbg;

	odbg = debug;
	debug = 2;
	dumpdevs(acpidevs, 0);
	debug = odbg;
}

typedef void (*Hwctl)(Cmdbuf*);
static Hwctl hwctls[] = {
	[CMinit]	initctl,
	[CMregion]	regionctl,
	[CMgpe]		gpectl,
	[CMpwr]		pwrctl,
	[CMdev]		devctl,
	[CMpush]	pushctl,
	[CMpop]		popctl,
	[CMirq]		irqctl,
	[CMoirq]	irqctl,
	[CMdma]		dmactl,
	[CModma]	dmactl,
	[CMio]		ioctl,
	[CMoio]		ioctl,
	[CMmem]		memctl,
	[CMomem]	memctl,
	[CMas]		asctl,
	[CMoas]		asctl,
	[CMpir]		pirctl,
	[CMbbn]		bbnctl,
	[CMdebug]	debugctl,
	[CMdump]	dumpctl,
};

static long
hwwrite(Chan *c, void *a, long n, vlong off)
{
	Cmdtab *ct;
	Cmdbuf *cb;

	if(c->qid.path == Qio){
		if(reg == nil)
			error("region not configured");
		return acpiregio(reg, a, n, off, 1);
	}
	if(c->qid.path != Qctl)
		error(Eperm);
	cb = parsecmd(a, n);
	if(waserror()){
		free(cb);
		nexterror();
	}
	ct = lookupcmd(cb, ctls, nelem(ctls));
	if(ct->index < 0 || ct->index >= nelem(hwctls))
		panic("p9am: hwwrite bug");
	hwctls[ct->index](cb);
	poperror();
	free(cb);
	return n;
}

Dev p9amdevtab = {
	L'9',
	"9am",

	devreset,
	hwinit,
	devshutdown,
	hwattach,
	hwwalk,
	hwstat,
	hwopen,
	devcreate,
	hwclose,
	hwread,
	devbread,
	hwwrite,
	devbwrite,
	devremove,
	devwstat,
};
