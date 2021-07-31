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
};

#define TYPE(x) 	(int)( (c)->qid.path & 0x7 )
#define SEG(x)	 	( ((c)->qid.path >> 3) & 0x3f )
#define PATH(s, t) 	( ((s)<<3) | (t) )

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
	long	off;
	int	dlen;
	int	cmd;	
	char	err[64];
};

static Globalseg *globalseg[100];
static Lock globalseglock;


	Segment* (*_globalsegattach)(Proc*, char*);
static	Segment* globalsegattach(Proc *p, char *name);
static	int	cmddone(void*);
static	void	segmentkproc(void*);
static	void	docmd(Globalseg *g, int cmd);

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
		panic("getgseg");
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

static int
segmentgen(Chan *c, char*, Dirtab*, int, int s, Dir *dp)
{
	Qid q;
	Globalseg *g;
	ulong size;

	switch(TYPE(c)) {
	case Qtopdir:
		if(s == DEVDOTDOT){
			q.vers = 0;
			q.path = PATH(0, Qtopdir);
			q.type = QTDIR;
			devdir(c, q, "#g", 0, eve, DMDIR|0777, dp);
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
		q.vers = 0;
		q.path = PATH(s, Qsegdir);
		q.type = QTDIR;
		devdir(c, q, g->name, 0, g->uid, DMDIR|0777, dp);
		unlock(&globalseglock);

		break;
	case Qsegdir:
		if(s == DEVDOTDOT){
			q.vers = 0;
			q.path = PATH(0, Qtopdir);
			q.type = QTDIR;
			devdir(c, q, "#g", 0, eve, DMDIR|0777, dp);
			break;
		}
		/* fall through */
	case Qctl:
	case Qdata:
		switch(s){
		case 0:
			g = getgseg(c);
			q.vers = 0;
			q.path = PATH(SEG(c), Qctl);
			q.type = QTFILE;
			devdir(c, q, "ctl", 0, g->uid, g->perm, dp);
			putgseg(g);
			break;
		case 1:
			g = getgseg(c);
			q.vers = 0;
			q.path = PATH(SEG(c), Qdata);
			q.type = QTFILE;
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
	return devattach('g', spec);
}

static Walkqid*
segmentwalk(Chan *c, Chan *nc, char **name, int nname)
{
	return devwalk(c, nc, name, nname, 0, 0, segmentgen);
}

static int
segmentstat(Chan *c, uchar *db, int n)
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
			error("segment not yet allocated");
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
segmentcreate(Chan *c, char *name, int omode, ulong perm)
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
	g = smalloc(sizeof(Globalseg));
	g->ref = 1;
	kstrdup(&g->name, name);
	kstrdup(&g->uid, up->user);
	g->perm = 0660; 
	globalseg[xfree] = g;
	unlock(&globalseglock);
	poperror();

	c->qid.path = PATH(x, Qsegdir);
	c->qid.type = QTDIR;
	c->qid.vers = 0;
	c->mode = openmode(omode);
	c->mode = OWRITE;
}

static long
segmentread(Chan *c, void *a, long n, vlong voff)
{
	Globalseg *g;
	char buf[32];

	if(c->qid.type == QTDIR)
		return devdirread(c, a, n, (Dirtab *)0, 0L, segmentgen);

	switch(TYPE(c)){
	case Qctl:
		g = c->aux;
		if(g->s == nil)
			error("segment not yet allocated");
		sprint(buf, "va %#lux %#lux\n", g->s->base, g->s->top-g->s->base);
		return readstr(voff, a, n, buf);
	case Qdata:
		g = c->aux;
		if(voff > g->s->top - g->s->base)
			error(Ebadarg);
		if(voff + n > g->s->top - g->s->base)
			n = g->s->top - g->s->base - voff;
		qlock(&g->l);
		g->off = voff + g->s->base;
		g->data = smalloc(n);
		if(waserror()){
			free(g->data);
			qunlock(&g->l);
			nexterror();
		}
		g->dlen = n;
		docmd(g, Cread);
		memmove(a, g->data, g->dlen);
		free(g->data);
		qunlock(&g->l);
		poperror();
		return g->dlen;
	default:
		panic("segmentread");
	}
	return 0;	/* not reached */
}

static long
segmentwrite(Chan *c, void *a, long n, vlong voff)
{
	Cmdbuf *cb;
	Globalseg *g;
	ulong va, len, top;

	if(c->qid.type == QTDIR)
		error(Eperm);

	switch(TYPE(c)){
	case Qctl:
		g = c->aux;
		cb = parsecmd(a, n);
		if(strcmp(cb->f[0], "va") == 0){
			if(g->s != nil)
				error("already has a virtual address");
			if(cb->nf < 3)
				error(Ebadarg);
			va = strtoul(cb->f[1], 0, 0);
			len = strtoul(cb->f[2], 0, 0);
			top = PGROUND(va + len);
			va = va&~(BY2PG-1);
			len = (top - va) / BY2PG;
			if(len == 0)
				error(Ebadarg);
			g->s = newseg(SG_SHARED, va, len);
		} else
			error(Ebadctl);
		break;
	case Qdata:
		g = c->aux;
		if(voff + n > g->s->top - g->s->base)
			error(Ebadarg);
		qlock(&g->l);
		g->off = voff + g->s->base;
		g->data = smalloc(n);
		if(waserror()){
			free(g->data);
			qunlock(&g->l);
			nexterror();
		}
		g->dlen = n;
		memmove(g->data, a, g->dlen);
		docmd(g, Cwrite);
		free(g->data);
		qunlock(&g->l);
		poperror();
		return g->dlen;
	default:
		panic("segmentwrite");
	}
	return 0;	/* not reached */
}

static int
segmentwstat(Chan *c, uchar *dp, int n)
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
		error("overlaps existing segment");
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
	int done;
	int sno;

	for(sno = 0; sno < NSEG; sno++)
		if(up->seg[sno] == nil && sno != ESEG)
			break;
	if(sno == NSEG)
		panic("segmentkproc");
	g->kproc = up;

	incref(g->s);
	up->seg[sno] = g->s;

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

