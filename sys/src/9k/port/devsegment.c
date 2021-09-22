#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

enum
{
	Qtopdir,
	Qsegdir,
	Qctl,
	Qdata,

	/* commands to kproc */
	Cnone=0,
	Cread,
	Cwrite,
	Cstart,
	Cdie,

	Lg2nsegs = 7,
};

#define TYPE(c) 	(int)((c)->qid.path & MASK(3))
#define SEG(c)	 	(((c)->qid.path >> 3) & MASK(Lg2nsegs))
#define PATH(s, t) 	((s)<<3 | (t))

typedef struct Globalseg Globalseg;
struct Globalseg
{
	Ref;
	Segment	*s;

	char	*name;
	char	*uid;
	vlong	length;
	long	perm;

	/* kproc to do reading and writing */
	QLock	l;		/* sync kproc access */
	Rendez	cmdwait;	/* where kproc waits */
	Rendez	replywait;	/* where requestor waits */
	Proc	*kproc;
	char	*data;
	vlong	off;
	int	dlen;		/* size of a transfer */
	int	cmd;	
	char	err[64];
};

static Globalseg *globalseg[1<<Lg2nsegs];
static Lock globalseglock;
static char Esegnotalloc[] = "segment not yet allocated";

	Segment* (*_globalsegattach)(Proc*, char*);
static	Segment* globalsegattach(Proc *p, char *name);
static	int	cmddone(void*);
static	void	segmentkproc(void*);
static	void	docmd(Globalseg *g, int cmd);

Dev segmentdevtab;

/*
 *  returns with globalseg incref'd
 */
static Globalseg*
getgseg(Chan *c)
{
	int x;
	Globalseg *g;

	x = SEG(c);
	lock(&globalseglock);
	if(x >= nelem(globalseg))
		error("out of global segments");
	g = globalseg[x];
	if(g != nil)
		incref(g);
	unlock(&globalseglock);
	if(g == nil)
		error("global segment disappeared");
	return g;
}

static void
putgseg(Globalseg *g)
{
	if(decref(g) > 0)
		return;
	if(g->s != nil)
		putseg(g->s);
	if(g->kproc)
		docmd(g, Cdie);
	free(g->name);
	free(g->uid);
	free(g);
}

static void
fillqid(Qid *qp, vlong path, int type)
{
	qp->vers = 0;
	qp->path = PATH(0, path);
	qp->type = type;
}

static void
rootgen(Chan *c, Dir *dp)
{
	Qid q;

	fillqid(&q, Qtopdir, QTDIR);
	devdir(c, q, "#g", 0, eve, DMDIR|0777, dp);
}

static int
segmentgen(Chan *c, char*, Dirtab*, int, int s, Dir *dp)
{
	Qid q;
	Globalseg *g;
	uintptr size;

	switch(TYPE(c)) {
	case Qtopdir:
		if(s == DEVDOTDOT){
			rootgen(c, dp);
			break;
		}

		if(s >= nelem(globalseg))
			return -1;

		lock(&globalseglock);
		g = globalseg[s];
		if(g == nil){
			unlock(&globalseglock);
			return 0;
		}
		fillqid(&q, Qsegdir, QTDIR);
		devdir(c, q, g->name, 0, g->uid, DMDIR|0777, dp);
		unlock(&globalseglock);

		break;
	case Qsegdir:
		if(s == DEVDOTDOT){
			rootgen(c, dp);
			break;
		}
		/* fall through */
	case Qctl:
	case Qdata:
		switch(s){
		case 0:
			g = getgseg(c);
			fillqid(&q, Qctl, QTFILE);
			q.path = PATH(SEG(c), Qctl);
			devdir(c, q, "ctl", 0, g->uid, g->perm, dp);
			putgseg(g);
			break;
		case 1:
			g = getgseg(c);
			fillqid(&q, Qdata, QTFILE);
			q.path = PATH(SEG(c), Qdata);
			if(g->s != nil)
				size = g->s->top - g->s->base;
			else
				size = 0;
			devdir(c, q, "data", size, g->uid, g->perm, dp);
			putgseg(g);
			break;
		default:
			return -1;
		}
		break;
	}
	return 1;
}

static void
segmentinit(void)
{
	_globalsegattach = globalsegattach;
}

static Chan*
segmentattach(char *spec)
{
	return devattach(segmentdevtab.dc, spec);
}

static Walkqid*
segmentwalk(Chan *c, Chan *nc, char **name, int nname)
{
	return devwalk(c, nc, name, nname, 0, 0, segmentgen);
}

static long
segmentstat(Chan *c, uchar *db, long n)
{
	return devstat(c, db, n, 0, 0, segmentgen);
}

static int
cmddone(void *arg)
{
	Globalseg *g = arg;

	return g->cmd == Cnone;
}

static Chan*
segmentopen(Chan *c, int omode)
{
	Globalseg *g;

	switch(TYPE(c)){
	case Qtopdir:
	case Qsegdir:
		if(omode != 0)
			error(Eisdir);
		break;
	case Qctl:
		g = getgseg(c);
		if(waserror()){
			putgseg(g);
			nexterror();
		}
		devpermcheck(g->uid, g->perm, omode);
		c->aux = g;
		poperror();
		c->flag |= COPEN;
		break;
	case Qdata:
		g = getgseg(c);
		if(waserror()){
			putgseg(g);
			nexterror();
		}
		devpermcheck(g->uid, g->perm, omode);
		if(g->s == nil)
			error(Esegnotalloc);
		if(g->kproc == nil){
			qlock(&g->l);
			if(waserror()){
				qunlock(&g->l);
				nexterror();
			}
			if(g->kproc == nil){
				g->cmd = Cnone;
				kproc(g->name, segmentkproc, g);
				docmd(g, Cstart);
			}
			qunlock(&g->l);
			poperror();
		}
		c->aux = g;
		poperror();
		c->flag |= COPEN;
		break;
	default:
		panic("segmentopen");
	}
	c->mode = openmode(omode);
	c->offset = 0;
	return c;
}

static void
segmentclose(Chan *c)
{
	if(TYPE(c) == Qtopdir)
		return;
	if(c->flag & COPEN)
		putgseg(c->aux);
}

static void
segmentcreate(Chan *c, char *name, int omode, int perm)
{
	int x, xfree;
	Globalseg *g;

	if(TYPE(c) != Qtopdir)
		error(Eperm);

	if(isphysseg(name))
		error(Eexist);

	if((perm & DMDIR) == 0)
		error(Ebadarg);

	if(waserror()){
		unlock(&globalseglock);
		nexterror();
	}
	lock(&globalseglock);
	xfree = -1;
	for(x = 0; x < nelem(globalseg); x++){
		g = globalseg[x];
		if(g == nil){
			if(xfree < 0)
				xfree = x;
		} else {
			if(strcmp(g->name, name) == 0)
				error(Eexist);
		}
	}
	if(xfree < 0)
		error("too many global segments");
	/* don't smalloc, which could sleep, while holding a Lock */
//	g = smalloc(sizeof(Globalseg));
	g = malloc(sizeof(Globalseg));
	if (g == nil)
		error(Enomem);
	g->ref = 1;
	kstrdup(&g->name, name);
	kstrdup(&g->uid, up->user);
	g->perm = 0660; 
	globalseg[xfree] = g;
	unlock(&globalseglock);
	poperror();

	fillqid(&c->qid, Qsegdir, QTDIR);
	c->qid.path = PATH(x, Qsegdir);
	c->mode = openmode(omode);
	c->mode = OWRITE;
}

static long
segmentread(Chan *c, void *a, long n, vlong voff)
{
	uintptr segsize;
	Segment *s;
	Globalseg *g;
	char buf[32];

	if(c->qid.type == QTDIR)
		return devdirread(c, a, n, (Dirtab *)0, 0L, segmentgen);

	g = c->aux;
	s = g->s;
	if(s == nil)
		error(Esegnotalloc);
	switch(TYPE(c)){
	case Qctl:
		snprint(buf, sizeof buf, "va %#llux %#llux\n", (vlong)s->base,
			(vlong)s->top - s->base);
		return readstr(voff, a, n, buf);
	case Qdata:
		segsize = s->top - s->base;
		if(voff < 0 || voff > segsize || n < 0)
			error(Ebadarg);
		if(voff + n > segsize)
			n = segsize - voff;	/* truncate read */
		if (n <= 0)
			return 0;

		qlock(&g->l);
		g->off = voff + s->base;
		g->data = smalloc(n);
		if(waserror()){
			free(g->data);
			qunlock(&g->l);
			nexterror();
		}
		g->dlen = n;
		docmd(g, Cread);		/* from g->off to g->data */
		memmove(a, g->data, g->dlen);	/* copy to user space */
		poperror();
		free(g->data);
		qunlock(&g->l);
		return g->dlen;
	default:
		panic("segmentread");
		notreached();
	}
}

static long
segmentwrite(Chan *c, void *a, long n, vlong voff)
{
	uintptr va, len, top, segsize;
	Cmdbuf *cb;
	Globalseg *g;
	Segment *s;

	if(c->qid.type == QTDIR)
		error(Eperm);

	g = c->aux;
	s = g->s;
	switch(TYPE(c)){
	case Qctl:
		cb = parsecmd(a, n);
		if(strcmp(cb->f[0], "va") == 0){
			if(s != nil)
				error("already has a virtual address");
			if(cb->nf < 3)
				error(Ecmdargs);
			va = strtoull(cb->f[1], 0, 0);
			len = strtoull(cb->f[2], 0, 0);
			top = PGROUND(va + len);
			va &= ~((uintptr)BY2PG-1);
			if((top - va) / BY2PG == 0)
				error(Ebadarg);
			g->s = newseg(SG_SHARED, va, top);
		} else
			error(Ebadctl);
		break;
	case Qdata:
		if(s == nil)
			error(Esegnotalloc);
		segsize = s->top - s->base;
		if(voff < 0 || voff > segsize || n < 0)
			error(Ebadarg);
		if(voff + n > segsize)
			n = segsize - voff;	/* truncate write */
		if (n <= 0)
			return 0;

		qlock(&g->l);
		g->off = voff + s->base;
		g->data = smalloc(n);
		if(waserror()){
			free(g->data);
			qunlock(&g->l);
			nexterror();
		}
		g->dlen = n;
		memmove(g->data, a, g->dlen);	/* copy from user space */
		docmd(g, Cwrite);		/* g->data to g->off */
		poperror();
		free(g->data);
		qunlock(&g->l);
		return g->dlen;
	default:
		panic("segmentwrite");
		notreached();
	}
	return 0;
}

static long
segmentwstat(Chan *c, uchar *dp, long n)
{
	Globalseg *g;
	Dir *d;

	if(c->qid.type == QTDIR)
		error(Eperm);

	g = getgseg(c);
	if(waserror()){
		putgseg(g);
		nexterror();
	}

	if(strcmp(g->uid, up->user) && !iseve())
		error(Eperm);
	d = smalloc(sizeof(Dir)+n);
	n = convM2D(dp, n, &d[0], (char*)&d[1]);
	g->perm = d->mode & 0777;

	putgseg(g);
	poperror();

	free(d);
	return n;
}

static void
segmentremove(Chan *c)
{
	Globalseg *g;
	int x;

	if(TYPE(c) != Qsegdir)
		error(Eperm);
	lock(&globalseglock);
	x = SEG(c);
	g = globalseg[x];
	globalseg[x] = nil;
	unlock(&globalseglock);
	if(g != nil)
		putgseg(g);
}

/*
 *  called by segattach()
 */
static Segment*
globalsegattach(Proc *p, char *name)
{
	int x;
	Globalseg *g;
	Segment *s;

	g = nil;
	if(waserror()){
		unlock(&globalseglock);
		nexterror();
	}
	lock(&globalseglock);
	for(x = 0; x < nelem(globalseg); x++){
		g = globalseg[x];
		if(g != nil && strcmp(g->name, name) == 0)
			break;
	}
	if(x == nelem(globalseg)){
		unlock(&globalseglock);
		poperror();
		return nil;
	}
	devpermcheck(g->uid, g->perm, ORDWR);
	s = g->s;
	if(s == nil)
		error("global segment not assigned a virtual address");
	if(isoverlap(p, s->base, s->top - s->base) != nil)
		error(Esoverlap);
	incref(s);
	unlock(&globalseglock);
	poperror();
	return s;
}

static void
docmd(Globalseg *g, int cmd)
{
	g->err[0] = 0;
	g->cmd = cmd;
	wakeup(&g->cmdwait);
	sleep(&g->replywait, cmddone, g);
	if(g->err[0])
		error(g->err);
}

static int
cmdready(void *arg)
{
	Globalseg *g = arg;

	return g->cmd != Cnone;
}

static void
segmentkproc(void *arg)
{
	Globalseg *g = arg;
	int done, sno;

	for(sno = 0; sno < NSEG; sno++)
		if(up->seg[sno] == nil && sno != ESEG)
			break;
	if(sno >= NSEG)
		panic("segmentkproc");
	g->kproc = up;

	incref(g->s);
	up->seg[sno] = g->s;		/* steal a seg for g */

	for(done = 0; !done;){
		sleep(&g->cmdwait, cmdready, g);
		if(waserror()){
			strncpy(g->err, up->errstr, sizeof(g->err));
		} else {
			switch(g->cmd){
			case Cstart:
				break;
			case Cdie:
				done = 1;
				break;
			case Cread:
				memmove(g->data, (char*)g->off, g->dlen);
				break;
			case Cwrite:
				memmove((char*)g->off, g->data, g->dlen);
				break;
			}
			poperror();
		}
		g->cmd = Cnone;
		wakeup(&g->replywait);
	}
	pexit("", 1);
}

Dev segmentdevtab = {
	'g',
	"segment",

	devreset,
	segmentinit,
	devshutdown,
	segmentattach,
	segmentwalk,
	segmentstat,
	segmentopen,
	segmentcreate,
	segmentclose,
	segmentread,
	devbread,
	segmentwrite,
	devbwrite,
	segmentremove,
	segmentwstat,
};
