#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

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
	uvlong	stime;		/* start time for mnt statistics */
	ulong	reqlen;		/* request length for mnt statistics */
	ulong	replen;		/* reply length for mnt statistics */
	Mntrpc*	flushed;	/* message this one flushes */
};

struct Mntalloc
{
	Lock;
	Mnt*	list;		/* Mount devices in use */
	Mnt*	mntfree;	/* Free list */
	Mntrpc*	rpcfree;
	int	nrpcfree;
	int	nrpcused;
	ulong	id;
	int	rpctag;
}mntalloc;

void	mattach(Mnt*, Chan*, char*);
void	mntauth(Mnt*, Mntrpc*, char*, ushort);
Mnt*	mntchk(Chan*);
void	mntdirfix(uchar*, Chan*);
Mntrpc*	mntflushalloc(Mntrpc*);
void	mntflushfree(Mnt*, Mntrpc*);
void	mntfree(Mntrpc*);
void	mntgate(Mnt*);
void	mntpntfree(Mnt*);
void	mntqrm(Mnt*, Mntrpc*);
Mntrpc*	mntralloc(Chan*);
long	mntrdwr(int, Chan*, void*, long, vlong);
long	mnt9prdwr(int, Chan*, void*, long, vlong);
void	mntrpcread(Mnt*, Mntrpc*);
void	mountio(Mnt*, Mntrpc*);
void	mountmux(Mnt*, Mntrpc*);
void	mountrpc(Mnt*, Mntrpc*);
int	rpcattn(void*);
void	mclose(Mnt*, Chan*);
Chan*	mntchan(void);

void (*mntstats)(int, Chan*, uvlong, ulong);

enum
{
	Tagspace	= 1,
};

static void
mntreset(void)
{
	mntalloc.id = 1;
	mntalloc.rpctag = Tagspace;

	cinit();
}

static Chan*
mntattach(char *muxattach)
{
	Mnt *m;
	Chan *c, *mc;
	char buf[NAMELEN];
	struct bogus{
		Chan	*chan;
		char	*spec;
		int	flags;
	}bogus;

	bogus = *((struct bogus *)muxattach);
	c = bogus.chan;

	lock(&mntalloc);
	for(m = mntalloc.list; m; m = m->list) {
		if(m->c == c && m->id) {
			lock(m);
			if(m->id && m->ref > 0 && m->c == c) {
				m->ref++;
				unlock(m);
				unlock(&mntalloc);
				c = mntchan();
				if(waserror()) {
					chanfree(c);
					nexterror();
				}
				mattach(m, c, bogus.spec);
				poperror();
				if(bogus.flags&MCACHE)
					c->flag |= CCACHE;
				return c;
			}
			unlock(m);
		}
	}

	m = mntalloc.mntfree;
	if(m != 0)
		mntalloc.mntfree = m->list;
	else {
		m = malloc(sizeof(Mnt));
		if(m == 0) {
			unlock(&mntalloc);
			exhausted("mount devices");
		}
	}
	m->list = mntalloc.list;
	mntalloc.list = m;
	m->id = mntalloc.id++;
	unlock(&mntalloc);

	lock(m);
	m->ref = 1;
	m->queue = 0;
	m->rip = 0;
	m->c = c;
	m->c->flag |= CMSG;
	if(strncmp(bogus.spec, "mntblk=", 7) == 0) {
		m->blocksize = strtoul(bogus.spec+7, 0, 0);
		if(m->blocksize > MAXFDATA)
			m->blocksize = MAXFDATA;
		print("mount blk %d\n", m->blocksize);
		bogus.spec = "";
	}
	else
		m->blocksize = MAXFDATA;
	m->flags = bogus.flags & ~MCACHE;

	incref(m->c);

	sprint(buf, "#M%lud", m->id);

	unlock(m);

	c = mntchan();
	if(waserror()) {
		mclose(m, c);
		/* Close must not be called since it will
		 * call mnt recursively
		 */
		chanfree(c);
		nexterror();
	}

	mattach(m, c, bogus.spec);
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
		c->mntptr = mc->mntptr;
		c->mchan = c->mntptr->c;
		c->mqid = c->qid;
		incref(c->mntptr);
	}

	if(bogus.flags & MCACHE)
		c->flag |= CCACHE;

	return c;
}

Chan*
mntchan(void)
{
	Chan *c;

	c = devattach('M', 0);
	lock(&mntalloc);
	c->dev = mntalloc.id++;
	unlock(&mntalloc);

	return c;
}

void
mattach(Mnt *m, Chan *c, char *spec)
{
	ulong id;
	Mntrpc *r;

	r = mntralloc(0);
	c->mntptr = m;

	if(waserror()) {
		mntfree(r);
		nexterror();
	}

	r->request.type = Tattach;
	r->request.fid = c->fid;
	memmove(r->request.uname, up->user, NAMELEN);
	strncpy(r->request.aname, spec, NAMELEN);
	id = authrequest(m->c->session, &r->request);
	mountrpc(m, r);
	authreply(m->c->session, id, &r->reply);

	c->qid = r->reply.qid;
	c->mchan = m->c;
	c->mqid = c->qid;

	poperror();
	mntfree(r);
}

static Chan*
mntclone(Chan *c, Chan *nc)
{
	Mnt *m;
	Mntrpc *r;
	int alloc = 0;

	m = mntchk(c);
	r = mntralloc(c);
	if(nc == 0) {
		nc = newchan();
		alloc = 1;
	}
	if(waserror()) {
		mntfree(r);
		if(alloc)
			cclose(nc);
		nexterror();
	}

	r->request.type = Tclone;
	r->request.fid = c->fid;
	r->request.newfid = nc->fid;
	mountrpc(m, r);

	devclone(c, nc);
	nc->mqid = c->qid;
	incref(m);

	USED(alloc);
	poperror();
	mntfree(r);
	return nc;
}

static int
mntwalk(Chan *c, char *name)
{
	Mnt *m;
	Mntrpc *r;

	m = mntchk(c);
	r = mntralloc(c);
	if(waserror()) {
		mntfree(r);
		return 0;
	}
	r->request.type = Twalk;
	r->request.fid = c->fid;
	strncpy(r->request.name, name, NAMELEN);
	mountrpc(m, r);

	c->qid = r->reply.qid;

	poperror();
	mntfree(r);
	return 1;
}

static void
mntstat(Chan *c, char *dp)
{
	Mnt *m;
	Mntrpc *r;

	m = mntchk(c);
	r = mntralloc(c);
	if(waserror()) {
		mntfree(r);
		nexterror();
	}
	r->request.type = Tstat;
	r->request.fid = c->fid;
	mountrpc(m, r);

	memmove(dp, r->reply.stat, DIRLEN);
	mntdirfix((uchar*)dp, c);
	poperror();
	mntfree(r);
}

static Chan*
mntopen(Chan *c, int omode)
{
	Mnt *m;
	Mntrpc *r;

	m = mntchk(c);
	r = mntralloc(c);
	if(waserror()) {
		mntfree(r);
		nexterror();
	}
	r->request.type = Topen;
	r->request.fid = c->fid;
	r->request.mode = omode;
	mountrpc(m, r);

	c->qid = r->reply.qid;
	c->offset = 0;
	c->mode = openmode(omode);
	c->flag |= COPEN;
	poperror();
	mntfree(r);

	if(c->flag & CCACHE)
		copen(c);

	return c;
}

static void
mntcreate(Chan *c, char *name, int omode, ulong perm)
{
	Mnt *m;
	Mntrpc *r;

	m = mntchk(c);
	r = mntralloc(c);
	if(waserror()) {
		mntfree(r);
		nexterror();
	}
	r->request.type = Tcreate;
	r->request.fid = c->fid;
	r->request.mode = omode;
	r->request.perm = perm;
	strncpy(r->request.name, name, NAMELEN);
	mountrpc(m, r);

	c->qid = r->reply.qid;
	c->flag |= COPEN;
	c->mode = openmode(omode);
	poperror();
	mntfree(r);

	if(c->flag & CCACHE)
		copen(c);
}

static void
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
mclose(Mnt *m, Chan*)
{
	Mntrpc *q, *r;

	if(decref(m) != 0)
		return;

	for(q = m->queue; q; q = r) {
		r = q->list;
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

	lock(&mntalloc);
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
	unlock(&mntalloc);
}

static void
mntclose(Chan *c)
{
	mntclunk(c, Tclunk);
}

static void
mntremove(Chan *c)
{
	mntclunk(c, Tremove);
}

static void
mntwstat(Chan *c, char *dp)
{
	Mnt *m;
	Mntrpc *r;

	m = mntchk(c);
	r = mntralloc(c);
	if(waserror()) {
		mntfree(r);
		nexterror();
	}
	r->request.type = Twstat;
	r->request.fid = c->fid;
	memmove(r->request.stat, dp, DIRLEN);
	mountrpc(m, r);
	poperror();
	mntfree(r);
}

long
mntread9p(Chan *c, void *buf, long n, vlong off)
{
	return mnt9prdwr(Tread, c, buf, n, off);
}

static long
mntread(Chan *c, void *buf, long n, vlong off)
{
	uchar *p, *e;
	int nc, cache, isdir;

	isdir = 0;
	cache = c->flag & CCACHE;
	if(c->qid.path & CHDIR) {
		cache = 0;
		isdir = 1;
	}

	p = buf;
	if(cache) {
		nc = cread(c, buf, n, off);
		if(nc > 0) {
			n -= nc;
			if(n == 0)
				return nc;
			p += nc;
			off += nc;
		}
		n = mntrdwr(Tread, c, p, n, off);
		cupdate(c, p, n, off);
		return n + nc;
	}

	n = mntrdwr(Tread, c, buf, n, off);
	if(isdir) {
		for(e = &p[n]; p < e; p += DIRLEN)
			mntdirfix(p, c);
	}

	return n;
}

long
mntwrite9p(Chan *c, void *buf, long n, vlong off)
{
	return mnt9prdwr(Twrite, c, buf, n, off);
}

static long
mntwrite(Chan *c, void *buf, long n, vlong off)
{
	return mntrdwr(Twrite, c, buf, n, off);
}

long
mnt9prdwr(int type, Chan *c, void *buf, long n, vlong off)
{
	Mnt *m;
 	ulong nr;
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
	r->request.offset = off;
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
mntrdwr(int type, Chan *c, void *buf, long n, vlong off)
{
	Mnt *m;
 	Mntrpc *r;
	char *uba;
	int cache;
	ulong cnt, nr, nreq;

	m = mntchk(c);
	uba = buf;
	cnt = 0;
	cache = c->flag & CCACHE;
	if(c->qid.path & CHDIR)
		cache = 0;
	for(;;) {
		r = mntralloc(c);
		if(waserror()) {
			mntfree(r);
			nexterror();
		}
		r->request.type = type;
		r->request.fid = c->fid;
		r->request.offset = off;
		r->request.data = uba;
		if(n > m->blocksize){
			if(c->qid.path & CHDIR)
				r->request.count = (m->blocksize/DIRLEN)*DIRLEN;
			else
				r->request.count = m->blocksize;
		} else
			r->request.count = n;
		mountrpc(m, r);
		nreq = r->request.count;
		nr = r->reply.count;
		if(nr > nreq)
			nr = nreq;

		if(type == Tread)
			memmove(uba, r->reply.data, nr);
		else if(cache)
			cwrite(c, (uchar*)uba, nr, off);

		poperror();
		mntfree(r);
		off += nr;
		uba += nr;
		cnt += nr;
		n -= nr;
		if(nr != nreq || n == 0 || up->nnote)
			break;
	}
	return cnt;
}

void
mountrpc(Mnt *m, Mntrpc *r)
{
	int t;

	r->reply.tag = 0;
	r->reply.type = Tmax;	/* can't ever be a valid message type */

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
		print("mnt: proc %s %lud: mismatch rep 0x%lux tag %d fid %d T%d R%d rp %d\n",
			up->text, up->pid,
			r, r->request.tag, r->request.fid, r->request.type, r->reply.type,
			r->reply.tag);
		error(Emountrpc);
	}
}

void
mountio(Mnt *m, Mntrpc *r)
{
	int n;

	while(waserror()) {
		if(m->rip == up)
			mntgate(m);
		if(strcmp(up->error, Eintr) != 0){
			mntflushfree(m, r);
			nexterror();
		}
		r = mntflushalloc(r);
	}

	lock(m);
	r->m = m;
	r->list = m->queue;
	m->queue = r;
	unlock(m);

	/* Transmit a file system rpc */
	n = convS2M(&r->request, r->rpc);
	if(n < 0)
		panic("bad message type in mountio");
	if(devtab[m->c->type]->dc == L'M'){
		if(mnt9prdwr(Twrite, m->c, r->rpc, n, 0) != n)
			error(Emountrpc);
	}else{
		if(devtab[m->c->type]->write(m->c, r->rpc, n, 0) != n)
			error(Emountrpc);
	}
	r->stime = fastticks(nil);
	r->reqlen = n;

	/* Gate readers onto the mount point one at a time */
	for(;;) {
		lock(m);
		if(m->rip == 0)
			break;
		unlock(m);
		sleep(&r->r, rpcattn, r);
		if(r->done){
			poperror();
			mntflushfree(m, r);
			return;
		}
	}
	m->rip = up;
	unlock(m);
	while(r->done == 0) {
		mntrpcread(m, r);
		mountmux(m, r);
	}
	mntgate(m);
	poperror();
	mntflushfree(m, r);
}

void
mntrpcread(Mnt *m, Mntrpc *r)
{
	int n;

	for(;;) {
		r->reply.type = 0;
		r->reply.tag = 0;
		if(devtab[m->c->type]->dc == L'M')
			n = mnt9prdwr(Tread, m->c, r->rpc, MAXRPC, 0);
		else
			n = devtab[m->c->type]->read(m->c, r->rpc, MAXRPC, 0);
		if(n == 0)
			continue;

		r->replen = n;
		if(convM2S(r->rpc, &r->reply, n) != 0)
			return;
	}
}

void
mntgate(Mnt *m)
{
	Mntrpc *q;

	lock(m);
	m->rip = 0;
	for(q = m->queue; q; q = q->list) {
		if(q->done == 0)
		if(wakeup(&q->r))
			break;
	}
	unlock(m);
}

void
mountmux(Mnt *m, Mntrpc *r)
{
	char *dp;
	Mntrpc **l, *q;

	lock(m);
	l = &m->queue;
	for(q = *l; q; q = q->list) {
		/* look for a reply to a message */
		if(q->request.tag == r->reply.tag) {
			*l = q->list;
			if(q != r) {
				/*
				 * Completed someone else.
				 * Trade pointers to receive buffer.
				 */
				dp = q->rpc;
				q->rpc = r->rpc;
				r->rpc = dp;
				q->reply = r->reply;
			}
			q->done = 1;
			unlock(m);
			if(mntstats != nil)
				(*mntstats)(q->request.type,
					m->c, q->stime,
					q->reqlen + r->replen);
			if(q != r)
				wakeup(&q->r);
			return;
		}
		l = &q->list;
	}
	unlock(m);
}

/*
 * Create a new flush request and chain the previous
 * requests from it
 */
Mntrpc*
mntflushalloc(Mntrpc *r)
{
	Mntrpc *fr;

	fr = mntralloc(0);

	fr->request.type = Tflush;
	if(r->request.type == Tflush)
		fr->request.oldtag = r->request.oldtag;
	else
		fr->request.oldtag = r->request.tag;
	fr->flushed = r;

	return fr;
}

/*
 *  Free a chain of flushes.  Remove each unanswered
 *  flush and the original message from the unanswered
 *  request queue.  Mark the original message as done
 *  and if it hasn't been answered set the reply to to
 *  Rflush.
 */
void
mntflushfree(Mnt *m, Mntrpc *r)
{
	Mntrpc *fr;

	while(r){
		fr = r->flushed;
		if(!r->done){
			r->reply.type = Rflush;
			mntqrm(m, r);
		}
		if(fr)
			mntfree(r);
		r = fr;
	}
}

Mntrpc*
mntralloc(Chan *c)
{
	Mntrpc *new;

	lock(&mntalloc);
	new = mntalloc.rpcfree;
	if(new == nil){
		new = malloc(sizeof(Mntrpc));
		if(new == nil) {
			unlock(&mntalloc);
			exhausted("mount rpc header");
		}
		/*
		 * The header is split from the data buffer as
		 * mountmux may swap the buffer with another header.
		 */
		new->rpc = mallocz(MAXRPC, 0);
		if(new->rpc == nil){
			free(new);
			unlock(&mntalloc);
			exhausted("mount rpc buffer");
		}
		new->request.tag = mntalloc.rpctag++;
	}
	else {
		mntalloc.rpcfree = new->list;
		mntalloc.nrpcfree--;
	}
	mntalloc.nrpcused++;
	unlock(&mntalloc);
	new->c = c;
	new->done = 0;
	new->flushed = nil;
	return new;
}

void
mntfree(Mntrpc *r)
{
	lock(&mntalloc);
	if(mntalloc.nrpcfree >= 10){
		free(r->rpc);
		free(r);
	}
	else{
		r->list = mntalloc.rpcfree;
		mntalloc.rpcfree = r;
		mntalloc.nrpcfree++;
	}
	mntalloc.nrpcused--;
	unlock(&mntalloc);
}

void
mntqrm(Mnt *m, Mntrpc *r)
{
	Mntrpc **l, *f;

	lock(m);
	r->done = 1;

	l = &m->queue;
	for(f = *l; f; f = f->list) {
		if(f == r) {
			*l = r->list;
			break;
		}
		l = &f->list;
	}
	unlock(m);
}

Mnt*
mntchk(Chan *c)
{
	Mnt *m;

	m = c->mntptr;

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
	int r;

	r = devtab[c->type]->dc;
	dirbuf[DIRLEN-4] = r>>0;
	dirbuf[DIRLEN-3] = r>>8;
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

Dev mntdevtab = {
	'M',
	"mnt",

	mntreset,
	devinit,
	mntattach,
	mntclone,
	mntwalk,
	mntstat,
	mntopen,
	mntcreate,
	mntclose,
	mntread,
	devbread,
	mntwrite,
	devbwrite,
	mntremove,
	mntwstat,
};
