#include	"all.h"

#define	SCSInone	SCSIread
#define	MAXDRIVE	10
#define	MAXSIDE		300
#define	FIXEDSIZE	1
#define	TWORM		MINUTE(10)
#define	THYSTER		SECOND(10)

#define	EXIT	0

typedef	struct	Side	Side;
struct	Side
{
	QLock;			/* protects loading/unloading */
	int	elem;		/* element number */
	int	drive;		/* if loaded, where */
	uchar	status;		/* Sunload, etc */
	uchar	rot;		/* if backside */
	int	ord;		/* ordinal number for labeling */

	long	time;		/* time since last access, to unspin */
	long	stime;		/* time since last spinup, for hysteresis */
	long	nblock;		/* number of native blocks */
	long	block;		/* bytes per native block */
	long	mult;		/* multiplier to get plan9 blocks */
	long	max;		/* max size in plan9 blocks */
};

typedef	struct	Juke	Juke;
struct	Juke
{
	QLock;			/* protects drive mechanism */
	Side	side[MAXSIDE];
	int	nside;		/* how many storage elements (*2 if rev) */
	int	ndrive;		/* number of transfer elements */
	int	swab;		/* data is swabbed */
	Device*	juke;		/* devworm of changer */
	Device*	drive[MAXDRIVE];/* devworm for i/o */
	long	fixedsize;	/* one size fits all */
	int	probeok;	/* wait for init to probe */

	/* geometry returned by mode sense */
	int	mt0,	nmt;
	int	se0,	nse;
	int	ie0,	nie;
	int	dt0,	ndt;
	int	rot;

	Juke*	link;
};
static	Juke*	jukelist;

enum
{
	Sempty = 0,	/* does not exist */
	Sunload,	/* on the shelf */
	Sstart,		/* loaded and spinning */
};

static	Side*	wormunit(Device*);
static	void	shelves(void);
static	void	mmove(Juke*, int, int, int, int);
static	int	bestdrive(Juke*, int);
static	void	waitready(Device*);
static	int	wormsense(Juke*, int);

/*
 * mounts and spins up the device
 *	locks the structure
 */
static
Side*
wormunit(Device *d)
{
	int p, s, drive;
	Side *v;
	Juke *w;
	Device *dev;
	uchar cmd[10], buf[8];

	w = d->private;
	p = d->wren.targ;
	if(p < 0 || p >= w->nside)
		panic("wormunit partition %Z\n", d);

	/*
	 * if disk is unloaded, must load it
	 * into next (circular) logical unit
	 */
	v = &w->side[p];
	qlock(v);
	if(v->status == Sunload) {
		for(;;) {
			qlock(w);
			drive = bestdrive(w, p);
			if(drive >= 0)
				break;
			qunlock(w);
			waitsec(100);
		}
		print("	load  r%ld drive %Z\n", v-w->side, w->drive[drive]);
		mmove(w, w->mt0, v->elem, w->dt0+w->drive[drive]->wren.lun, v->rot);
		v->drive = drive;
		v->status = Sstart;
		v->stime = toytime();
		qunlock(w);
		waitready(w->drive[drive]);
		v->stime = toytime();
	}
	if(v->status != Sstart) {
		if(v->status == Sempty)
			print("worm: unit empty %Z\n", d);
		else
			print("worm: not started %Z\n", d);
		goto sbad;
	}

	v->time = toytime();
	if(v->block)
		return v;

	/*
	 * capacity command
	 */
	dev = w->drive[v->drive];
	memset(cmd, 0, sizeof(cmd));
	memset(buf, 0, sizeof(buf));
	cmd[0] = 0x25;	/* read capacity */
	cmd[1] = (dev->wren.lun&0x7) << 5;
	s = scsiio(w->drive[v->drive], SCSIread,
		cmd, sizeof(cmd), buf, sizeof(buf));
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

	print("	worm %Z: drive %Z\n", d, w->drive[v->drive]);
	print("		%ld blocks at %ld bytes each\n",
		v->nblock, v->block);
	print("		%ld logical blocks at %d bytes each\n",
		v->max, RBUFSIZE);
	print("		%ld multiplier\n",
		v->mult);
	if(d->type != Devlworm)
		return v;
	/* check for label */
	print("label %Z ordinal %d\n", d, v->ord);
	qunlock(v);
	return wormunit(d);

sbad:
	qunlock(v);
	panic("wormunit sbad");
	return 0;
}

static
void
waitready(Device *d)
{
	uchar cmd[6];
	int s, e;

	for(e=0;e<100;e++) {
		memset(cmd, 0, sizeof(cmd));
		cmd[1] = (d->wren.lun&0x7) << 5;
		s = scsiio(d, SCSInone, cmd, sizeof(cmd), cmd, 0);
		if(s == 0)
			break;
		waitsec(100);
	}
}

static
int
bestdrive(Juke *w, int side)
{
	Side *v, *bv[MAXDRIVE];
	int i, s, e, drive;
	long t, t0;

loop:
	/* build table of what platters on what drives */
	for(i=0; i<w->ndt; i++)
		bv[i] = 0;

	v = &w->side[0];
	for(i=0; i<w->nside; i++, v++) {
		s = v->status;
		if(s == Sstart) {
			drive = v->drive;
			if(drive >= 0 && drive < w->ndt)
				bv[drive] = v;
		}
	}

	/*
	 * find oldest drive, but must be
	 * at least THYSTER old.
	 */
	e = w->side[side].elem;
	t0 = toytime() - THYSTER;
	t = t0;
	drive = -1;
	for(i=0; i<w->ndt; i++) {
		v = bv[i];
		if(v == 0) {		/* 2nd priority: empty drive */
			if(w->drive[i] != devnone) {
				drive = i;
				t = 0;
			}
			continue;
		}
		if(v->elem == e) {	/* 1st priority: other side */
			drive = -1;
			if(v->stime < t0)
				drive = i;
			break;
		}
		if(v->stime < t) {	/* 3rd priority: by time */
			drive = i;
			t = v->stime;
		}
	}

	if(drive >= 0) {
		v = bv[drive];
		if(v) {
			qlock(v);
			if(v->status != Sstart) {
				qunlock(v);
				goto loop;
			}
			print("	unload r%ld drive %Z\n",
				v-w->side, w->drive[drive]);
			mmove(w, w->mt0, w->dt0+w->drive[drive]->wren.lun,
				v->elem, v->rot);
			v->status = Sunload;
			qunlock(v);
		}
	}
	return drive;
}

long
wormsize(Device *d)
{
	Side *v;
	Juke *w;
	long size;

	w = d->private;
	if(w->fixedsize) {
		size = w->fixedsize;
		goto out;
	}

	v = wormunit(d);
	if(v == 0)
		return 0;
	size = v->max;
	qunlock(v);
	if(FIXEDSIZE)
		w->fixedsize = size;
out:
	if(d->type == Devlworm)
		return size-1;
	return size;
}

static
int
wormiocmd(Device *d, int io, long b, void *c)
{
	Side *v;
	Juke *w;
	long l, m;
	int s;
	Device *dev;
	uchar cmd[10];

	w = d->private;
	v = wormunit(d);
	if(v == 0) {
		qunlock(v);
		return 0x71;
	}
	if(b >= v->max) {
		qunlock(v);
		print("worm: wormiocmd out of range %Z(%ld)\n", d, b);
		return 0x071;
	}

	dev = w->drive[v->drive];
	memset(cmd, 0, sizeof(cmd));
	cmd[0] = 0x28;		/* extended read */
	if(io != SCSIread) {
		cmd[0] = 0x2a;	/* extended write */
		if(w->swab)
			swab(c, 1);
	}
	cmd[1] = (dev->wren.lun&0x7) << 5;

	m = v->mult;
	l = b * m;
	cmd[2] = l>>24;
	cmd[3] = l>>16;
	cmd[4] = l>>8;
	cmd[5] = l;

	cmd[7] = m>>8;
	cmd[8] = m;

	s = scsiio(dev, io, cmd, sizeof(cmd), c, RBUFSIZE);
	if(w->swab)
		swab(c, 0);
	qunlock(v);
	return s;
}

int
wormread(Device *d, long b, void *c)
{
	int s;

	s = wormiocmd(d, SCSIread, b, c);
	if(s) {
		print("wormread: %Z(%ld) bad status #%x\n", d, b, s);
		cons.nwormre++;
		return s;
	}
	return 0;
}

int
wormwrite(Device *d, long b, void *c)
{
	int s;

	s = wormiocmd(d, SCSIwrite, b, c);
	if(s) {
		print("wormwrite: %Z(%ld) bad status #%x\n", d, b, s);
		cons.nwormwe++;
		return s;
	}
	return 0;
}

static
void
mmove(Juke *w, int trans, int from, int to, int rot)
{
	uchar cmd[10], buf[8];
	int s, u, p;

	s = 0;
	if(trans != w->mt0)
		goto bad;

	if(from >= w->se0 && from <= w->se0+w->nse && 
	   to >= w->dt0 && to < w->dt0+w->ndt) {
		p = from - w->se0;
		u = to - w->dt0;

		print("worm: set %d; lun %d\n", p, u);
		memset(cmd, 0, 6);
		cmd[0] = 0xd6;	/* disk set */
		cmd[1] = u << 5;
		cmd[3] = (p << 1) | rot;
		s = scsiio(w->juke, SCSInone, cmd, 6, buf, 0);
		if(s)
			goto bad;

		memset(cmd, 0, 6);
		cmd[0] = 0x15;	/* mode select */
		cmd[1] = u << 5;
		cmd[4] = 6;
		memset(buf, 0, 6);
		buf[2] = 1;	/* ebc */
		buf[4] = 7;	/* ealt,evrf,edtre */
		s = scsiio(w->juke, SCSIwrite, cmd, 6, buf, 6);
		if(s)
			goto bad;

		print("worm: start %d; lun %d\n", p, u);
		memset(cmd, 0, 6);
		cmd[0] = 0x1b;	/* disk start */
		cmd[1] = u << 5;
		cmd[4] = 1;
		s = scsiio(w->juke, SCSInone, cmd, 6, buf, 0);
		if(s)
			goto bad;
		return;
	}
	if(from >= w->dt0 && from <= w->dt0+w->ndt && 
	   to >= w->se0 && to < w->se0+w->nse) {
		p = to - w->se0;
		u = from - w->dt0;

		print("worm: stop %d; lun %d\n", p, u);
		memset(cmd, 0, 6);
		cmd[0] = 0x1b;	/* disk stop */
		cmd[1] = u << 5;
		s = scsiio(w->juke, SCSInone, cmd, 6, buf, 0);
		if(s)
			goto bad;

		print("worm: release %d; lun %d\n", p, u);
		memset(cmd, 0, 6);
		cmd[0] = 0xd7;	/* disk release */
		cmd[1] = u << 5;
		s = scsiio(w->juke, SCSInone, cmd, 6, buf, 0);
		if(s)
			goto bad;
		return;
	}
	print("not load or unload\n");

bad:
	print("scsio status #%x\n", s);
	print("move meduim t=%d fr=%d to=%d rot=%d", trans, from, to, rot);
	panic("mmove");

}

static
void
geometry(Juke *w)
{
	int s;

	s = wormsense(w, 0);
	while(s == 0x70) {
		delay(3000);
		s = wormsense(w, 0);
	}

	w->se0 = 0;
	w->nse = 50;
	w->dt0 = 110;
	w->ndt = 2;		/* 1-8 */
	w->mt0 = 110;
	w->nmt = 1;
	w->ie0 = 111;
	w->nie = 1;


	w->rot = 1;

	print("	mt %d %d\n", w->mt0, w->nmt);
	print("	se %d %d\n", w->se0, w->nse);
	print("	ie %d %d\n", w->ie0, w->nie);
	print("	dt %d %d\n", w->dt0, w->ndt);
	print("	rot %d\n", w->rot);
	prflush();	/**/

}

static
int
wormsense(Juke *w, int lun)
{
	uchar cmd[6], buf[4];
	int s;
	char *sc;

	memset(cmd, 0, 6);
	cmd[0] = 0x03;	/* request sense */
	cmd[1] = lun<<5;
	memset(buf, 0, 4);
	s = 0x70;	/* robs fault */
	if(!scsiio(w->juke, SCSIread, cmd, 6, buf, 4))
		s = buf[0] & 0x7f;
	sc = wormscode[s];
	if(!sc)
		sc = "unknown";
	print("	sense code %.2x %s\n", s, sc);
	return s;
}

static
void
element(Juke *w, int e)
{
	uchar cmd[6], buf[128], *p, *q;
	int s, i, j, pass, flag;

	if(e != 0)
		return;
	pass = 0;

loop:
	flag = 0;
	memset(cmd, 0, 6);
	cmd[0] = 0x1d;	/* send diagnostics */
	cmd[4] = 10;
	memset(buf, 0, 10);
	buf[0] = 0xe2;	/* read internal status */
	s = scsiio(w->juke, SCSIwrite, cmd, 6, buf, 10);
	if(s) {
		print("element: read internal status %Z: %d\n", w->juke, s);
		goto sbad;
	}

	memset(cmd, 0, 6);
	cmd[0] = 0x1c;	/* recv diagnostics */
	cmd[4] = 128;
	memset(buf, 0, 128);
	s = scsiio(w->juke, SCSIread, cmd, 6, buf, 128);
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
				for(j=0; j<w->nse; j++) {
					q = buf+48+j;
					if(q[0] & 0x80)		/* empty */
						continue;
					memset(cmd, 0, 6);
					cmd[0] = 0xd7;	/* disk release */
					cmd[1] = (i << 5) | 1;
					cmd[3] = j<<1;
					print("	lun %d drive loaded, assigning shelf %d\n", i, j);
					s = scsiio(w->juke, SCSInone, cmd, 6, buf, 0);
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
			s = scsiio(w->juke, SCSInone, cmd, 6, buf, 0);
			if(s)
				goto bad;
		}
	cont1:;
	}
	for(i=0; i<w->nse; i++) {
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
		s = scsiio(w->juke, SCSInone, cmd, 6, buf, 0);
		if(s)
			goto sbad;
	
		memset(cmd, 0, 6);
		cmd[0] = 0xd7;	/* disk release */
		cmd[1] = 1;
		cmd[3] = i << 1;
		s = scsiio(w->juke, SCSInone, cmd, 6, buf, 0);
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
	for(i=0; i<w->nse; i++) {
		w->side[i].status = Sempty;
		if(buf[i+48] & 0x80)
			w->side[i].status = Sunload;
		w->side[w->nse+i].status = w->side[i].status;
	}
	return;

sbad:
	wormsense(w, 0);
bad:
	panic("cant fix worm shelves");
}

static
void
positions(Juke *w)
{
	int i, f;

	/* mark empty shelves */
	for(i=0; i<w->nse; i++)
		element(w, w->se0+i);
	for(i=0; i<w->nmt; i++)
		element(w, w->mt0+i);
	for(i=0; i<w->nie; i++)
		element(w, w->ie0+i);
	for(i=0; i<w->ndt; i++)
		element(w, w->dt0+i);

	f = 0;
	for(i=0; i<w->nse; i++) {
		if(w->side[i].status == Sempty) {
			if(f) {
				print("r%d\n", i-1);
				f = 0;
			}
		} else {
			if(!f) {
				print("	shelves r%d-", i);
				f = 1;
			}
		}
	}
	if(f)
		print("r%d\n", i-1);
	if(EXIT)
		panic("exit");
}

static
void
jinit(Juke *w, Device *d, int o)
{
	int p;

	switch(d->type) {
	default:
		print("juke platter not devworm: %Z\n", d);
		goto bad;

	case Devmcat:
		for(d=d->cat.first; d; d=d->link)
			jinit(w, d, o++);
		break;

	case Devlworm:
		p = d->wren.targ;
		if(p < 0 || p >= w->nside)
			panic("jinit partition %Z\n", d);
		w->side[p].ord = o;

	case Devworm:
		if(d->private) {
			print("juke platter private pointer set %p\n",
				d->private);
			goto bad;
		}
		d->private = w;
		break;
	}
	return;

bad:
	panic("jinit");
}

Side*
wormi(char *arg)
{
	int i, j;
	Juke *w;
	Side *v;

	w = jukelist;
	i = number(arg, -1, 10) - 1;
	if(i < 0 || i >= w->nside) {
		print("bad unit number %s (%d)\n", arg, i+1);
		return 0;
	}
	j = i;
	if(j >= w->nse)
		j -= w->nse;
	if(j < w->nside) {
		v = &w->side[j];
		qlock(v);
		if(v->status == Sstart) {
			mmove(w, w->mt0, w->dt0+v->drive, v->elem, v->rot);
			v->status = Sunload;
		}
		qunlock(v);
	}
	j += w->nse;
	if(j < w->nside) {
		v = &w->side[j];
		qlock(v);
		if(v->status == Sstart) {
			mmove(w, w->mt0, w->dt0+v->drive, v->elem, v->rot);
			v->status = Sunload;
		}
		qunlock(v);
	}
	v = &w->side[i];
	if(v->status != Sunload) {
		print("unit not on shelve %s (%d)\n", arg, i+1);
		return 0;
	}
	return v;
}

static
void
cmd_wormeject(int argc, char *argv[])
{
	Juke *w;
	Side *v;

	if(argc <= 1) {
		print("usage: wormeject unit\n");
		return;
	}
	w = jukelist;
	v = wormi(argv[1]);
	if(v == 0)
		return;
	qlock(v);
	mmove(w, w->mt0, v->elem, w->ie0, 0);
	qunlock(v);
}

static
void
cmd_wormingest(int argc, char *argv[])
{
	Juke *w;
	Side *v;

	w = jukelist;
	if(argc <= 1) {
		print("usage: wormingest unit\n");
		return;
	}
	v = wormi(argv[1]);
	if(v == 0)
		return;
	qlock(v);
	mmove(w, w->mt0, w->ie0, v->elem, 0);
	qunlock(v);
}

void
jukeinit(Device *d)
{
	Juke *w;
	Device *xdev;
	Side *v;
	int i;

	/* j(w<changer>w<station0>...)(r<platters>) */
	xdev = d->j.j;
	if(xdev->type != Devmcat) {
		print("juke union not mcat\n");
		goto bad;
	}

	/*
	 * pick up the changer device
	 */
	xdev = xdev->cat.first;
	if(xdev->type != Devwren) {
		print("juke changer not wren %Z\n", xdev);
		goto bad;
	}
	for(w=jukelist; w; w=w->link)
		if(xdev == w->juke)
			goto found;

	/*
	 * allocate a juke structure
	 * no locking problems.
	 */
	w = ialloc(sizeof(Juke), 0);
	w->link = jukelist;
	jukelist = w;

	print("alloc juke %Z\n", xdev);
	qlock(w);
	qunlock(w);
	w->name = "juke";
	w->juke = xdev;
	geometry(w);

	/*
	 * pick up each side
	 */
	w->nside = w->nse;
	if(w->rot)
		w->nside += w->nside;
	if(w->nside > MAXSIDE) {
		print("too many sides: %d max %d\n", w->nside, MAXSIDE);
		goto bad;
	}
	for(i=0; i<w->nse; i++) {
		v = &w->side[i];
		qlock(v);
		qunlock(v);
		v->name = "shelf";
		v->elem = w->se0 + i;
		v->rot = 0;
		v->status = Sempty;
		v->time = toytime();
		if(w->rot) {
			v += w->nse;
			qlock(v);
			qunlock(v);
			v->name = "shelf";
			v->elem = w->se0 + i;
			v->rot = 1;
			v->status = Sempty;
			v->time = toytime();
		}
	}
	positions(w);

	w->ndrive = w->ndt;
	if(w->ndrive > MAXDRIVE) {
		print("ndrives truncated to %d\n", MAXDRIVE);
		w->ndrive = MAXDRIVE;
	}

	/*
	 * pick up each drive
	 */
	for(i=0; i<w->ndrive; i++)
		w->drive[i] = devnone;
	w->swab = 0;

	cmd_install("wormeject", "unit -- shelf to outside", cmd_wormeject);
	cmd_install("wormingest", "unit -- outside to shelf", cmd_wormingest);

found:
	i = 0;
	while(xdev = xdev->link) {
		if(i >= w->ndrive) {
			print("too many drives %Z\n", xdev);
			goto bad;
		}
		if(xdev->type == Devswab) {
			xdev = xdev->swab.d;
			w->swab = 1;
		}
		if(xdev->type != Devwren) {
			print("drive not devwren: %Z\n", xdev);
			goto bad;
		}
		if(w->drive[i]->type != Devnone &&
		   xdev != w->drive[i]) {
			print("double init drive %d %Z %Z\n", i, w->drive[i], xdev);
			goto bad;
		}
		w->drive[i++] = xdev;
	}

	if(i <= 0) {
		print("no drives\n");
		goto bad;
	}

	/*
	 * put w pointer in each platter
	 */
	d->private = w;
	jinit(w, d->j.m, 0);
	w->probeok = 1;
	return;

bad:
	panic("juke init");
}

int
dowcp(void)
{
	return 0;
}

/*
 * called periodically
 */
void
wormprobe(void)
{
	int i, drive;
	long t;
	Side *v;
	Juke *w;

	t = toytime() - TWORM;
	for(w=jukelist; w; w=w->link) {
		if(w->probeok == 0 || !canqlock(w))
			continue;
		for(i=0; i<w->nside; i++) {
			v = &w->side[i];
			if(!canqlock(v))
				continue;
			if(v->status == Sstart && t > v->time) {
				drive = v->drive;
				print("	time   r%ld drive %Z\n",
					v-w->side, w->drive[drive]);
				mmove(w, w->mt0, w->dt0+w->drive[drive]->wren.lun,
					v->elem, v->rot);
				v->status = Sunload;
			}
			qunlock(v);
		}
		qunlock(w);
	}
}
