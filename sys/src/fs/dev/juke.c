#include	"all.h"

#define	SCSInone	SCSIread
#define	MAXDRIVE	10
#define	MAXSIDE		500
#define	FIXEDSIZE	1
#define	TWORM		MINUTE(10)
#define	THYSTER		SECOND(10)

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
	QLock;				/* protects drive mechanism */
	Side	side[MAXSIDE];
	int	nside;			/* how many storage elements (*2 if rev) */
	int	ndrive;			/* number of transfer elements */
	Device*	juke;			/* devworm of changer */
	Device*	drive[MAXDRIVE];	/* devworm for i/o */
	uchar	offline[MAXDRIVE];	/* drives removed from service */
	long	fixedsize;		/* one size fits all */
	int	probeok;		/* wait for init to probe */

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

static	int	wormsense(Device*);
static	Side*	wormunit(Device*);
static	void	shelves(void);
static	int	mmove(Juke*, int, int, int, int);
static	int	bestdrive(Juke*, int);
static	void	waitready(Device*);
static	void	element(Juke*, int);

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
	uchar cmd[10], buf[8];

	w = d->private;
	p = d->wren.targ;
	if(p < 0 || p >= w->nside) {
//		panic("wormunit partition %Z\n", d);
		return 0;
	}

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
		print("	load   r%ld drive %Z\n", v-w->side, w->drive[drive]);
		if(mmove(w, w->mt0, v->elem, w->dt0+drive, v->rot)) {
			qunlock(w);
			goto sbad;
		}
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
	memset(cmd, 0, sizeof(cmd));
	memset(buf, 0, sizeof(buf));
	cmd[0] = 0x25;	/* read capacity */
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
//	panic("wormunit sbad");
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
			if(w->offline[i])
				continue;
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
			if(mmove(w, w->mt0, w->dt0+drive, v->elem, v->rot)) {
				qunlock(v);
				goto loop;
			}
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
	uchar cmd[10];

	w = d->private;
	v = wormunit(d);
	if(v == 0)
		return 0x71;
	if(b >= v->max) {
		qunlock(v);
		print("worm: wormiocmd out of range %Z(%ld)\n", d, b);
		return 0x071;
	}

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = 0x28;		/* extended read */
	if(io != SCSIread)
		cmd[0] = 0x2a;	/* extended write */

	m = v->mult;
	l = b * m;
	cmd[2] = l>>24;
	cmd[3] = l>>16;
	cmd[4] = l>>8;
	cmd[5] = l;

	cmd[7] = m>>8;
	cmd[8] = m;

	s = scsiio(w->drive[v->drive], io, cmd, sizeof(cmd), c, RBUFSIZE);
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
int
mmove(Juke *w, int trans, int from, int to, int rot)
{
	uchar cmd[12], buf[4];
	int s;
	static recur = 0;

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = 0xa5;	/* move medium */
	cmd[2] = trans>>8;
	cmd[3] = trans;
	cmd[4] = from>>8;
	cmd[5] = from;
	cmd[6] = to>>8;
	cmd[7] = to;
	if(rot)
		cmd[10] = 1;
	s = scsiio(w->juke, SCSInone, cmd, sizeof(cmd), buf, 0);
	if(s) {
		print("scsio status #%x\n", s);
		print("move medium t=%d fr=%d to=%d rot=%d", trans, from, to, rot);
//		panic("mmove");
		if(recur == 0) {
			recur = 1;
			print("element from=%d\n", from);
			element(w, from);
			print("element to=%d\n", to);
			element(w, to);
			print("element trans=%d\n", trans);
			element(w, trans);
			recur = 0;
		}
		return 1;
	}
	return 0;
}

static
void
geometry(Juke *w)
{
	int s;
	uchar cmd[6], buf[4+20];

	memset(cmd, 0, sizeof(cmd));
	memset(buf, 0, sizeof(buf));
	cmd[0] = 0x1a;		/* mode sense */
	cmd[2] = 0x1d;		/* element address assignment */
	cmd[4] = sizeof(buf);	/* allocation length */

	s = scsiio(w->juke, SCSIread, cmd, sizeof(cmd), buf, sizeof(buf));
	if(s)
		panic("geometry #%x\n", s);

	w->mt0 = (buf[4+2]<<8) | buf[4+3];
	w->nmt = (buf[4+4]<<8) | buf[4+5];
	w->se0 = (buf[4+6]<<8) | buf[4+7];
	w->nse = (buf[4+8]<<8) | buf[4+9];
	w->ie0 = (buf[4+10]<<8) | buf[4+11];
	w->nie = (buf[4+12]<<8) | buf[4+13];
	w->dt0 = (buf[4+14]<<8) | buf[4+15];
	w->ndt = (buf[4+16]<<8) | buf[4+17];

	memset(cmd, 0, 6);
	memset(buf, 0, sizeof(buf));
	cmd[0] = 0x1a;		/* mode sense */
	cmd[2] = 0x1e;		/* transport geometry */
	cmd[4] = sizeof(buf);	/* allocation length */

	s = scsiio(w->juke, SCSIread, cmd, sizeof(cmd), buf, sizeof(buf));
	if(s)
		panic("geometry #%x\n", s);

	w->rot = buf[4+2] & 1;

	print("	mt %d %d\n", w->mt0, w->nmt);
	print("	se %d %d\n", w->se0, w->nse);
	print("	ie %d %d\n", w->ie0, w->nie);
	print("	dt %d %d\n", w->dt0, w->ndt);
	print("	rot %d\n", w->rot);
	prflush();

}

static
void
element(Juke *w, int e)
{
	uchar cmd[12], buf[8+8+88];
	int s, t;

//loop:
	memset(cmd, 0, sizeof(cmd));
	memset(buf, 0, sizeof(buf));
	cmd[0] = 0xb8;		/* read element status */
	cmd[2] = e>>8;		/* starting element */
	cmd[3] = e;
	cmd[5] = 1;		/* number of elements */
	cmd[9] = sizeof(buf);	/* allocation length */

	s = scsiio(w->juke, SCSIread, cmd, sizeof(cmd), buf, sizeof(buf));
	if(s) {
		print("scsiio #%x\n", s);
		goto bad;
	}

	s = (buf[0]<<8) | buf[1];
	if(s != e) {
		print("element = %d\n", s);
		goto bad;
	}
	if(buf[3] != 1) {
		print("number reported = %d\n", buf[3]);
		goto bad;
	}
	s = (buf[8+8+0]<<8) | buf[8+8+1];
	if(s != e) {
		print("element1 = %d\n", s);
		goto bad;
	}

	switch(buf[8+0]) {	/* element type */
	default:
		print("unknown element %d: %d\n", e, buf[8+0]);
		goto bad;
	case 1:			/* transport */
		s = e - w->mt0;
		if(s < 0 || s >= w->nmt)
			goto bad;
		if(buf[8+8+2] & 1)
			print("transport %d full %d.%d\n", s,
				(buf[8+8+10]<<8) | buf[8+8+11],
				(buf[8+8+9]>>6) & 1);
		break;
	case 2:			/* storage */
		s = e - w->se0;
		if(s < 0 || s >= w->nse)
			goto bad;
		w->side[s].status = Sempty;
		if(buf[8+8+2] & 1)
			w->side[s].status = Sunload;
		if(w->rot)
			w->side[w->nse+s].status = w->side[s].status;
		break;
	case 3:			/* import/export */
		s = e - w->ie0;
		if(s < 0 || s >= w->nie)
			goto bad;
		print("import/export %d #%.2x %d.%d\n", s,
			buf[8+8+2],
			(buf[8+8+10]<<8) | buf[8+8+11],
			(buf[8+8+9]>>6) & 1);
		break;
	case 4:			/* data transfer */
		s = e - w->dt0;
		if(s < 0 || s >= w->ndt)
			goto bad;
		print("data transfer %d #%.2x %d.%d\n", s,
			buf[8+8+2],
			(buf[8+8+10]<<8) | buf[8+8+11],
			(buf[8+8+9]>>6) & 1);
		if(buf[8+8+2] & 1) {
			t = ((buf[8+8+10]<<8) | buf[8+8+11]) - w->se0;
			print("r%d in drive %d\n", t, s);
			if(mmove(w, w->mt0, w->dt0+s, w->se0+t, (buf[8+8+9]>>6) & 1)) {
				print("mmove initial unload\n");
				goto bad;
			}
			w->side[t].status = Sunload;
			if(w->rot)
				w->side[w->nse+t].status = Sunload;
		}
		if(buf[8+8+2] & 4) {
			print("drive w%d has exception #%.2x #%.2x\n", s,
				buf[8+8+4], buf[8+8+5]);
			goto bad;
		}
		break;
	}
	return;

bad:
//	panic("element");
	return;
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
			if(mmove(w, w->mt0, w->dt0+v->drive, v->elem, v->rot)) {
				qunlock(v);
				return 0;
			}
			v->status = Sunload;
		}
		qunlock(v);
	}
	j += w->nse;
	if(j < w->nside) {
		v = &w->side[j];
		qlock(v);
		if(v->status == Sstart) {
			if(mmove(w, w->mt0, w->dt0+v->drive, v->elem, v->rot)) {
				qunlock(v);
				return 0;
			}
			v->status = Sunload;
		}
		qunlock(v);
	}
	v = &w->side[i];
	qlock(v);
	return v;
}

static
void
cmd_wormoffline(int argc, char *argv[])
{
	int u, i;
	Juke *w;

	w = jukelist;
	if(argc <= 1) {
		print("usage: wormoffline drive\n");
		return;
	}
	u = number(argv[1], -1, 10);
	if(u < 0 || u >= w->ndrive) {
		print("bad drive %s (0<=%d<%d)\n", argv[1], u, w->ndrive);
		return;
	}
	if(w->offline[u])
		print("drive %d already offline\n", u);
	w->offline[u] = 1;
	for(i=0; i<w->ndrive; i++)
		if(w->offline[i] == 0)
			return;
	print("that would take all drives offline\n");
	w->offline[u] = 0;
}

static
void
cmd_wormonline(int argc, char *argv[])
{
	int u;
	Juke *w;

	w = jukelist;
	if(argc <= 1) {
		print("usage: wormonline drive\n");
		return;
	}
	u = number(argv[1], -1, 10);
	if(u < 0 || u >= w->ndrive) {
		print("bad drive %s (0<=%d<%d)\n", argv[1], u, w->ndrive);
		return;
	}
	if(w->offline[u] == 0)
		print("drive %d already online\n", u);
	w->offline[u] = 0;
}

static
void
cmd_wormreset(int, char *[])
{
	Juke *w;

	for(w=jukelist; w; w=w->link) {
		qlock(w);
		positions(w);
		qunlock(w);
	}
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

	cmd_install("wormreset", "-- put drives back where jukebox thinks they belong", cmd_wormreset);
	cmd_install("wormeject", "unit -- shelf to outside", cmd_wormeject);
	cmd_install("wormingest", "unit -- outside to shelf", cmd_wormingest);
	cmd_install("wormoffline", "unit -- disable drive", cmd_wormoffline);
	cmd_install("wormonline", "unit -- enable drive", cmd_wormonline);

found:
	i = 0;
	while(xdev = xdev->link) {
		if(xdev->type != Devwren) {
			print("drive not devwren: %Z\n", xdev);
			goto bad;
		}
		if(w->drive[i]->type != Devnone &&
		   xdev != w->drive[i]) {
			print("double init drive %d %Z %Z\n", i, w->drive[i], xdev);
			goto bad;
		}
		if(i >= w->ndrive) {
			print("too many drives %Z\n", xdev);
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
				mmove(w, w->mt0, w->dt0+drive, v->elem, v->rot);
				v->status = Sunload;
			}
			qunlock(v);
		}
		qunlock(w);
	}
}
