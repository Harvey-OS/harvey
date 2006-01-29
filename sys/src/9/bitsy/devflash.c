#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"../port/error.h"

/*
 *  on the bitsy, all 32 bit accesses to flash are mapped to two 16 bit
 *  accesses, one to the low half of the chip and the other to the high
 *  half.  Therefore for all command accesses, ushort indices in the
 *  manuals turn into ulong indices in our code.  Also, by copying all
 *  16 bit commands to both halves of a 32 bit command, we erase 2
 *  sectors for each request erase request.
 */

#define mirror(x) (((x)<<16)|(x))

/* this defines a contiguous set of erase blocks of one size */
typedef struct FlashRegion FlashRegion;
struct FlashRegion
{
	ulong	addr;		/* start of region */
	ulong	end;		/* end of region + 1 */
	ulong	n;		/* number of blocks */
	ulong	size;		/* size of each block */
};

/* this defines a particular access algorithm */
typedef struct FlashAlg FlashAlg;
struct FlashAlg
{
	int	id;
	char	*name;
	void	(*identify)(void);	/* identify device */
	void	(*erase)(ulong);	/* erase a region */
	void	(*write)(void*, long, ulong);	/* write a region */
};

static void	ise_id(void);
static void	ise_erase(ulong);
static void	ise_write(void*, long, ulong);

static void	afs_id(void);
static void	afs_erase(ulong);
static void	afs_write(void*, long, ulong);

static ulong	blockstart(ulong);
static ulong	blockend(ulong);

FlashAlg falg[] =
{
	{ 1,	"Intel/Sharp Extended",	ise_id, ise_erase, ise_write	},
	{ 2,	"AMD/Fujitsu Standard",	afs_id, afs_erase, afs_write	},
};

struct
{
	RWlock;
	ulong		*p;
	ushort		algid;		/* access algorithm */
	FlashAlg	*alg;
	ushort		manid;		/* manufacturer id */
	ushort		devid;		/* device id */
	ulong		size;		/* size in bytes */
	int		wbsize;		/* size of write buffer */ 
	ulong		nr;		/* number of regions */
	uchar		bootprotect;
	FlashRegion	r[32];
} flash;

enum
{
	Maxwchunk=	1024,	/* maximum chunk written by one call to falg->write */
};

/*
 *  common flash interface
 */
static uchar
cfigetc(int off)
{
	uchar rv;

	flash.p[0x55] = mirror(0x98);
	rv = flash.p[off];
	flash.p[0x55] = mirror(0xFF);
	return rv;
}

static ushort
cfigets(int off)
{
	return (cfigetc(off+1)<<8)|cfigetc(off);
}

static ulong
cfigetl(int off)
{
	return (cfigetc(off+3)<<24)|(cfigetc(off+2)<<16)|
		(cfigetc(off+1)<<8)|cfigetc(off);
}

static void
cfiquery(void)
{
	uchar q, r, y;
	ulong x, addr;

	q = cfigetc(0x10);
	r = cfigetc(0x11);
	y = cfigetc(0x12);
	if(q != 'Q' || r != 'R' || y != 'Y'){
		print("cfi query failed: %ux %ux %ux\n", q, r, y);
		return;
	}
	flash.algid = cfigetc(0x13);
	flash.size = 1<<(cfigetc(0x27)+1);
	flash.wbsize = 1<<(cfigetc(0x2a)+1);
	flash.nr = cfigetc(0x2c);
	if(flash.nr > nelem(flash.r)){
		print("cfi reports > %d regions\n", nelem(flash.r));
		flash.nr = nelem(flash.r);
	}
	addr = 0;
	for(q = 0; q < flash.nr; q++){
		x = cfigetl(q+0x2d);
		flash.r[q].size = 2*256*(x>>16);
		flash.r[q].n = (x&0xffff)+1;
		flash.r[q].addr = addr;
		addr += flash.r[q].size*flash.r[q].n;
		flash.r[q].end = addr;
	}
}

/*
 *  flash device interface
 */

enum
{
	Qtopdir,
	Q2nddir,
	Qfctl,
	Qfdata,

	Maxpart= 8,
};


typedef struct FPart FPart;
struct FPart
{
	char	*name;
	char	*ctlname;
	ulong	start;
	ulong	end;
};
static FPart	part[Maxpart];

#define FQID(p,q)	((p)<<8|(q))
#define FTYPE(q)	((q) & 0xff)
#define FPART(q)	(&part[(q) >>8])

static int
gen(Chan *c, char*, Dirtab*, int, int i, Dir *dp)
{
	Qid q;
	FPart *fp;

	q.vers = 0;

	/* top level directory contains the name of the network */
	if(c->qid.path == Qtopdir){
		switch(i){
		case DEVDOTDOT:
			q.path = Qtopdir;
			q.type = QTDIR;
			devdir(c, q, "#F", 0, eve, DMDIR|0555, dp);
			break;
		case 0:
			q.path = Q2nddir;
			q.type = QTDIR;
			devdir(c, q, "flash", 0, eve, DMDIR|0555, dp);
			break;
		default:
			return -1;
		}
		return 1;
	}

	/* second level contains all partitions and their control files */
	switch(i) {
	case DEVDOTDOT:
		q.path = Qtopdir;
		q.type = QTDIR;
		devdir(c, q, "#F", 0, eve, DMDIR|0555, dp);
		break;
	default:
		if(i >= 2*Maxpart)
			return -1;
		fp = &part[i>>1];
		if(fp->name == nil)
			return 0;
		if(i & 1){
			q.path = FQID(i>>1, Qfdata);
			q.type = QTFILE;
			devdir(c, q, fp->name, fp->end-fp->start, eve, 0660, dp);
		} else {
			q.path = FQID(i>>1, Qfctl);
			q.type = QTFILE;
			devdir(c, q, fp->ctlname, 0, eve, 0660, dp);
		}
		break;
	}
	return 1;
}

static FPart*
findpart(char *name)
{
	int i;

	for(i = 0; i < Maxpart; i++)
		if(part[i].name != nil && strcmp(name, part[i].name) == 0)
			break;
	if(i >= Maxpart)
		return nil;
	return &part[i];
}

static void
addpart(FPart *fp, char *name, ulong start, ulong end)
{
	int i;
	char ctlname[64];

	if(fp == nil){
		if(start >= flash.size || end > flash.size)
			error(Ebadarg);
	} else {
		start += fp->start;
		end += fp->start;
		if(start >= fp->end || end > fp->end)
			error(Ebadarg);
	}
	if(blockstart(start) != start)
		error("must start on erase boundary");
	if(blockstart(end) != end && end != flash.size)
		error("must end on erase boundary");

	fp = findpart(name);
	if(fp != nil)
		error(Eexist);
	for(i = 0; i < Maxpart; i++)
		if(part[i].name == nil)
			break;
	if(i == Maxpart)
		error("no more partitions");
	fp = &part[i];
	kstrdup(&fp->name, name);
	snprint(ctlname, sizeof ctlname, "%sctl", name);
	kstrdup(&fp->ctlname, ctlname);
	fp->start = start;
	fp->end = end;
}

static void
rempart(FPart *fp)
{
	char *p, *cp;

	p = fp->name;
	fp->name = nil;
	cp = fp->ctlname;
	fp->ctlname = nil;
	free(p);
	free(cp);
}

void
flashinit(void)
{
	int i;

	flash.p = (ulong*)FLASHZERO;
	cfiquery();
	for(i = 0; i < nelem(falg); i++)
		if(flash.algid == falg[i].id){
			flash.alg = &falg[i];
			(*flash.alg->identify)();
			break;
		}
	flash.bootprotect = 1;

	addpart(nil, "flash", 0, flash.size);
}

static Chan*
flashattach(char* spec)
{
	return devattach('F', spec);
}

static Walkqid*
flashwalk(Chan *c, Chan *nc, char **name, int nname)
{
	return devwalk(c, nc, name, nname, nil, 0, gen);
}

static int	 
flashstat(Chan *c, uchar *db, int n)
{
	return devstat(c, db, n, nil, 0, gen);
}

static Chan*
flashopen(Chan* c, int omode)
{
	omode = openmode(omode);
	if(strcmp(up->user, eve)!=0)
		error(Eperm);
	return devopen(c, omode, nil, 0, gen);
}

static void	 
flashclose(Chan*)
{
}

static long
flashctlread(FPart *fp, void* a, long n, vlong off)
{
	char *buf, *p, *e;
	int i;
	ulong addr, end;

	buf = smalloc(1024);
	e = buf + 1024;
	p = seprint(buf, e, "0x%-9lux 0x%-9x 0x%-9ux 0x%-9ux\n", fp->end-fp->start,
		flash.wbsize, flash.manid, flash.devid);
	addr = fp->start;
	for(i = 0; i < flash.nr && addr < fp->end; i++)
		if(flash.r[i].addr <= addr && flash.r[i].end > addr){
			if(fp->end <= flash.r[i].end)
				end = fp->end;
			else
				end = flash.r[i].end;
			p = seprint(p, e, "0x%-9lux 0x%-9lux 0x%-9lux\n", addr,
				(end-addr)/flash.r[i].size, flash.r[i].size);
			addr = end;
		}
	n = readstr(off, a, n, buf);
	free(buf);
	return n;
}

static long
flashdataread(FPart *fp, void* a, long n, vlong off)
{
	rlock(&flash);
	if(waserror()){
		runlock(&flash);
		nexterror();
	}
	if(fp->name == nil)
		error("partition vanished");
	if(!iseve())
		error(Eperm);
	off += fp->start;
	if(off >= fp->end)
		n = 0;
	if(off+n >= fp->end)
		n = fp->end - off;
	if(n > 0)
		memmove(a, ((uchar*)FLASHZERO)+off, n);
	runlock(&flash);
	poperror();

	return n;
}

static long	 
flashread(Chan* c, void* a, long n, vlong off)
{
	int t;

	if(c->qid.type == QTDIR)
		return devdirread(c, a, n, nil, 0, gen);
	t = FTYPE(c->qid.path);
	switch(t){
	default:
		error(Eperm);
	case Qfctl:
		n = flashctlread(FPART(c->qid.path), a, n, off);
		break;
	case Qfdata:
		n = flashdataread(FPART(c->qid.path), a, n, off);
		break;
	}
	return n;
}

static void
bootprotect(ulong addr)
{
	FlashRegion *r;

	if(flash.bootprotect == 0)
		return;
	if(flash.nr == 0)
		error("writing over boot loader disallowed");
	r = flash.r;
	if(addr >= r->addr && addr < r->addr + r->size)
		error("writing over boot loader disallowed");
}

static ulong
blockstart(ulong addr)
{
	FlashRegion *r, *e;
	ulong x;

	r = flash.r;
	for(e = &flash.r[flash.nr]; r < e; r++)
		if(addr >= r->addr && addr < r->end){
			x = addr - r->addr;
			x /= r->size;
			return r->addr + x*r->size;
		}
			
	return (ulong)-1;
}

static ulong
blockend(ulong addr)
{
	FlashRegion *r, *e;
	ulong x;

	r = flash.r;
	for(e = &flash.r[flash.nr]; r < e; r++)
		if(addr >= r->addr && addr < r->end){
			x = addr - r->addr;
			x /= r->size;
			return r->addr + (x+1)*r->size;
		}
			
	return (ulong)-1;
}

static long
flashctlwrite(FPart *fp, char *p, long n)
{
	Cmdbuf *cmd;
	ulong off;

	if(fp == nil)
		panic("flashctlwrite");

	cmd = parsecmd(p, n);
	wlock(&flash);
	if(waserror()){
		wunlock(&flash);
		nexterror();
	}
	if(strcmp(cmd->f[0], "erase") == 0){
		switch(cmd->nf){
		case 2:
			/* erase a single block in the partition */
			off = atoi(cmd->f[1]);
			off += fp->start;
			if(off >= fp->end)
				error("region not in partition");
			if(off != blockstart(off))
				error("erase must be a block boundary");
			bootprotect(off);
			(*flash.alg->erase)(off);
			break;
		case 1:
			/* erase the whole partition */
			bootprotect(fp->start);
			for(off = fp->start; off < fp->end; off = blockend(off))
				(*flash.alg->erase)(off);
			break;
		default:
			error(Ebadarg);
		}
	} else if(strcmp(cmd->f[0], "add") == 0){
		if(cmd->nf != 4)
			error(Ebadarg);
		addpart(fp, cmd->f[1], strtoul(cmd->f[2], nil, 0), strtoul(cmd->f[3], nil, 0));
	} else if(strcmp(cmd->f[0], "remove") == 0){
		rempart(fp);
	} else if(strcmp(cmd->f[0], "protectboot") == 0){
		if(cmd->nf == 0 || strcmp(cmd->f[1], "off") != 0)
			flash.bootprotect = 1;
		else
			flash.bootprotect = 0;
	} else
		error(Ebadarg);
	poperror();
	wunlock(&flash);
	free(cmd);

	return n;
}

static long
flashdatawrite(FPart *fp, uchar *p, long n, long off)
{
	uchar *end;
	int m;
	int on;
	long ooff;
	uchar *buf;

	if(fp == nil)
		panic("flashctlwrite");

	buf = nil;
	wlock(&flash);
	if(waserror()){
		wunlock(&flash);
		if(buf != nil)
			free(buf);
		nexterror();
	}

	if(fp->name == nil)
		error("partition vanished");
	if(!iseve())
		error(Eperm);

	/* can't cross partition boundaries */
	off += fp->start;
	if(off >= fp->end || off+n > fp->end || n <= 0)
		error(Ebadarg);

	/* make sure we're not writing the boot sector */
	bootprotect(off);

	on = n;

	/*
	 *  get the data into kernel memory to avoid faults during writing.
	 *  if write is not on a quad boundary or not a multiple of 4 bytes,
	 *  extend with data already in flash.
	 */
	buf = smalloc(n+8);
	m = off & 3;
	if(m){
		*(ulong*)buf = flash.p[(off)>>2];
		n += m;
		off -= m;
	}
	if(n & 3){
		n -= n & 3;
		*(ulong*)(&buf[n]) = flash.p[(off+n)>>2];
		n += 4;
	}
	memmove(&buf[m], p, on);

	/* (*flash.alg->write) can't cross blocks */
	ooff = off;
	p = buf;
	for(end = p + n; p < end; p += m){
		m = blockend(off) - off;
		if(m > end - p)
			m = end - p;
		if(m > Maxwchunk)
			m = Maxwchunk;
		(*flash.alg->write)(p, m, off);
		off += m;
	}

	/* make sure write succeeded */
	if(memcmp(buf, &flash.p[ooff>>2], n) != 0)
		error("written bytes don't match");

	wunlock(&flash);
	free(buf);
	poperror();

	return on;
}

static long	 
flashwrite(Chan* c, void* a, long n, vlong off)
{
	int t;

	if(c->qid.type == QTDIR)
		error(Eperm);

	if(!iseve())
		error(Eperm);

	t = FTYPE(c->qid.path);
	switch(t){
	default:
		panic("flashwrite");
	case Qfctl:
		n = flashctlwrite(FPART(c->qid.path), a, n);
		break;
	case Qfdata:
		n = flashdatawrite(FPART(c->qid.path), a, n, off);
		break;
	}
	return n;
}

Dev flashdevtab = {
	'F',
	"flash",

	devreset,
	flashinit,
	devshutdown,
	flashattach,
	flashwalk,
	flashstat,
	flashopen,
	devcreate,
	flashclose,
	flashread,
	devbread,
	flashwrite,
	devbwrite,
	devremove,
	devwstat,
};

enum
{
	/* status register */
	ISEs_lockerr=		1<<1,
	ISEs_powererr=		1<<3,
	ISEs_progerr=		1<<4,
	ISEs_eraseerr=		1<<5,
	ISEs_ready=		1<<7,
	ISEs_err= (ISEs_lockerr|ISEs_powererr|ISEs_progerr|ISEs_eraseerr),

	/* extended status register */
	ISExs_bufavail=		1<<7,
};



/* intel/sharp extended command set */
static void
ise_reset(void)
{
	flash.p[0x55] = mirror(0xff);	/* reset */
}
static void
ise_id(void)
{
	ise_reset();
	flash.p[0x555] = mirror(0x90);	/* uncover vendor info */
	flash.manid = flash.p[00];
	flash.devid = flash.p[01];
	ise_reset();
}
static void
ise_clearerror(void)
{
	flash.p[0x100] = mirror(0x50);

}
static void
ise_error(int bank, ulong status)
{
	char err[64];

	if(status & (ISEs_lockerr)){
		sprint(err, "flash%d: block locked %lux", bank, status);
		error(err);
	}
	if(status & (ISEs_powererr)){
		sprint(err, "flash%d: low prog voltage %lux", bank, status);
		error(err);
	}
	if(status & (ISEs_progerr|ISEs_eraseerr)){
		sprint(err, "flash%d: i/o error %lux", bank, status);
		error(err);
	}
}
static void
ise_erase(ulong addr)
{
	ulong start;
	ulong x;

	addr >>= 2;	/* convert to ulong offset */

	flashprogpower(1);
	flash.p[addr] = mirror(0x20);
	flash.p[addr] = mirror(0xd0);
	start = m->ticks;
	do {
		x = flash.p[addr];
		if((x & mirror(ISEs_ready)) == mirror(ISEs_ready))
			break;
	} while(TK2MS(m->ticks-start) < 1500);
	flashprogpower(0);

	ise_clearerror();
	ise_error(0, x);
	ise_error(1, x>>16);

	ise_reset();
}
/*
 *  the flash spec claimes writing goes faster if we use
 *  the write buffer.  We fill the write buffer and then
 *  issue the write request.  After the write request,
 *  subsequent reads will yield the status register.
 *
 *  returns the status, even on timeouts.
 *
 *  NOTE: I tried starting back to back buffered writes
 *	without reading the status in between, as the
 *	flowchart in the intel data sheet suggests.
 *	However, it always responded with an illegal
 *	command sequence, so I must be missing something.
 *	If someone learns better, please email me, though
 *	I doubt it will be much faster. -  presotto@bell-labs.com
 */
static int
ise_wbwrite(ulong *p, int n, ulong off, ulong baddr, ulong *status)
{
	ulong x, start;
	int i;
	int s;

	/* put flash into write buffer mode */
	start = m->ticks;
	for(;;) {
		s = splhi();
		/* request write buffer mode */
		flash.p[baddr] = mirror(0xe8);

		/* look at extended status reg for status */
		if((flash.p[baddr] & mirror(1<<7)) == mirror(1<<7))
			break;
		splx(s);

		/* didn't work, keep trying for 2 secs */
		if(TK2MS(m->ticks-start) > 2000){
			/* set up to read status */
			flash.p[baddr] = mirror(0x70);
			*status = flash.p[baddr];
			pprint("write buffered cmd timed out\n");
			return -1;
		}
	}

	/* fill write buffer */
	flash.p[baddr] = mirror(n-1);
	for(i = 0; i < n; i++)
		flash.p[off+i] = *p++;

	/* program from buffer */
	flash.p[baddr] = mirror(0xd0);
	splx(s);

	/* wait till the programming is done */
	start = m->ticks;
	for(;;) {
		x = *status = flash.p[baddr];	/* read status register */
		if((x & mirror(ISEs_ready)) == mirror(ISEs_ready))
			break;
		if(TK2MS(m->ticks-start) > 2000){
			pprint("read status timed out\n");
			return -1;
		}
	}
	if(x & mirror(ISEs_err))
		return -1;

	return n;
}
static void
ise_write(void *a, long n, ulong off)
{
	ulong *p, *end;
	int i, wbsize;
	ulong x, baddr;

	/* everything in terms of ulongs */
	wbsize = flash.wbsize>>2;
	baddr = blockstart(off);
	off >>= 2;
	n >>= 2;
	p = a;
	baddr >>= 2;

	/* first see if write will succeed */
	for(i = 0; i < n; i++)
		if((p[i] & flash.p[off+i]) != p[i])
			error("flash needs erase");

	if(waserror()){
		ise_reset();
		flashprogpower(0);
		nexterror();
	}
	flashprogpower(1);

	/*
	 *  use the first write to reach
 	 *  a write buffer boundary.  the intel maunal
	 *  says writes startng at wb boundaries
	 *  maximize speed.
	 */
	i = wbsize - (off & (wbsize-1));
	for(end = p + n; p < end;){
		if(i > end - p)
			i = end - p;

		if(ise_wbwrite(p, i, off, baddr, &x) < 0)
			break;

		off += i;
		p += i;
		i = wbsize;
	}

	ise_clearerror();
	ise_error(0, x);
	ise_error(1, x>>16);

	ise_reset();
	flashprogpower(0);
	poperror();
}

/* amd/fujitsu standard command set
 *	I don't have an amd chipset to work with
 *	so I'm loathe to write this yet.  If someone
 *	else does, please send it to me and I'll
 *	incorporate it -- presotto@bell-labs.com
 */
static void
afs_reset(void)
{
	flash.p[0x55] = mirror(0xf0);	/* reset */
}
static void
afs_id(void)
{
	afs_reset();
	flash.p[0x55] = mirror(0xf0);	/* reset */
	flash.p[0x555] = mirror(0xaa);	/* query vendor block */
	flash.p[0x2aa] = mirror(0x55);
	flash.p[0x555] = mirror(0x90);
	flash.manid = flash.p[00];
	afs_reset();
	flash.p[0x555] = mirror(0xaa);	/* query vendor block */
	flash.p[0x2aa] = mirror(0x55);
	flash.p[0x555] = mirror(0x90);
	flash.devid = flash.p[01];
	afs_reset();
}
static void
afs_erase(ulong)
{
	error("amd/fujistsu erase not implemented");
}
static void
afs_write(void*, long, ulong)
{
	error("amd/fujistsu write not implemented");
}
