#include	"all.h"
#include	"io.h"

#define	SCSInone	SCSIread
#define	NLUN		2
#define	NSHLV		50
#define	NPLAT		(NSHLV*2)
#define	FIXEDSIZE	546000
#define	TWORM		MINUTE(2)
#define	THYSTER		SECOND(2)

typedef	struct	Plat	Plat;
struct	Plat
{
	char	status;		/* Sunload, etc */
	char	lun;		/* if loaded, where */
	long	time;		/* time since last access, to unspin */
	long	stime;		/* time since last spinup, for hysteresis */
	long	nblock;		/* number of native blocks */
	long	block;		/* bytes per native block */
	long	mult;		/* multiplier to get plan9 blocks */
	long	max;		/* max size in plan9 blocks */
};
static
struct
{
	Plat	plat[NPLAT];
	char	offline[NLUN];
	int	shinit;		/* beginning of time flag */
	int	active;		/* flag to slow down wormcopy */
	Device	jagu;		/* current lun */
	QLock;
} w;

enum
{
	Sunload,	/* not assigned a lun, on the shelf */
	Sstop,		/* loaded, but not spinning */
	Sstart,		/* loaded and spinning */
	Sempty,		/* no disk */
};


static	int	wormsense(int);
static	Plat*	wormunit(Device);
static	void	shelves(void);
static	void	waitworm(void);
static	void	prshit(void);
static	int	ascsiio(int, uchar*, int, void*, int);
static	void	cmd_wormcp(int, char*[]);
static	void	cmd_wormeject(int, char*[]);
static	void	cmd_wormingest(int, char*[]);
static	void	cmd_wormoffline(int, char*[]);
static	void	cmd_wormonline(int, char*[]);
static	void	wcpinit(void);
static	int	freelun(int, int);
static	int	wormcache(Device, long, void*);
static	int	bestlun(int plat);

/*
 * mounts and spins up the device
 *	locks the structure
 */
static
Plat*
wormunit(Device d)
{
	int p, s, lun;
	Plat *v;
	uchar cmd[10], buf[8];
	Drive *dr;

loop:
	qlock(&w);
	if(!w.shinit) {
		w.name = "worm";
		dr = scsidrive(d);
		if(dr == 0)
			panic("worm: jagu %D", d);
		w.jagu = d;
		waitworm();
		shelves();

		cmd_install("wormcp", "funit tunit [nblock] -- worm to worm copy", cmd_wormcp);
		cmd_install("wormeject", "unit -- shelf to outside", cmd_wormeject);
		cmd_install("wormingest", "unit -- outside to shelf", cmd_wormingest);
		cmd_install("wormoffline", "unit -- disable lun", cmd_wormoffline);
		cmd_install("wormonline", "unit -- enable lun", cmd_wormonline);
		wcpinit();

		w.shinit = 1;
	}
	if(w.jagu.ctrl != d.ctrl || w.jagu.unit != d.unit) {
		print("worm: two juke box units %D %D\n", d, w.jagu);
		goto sbad;
	}
	p = d.part;
	if(p < 0 || p > NPLAT) {
		print("wormunit partition %D\n", d);
		goto sbad;
	}

	/*
	 * if disk is unloaded, must load it
	 * into next (circular) logical unit
	 */
	v = &w.plat[p];
	if(v->status == Sunload) {

		lun = bestlun(p);
		if(lun < 0) {
			qunlock(&w);
			waitsec(1000);
			goto loop;
		}

		v->lun = lun;

		print("worm: set %d; lun %d\n", p, v->lun);
		memset(cmd, 0, 6);
		cmd[0] = 0xd6;	/* disk set */
		cmd[1] = v->lun << 5;
		cmd[3] = p << 1;
		if(p >= NSHLV)
			cmd[3] = ((p-NSHLV) << 1) | 1;
		s = ascsiio(SCSInone, cmd, 6, buf, 0);
		if(s)
			goto sbad;
		v->status = Sstop;

		memset(cmd, 0, 6);
		cmd[0] = 0x15;	/* mode select */
		cmd[1] = v->lun << 5;
		cmd[4] = 6;
		memset(buf, 0, 6);
		buf[2] = 1;	/* ebc */
		buf[4] = 7;	/* ealt,evrf,edtre */
		s = ascsiio(SCSIwrite, cmd, 6, buf, 6);
		if(s)
			goto sbad;
	}
	if(v->status == Sstop) {
		print("worm: start %d; lun %d\n", p, v->lun);
		memset(cmd, 0, 6);
		cmd[0] = 0x1b;	/* disk start */
		cmd[1] = v->lun << 5;
		cmd[4] = 1;
		s = ascsiio(SCSInone, cmd, 6, buf, 0);
		if(s)
			goto sbad;
		v->status = Sstart;
		v->stime = toytime();
	}

	if(v->status != Sstart) {
		if(v->status == Sempty)
			print("worm: unit empty %D\n", d);
		else
			print("worm: not started %D\n", d);
		goto sbad;
	}

	v->time = toytime();
	if(v->block)
		return v;

	/*
	 * capacity command
	 */
	memset(cmd, 0, sizeof(cmd));
	cmd[0] = 0x25;	/* read capacity */
	cmd[1] = v->lun << 5;
	s = ascsiio(SCSIread, cmd, sizeof(cmd), buf, sizeof(buf));
	if(s)
		goto sbad;

	v->nblock =
		(buf[0]<<24) |
		(buf[1]<<16) |
		(buf[2]<<8) |
		(buf[3]<<0);
	v->block =
		(buf[4]<<24) |
		(buf[5]<<16) |
		(buf[6]<<8) |
		(buf[7]<<0);
	v->mult =
		(RBUFSIZE + v->block - 1) /
		v->block;
	v->max =
		(v->nblock + 1) / v->mult;

	print("	drive %D: lun %d\n", d, v->lun);
	print("		%ld blocks at %ld bytes each\n",
		v->nblock, v->block);
	print("		%ld logical blocks at %d bytes each\n",
		v->max, RBUFSIZE);
	print("		%ld multiplier\n",
		v->mult);

	if(FIXEDSIZE && v->max != FIXEDSIZE)
		panic("size wrong\n");
	return v;

sbad:
	qunlock(&w);
	prshit();
	return 0;
}

/*
 * called periodically
 * to stop drives that have not
 * been used in a while
 */
void
wormprobe(void)
{
	int i;
	long t;
	Plat *x;
	uchar cmd[10], buf[8];

	t = toytime() - TWORM;
	x = &w.plat[0];
	for(i=0; i<NPLAT; i++, x++) {
		if(x->status == Sstart && t > x->time) {
			qlock(&w);
			if(x->status == Sstart && t > x->time) {
				print("worm: stop %d; lun %d\n",
					i, x->lun);
				memset(cmd, 0, 6);
				cmd[0] = 0x1b;	/* disk stop */
				cmd[1] = x->lun << 5;
				ascsiio(SCSInone, cmd, 6, buf, 0);
				x->status = Sstop;
			}
			qunlock(&w);
		}
	}
}

static
int
freelun(int plat, int lun)
{
	Plat *x;
	int i, s;
	uchar cmd[10], buf[8];

	/*
	 * release all disks that have given lun.
	 * release all disks that conflict with sides
	 */
	plat += NSHLV;
	if(plat >= NPLAT)
		plat -= NPLAT;

	x = &w.plat[0];
	for(i=0; i<NPLAT; i++, x++) {
		if(x->lun != lun && plat != i)
			continue;
		if(x->status == Sstart) {
			print("worm: stop %d; lun %d\n",
				i, x->lun);
			memset(cmd, 0, 6);
			cmd[0] = 0x1b;	/* disk stop */
			cmd[1] = x->lun << 5;
			s = ascsiio(SCSInone, cmd, 6, buf, 0);
			if(s)
				return 1;
			x->status = Sstop;
		}
		if(x->status == Sstop) {
			print("worm: release %d; lun %d\n",
				i, x->lun);
			memset(cmd, 0, 6);
			cmd[0] = 0xd7;	/* disk release */
			cmd[1] = x->lun << 5;
			s = ascsiio(SCSInone, cmd, 6, buf, 0);
			if(s)
				return 1;
			x->status = Sunload;
		}
	}
	return 0;
}

static
int
bestlun(int plat)
{
	Plat *x, *bx[NLUN];
	int i, s, lun;
	long t;

	/* build table of what platters on what luns */
	for(i=0; i<NLUN; i++)
		bx[i] = 0;

	x = &w.plat[0];
	for(i=0; i<NPLAT; i++, x++) {
		s = x->status;
		if(s == Sstart || s == Sstop) {
			lun = x->lun;
			if(lun >= 0 && lun < NLUN)
				bx[lun] = x;
		}
	}

	/*
	 * find oldest lun, but must be
	 * at least THYSTER old.
	 */
	t = toytime() - THYSTER;
	lun = -1;
	for(i=0; i<NLUN; i++) {
		x = bx[i];
		if(x == 0) {
			if(w.offline[i])
				continue;
			lun = i;
			break;
		}
		if(w.offline[i]) {
			freelun(plat, i);
			continue;
		}
		if(x->stime < t) {
			lun = i;
			t = x->stime;
		}
	}

	if(lun >= 0)
		freelun(plat, lun);
	return lun;
}

long
wormsize(Device d)
{
	Plat *v;
	long size;

	if(FIXEDSIZE)
		return FIXEDSIZE;

	v = wormunit(d);
	if(v == 0)
		return 0;
	size = v->max;
	qunlock(&w);
	return size;
}

int
wormiocmd(Device d, int io, long b, void *c)
{
	int s, i;
	Plat *v;
	long l, m;
	uchar cmd[10];

	v = wormunit(d);
	if(v == 0)
		return 0x71;
	w.active = 1;
	if(b >= v->max) {
		qunlock(&w);
		print("worm: wormiocmd out of range %D(%ld)\n", d, b);
		return 0x071;
	}

	cmd[0] = 0x28;		/* extended read */
	if(io != SCSIread)
		cmd[0] = 0x2a;	/* extended write */
	cmd[1] = v->lun << 5;

	m = v->mult;
	l = b * m;
	cmd[2] = l>>24;
	cmd[3] = l>>16;
	cmd[4] = l>>8;
	cmd[5] = l;
	cmd[6] = 0;

	cmd[7] = m>>8;
	cmd[8] = m;
	cmd[9] = 0;

	s = ascsiio(io, cmd, 10, c, RBUFSIZE);
	if(s == 0x61 && io != SCSIread) {
		cmd[7] = 0;
		cmd[8] = 1;
		for(i=0; i<m; i++) {
			cmd[2] = l>>24;
			cmd[3] = l>>16;
			cmd[4] = l>>8;
			cmd[5] = l;
			ascsiio(io, cmd, 10, c, v->block);
			l += 1;
			c = (char*)c + v->block;
		}
	}
	qunlock(&w);
	return s;
}

int
exchgerr(int s)
{
	if(s == 0x42 || s == 0x46 || s == 0x47)
		return 1;
	return 0;
}

/* BOTCH - indirect level to cope with crappy sony drive */
int
wormiocmdx(Device d, int io, long b, void *c)
{
	int s, i;
	Device xd;
	Plat *v;

	s = 0;
	for(i=0; i<4; i++) {
		s = wormiocmd(d, io, b, c);
		if(!exchgerr(s))
			break;

		/* provoke disk exchange */
		xd = d;
		xd.part = i;
		v = wormunit(xd);
		if(v)
			qunlock(&w);
	}
	return s;
}

int
wormread(Device d, long b, void *c)
{
	int s;

	if(wormcache(d, b, c)) {
		cons.nwormhit++;
		return 0;
	}
	cons.nwormmiss++;
	s = wormiocmdx(d, SCSIread, b, c);
	if(s) {
		print("wormread: %D(%ld) bad status %.4x\n", d, b, s);
		cons.nwormre++;
		return s;
	}
	return 0;
}

int
wormwrite(Device d, long b, void *c)
{
	int s;

	s = wormiocmdx(d, SCSIwrite, b, c);
	if(s) {
		print("wormwrite: %D(%ld) bad status %.4x\n", d, b, s);
		cons.nwormwe++;
		return s;
	}
	return 0;
}

static
void
waitworm(void)
{
	int s;

loop:
	s = wormsense(0);
	if(s == 0x70) {
		delay(3000);
		goto loop;
	}
}

static
int
wormsense(int lun)
{
	uchar cmd[6], buf[4];
	int s;
	char *sc;

	memset(cmd, 0, 6);
	cmd[0] = 0x03;	/* request sense */
	cmd[1] = lun<<5;
	memset(buf, 0, 4);
	s = 0x70;	/* robs fault */
	if(!scsiio(w.jagu, SCSIread, cmd, 6, buf, 4))
		s = buf[0] & 0x7f;
	sc = wormscode[s];
	if(!sc)
		sc = "unknown";
	print("	sense code %.2x %s\n", s, sc);
	return s;
}

static
void
shelves(void)
{
	uchar cmd[6], buf[128], *p, *q;
	int s, i, j, pass, flag;

	pass = 0;

loop:
	flag = 0;
	memset(cmd, 0, 6);
	cmd[0] = 0x1d;	/* send diagnostics */
	cmd[4] = 10;
	memset(buf, 0, 10);
	buf[0] = 0xe2;	/* read internal status */
	s = ascsiio(SCSIwrite, cmd, 6, buf, 10);
	if(s)
		goto sbad;

	memset(cmd, 0, 6);
	cmd[0] = 0x1c;	/* recv diagnostics */
	cmd[4] = 128;
	memset(buf, 0, 128);
	s = ascsiio(SCSIread, cmd, 6, buf, 128);
	if(s)
		goto sbad;

	for(i=0; i<8; i++) {
		p = buf+16+ 4*i;
		if(p[0] & 0x80) {
			print("	lun %d drive power off\n", i);
			goto bad;
		}
		if(p[0] & 0x40) {
			if(!(p[2] & 0x80)) {
				for(j=0; j<NSHLV; j++) {
					q = buf+48+j;
					if(q[0] & 0x80)		/* empty */
						continue;
					memset(cmd, 0, 6);
					cmd[0] = 0xd7;	/* disk release */
					cmd[1] = (i << 5) | 1;
					cmd[3] = j<<1;
					print("	lun %d drive loaded, assigning shelf %d\n", i, j);
					s = ascsiio(SCSInone, cmd, 6, buf, 0);
					if(s)
						goto bad;
					q[0] = 0x40;		/* not empty and assigned */
					goto cont1;
				}
				print("	lun %d drive loaded, no return shelf\n", i);
				goto bad;
			}
			print("	lun %d drive loaded, unloading\n", i);
			flag++;

			memset(cmd, 0, 6);
			cmd[0] = 0xd7;	/* disk release */
			cmd[1] = i << 5;
			s = ascsiio(SCSInone, cmd, 6, buf, 0);
			if(s)
				goto bad;
		}
	cont1:;
	}
	for(i=0; i<NSHLV; i++) {
		p = buf+48+i;
		if(!(p[0] & 0x80))		/* empty */
			continue;
		if(p[0] & 0x40)
			continue;		/* assigned */
		print("	shelf %2d unassigned\n", i);
		flag++;

		memset(cmd, 0, 6);
		cmd[0] = 0xd6;			/* disk set */
		cmd[3] = (127<<1);
		s = ascsiio(SCSInone, cmd, 6, buf, 0);
		if(s)
			goto sbad;
	
		memset(cmd, 0, 6);
		cmd[0] = 0xd7;	/* disk release */
		cmd[1] = 1;
		cmd[3] = i << 1;
		s = ascsiio(SCSInone, cmd, 6, buf, 0);
		if(s)
			goto sbad;
	}
	if(flag) {
		if(pass) {
			print("	too many passes\n");
			goto bad;
		}
		pass++;
		goto loop;
	}
	if(buf[98])
		print("	i/o unit status: %.2x\n", buf[98]);
	if(buf[99])
		print("	carrier  status: %.2x\n", buf[99]);
	if(buf[100])
		print("	upper dr status: %.2x\n", buf[100]);
	if(buf[101])
		print("	lower dr status: %.2x\n", buf[101]);
	for(i=0; i<NSHLV; i++)
		if(!(buf[i+48] & 0x80)) {
			w.plat[i].status = Sempty;
			w.plat[NSHLV+i].status = Sempty;
			continue;
		}
	for(i=0; i<NSHLV; i++) {
		if(!(buf[i+48] & 0x80))
			continue;
		s = i;				/* first shelf with disk */
		for(i++; i<NSHLV; i++)
			if(!(buf[i+48] & 0x80))
				break;
		print("	shelves %2d thru %2d contain disks\n", s, i-1);
	}
	return;

sbad:
	wormsense(0);
bad:
	panic("cant fix worm shelves");
}

void
prshit(void)
{
	uchar cmd[6], buf[128], *p;
	int s, i;

	memset(cmd, 0, 6);
	cmd[0] = 0x1d;				/* send diagnostics */
	cmd[4] = 10;
	memset(buf, 0, 10);
	buf[0] = 0xe2;				/* read internal status */
	s = ascsiio(SCSIwrite, cmd, 6, buf, 10);
	if(s)
		goto sbad;

	memset(cmd, 0, 6);
	cmd[0] = 0x1c;				/* recv diagnostics */
	cmd[4] = 128;
	memset(buf, 0, 128);
	s = ascsiio(SCSIread, cmd, 6, buf, 128);
	if(s)
		goto sbad;

	for(i=0; i<8; i++) {
		p = buf+16+ 4*i;
		print("%d: %.2x %.2x %.2x %.2x\n", i,
			p[0], p[1], p[2], p[3]);
	}
	return;

sbad:
	wormsense(0);
}

static
int
ascsiio(int rw, uchar *param, int nparam, void *addr, int size)
{
	int s, l;

	s = scsiio(w.jagu, rw, param, nparam, addr, size);
	if(s) {
		l = (param[1]>>5) & 7;
		s = wormsense(l);
		if(s == 0x06) {
			print("unit attention, reissue\n");
			s = scsiio(w.jagu, rw, param, nparam, addr, size);
			if(s)
				s = wormsense(l);
		}
		print("ascsiio: bad status %.2x on opcode %.2x lun %d\n",
			s, param[0], l);
	}
	return s;
}

static
Device
getcatworm(void)
{
	Device d, d1;
	int i, lb, hb;

	for(i=0; i<nelem(cwdevs); i++) {
		d = cwdevs[i];
		if(d.type != Devmcat)
			continue;
		hb = d.part;
		for(lb = d.unit; lb < hb; lb++) {
			d1 = cwdevs[lb];
			if(d1.type != Devworm)
				goto miss;
		}
		return d;
	miss:;
	}
	print("getcatworm failed\n");
	return devnone;
}

/*
 * pick up a unit number from
 * the command line. flag:
 *	0 must not be in active fs
 *	1 must be in active fs
 *	2 dont care
 */
static
Device
getwormunit(char *arg, int flag)
{
	Device mc;
	int hb, lb, i;


	if(*arg == 'f') {
		flag = 2;
		arg++;
	}
	i = number(arg, -1, 10);
	if(i < 0 || i >= NPLAT) {
		print("bad unit number %s (%d)\n", arg, i);
		goto bad;
	}

	if(flag == 2)
		goto ok;

	mc = getcatworm();
	if(mc.type != Devmcat)
		goto bad;
	hb = mc.part;
	for(lb = mc.unit; lb < hb; lb++)
		if(cwdevs[lb].part == i) {
			if(flag == 0) {
				print("unit %s is active\n", arg);
				goto bad;
			}
			goto ok;
		}
	if(flag == 1) {
		print("unit %s is not active\n", arg);
		goto bad;
	}

ok:
	mc = w.jagu;
	mc.part = i;
	return mc;

bad:
	return devnone;
}

typedef	struct	Wcache	Wcache;
struct	Wcache
{
	Device	dev;
	long	off;
	uchar*	p;		/* pointer into wcp.memp */
};
enum
{
	NCACHE	= 12,
};

static
struct
{
	int	active;			/* yes, we are copying */
	uchar*	memp;			/* pointer to scratch memory */
	long	memc;			/* size of scratch memory */
	long	memb;			/* size of scratch memory in native blocks */
	Device	fr;			/* 'from' device */
	Device	to;			/* 'to' device */
	long	nblock;			/* number of native blocks */
	long	block;			/* bytes per native block */
	long	off;			/* current native block count */

	Wcache	cache[NCACHE];
	int	cachei;			/* circular index */
	int	cachen;			/* number of blocks per cache entry */
	QLock	cachelock;
} wcp;

static
int
wormcache(Device d, long b, void *c)
{
	Wcache *w;

	if(wcp.memc == 0 || wcp.memp == 0 || wcp.active)
		return 0;
	qlock(&wcp.cachelock);
	for(w=wcp.cache; w<wcp.cache+NCACHE; w++)
		if(devcmp(d, w->dev) == 0 && b >= w->off && b < w->off+wcp.cachen) {
			memmove(c, w->p+(b-w->off)*RBUFSIZE, RBUFSIZE);
			qunlock(&wcp.cachelock);
			return 1;
		}
	qunlock(&wcp.cachelock);
	return 0;
}

static
void
clearcache(void)
{
	Wcache *w;
	uchar *cp;

	wcp.cachen = wcp.memc / (RBUFSIZE * NCACHE);
	cp = wcp.memp;

	qlock(&wcp.cachelock);
	for(w=wcp.cache; w<wcp.cache+NCACHE; w++) {
		w->dev = devnone;
		w->off = 0;
		w->p = cp;
		cp += wcp.cachen * RBUFSIZE;
	}
	qunlock(&wcp.cachelock);
}

static
void
wcpinit(void)
{
	if(wcp.memp == 0) {
		wcp.memc = conf.wcpsize;
		wcp.memp = ialloc(wcp.memc, LINESIZE);
		clearcache();
	}
}

static
void
cmd_wormeject(int argc, char *argv[])
{
	Plat *v;
	Device dev;
	int s;
	uchar cmd[6], buf[6];

	if(argc <= 1) {
		print("usage: wormeject [f]unit\n");
		return;
	}
	dev = getwormunit(argv[1], 0);
	if(!devcmp(dev, devnone))
		return;

	if(dev.part >= NSHLV) {
		print("dev > NSHLV %d\n", dev.part);
		return;
	}

	v = wormunit(dev);
	if(!v)
		return;

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = 0xc0;		/* disk eject */
	cmd[1] = v->lun << 5;
	s = ascsiio(SCSInone, cmd, 6, buf, 0);
	if(s)
		print("status return from eject %x\n", s);

	v->status = Sempty;
	v->block = 0;

	v += NSHLV;
	v->status = Sempty;
	v->block = 0;

	qunlock(&w);
}

static
void
cmd_wormingest(int argc, char *argv[])
{
	Plat *v;
	Device dev;
	int s, lun;
	uchar cmd[6], buf[6];

	dev = getcatworm();
	if(dev.type != Devmcat) {
		print("device not mcat\n");
		return;
	}
	if(argc <= 1) {
		print("usage: wormingest [f]unit\n");
		return;
	}
	dev = getwormunit(argv[1], 0);
	if(!devcmp(dev, devnone))
		return;
	if(dev.part >= NSHLV) {
		print("dev > NSHLV %d\n", dev.part);
		return;
	}

loop:
	qlock(&w);

	v = &w.plat[dev.part];
	if(v->status != Sempty) {
		print("unit not empty\n");
		goto out;
	}

	lun = bestlun(dev.part);
	if(lun < 0) {
		qunlock(&w);
		waitsec(1000);
		goto loop;
	}

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = 0xd6;		/* disk set */
	cmd[1] = lun << 5;
	cmd[3] = 127 << 1;
	s = ascsiio(SCSInone, cmd, 6, buf, 0);
	if(s) {
		print("status return from disk set of mouth %x\n", s);
		goto out;
	}

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = 0xd7;		/* disk release */
	cmd[1] = (lun << 5) | 1;
	cmd[3] = dev.part << 1;
	s = ascsiio(SCSInone, cmd, 6, buf, 0);
	if(s) {
		print("status return from disk release %x\n", s);
		goto out;
	}

	v->status = Sunload;
	v->block = 0;

	v += NSHLV;
	v->status = Sunload;
	v->block = 0;

out:
	qunlock(&w);
}

static
void
cmd_wormoffline(int argc, char *argv[])
{
	int u, i;

	if(argc <= 1) {
		print("usage: wormoffline lun\n");
		return;
	}
	u = number(argv[1], -1, 10);
	if(u < 0 || u >= NLUN) {
		print("bad lun %s (%d)\n", argv[1], u);
		return;
	}
	if(w.offline[u])
		print("lun %d already offline\n", u);
	w.offline[u] = 1;
	for(i=0; i<NLUN; i++)
		if(w.offline[i] == 0)
			return;
	print("that would take all units offline\n");
	w.offline[u] = 0;
}

static
void
cmd_wormonline(int argc, char *argv[])
{
	int u;

	if(argc <= 1) {
		print("usage: wormonline lun\n");
		return;
	}
	u = number(argv[1], -1, 10);
	if(u < 0 || u >= NLUN) {
		print("bad lun %s (%d)\n", argv[1], u);
		return;
	}
	if(!w.offline[u])
		print("lun %d already online\n", u);
	w.offline[u] = 0;
}

static
void
cmd_wormcp(int argc, char *argv[])
{
	Device fr, to;
	Plat *v;

	clearcache();
	if(argc <= 1) {
		print("wcp active turned off\n");
		wcp.active = 0;
		return;
	}
	if(wcp.active) {
		print("wcp active already on\n");
		return;
	}
	if(wcp.memc == 0 || wcp.memp == 0) {
		print("memory not allocated\n");
		return;
	}

	if(argc <= 2) {
		print("argcount: wcp from to\n");
		return;
	}

	fr = getwormunit(argv[1], 1);
	to = getwormunit(argv[2], 0);
	if(!devcmp(fr, devnone) || !devcmp(to, devnone))
		return;

	wcp.off = 0;
	if(argc > 3)
		wcp.off = number(argv[3], wcp.off, 10);

	print("'from' device %D; 'to' device %D %ld\n", fr, to, wcp.off);
	wcp.fr = fr;
	wcp.to = to;

	v = wormunit(fr);
	if(v == 0)
		return;
	wcp.nblock = v->nblock;
	wcp.block = v->block;
	wcp.memb = wcp.memc / v->block;
	qunlock(&w);

	v = wormunit(to);
	if(v == 0)
		return;
	if(wcp.nblock != v->nblock) {
		print("fr and to have different number of blocks %ld %ld\n",
			wcp.nblock, v->nblock);
		qunlock(&w);
		return;
	}
	if(wcp.block != v->block) {
		print("fr and to have different size of blocks %ld %ld\n",
			wcp.nblock, v->nblock);
		qunlock(&w);
		return;
	}
	qunlock(&w);

	if(argc > 4)
		wcp.nblock = number(argv[4], wcp.nblock, 10);

	wcp.active = 1;
}

/*
 * binary search for non-blank/blank boundary
 * on every non-blank probe, read and compare.
 * assert that lb is non-blank
 */
void
wbsrch(long lb, long ub)
{
	Plat *v;
	long mb;
	int s, nbad;
	uchar cmd[10], buf[6];

	nbad = 0;

loop:
	v = wormunit(wcp.to);
	if(v == 0)
		goto stop1;

loop1:
	mb = (lb + ub) / 2;
	if(cons.flags & roflag)
		print("probe %ld\n", mb);
	if(mb <= lb) {
		wcp.off = lb+1;
		print("binary search found %ld\n", wcp.off);
		qunlock(&w);
		return;
	}

	/*
	 * non-blank search
	 */
	cmd[0] = 0x2d;	/* written sector search */
	cmd[1] = v->lun << 5;

	cmd[2] = mb>>24;
	cmd[3] = mb>>16;
	cmd[4] = mb>>8;
	cmd[5] = mb;
	cmd[6] = 0;

	cmd[7] = 0;
	cmd[8] = 1;
	cmd[9] = 0;

	s = ascsiio(SCSIread, cmd, 10, buf, 6);
	if(s) {
		print("wbsrch %.2x: non blank search\n", s);
		goto stop;
	}
	if((buf[1]&1) == 0) {
		/*
		 * it is blank, seek back
		 */
		ub = mb;
		goto loop1;
	}
	/*
	 * it is full, read and compare
	 * if match, seek forw
	 */
	cmd[0] = 0x28;	/* read */
	s = ascsiio(SCSIread, cmd, 10, wcp.memp, wcp.block);
	if(s) {
		print("wbsrch: read 'to' status %.2x\n", s);
		goto stop;
	}
	qunlock(&w);

	v = wormunit(wcp.fr);
	if(v == 0)
		goto stop1;
	cmd[0] = 0x28;	/* read */
	cmd[1] = v->lun << 5;
	s = ascsiio(SCSIread, cmd, 10, wcp.memp+wcp.block, wcp.block);
	if(s) {
		print("wbsrch: read 'fr' status %.2x\n", s);
		goto stop;
	}
	if(memcmp(wcp.memp, wcp.memp+wcp.block, wcp.block) != 0) {
		print("wbsrch: compare %ld\n", mb);
		nbad++;
		if(nbad > 2)
			goto stop;
	}
	qunlock(&w);

	lb = mb;
	goto loop;

stop:
	qunlock(&w);
stop1:
	wcp.active = 0;
}

/*
 * one wormcp pass
 * will copy one buffer across.
 * return flag is if more to do.
 */
int
dowcp(void)
{
	Plat *v;
	int s;
	long l, n, count;
	Device xd;
	uchar cmd[10], buf[6];

	if(!wcp.active)
		return 0;
	count = 0;
	goto loop0;

sonysucks:
	count++;
	if(count > 4)
		goto stop;
	qunlock(&w);

	/* provoke disk exchange */
	xd = wcp.fr;
	xd.part = count;
	v = wormunit(xd);
	if(v)
		qunlock(&w);

loop0:
	v = wormunit(wcp.fr);
	if(v == 0)
		return 0;
	if(w.active) {
		w.active = 0;
		qunlock(&w);
		return 0;
	}

loop:
	if(!wcp.active)
		goto stop;

	/*
	 * non-blank search
	 */
	if(wcp.off >= wcp.nblock) {
		print("done wormcp %D %D; offset = %ld\n",
			wcp.fr, wcp.to, wcp.off);
		goto stop;
	}
	n = 10000;
	if(wcp.off+n > wcp.nblock)
		n = wcp.nblock - wcp.off;

	cmd[0] = 0x2d;	/* written sector search */
	cmd[1] = v->lun << 5;

	l = wcp.off;
	cmd[2] = l>>24;
	cmd[3] = l>>16;
	cmd[4] = l>>8;
	cmd[5] = l;
	cmd[6] = 0;

	cmd[7] = n>>8;
	cmd[8] = n;
	cmd[9] = 0;

	s = ascsiio(SCSIread, cmd, 10, buf, 6);

	if(s) {
		if(exchgerr(s))
			goto sonysucks;
		print("dowcp: non-blank search status %.2x\n", s);
		goto stop;
	}
	if((buf[1]&1) == 0) {
		if(cons.flags & roflag)
			print("non-blank failed %ld(%ld)\n", wcp.off, n);
		wcp.off += n;
		goto loop;
	}
	l = (buf[2]<<24) | (buf[3]<<16) | (buf[4]<<8) | buf[5];
	if(cons.flags & roflag)
		print("non-blank found %ld(%ld)\n", wcp.off, l-wcp.off);
	if(l < wcp.off) {
		print("dowcp: non-blank backwards\n");
		goto stop;
	}
	wcp.off = l;

	/*
	 * blank search
	 */
	n = wcp.memb;
	if(wcp.off+n > wcp.nblock)
		n = wcp.nblock - wcp.off;

	cmd[0] = 0x2c;	/* blank sector search */
	cmd[1] = v->lun << 5;

	l = wcp.off;
	cmd[2] = l>>24;
	cmd[3] = l>>16;
	cmd[4] = l>>8;
	cmd[5] = l;
	cmd[6] = 0;

	cmd[7] = n>>8;
	cmd[8] = n;
	cmd[9] = 0;

	s = ascsiio(SCSIread, cmd, 10, buf, 6);

	if(s) {
		if(exchgerr(s))
			goto sonysucks;
		print("dowcp: blank search status %.2x\n", s);
		goto stop;
	}
	if((buf[1]&1) == 0) {
		if(cons.flags & roflag)
			print("blank failed %ld(%ld)\n", wcp.off, n);
	} else {
		l = (buf[2]<<24) | (buf[3]<<16) | (buf[4]<<8) | buf[5];
		n = l - wcp.off;
		if(cons.flags & roflag)
			print("blank found %ld(%ld)\n", wcp.off, n);
	}

	/*
	 * read
	 */
rloop:
	if(cons.flags & roflag)
		print("read %ld(%ld)\n", wcp.off, n);

	cmd[0] = 0x28;	/* read */

	cmd[7] = n>>8;
	cmd[8] = n;
	cmd[9] = 0;

	s = ascsiio(SCSIread, cmd, 10, wcp.memp, n*wcp.block);
	if(s) {
		if(exchgerr(s))
			goto sonysucks;

		if(s != 0x54 && s != 0x50 && s != 0x53) {	/* unrecov read error */
			print("dowcp: read status %.2x\n", s);
			goto stop;
		}
		if(n == 1) {
			print("dowcp: unrecov read error %ld\n", wcp.off);
			wcp.off++;
			qunlock(&w);
			return 1;
		}
		n = n/2;
		goto rloop;
	}

	/*
	 * write
	 */
	qunlock(&w);
	v = wormunit(wcp.to);
	if(v == 0)
		return 0;

	if(cons.flags & roflag)
		print("write %ld(%ld)\n", wcp.off, n);

	cmd[0] = 0x2a;	/* write */
	cmd[1] = v->lun << 5;

	s = ascsiio(SCSIwrite, cmd, 10, wcp.memp, n*wcp.block);
	if(s) {
		if(exchgerr(s))
			goto sonysucks;

		if(s == 0x61) {
			qunlock(&w);
			wbsrch(wcp.off, wcp.nblock);
			return wcp.active;
		}
		print("dowcp: write status %.2x\n", s);
		goto stop;
	}
	wcp.off += n;

	qunlock(&w);
	return 1;

stop:
	wcp.active = 0;
	qunlock(&w);
	return 0;
}
