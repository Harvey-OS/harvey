#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"../port/error.h"

#define	concdebug	1

#define	DPRINT	if(concdebug)kprint

typedef struct Conc	Conc;
typedef struct Subdev	Subdev;

/*
 *  A concentrator.  One exists for every stream that a 
 *  conc line discipline is pushed onto.
 */
struct Conc
{
	QLock;
	Stream *s;
	Queue *	wq;
	char	name[64];
	int	ndev;
	Subdev *dev;
	Block *	bp;
};

struct Subdev
{
	Conc *	cp;
	int	chan0;
	Queue *	rq;
};

static Lock	conclock;
static Conc	*concs;

/*
 * Concentrator line discipline (push here).
 */

static void concstopen(Queue*, Stream*);
static void concstclose(Queue*);
static void concstoput(Queue*, Block*);
static void concstiput(Queue*, Block*);
Qinfo concinfo =
{
	concstiput,
	concstoput,
	concstopen,
	concstclose,
	"conc"
};

/*
 * Stream interface to concentrator device (open here).
 */

static void concdevstopen(Queue*, Stream*);
static void concdevstclose(Queue*);
static void concdevstoput(Queue*, Block*);
static void concdevstiput(Queue*, Block*);
Qinfo concdevinfo =
{
	concdevstiput,
	concdevstoput,
	concdevstopen,
	concdevstclose,
	"concdev"
};

static void
concstopen(Queue *q, Stream *s)
{
	DPRINT("concstopen q=0x%ux s=0x%ux\n", q, s);

	RD(q)->ptr = 0;		/* (Conc *) for concstiput */
	WR(q)->ptr = s;

	s->opens++;		/* Hold this queue in place */
	s->inuse++;
}

static void
concstclose(Queue *q)
{
	Conc *cp = q->ptr;
	Subdev *dp;
	Block *bp;
	int ndev;

	if(RD(q)->ptr == 0)
		return;
	DPRINT("concstclose \"%s\"\n", cp->name);

	qlock(cp);
	ndev = cp->ndev;
	cp->ndev = 0;
	for(dp=cp->dev; dp<&cp->dev[ndev]; dp++){
			continue;
		bp = allocb(0);
		bp->type = M_HANGUP;
		PUTNEXT(dp->rq, bp);
	}
	qunlock(cp);
	cp->name[0] = 0;
}

static Conc *
concalloc(char *name, int aflag)
{
	Conc *free, *cp;

	DPRINT("concalloc \"%s\" %d...", name, aflag);
	lock(&conclock);
	free = 0;
	for(cp = concs; cp < &concs[conf.nconc]; cp++){
		if(strcmp(name, cp->name) == 0){
			unlock(&conclock);
			DPRINT("found\n");
			return cp;
		}
		if(cp->name[0] == 0)
			free = cp;
	}
	if(!aflag || free == 0){
		unlock(&conclock);
		DPRINT("not found/none free\n");
		return 0;
	}
	strncpy(free->name, name, sizeof free->name-1);
	unlock(&conclock);
	DPRINT("alloc'd\n");
	return free;
}

static void
concstoput(Queue *q, Block *bp)
{
	Conc *cp = 0;
	Subdev *dp;
	char *p, *err = 0;
	int n, ndev, k;

	if(bp->type != M_CTL || streamparse("config", bp) == 0){
		PUTNEXT(q, bp);
		return;
	}
	if(concdebug){
		char fmt[32];
		sprint(fmt, "concstoput config: %%.%ds\n", bp->wptr-bp->rptr);
		kprint(fmt, bp->rptr);
	}
	if(RD(q)->ptr){
		err = "stream already configured";
		goto out;
	}
	n = BLEN(bp);
	if(bp->wptr >= bp->lim){
		memmove(bp->base, bp->rptr, n);
		bp->rptr = bp->base;
		bp->wptr = bp->rptr+n;
	}
	*bp->wptr++ = 0;
	p = strchr((char *)bp->rptr, ' ');
	if(p == 0){
		err = "bad config command";
		goto out;
	}
	*p++ = 0;
	cp = concalloc((char *)bp->rptr, 1);
	bp->rptr = (uchar *)p;
	if(cp == 0){
		err = "no free conc's";
		goto out;
	}
	qlock(cp);
	if(cp->ndev){
		err = "conc name in use";
		goto out;
	}
	p = (char *)bp->rptr;
	for(ndev=0; strtoul(p, &p, 0); ndev++) ;
	DPRINT("concstoput: ndev=%d\n", ndev);
	cp->bp = allocb(ndev*sizeof(Subdev));
	if(cp->bp == 0){
		err = "can't allocb for subdevices";
		cp->name[0] = 0;
		goto out;
	}
	cp->dev = (Subdev *)cp->bp->base;
	p = (char *)bp->rptr;
	k = 0;
	DPRINT("concstoput: chan0=");
	for(dp=cp->dev; dp<&cp->dev[ndev]; dp++){
		dp->cp = cp;
		dp->chan0 = k;
		DPRINT(" %d", dp->chan0);
		k += strtoul(p, &p, 0);
		dp->rq = 0;
	}
	DPRINT("\n");
	cp->s = q->ptr;
	cp->wq = q;
	cp->ndev = ndev;
	q->ptr = cp;
	q->other->ptr = cp;
out:
	freeb(bp);
	if(cp)
		qunlock(cp);
	if(err){
		DPRINT("concstoput: err=\"%s\"\n", err);
		error(err);
	}
}

/*
 *  Simplifying assumption:  one put == one message && the channel number
 *	is in the first block (cf. dkmuxiput).
 */
static void
concstiput(Queue *q, Block *bp)
{
	Conc *cp = q->ptr;
	Subdev *dp;
	int chan;

	if(bp->type != M_DATA){
		PUTNEXT(q, bp);
		return;
	}
	if(BLEN(bp) < 2 || cp == 0)
		goto Drop;
	chan = bp->rptr[0] | (bp->rptr[1]<<8);
	for(dp=&cp->dev[cp->ndev-1]; dp>=cp->dev; dp--)
		if(chan >= dp->chan0)
			break;
	if(dp < cp->dev || dp->rq == 0)
		goto Drop;
	chan -= dp->chan0;
	bp->rptr[0] = chan;
	bp->rptr[1] = chan>>8;
	bp->flags |= S_DELIM;	/* a process is probably waiting for this */
	PUTNEXT(dp->rq, bp);
	return;
	
Drop:
	freeb(bp);
	return;
}

static void
concdevstopen(Queue *q, Stream *s)
{
	Conc *cp;
	Subdev *dp;

	cp = &concs[s->dev];
	qlock(cp);
	if(s->id > cp->ndev){
		qunlock(cp);
		error("no such subdevice");
	}
	dp = &cp->dev[s->id];
	q->ptr = dp;
	q->other->ptr = dp;
	dp->rq = RD(q);
	streamenter(cp->s);
	qunlock(cp);
}

static void
concdevstclose(Queue *q)
{
	Subdev *dp = q->ptr;
	Conc *cp = dp->cp;

	qlock(cp);
	q->ptr = 0;
	q->other->ptr = 0;
	dp->rq = 0;
	streamexit(cp->s);
	qunlock(cp);
}

/*
 *  this is only called by concstclose (hangup)
 */
static void
concdevstiput(Queue *q, Block *bp)
{
	PUTNEXT(q, bp);
}

static void
concdevstoput(Queue *q, Block *bp)
{
	Subdev *dp = q->ptr;
	Conc *cp;
	int chan;

	if(dp == 0){
		DPRINT("concdevstoput: not config'd, dropped %d\n", BLEN(bp));
		freeb(bp);
		return;
	}
	cp = dp->cp;
	qlock(cp);
	if(!putq(q, bp)){	/* send only whole messages */
		qunlock(cp);
		return;
	}
	bp = grabq(q);
	qunlock(cp);
	if(BLEN(bp) < 2 || bp->type != M_DATA){
		DPRINT("concdevstoput: dropped %d\n", BLEN(bp));
		freeb(bp);
		return;
	}
	chan = bp->rptr[0] | (bp->rptr[1]<<8);
	chan += dp->chan0;
	bp->rptr[0] = chan;
	bp->rptr[1] = chan>>8;
	PUTNEXT(cp->wq, bp);
}

void
concreset(void)
{
	concs = xalloc(conf.nconc*sizeof(Conc));
	newqinfo(&concinfo);
}

void
concinit(void)
{
}

Chan *
concattach(char *spec)
{
	Chan *c;
	Conc *cp;

	/*
	 *  find a concentrator with the same name (default conc)
	 */
	if(*spec == 0)
		spec = "conc";
	cp = concalloc(spec, 0);
	if(cp == 0 || cp->ndev == 0)
		error("device not configured");

	c = devattach('K', spec);
	c->dev = cp-concs;
	c->qid = (Qid){CHDIR, 0};
	return c;
}

Chan *
concclone(Chan *c, Chan *nc)
{
	return devclone(c, nc);
}

#define	TOPDIR		(CHDIR+1)
#define	CHANDIR		(CHDIR+2)

static int
concgen(Chan *c, Dirtab *tab, int ntab, int i, Dir *dp)
{
	Conc *cp;
	char buf[16];

	USED(tab, ntab);
	cp = &concs[c->dev];
	switch(c->qid.path){
	case CHDIR:
		devdir(c, (Qid){TOPDIR,0}, concs[c->dev].name, 0, eve, 0555, dp);
		return i==0 ? 1 : -1;
	case TOPDIR:
		if(i<0 || i >= cp->ndev)
			return -1;
		sprint(buf, "%d", i);
		devdir(c, (Qid){STREAMQID(i,CHANDIR),0}, buf, 0, eve, 0555, dp);
		return 1;
	}
	return streamgen(c, 0, 0, i, dp);
}

int	 
concwalk(Chan *c, char *name)
{
	return devwalk(c, name, 0, 0, concgen);
}

void	 
concstat(Chan *c, char *dp)
{
	char buf[16], *name;
	Dir dir;

	if(c->qid.path&CHDIR)switch(c->qid.path){
	case CHDIR:
		name = ".";
		break;
	case TOPDIR:
		name = concs[c->dev].name;
		break;
	default:
		sprint(name = buf, "%d", STREAMID(c->qid.path));
		break;
	}else switch(STREAMTYPE(c->qid.path)){
	case Sdataqid:
		name = "data";
		break;
	case Sctlqid:
		name = "ctl";
		break;
	default:
		name = "panic";
		break;
	}
	devdir(c, c->qid, name, 0, eve, (c->qid.path&CHDIR)?0555:0600, &dir);
	convD2M(&dir, dp);
}

Chan *
concopen(Chan *c, int omode)
{
	if(c->qid.path & CHDIR){
		if(omode != OREAD)
			error(Eperm);
	}else
		streamopen(c, &concdevinfo);
	c->mode = openmode(omode);
	c->flag |= COPEN;
	c->offset = 0;
	return c;
}

void	 
conccreate(Chan *c, char *name, int omode, ulong perm)
{
	USED(c, name, omode, perm);
	error(Eperm);
}

void	 
concclose(Chan *c)
{
	if(c->stream)
		streamclose(c);
}

long	 
concread(Chan *c, void *a, long n, ulong offset)
{
	USED(offset);
	if(c->stream)
		return streamread(c, a, n);
	if(c->qid.path & CHDIR)
		return devdirread(c, a, n, 0, 0, concgen);
	error(Eio);
	return -1;	/* does not return */
}

long	 
concwrite(Chan *c, void *a, long n, ulong offset)
{
	USED(offset);
	return streamwrite(c, a, n, 0);
}

void	 
concremove(Chan *c)
{
	USED(c);
	error(Eperm);
}

void	 
concwstat(Chan *c, char *dp)
{
	USED(c, dp);
	error(Eperm);
}
