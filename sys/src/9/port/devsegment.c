/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

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
	Qfree,

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
typedef struct Freemsg Freemsg;

struct Freemsg
{
	Freemsg *next;
};

struct Globalseg
{
	Ref;
	Segment	*s;

	char	*name;
	char	*uid;
	vlong	length;
	long	perm;

	Freemsg	*free;

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
Segment *heapseg;

	Segment* (*_globalsegattach)(Proc*, char*);
static	Segment* globalsegattach(Proc*, char*);
static	int	cmddone(void*);
static	void	segmentkproc(void*);
static	void	docmd(Globalseg*, int);

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
	if(g->s == heapseg)
		heapseg = nil;
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
	uint32_t size;

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
	case Qfree:
		g = getgseg(c);
		if(waserror()){
			putgseg(g);
			nexterror();
		}
		q.vers = 0;
		q.type = QTFILE;
		switch(s){
		case 0:
			q.path = PATH(SEG(c), Qctl);
			devdir(c, q, "ctl", 0, g->uid, g->perm, dp);
			break;
		case 1:
			q.path = PATH(SEG(c), Qdata);
			if(g->s != nil)
				size = g->s->top - g->s->base;
			else
				size = 0;
			devdir(c, q, "data", size, g->uid, g->perm, dp);
			break;
		case 2:
			q.path = PATH(SEG(c), Qfree);
			devdir(c, q, "free", 0, g->uid, g->perm&0444, dp);
			break;

		default:
			poperror();
			putgseg(g);
			return -1;
		}
		poperror();
		putgseg(g);
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

static int32_t
segmentstat(Chan *c, uint8_t *db, int32_t n)
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
	case Qfree:
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
			poperror();
			qunlock(&g->l);
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
	char *ep;

	if(TYPE(c) != Qtopdir)
		error(Eperm);

	if(isphysseg(name))
		error(Eexist);

	if((perm & DMDIR) == 0)
		error("must create directory");

	if(waserror()){
		unlock(&globalseglock);
		nexterror();
	}
	xfree = -1;
	if(name[0] == '#' && name[1] >= '0' && name[1] <= '9'){
		/* hack for cnk: if #n, treat it as index n */
		xfree = strtoul(name+1, &ep, 0);
		if(*ep)
			xfree = -1;
		else if(xfree < 0 || xfree >= nelem(globalseg))
			error("invalid global segment index");
	}
	lock(&globalseglock);
	if(xfree < 0){
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
	}else{
		g = globalseg[xfree];
		if(g != nil)
			error(Eexist);
	}
	g = smalloc(sizeof(Globalseg));
	g->ref = 1;
	kstrdup(&g->name, name);
	kstrdup(&g->uid, up->user);
	g->perm = 0660; 
	globalseg[xfree] = g;
	unlock(&globalseglock);
	poperror();

	c->qid.path = PATH(xfree, Qsegdir);
	c->qid.type = QTDIR;
	c->qid.vers = 0;
	c->mode = openmode(omode);
	c->mode = OWRITE;

	DBG("segmentcreate(%s, %#o %#ux)\n", name, omode, perm);
}

enum{PTRSIZE = 19};	/* "0x1234567812345678 " */
static int
readptr(char *buf, int32_t n, uintptr val)
{
	if(n < PTRSIZE)
		return 0;
	snprint(buf, sizeof buf, "%*#ullx", PTRSIZE-1, val);
	buf[PTRSIZE-1] = ' ';
	return PTRSIZE;
}

static int
znotempty(void *x)
{
	Zseg *zs;

	zs = x;
	return zs->end != 0;
}

static int32_t
segmentread(Chan *c, void *a, int32_t n, int64_t voff)
{
	Globalseg *g;
	Zseg *zs;
	uintptr va;
	char *p, *s;
	int32_t tot;
	char buf[64];

	if(c->qid.type == QTDIR)
		return devdirread(c, a, n, (Dirtab *)0, 0L, segmentgen);

	g = c->aux;
	switch(TYPE(c)){
	case Qfree:
		if(g->s == nil)
			error("segment not yet allocated");
		if(n < PTRSIZE)
			error("read buffer too small");
		zs = &g->s->zseg;
		qlock(&g->s->lk);
		if(waserror()){
			qunlock(&g->s->lk);
			nexterror();
		}
		while((va = zgetaddr(g->s)) == 0ULL){
			qunlock(&g->s->lk);
			sleep(&zs->rr, znotempty, zs);
			qlock(&g->s->lk);
		}
		p = a;
		for(tot = 0; n-tot > PTRSIZE; tot += PTRSIZE){
			p += readptr(p, n, va);
			if((va = zgetaddr(g->s)) == 0ULL)
				break;
		}
		poperror();
		qunlock(&g->s->lk);
		return tot;
	case Qctl:
		if(g->s == nil)
			error("segment not yet allocated");
		if(g->s->type&SG_KZIO)
			s = "kmsg";
		else if(g->s->type&SG_ZIO)
			s = "umsg";
		else
			s = "addr";
		snprint(buf, sizeof(buf), "%s %#p %#p\n",
			s, g->s->base, (uintptr)(g->s->top-g->s->base));
		return readstr(voff, a, n, buf);
	case Qdata:
		if(voff < 0)
			error(Enegoff);
		if(voff + n > g->s->top - g->s->base){
			n = g->s->top - voff;
			if(n <= 0)
				break;
		}
		qlock(&g->l);
		if(waserror()){
			qunlock(&g->l);
			nexterror();
		}

		g->off = voff + g->s->base;
		g->data = smalloc(n);
		if(waserror()){
			free(g->data);
			nexterror();
		}
		g->dlen = n;
		docmd(g, Cread);
		memmove(a, g->data, g->dlen);
		poperror();
		free(g->data);

		poperror();
		qunlock(&g->l);
		return g->dlen;
	default:
		panic("segmentread");
	}
	return 0;	/* not reached */
}

/*
 * BUG: we allocate virtual addresses but never give them
 * back when the segment is destroyed.
 * BUG: what if we overlap other segments attached by the user?
 */
static uint32_t
placeseg(uint32_t len)
{
	static Lock lck;
	static uint32_t va = HEAPTOP;
	uint32_t v;

	len += BIGPGSZ;	/* so we fault upon overflows */
	lock(&lck);
	len = BIGPGROUND(len);
	va -= len;
	v = va;
	unlock(&lck);

	return v;
}

static int32_t
segmentwrite(Chan *c, void *a, int32_t n, int64_t voff)
{
	Cmdbuf *cb;
	Globalseg *g;
	uintptr va, len, top;
	int i;
	struct{
		char *name;
		int type;
	}segs[] = {
		{"kmsg", SG_SHARED|SG_ZIO|SG_KZIO},
		{"umsg", SG_SHARED|SG_ZIO},
		{"addr", SG_SHARED},
	};

	if(c->qid.type == QTDIR)
		error(Eperm);

	switch(TYPE(c)){
	case Qfree:
		error(Eperm);
		break;
	case Qctl:
		g = c->aux;
		cb = parsecmd(a, n);
		for(i = 0; i < nelem(segs); i++)
			if(strcmp(cb->f[0], segs[i].name) == 0)
				break;
		if(i < nelem(segs)){
			if(g->s != nil)
				error("already has a virtual address");
			if(cb->nf < 3)
				cmderror(cb, Ebadarg);
			va = strtoul(cb->f[1], 0, 0);
			len = strtoul(cb->f[2], 0, 0);
			if(va == 0)
				va = placeseg(len);
			top = BIGPGROUND(va + len);
			va = va&~(BIGPGSZ-1);
			len = (top - va) / BIGPGSZ;
			if(len == 0)
				cmderror(cb, "empty segment");

			g->s = newseg(segs[i].type, va, len);
			if(i == 0)
				newzmap(g->s);
			else if(i == 1)
				zgrow(g->s);
			DBG("newseg %s base %#ullx len %#ullx\n",
				cb->f[0], va, len*BIGPGSZ);
			if(i == 0 || i == 1)
				dumpzseg(g->s);
		}else if(strcmp(cb->f[0], "heap") == 0){
			if(g == nil)
				error("no globalseg");
			if(g->s == nil)
				error("no segment");
			if(heapseg)
				error("heap already set");
			else
				heapseg = g->s;
		}else
			error(Ebadctl);
		break;
	case Qdata:
		g = c->aux;
		if(voff < 0)
			error(Enegoff);
		if(voff + n > g->s->top - g->s->base){
			n = g->s->top - voff;
			if(n <= 0)
				break;
		}
		qlock(&g->l);
		if(waserror()){
			qunlock(&g->l);
			nexterror();
		}

		g->off = voff + g->s->base;
		g->data = smalloc(n);
		if(waserror()){
			free(g->data);
			nexterror();
		}
		g->dlen = n;
		memmove(g->data, a, g->dlen);
		docmd(g, Cwrite);
		poperror();
		free(g->data);

		poperror();
		qunlock(&g->l);
		break;
	default:
		panic("segmentwrite");
	}
	return n;
}

static int32_t
segmentwstat(Chan *c, uint8_t *dp, int32_t n)
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

	if(strcmp(g->uid, up->user)!=0 && !iseve())
		error(Eperm);
	d = smalloc(sizeof(Dir)+n);
	if(waserror()){
		free(d);
		nexterror();
	}
	n = convM2D(dp, n, &d[0], (char*)&d[1]);
	if(!emptystr(d->uid) && strcmp(d->uid, g->uid) != 0)
		kstrdup(&g->uid, d->uid);
	if(d->mode != ~0UL)
		g->perm = d->mode & 0777;
	poperror();
	free(d);

	poperror();
	putgseg(g);

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
	while(waserror())
		{}	/* no interrupts */
	sleep(&g->replywait, cmddone, g);
	poperror();
	if(g->err[0])
		error(g->err);
}

static int
cmdready(void *arg)
{
	Globalseg *g = arg;

	return g->cmd != Cnone;
}

/*
 * TO DO: better approach is to send segment with command,
 * temporarily add it to segment array at SEG1, do the operation, then putseg.
 * otherwise there are as many kprocs as segments.
 */
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

