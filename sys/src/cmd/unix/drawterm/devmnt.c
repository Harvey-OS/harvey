#include	"lib9.h"
#include	"sys.h"
#include	"error.h"

struct Mntrpc
{
	Chan*	c;		/* Channel for whom we are working */
	Mntrpc*	list;		/* Free/pending list */
	Fcall	request;	/* Outgoing file system protocol message */
	Fcall	reply;		/* Incoming reply */
	Mnt*	m;		/* Mount device during rpc */
	Rendez	r;		/* Place to hang out */
	char*	rpc;		/* I/O Data buffer */
	char	done;		/* Rpc completed */
	char	flushed;	/* Flush was sent */
	ushort	flushtag;	/* Tag flush sent on */
	char	flush[MAXMSG];	/* Somewhere to build flush */
};

struct Mntalloc
{
	Lock	l;
	Mnt*	list;		/* Mount devices in use */
	Mnt*	mntfree;	/* Free list */
	Mntrpc*	rpcfree;
	ulong	id;
	int	rpctag;
}mntalloc;

#define MAXRPC		(MAXFDATA+MAXMSG)
#define limit(n, max)	(n > max ? max : n)

static void	mattach(Mnt*, Chan*, char*);
/* static void	mntauth(Mnt *, Mntrpc *, char *, ushort); */
static Mnt*	mntchk(Chan*);
static void	mntdirfix(uchar*, Chan*);
static int	mntflush(Mnt*, Mntrpc*);
static void	mntfree(Mntrpc*);
static void	mntgate(Mnt*);
static void	mntpntfree(Mnt*);
static void	mntqrm(Mnt*, Mntrpc*);
static Mntrpc*	mntralloc(Chan*);
static long	mntrdwr(int , Chan*, void*,long , ulong);
static long	mnt9prdwr(int , Chan*, void*,long , ulong);
static void	mntrpcread(Mnt*, Mntrpc*);
static void	mountio(Mnt*, Mntrpc*);
static void	mountmux(Mnt*, Mntrpc*);
static void	mountrpc(Mnt*, Mntrpc*);
static int	rpcattn(void*);
static void	mclose(Mnt*, Chan*);
static Chan*	mntchan(void);

static int	defmaxmsg = MAXFDATA;
static int	mntdebug;

enum
{
	Tagspace	= 1,
	Tagfls		= 0x8000,
	Tagend		= 0xfffe
};

void
mntinit(void)
{
	mntalloc.id = 1;
	mntalloc.rpctag = Tagspace;

	fmtinstall('F', fcallconv);
}

Chan*
mntattach(void *muxattach)
{
	Mnt *m;
	Chan *mc;
	Chan *c;
	char buf[NAMELEN];
	struct {
		Chan	*chan;
		char	*spec;
		int	flags;
	} mntparam;

	memmove(&mntparam, muxattach, sizeof(mntparam));
	c = mntparam.chan;

	lock(&mntalloc.l);
	for(m = mntalloc.list; m; m = m->list) {
		if(m->c == c && m->id) {
			lock(&m->r.l);
			if(m->id && m->r.ref > 0 && m->c == c) {
				unlock(&mntalloc.l);
				m->r.ref++;
				unlock(&m->r.l);
				c = mntchan();
				if(waserror()) {
					chanfree(c);
					nexterror();
				}
				mattach(m, c, mntparam.spec);
				poperror();
				return c;
			}
			unlock(&m->r.l);	
		}
	}

	m = mntalloc.mntfree;
	if(m != 0)
		mntalloc.mntfree = m->list;	
	else {
		m = mallocz(sizeof(Mnt)+MAXRPC);
		if(m == 0) {
			unlock(&mntalloc.l);
			error("no mount devices");
		}
		m->flushbase = Tagfls;
		m->flushtag = Tagfls;
	}
	m->list = mntalloc.list;
	mntalloc.list = m;
	m->id = mntalloc.id++;
	lock(&m->r.l);
	unlock(&mntalloc.l);

	m->r.ref = 1;
	m->queue = 0;
	m->rip = 0;
	m->c = c;
	m->c->flag |= CMSG;
	m->blocksize = defmaxmsg;
	m->flags = mntparam.flags;

	refinc(&m->c->r);

	sprint(buf, "#M%d", m->id);
	m->tree.root = ptenter(&m->tree, 0, buf);

	unlock(&m->r.l);

	c = mntchan();
	if(waserror()) {
		mclose(m, c);
		/* Close must not be called since it will
		 * call mnt recursively
		 */
		chanfree(c);
		nexterror();
	}

	mattach(m, c, mntparam.spec);
	poperror();

	/*
	 * Detect a recursive mount for a mount point served by exportfs.
	 * If CHDIR is clear in the returned qid, the foreign server is
	 * requesting the mount point be folded into the connection
	 * to the exportfs. In this case the remote mount driver does
	 * the multiplexing.
	 */
	mc = m->c;
	if(mc->type == devno('M', 0) && (c->qid.path&CHDIR) == 0) {
		mclose(m, c);
		c->qid.path |= CHDIR;
		c->u.mntptr = mc->u.mntptr;
		c->mchan = c->u.mntptr->c;
		c->mqid = c->qid;
		c->path = c->u.mntptr->tree.root;
		refinc(&c->path->r);
		refinc(&c->u.mntptr->r);
	}

	return c;
}

Chan*
mntchan(void)
{
	Chan *c;

	c = devattach('M', 0);
	lock(&mntalloc.l);
	c->dev = mntalloc.id++;
	unlock(&mntalloc.l);

	return c;
}

void
mattach(Mnt *m, Chan *c, char *spec)
{
	Mntrpc *r;

	r = mntralloc(0);
	c->u.mntptr = m;

	if(waserror()) {
		mntfree(r);
		nexterror();
	}

	r->request.type = Tattach;
	r->request.fid = c->fid;
	memmove(r->request.uname, up->user, NAMELEN);
	strncpy(r->request.aname, spec, NAMELEN);

	mountrpc(m, r);

	c->qid = r->reply.qid;
	c->mchan = m->c;
	c->mqid = c->qid;
	c->path = m->tree.root;
	refinc(&c->path->r);

	poperror();
	mntfree(r);
}

Chan*
mntclone(Chan *c, Chan *xnc)
{
	Mnt *m;
	volatile struct { Chan *nc; } nc;
	volatile struct { Mntrpc *r; } r;
	volatile int alloc = 0;

	nc.nc = xnc;
	m = mntchk(c);
	r.r = mntralloc(c);
	if(nc.nc == 0) {
		nc.nc = newchan();
		alloc = 1;
	}
	if(waserror()) {
		mntfree(r.r);
		if(alloc)
			cclose(nc.nc);
		nexterror();
	}

	r.r->request.type = Tclone;
	r.r->request.fid = c->fid;
	r.r->request.newfid = nc.nc->fid;
	mountrpc(m, r.r);

	devclone(c, nc.nc);
	nc.nc->mqid = c->qid;
	refinc(&m->r);

	USED(alloc);
	poperror();
	mntfree(r.r);
	return nc.nc;
}

int	 
mntwalk(Chan *c, char *name)
{
	Mnt *m;
	Path *op;
	volatile struct { Mntrpc *r; } r;

	m = mntchk(c);
	r.r = mntralloc(c);
	if(waserror()) {
		mntfree(r.r);
		return 0;
	}
	r.r->request.type = Twalk;
	r.r->request.fid = c->fid;
	strncpy(r.r->request.name, name, NAMELEN);
	mountrpc(m, r.r);

	c->qid = r.r->reply.qid;
	op = c->path;
	c->path = ptenter(&m->tree, op, name);

	refdec(&op->r);

	poperror();
	mntfree(r.r);
	return 1;
}

void	 
mntstat(Chan *c, char *dp)
{
	Mnt *m;
	volatile struct { Mntrpc *r; } r;

	m = mntchk(c);
	r.r = mntralloc(c);
	if(waserror()) {
		mntfree(r.r);
		nexterror();
	}
	r.r->request.type = Tstat;
	r.r->request.fid = c->fid;
	mountrpc(m, r.r);

	memmove(dp, r.r->reply.stat, DIRLEN);
	mntdirfix((uchar*)dp, c);
	poperror();
	mntfree(r.r);
}

Chan*
mntopen(Chan *c, int omode)
{
	Mnt *m;
	volatile struct { Mntrpc *r; } r;

	m = mntchk(c);
	r.r = mntralloc(c);
	if(waserror()) {
		mntfree(r.r);
		nexterror();
	}
	r.r->request.type = Topen;
	r.r->request.fid = c->fid;
	r.r->request.mode = omode;
	mountrpc(m, r.r);

	c->qid = r.r->reply.qid;
	c->offset = 0;
	c->mode = openmode(omode);
	c->flag |= COPEN;
	poperror();
	mntfree(r.r);

	return c;
}

void	 
mntcreate(Chan *c, char *name, int omode, ulong perm)
{
	Mnt *m;
	volatile struct { Mntrpc *r; } r;

	m = mntchk(c);
	r.r = mntralloc(c);
	if(waserror()) {
		mntfree(r.r);
		nexterror();
	}
	r.r->request.type = Tcreate;
	r.r->request.fid = c->fid;
	r.r->request.mode = omode;
	r.r->request.perm = perm;
	strncpy(r.r->request.name, name, NAMELEN);
	mountrpc(m, r.r);

	c->qid = r.r->reply.qid;
	c->flag |= COPEN;
	c->mode = openmode(omode);
	poperror();
	mntfree(r.r);
}

void	 
mntclunk(Chan *c, int t)
{
	Mnt *m;
	Mntrpc *r;

	m = mntchk(c);
	r = mntralloc(c);
	if(waserror()){
		mntfree(r);
		mclose(m, c);
		nexterror();
	}

	r->request.type = t;
	r->request.fid = c->fid;
	mountrpc(m, r);
	mntfree(r);
	mclose(m, c);
	poperror();
}

void
mclose(Mnt *m, Chan *c)
{
	Mntrpc *q, *r;

	if(refdec(&m->r) != 0)
		return;

	c->path = 0;
	ptclose(&m->tree);

	for(q = m->queue; q; q = r) {
		r = q->list;
		q->flushed = 0;
		mntfree(q);
	}
	m->id = 0;
	cclose(m->c);
	mntpntfree(m);
}

void
mntpntfree(Mnt *m)
{
	Mnt *f, **l;

	lock(&mntalloc.l);
	l = &mntalloc.list;
	for(f = *l; f; f = f->list) {
		if(f == m) {
			*l = m->list;
			break;
		}
		l = &f->list;
	}

	m->list = mntalloc.mntfree;
	mntalloc.mntfree = m;
	unlock(&mntalloc.l);
}

void
mntclose(Chan *c)
{
	mntclunk(c, Tclunk);
}

void	 
mntremove(Chan *c)
{
	mntclunk(c, Tremove);
}

void
mntwstat(Chan *c, char *dp)
{
	Mnt *m;
	volatile struct { Mntrpc *r; } r;

	m = mntchk(c);
	r.r = mntralloc(c);
	if(waserror()) {
		mntfree(r.r);
		nexterror();
	}
	r.r->request.type = Twstat;
	r.r->request.fid = c->fid;
	memmove(r.r->request.stat, dp, DIRLEN);
	mountrpc(m, r.r);
	poperror();
	mntfree(r.r);
}

long	 
mntread9p(Chan *c, void *buf, long n, ulong offset)
{
	return mnt9prdwr(Tread, c, buf, n, offset);
}

long	 
mntread(Chan *c, void *buf, long n, ulong offset)
{
	int isdir;
	uchar *p, *e;

	isdir = 0;
	if(c->qid.path & CHDIR)
		isdir = 1;

	p = buf;
	n = mntrdwr(Tread, c, buf, n, offset);
	if(isdir) {
		for(e = &p[n]; p < e; p += DIRLEN)
			mntdirfix(p, c);
	}
	return n;
}

long	 
mntwrite9p(Chan *c, void *buf, long n, ulong offset)
{
	return mnt9prdwr(Twrite, c, buf, n, offset);
}

long	 
mntwrite(Chan *c, void *buf, long n, ulong offset)
{
	return mntrdwr(Twrite, c, buf, n, offset);
}

long
mnt9prdwr(int type, Chan *c, void *buf, long n, ulong offset)
{
	Mnt *m;
 	long nr;
	Mntrpc *r;

	if(n > MAXRPC-32) {
		if(type == Twrite)
			error("write9p too long");
		n = MAXRPC-32;
	}

	m = mntchk(c);
	r = mntralloc(c);
	if(waserror()) {
		mntfree(r);
		nexterror();
	}
	r->request.type = type;
	r->request.fid = c->fid;
	r->request.offset = offset;
	r->request.data = buf;
	r->request.count = n;
	mountrpc(m, r);
	nr = r->reply.count;
	if(nr > r->request.count)
		nr = r->request.count;

	if(type == Tread)
		memmove(buf, r->reply.data, nr);

	poperror();
	mntfree(r);
	return nr;
}

long
mntrdwr(int type, Chan *c, void *buf, long n, ulong offset)
{
	Mnt *m;
	char *uba;
	ulong cnt, nr;
 	Mntrpc *r;

	m = mntchk(c);
	uba = buf;
	cnt = 0;
	for(;;) {
		r = mntralloc(c);
		if(waserror()) {
			mntfree(r);
			nexterror();
		}
		r->request.type = type;
		r->request.fid = c->fid;
		r->request.offset = offset;
		r->request.data = uba;
		r->request.count = limit(n, m->blocksize);
		mountrpc(m, r);
		nr = r->reply.count;
		if((int)nr > r->request.count)
			nr = r->request.count;

		if(type == Tread)
			memmove(uba, r->reply.data, nr);

		poperror();
		mntfree(r);
		offset += nr;
		uba += nr;
		cnt += nr;
		n -= nr;
		if((int)nr != r->request.count || n == 0)
			break;
	}
	return cnt;
}

void
mountrpc(Mnt *m, Mntrpc *r)
{
	int t;

	r->reply.tag = 0;
	r->reply.type = 4;

	mountio(m, r);

	t = r->reply.type;
	switch(t) {
	case Rerror:
		error(r->reply.ename);
	case Rflush:
		error(Eintr);
	default:
		if(t == r->request.type+1)
			break;
		print("mnt: mismatch rep 0x%lux T%d R%d rq %d fls %d rp %d\n",
			r, r->request.type, r->reply.type, r->request.tag, 
			r->flushtag, r->reply.tag);
		error(Emountrpc);
	}
}

void
mountio(Mnt *xm, Mntrpc *xr)
{
	int n;
	Mnt *m;
	Mntrpc *r;

	m = xm;
	r = xr;

	lock(&m->r.l);
	r->flushed = 0;
	r->m = m;
	r->list = m->queue;
	m->queue = r;
	unlock(&m->r.l);

	/* Transmit a file system rpc */
	n = convS2M(&r->request, r->rpc);
	if(mntdebug)
		print("mnt: <- %F\n", &r->request);
	if(waserror()) {
		if(mntflush(m, r) == 0)
			nexterror();
	}
	else {
		if(devchar[m->c->type] == L'M'){
			if(mnt9prdwr(Twrite, m->c, r->rpc, n, 0) != n)
				error(Emountrpc);
		}
		else
		if((*devtab[m->c->type].write)(m->c, r->rpc, n, 0) != n)
				error(Emountrpc);
		poperror();
	}

	/* Gate readers onto the mount point one at a time */
	for(;;) {
		lock(&m->r.l);
		if(m->rip == 0)
			break;
		unlock(&m->r.l);
		if(waserror()) {
			if(mntflush(m, r) == 0)
				nexterror();
			continue;
		}
		rendsleep(&r->r, rpcattn, r);
		poperror();
		if(r->done)
			return;
	}
	m->rip = 1;
	unlock(&m->r.l);
	while(r->done == 0) {
		mntrpcread(m, r);
		mountmux(m, r);
	}
	mntgate(m);
}

void
mntrpcread(Mnt *m, Mntrpc *r)
{
	char *buf;
	int n, cn, len;

	buf = r->rpc;
	len = MAXRPC;
	n = m->npart;
	if(n > 0){
		memmove(buf, m->part, n);
		buf += n;
		len -= n;
		m->npart = 0;
		goto chk;
	}


	for(;;) {
		if(waserror()) {
			if(mntflush(m, r) == 0) {
				mntgate(m);
				nexterror();
			}
			continue;
		}

		r->reply.tag = 0;
		r->reply.type = 0;

		if(devchar[m->c->type] == L'M')
			n = mnt9prdwr(Tread, m->c, buf, len, 0);
		else
			n = devtab[m->c->type].read(m->c, buf, len, 0);

		poperror();

		if(n == 0)
			continue;

		buf += n;
		len -= n;
	chk:
		n = buf - r->rpc;

		/* convM2S returns size of correctly decoded message */
		cn = convM2S(r->rpc, &r->reply, n);
		if(cn < 0)
			error("bad message type in devmnt");
		if(cn > 0) {
			n -= cn;
			if(n < 0)
				panic("negative size in devmnt");
			m->npart = n;
			if(n != 0)
				memmove(m->part, r->rpc+cn, n);
			if(mntdebug)
				print("mnt: %s: <- %F\n", up->user, &r->reply);
			return;
		}
	}
}

void
mntgate(Mnt *m)
{
	Mntrpc *q;

	lock(&m->r.l);
	m->rip = 0;
	for(q = m->queue; q; q = q->list) {
		if(q->done == 0) {
			lock(&q->r.l);
			if(q->r.t) {
				unlock(&q->r.l);
				unlock(&m->r.l);
				rendwakeup(&q->r);
				return;
			}
			unlock(&q->r.l);
		}
	}
	unlock(&m->r.l);
}

void
mountmux(Mnt *m, Mntrpc *r)
{
	char *dp;
	Mntrpc **l, *q;

	lock(&m->r.l);
	l = &m->queue;
	for(q = *l; q; q = q->list) {
		if(q->request.tag == r->reply.tag
		|| (q->flushed && q->flushtag == r->reply.tag)) {
			*l = q->list;
			unlock(&m->r.l);
			if(q != r) {		/* Completed someone else */
				dp = q->rpc;
				q->rpc = r->rpc;
				r->rpc = dp;
				q->reply = r->reply;
				q->done = 1;
				rendwakeup(&q->r);
			}
			else
				q->done = 1;
			return;
		}
		l = &q->list;
	}
	unlock(&m->r.l);
}

int
mntflush(Mnt *m, Mntrpc *r)
{
	int n, l;
	Fcall flush;

	lock(&m->r.l);
	r->flushtag = m->flushtag++;
	if(m->flushtag == Tagend)
		m->flushtag = m->flushbase;
	r->flushed = 1;
	unlock(&m->r.l);

	flush.type = Tflush;
	flush.tag = r->flushtag;
	flush.oldtag = r->request.tag;
	n = convS2M(&flush, r->flush);

	if(waserror()) {
		if(strcmp(threaderr(), Eintr) == 0)
			return 1;
		mntqrm(m, r);
		return 0;
	}
	l = (*devtab[m->c->type].write)(m->c, r->flush, n, 0);
	if(l != n)
		error(Ehungup);
	poperror();
	return 1;
}

Mntrpc *
mntralloc(Chan *c)
{
	Mntrpc *new;

	lock(&mntalloc.l);
	new = mntalloc.rpcfree;
	if(new != 0)
		mntalloc.rpcfree = new->list;
	else {
		new = mallocz(sizeof(Mntrpc)+MAXRPC);
		if(new == 0) {
			unlock(&mntalloc.l);
			error(Enovmem);
		}
		new->rpc = (char*)new+sizeof(Mntrpc);
		new->request.tag = mntalloc.rpctag++;
	}
	unlock(&mntalloc.l);
	new->c = c;
	new->done = 0;
	new->flushed = 0;
	return new;
}

void
mntfree(Mntrpc *r)
{
	lock(&mntalloc.l);
	r->list = mntalloc.rpcfree;
	mntalloc.rpcfree = r;
	unlock(&mntalloc.l);
}

void
mntqrm(Mnt *m, Mntrpc *r)
{
	Mntrpc **l, *f;

	lock(&m->r.l);
	r->done = 1;
	r->flushed = 0;

	l = &m->queue;
	for(f = *l; f; f = f->list) {
		if(f == r) {
			*l = r->list;
			break;
		}
		l = &f->list;
	}
	unlock(&m->r.l);
}

Mnt*
mntchk(Chan *c)
{
	Mnt *m;

	m = c->u.mntptr;

	/*
	 * Was it closed and reused
	 */
	if(m->id == 0 || m->id >= c->dev)
		error(Eshutdown);

	return m;
}

void
mntdirfix(uchar *dirbuf, Chan *c)
{
	dirbuf[DIRLEN-4] = devchar[c->type]>>0;
	dirbuf[DIRLEN-3] = devchar[c->type]>>8;
	dirbuf[DIRLEN-2] = c->dev;
	dirbuf[DIRLEN-1] = c->dev>>8;
}

int
rpcattn(void *v)
{
	Mntrpc *r;
	r = v;
	return r->done || r->m->rip == 0;
}
