#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

/*
 * References are managed as follows:
 * The channel to the server - a network connection or pipe - has one
 * reference for every Chan open on the server.  The server channel has
 * c->mux set to the Mnt used for muxing control to that server.  Mnts
 * have no reference count; they go away when c goes away.
 * Each channel derived from the mount point has mchan set to c,
 * and increfs/decrefs mchan to manage references on the server
 * connection.
 */

#define MAXRPC (IOHDRSZ+8192)

struct Mntrpc
{
	Chan*	c;		/* Channel for whom we are working */
	Mntrpc*	list;		/* Free/pending list */
	Fcall	request;	/* Outgoing file system protocol message */
	Fcall 	reply;		/* Incoming reply */
	Mnt*	m;		/* Mount device during rpc */
	Rendez	r;		/* Place to hang out */
	uchar*	rpc;		/* I/O Data buffer */
	uint	rpclen;		/* len of buffer */
	Block	*b;		/* reply blocks */
	char	done;		/* Rpc completed */
	uvlong	stime;		/* start time for mnt statistics */
	ulong	reqlen;		/* request length for mnt statistics */
	ulong	replen;		/* reply length for mnt statistics */
	Mntrpc*	flushed;	/* message this one flushes */
};

enum
{
	TAGSHIFT = 5,			/* ulong has to be 32 bits */
	TAGMASK = (1<<TAGSHIFT)-1,
	NMASK = (64*1024)>>TAGSHIFT,
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
	ulong	tagmask[NMASK];
}mntalloc;

Mnt*	mntchk(Chan*);
void	mntdirfix(uchar*, Chan*);
Mntrpc*	mntflushalloc(Mntrpc*, ulong);
void	mntflushfree(Mnt*, Mntrpc*);
void	mntfree(Mntrpc*);
void	mntgate(Mnt*);
void	mntpntfree(Mnt*);
void	mntqrm(Mnt*, Mntrpc*);
Mntrpc*	mntralloc(Chan*, ulong);
long	mntrdwr(int, Chan*, void*, long, vlong);
int	mntrpcread(Mnt*, Mntrpc*);
void	mountio(Mnt*, Mntrpc*);
void	mountmux(Mnt*, Mntrpc*);
void	mountrpc(Mnt*, Mntrpc*);
int	rpcattn(void*);
Chan*	mntchan(void);

char	Esbadstat[] = "invalid directory entry received from server";
char	Enoversion[] = "version not established for mount channel";


void (*mntstats)(int, Chan*, uvlong, ulong);

static void
mntreset(void)
{
	mntalloc.id = 1;
	mntalloc.tagmask[0] = 1;			/* don't allow 0 as a tag */
	mntalloc.tagmask[NMASK-1] = 0x80000000UL;	/* don't allow NOTAG */
	fmtinstall('F', fcallfmt);
	fmtinstall('D', dirfmt);
/* We can't install %M since eipfmt does and is used in the kernel [sape] */

	cinit();
}

/*
 * Version is not multiplexed: message sent only once per connection.
 */
long
mntversion(Chan *c, char *version, int msize, int returnlen)
{
	Fcall f;
	uchar *msg;
	Mnt *m;
	char *v;
	long k, l;
	uvlong oo;
	char buf[128];

	qlock(&c->umqlock);	/* make sure no one else does this until we've established ourselves */
	if(waserror()){
		qunlock(&c->umqlock);
		nexterror();
	}

	/* defaults */
	if(msize == 0)
		msize = MAXRPC;
	if(msize > c->iounit && c->iounit != 0)
		msize = c->iounit;
	v = version;
	if(v == nil || v[0] == '\0')
		v = VERSION9P;

	/* validity */
	if(msize < 0)
		error("bad iounit in version call");
	if(strncmp(v, VERSION9P, strlen(VERSION9P)) != 0)
		error("bad 9P version specification");

	m = c->mux;

	if(m != nil){
		qunlock(&c->umqlock);
		poperror();

		strecpy(buf, buf+sizeof buf, m->version);
		k = strlen(buf);
		if(strncmp(buf, v, k) != 0){
			snprint(buf, sizeof buf, "incompatible 9P versions %s %s", m->version, v);
			error(buf);
		}
		if(returnlen > 0){
			if(returnlen < k)
				error(Eshort);
			memmove(version, buf, k);
		}
		return k;
	}

	f.type = Tversion;
	f.tag = NOTAG;
	f.msize = msize;
	f.version = v;
	msg = malloc(8192+IOHDRSZ);
	if(msg == nil)
		exhausted("version memory");
	if(waserror()){
		free(msg);
		nexterror();
	}
	k = convS2M(&f, msg, 8192+IOHDRSZ);
	if(k == 0)
		error("bad fversion conversion on send");

	lock(c);
	oo = c->offset;
	c->offset += k;
	unlock(c);

	l = devtab[c->type]->write(c, msg, k, oo);

	if(l < k){
		lock(c);
		c->offset -= k - l;
		unlock(c);
		error("short write in fversion");
	}

	/* message sent; receive and decode reply */
	k = devtab[c->type]->read(c, msg, 8192+IOHDRSZ, c->offset);
	if(k <= 0)
		error("EOF receiving fversion reply");

	lock(c);
	c->offset += k;
	unlock(c);

	l = convM2S(msg, k, &f);
	if(l != k)
		error("bad fversion conversion on reply");
	if(f.type != Rversion){
		if(f.type == Rerror)
			error(f.ename);
		error("unexpected reply type in fversion");
	}
	if(f.msize > msize)
		error("server tries to increase msize in fversion");
	if(f.msize<256 || f.msize>1024*1024)
		error("nonsense value of msize in fversion");
	k = strlen(f.version);
	if(strncmp(f.version, v, k) != 0)
		error("bad 9P version returned from server");

	/* now build Mnt associated with this connection */
	lock(&mntalloc);
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
	m->version = nil;
	kstrdup(&m->version, f.version);
	m->id = mntalloc.id++;
	m->q = qopen(10*MAXRPC, 0, nil, nil);
	m->msize = f.msize;
	unlock(&mntalloc);

	if(returnlen > 0){
		if(returnlen < k)
			error(Eshort);
		memmove(version, f.version, k);
	}

	poperror();	/* msg */
	free(msg);

	lock(m);
	m->queue = 0;
	m->rip = 0;

	c->flag |= CMSG;
	c->mux = m;
	m->c = c;
	unlock(m);

	poperror();	/* c */
	qunlock(&c->umqlock);

	return k;
}

Chan*
mntauth(Chan *c, char *spec)
{
	Mnt *m;
	Mntrpc *r;

	m = c->mux;

	if(m == nil){
		mntversion(c, VERSION9P, MAXRPC, 0);
		m = c->mux;
		if(m == nil)
			error(Enoversion);
	}

	c = mntchan();
	if(waserror()) {
		/* Close must not be called since it will
		 * call mnt recursively
		 */
		chanfree(c);
		nexterror();
	}

	r = mntralloc(0, m->msize);

	if(waserror()) {
		mntfree(r);
		nexterror();
	}

	r->request.type = Tauth;
	r->request.afid = c->fid;
	r->request.uname = up->user;
	r->request.aname = spec;
	mountrpc(m, r);

	c->qid = r->reply.aqid;
	c->mchan = m->c;
	incref(m->c);
	c->mqid = c->qid;
	c->mode = ORDWR;

	poperror();	/* r */
	mntfree(r);

	poperror();	/* c */

	return c;

}

static Chan*
mntattach(char *muxattach)
{
	Mnt *m;
	Chan *c;
	Mntrpc *r;
	struct bogus{
		Chan	*chan;
		Chan	*authchan;
		char	*spec;
		int	flags;
	}bogus;

	bogus = *((struct bogus *)muxattach);
	c = bogus.chan;

	m = c->mux;

	if(m == nil){
		mntversion(c, nil, 0, 0);
		m = c->mux;
		if(m == nil)
			error(Enoversion);
	}

	c = mntchan();
	if(waserror()) {
		/* Close must not be called since it will
		 * call mnt recursively
		 */
		chanfree(c);
		nexterror();
	}

	r = mntralloc(0, m->msize);

	if(waserror()) {
		mntfree(r);
		nexterror();
	}

	r->request.type = Tattach;
	r->request.fid = c->fid;
	if(bogus.authchan == nil)
		r->request.afid = NOFID;
	else
		r->request.afid = bogus.authchan->fid;
	r->request.uname = up->user;
	r->request.aname = bogus.spec;
	mountrpc(m, r);

	c->qid = r->reply.qid;
	c->mchan = m->c;
	incref(m->c);
	c->mqid = c->qid;

	poperror();	/* r */
	mntfree(r);

	poperror();	/* c */

	if(bogus.flags&MCACHE)
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

	if(c->mchan)
		panic("mntchan non-zero %p", c->mchan);
	return c;
}

static Walkqid*
mntwalk(Chan *c, Chan *nc, char **name, int nname)
{
	int i, alloc;
	Mnt *m;
	Mntrpc *r;
	Walkqid *wq;

	if(nc != nil)
		print("mntwalk: nc != nil\n");
	if(nname > MAXWELEM)
		error("devmnt: too many name elements");
	alloc = 0;
	wq = smalloc(sizeof(Walkqid)+(nname-1)*sizeof(Qid));
	if(waserror()){
		if(alloc && wq->clone!=nil)
			cclose(wq->clone);
		free(wq);
		return nil;
	}

	alloc = 0;
	m = mntchk(c);
	r = mntralloc(c, m->msize);
	if(nc == nil){
		nc = devclone(c);
		/*
		 * Until the other side accepts this fid, we can't mntclose it.
		 * Therefore set type to 0 for now; rootclose is known to be safe.
		 */
		nc->type = 0;
		alloc = 1;
	}
	wq->clone = nc;

	if(waserror()) {
		mntfree(r);
		nexterror();
	}
	r->request.type = Twalk;
	r->request.fid = c->fid;
	r->request.newfid = nc->fid;
	r->request.nwname = nname;
	memmove(r->request.wname, name, nname*sizeof(char*));

	mountrpc(m, r);

	if(r->reply.nwqid > nname)
		error("too many QIDs returned by walk");
	if(r->reply.nwqid < nname){
		if(alloc)
			cclose(nc);
		wq->clone = nil;
		if(r->reply.nwqid == 0){
			free(wq);
			wq = nil;
			goto Return;
		}
	}

	/* move new fid onto mnt device and update its qid */
	if(wq->clone != nil){
		if(wq->clone != c){
			wq->clone->type = c->type;
			wq->clone->mchan = c->mchan;
			incref(c->mchan);
		}
		if(r->reply.nwqid > 0)
			wq->clone->qid = r->reply.wqid[r->reply.nwqid-1];
	}
	wq->nqid = r->reply.nwqid;
	for(i=0; i<wq->nqid; i++)
		wq->qid[i] = r->reply.wqid[i];

    Return:
	poperror();
	mntfree(r);
	poperror();
	return wq;
}

static int
mntstat(Chan *c, uchar *dp, int n)
{
	Mnt *m;
	Mntrpc *r;

	if(n < BIT16SZ)
		error(Eshortstat);
	m = mntchk(c);
	r = mntralloc(c, m->msize);
	if(waserror()) {
		mntfree(r);
		nexterror();
	}
	r->request.type = Tstat;
	r->request.fid = c->fid;
	mountrpc(m, r);

	if(r->reply.nstat > n){
		n = BIT16SZ;
		PBIT16((uchar*)dp, r->reply.nstat-2);
	}else{
		n = r->reply.nstat;
		memmove(dp, r->reply.stat, n);
		validstat(dp, n);
		mntdirfix(dp, c);
	}
	poperror();
	mntfree(r);
	return n;
}

static Chan*
mntopencreate(int type, Chan *c, char *name, int omode, ulong perm)
{
	Mnt *m;
	Mntrpc *r;

	m = mntchk(c);
	r = mntralloc(c, m->msize);
	if(waserror()) {
		mntfree(r);
		nexterror();
	}
	r->request.type = type;
	r->request.fid = c->fid;
	r->request.mode = omode;
	if(type == Tcreate){
		r->request.perm = perm;
		r->request.name = name;
	}
	mountrpc(m, r);

	c->qid = r->reply.qid;
	c->offset = 0;
	c->mode = openmode(omode);
	c->iounit = r->reply.iounit;
	if(c->iounit == 0 || c->iounit > m->msize-IOHDRSZ)
		c->iounit = m->msize-IOHDRSZ;
	c->flag |= COPEN;
	poperror();
	mntfree(r);

	if(c->flag & CCACHE)
		copen(c);

	return c;
}

static Chan*
mntopen(Chan *c, int omode)
{
	return mntopencreate(Topen, c, nil, omode, 0);
}

static void
mntcreate(Chan *c, char *name, int omode, ulong perm)
{
	mntopencreate(Tcreate, c, name, omode, perm);
}

static void
mntclunk(Chan *c, int t)
{
	Mnt *m;
	Mntrpc *r;

	m = mntchk(c);
	r = mntralloc(c, m->msize);
	if(waserror()){
		mntfree(r);
		nexterror();
	}

	r->request.type = t;
	r->request.fid = c->fid;
	mountrpc(m, r);
	mntfree(r);
	poperror();
}

void
muxclose(Mnt *m)
{
	Mntrpc *q, *r;

	for(q = m->queue; q; q = r) {
		r = q->list;
		mntfree(q);
	}
	m->id = 0;
	free(m->version);
	m->version = nil;
	mntpntfree(m);
}

void
mntpntfree(Mnt *m)
{
	Mnt *f, **l;
	Queue *q;

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
	q = m->q;
	unlock(&mntalloc);

	qfree(q);
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

static int
mntwstat(Chan *c, uchar *dp, int n)
{
	Mnt *m;
	Mntrpc *r;

	m = mntchk(c);
	r = mntralloc(c, m->msize);
	if(waserror()) {
		mntfree(r);
		nexterror();
	}
	r->request.type = Twstat;
	r->request.fid = c->fid;
	r->request.nstat = n;
	r->request.stat = dp;
	mountrpc(m, r);
	poperror();
	mntfree(r);
	return n;
}

static long
mntread(Chan *c, void *buf, long n, vlong off)
{
	uchar *p, *e;
	int nc, cache, isdir, dirlen;

	isdir = 0;
	cache = c->flag & CCACHE;
	if(c->qid.type & QTDIR) {
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
		for(e = &p[n]; p+BIT16SZ < e; p += dirlen){
			dirlen = BIT16SZ+GBIT16(p);
			if(p+dirlen > e)
				break;
			validstat(p, dirlen);
			mntdirfix(p, c);
		}
		if(p != e)
			error(Esbadstat);
	}
	return n;
}

static long
mntwrite(Chan *c, void *buf, long n, vlong off)
{
	return mntrdwr(Twrite, c, buf, n, off);
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
	if(c->qid.type & QTDIR)
		cache = 0;
	for(;;) {
		r = mntralloc(c, m->msize);
		if(waserror()) {
			mntfree(r);
			nexterror();
		}
		r->request.type = type;
		r->request.fid = c->fid;
		r->request.offset = off;
		r->request.data = uba;
		nr = n;
		if(nr > m->msize-IOHDRSZ)
			nr = m->msize-IOHDRSZ;
		r->request.count = nr;
		mountrpc(m, r);
		nreq = r->request.count;
		nr = r->reply.count;
		if(nr > nreq)
			nr = nreq;

		if(type == Tread)
			r->b = bl2mem((uchar*)uba, r->b, nr);
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
	char *sn, *cn;
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
		sn = "?";
		if(m->c->path != nil)
			sn = m->c->path->s;
		cn = "?";
		if(r->c != nil && r->c->path != nil)
			cn = r->c->path->s;
		print("mnt: proc %s %lud: mismatch from %s %s rep %#p tag %d fid %d T%d R%d rp %d\n",
			up->text, up->pid, sn, cn,
			r, r->request.tag, r->request.fid, r->request.type,
			r->reply.type, r->reply.tag);
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
		if(strcmp(up->errstr, Eintr) != 0){
			mntflushfree(m, r);
			nexterror();
		}
		r = mntflushalloc(r, m->msize);
	}

	lock(m);
	r->m = m;
	r->list = m->queue;
	m->queue = r;
	unlock(m);

	/* Transmit a file system rpc */
	if(m->msize == 0)
		panic("msize");
	n = convS2M(&r->request, r->rpc, m->msize);
	if(n < 0)
		panic("bad message type in mountio");
	if(devtab[m->c->type]->write(m->c, r->rpc, n, 0) != n)
		error(Emountrpc);
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
		if(mntrpcread(m, r) < 0)
			error(Emountrpc);
		mountmux(m, r);
	}
	mntgate(m);
	poperror();
	mntflushfree(m, r);
}

static int
doread(Mnt *m, int len)
{
	Block *b;

	while(qlen(m->q) < len){
		b = devtab[m->c->type]->bread(m->c, m->msize, 0);
		if(b == nil)
			return -1;
		if(blocklen(b) == 0){
			freeblist(b);
			return -1;
		}
		qaddlist(m->q, b);
	}
	return 0;
}

int
mntrpcread(Mnt *m, Mntrpc *r)
{
	int i, t, len, hlen;
	Block *b, **l, *nb;

	r->reply.type = 0;
	r->reply.tag = 0;

	/* read at least length, type, and tag and pullup to a single block */
	if(doread(m, BIT32SZ+BIT8SZ+BIT16SZ) < 0)
		return -1;
	nb = pullupqueue(m->q, BIT32SZ+BIT8SZ+BIT16SZ);

	/* read in the rest of the message, avoid ridiculous (for now) message sizes */
	len = GBIT32(nb->rp);
	if(len > m->msize){
		qdiscard(m->q, qlen(m->q));
		return -1;
	}
	if(doread(m, len) < 0)
		return -1;

	/* pullup the header (i.e. everything except data) */
	t = nb->rp[BIT32SZ];
	switch(t){
	case Rread:
		hlen = BIT32SZ+BIT8SZ+BIT16SZ+BIT32SZ;
		break;
	default:
		hlen = len;
		break;
	}
	nb = pullupqueue(m->q, hlen);

	if(convM2S(nb->rp, len, &r->reply) <= 0){
		/* bad message, dump it */
		print("mntrpcread: convM2S failed\n");
		qdiscard(m->q, len);
		return -1;
	}

	/* hang the data off of the fcall struct */
	l = &r->b;
	*l = nil;
	do {
		b = qremove(m->q);
		if(hlen > 0){
			b->rp += hlen;
			len -= hlen;
			hlen = 0;
		}
		i = BLEN(b);
		if(i <= len){
			len -= i;
			*l = b;
			l = &(b->next);
		} else {
			/* split block and put unused bit back */
			nb = allocb(i-len);
			memmove(nb->wp, b->rp+len, i-len);
			b->wp = b->rp+len;
			nb->wp += i-len;
			qputback(m->q, nb);
			*l = b;
			return 0;
		}
	}while(len > 0);

	return 0;
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
				q->reply = r->reply;
				q->b = r->b;
				r->b = nil;
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
	print("unexpected reply tag %ud; type %d\n", r->reply.tag, r->reply.type);
}

/*
 * Create a new flush request and chain the previous
 * requests from it
 */
Mntrpc*
mntflushalloc(Mntrpc *r, ulong iounit)
{
	Mntrpc *fr;

	fr = mntralloc(0, iounit);

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

int
alloctag(void)
{
	int i, j;
	ulong v;

	for(i = 0; i < NMASK; i++){
		v = mntalloc.tagmask[i];
		if(v == ~0UL)
			continue;
		for(j = 0; j < 1<<TAGSHIFT; j++)
			if((v & (1<<j)) == 0){
				mntalloc.tagmask[i] |= 1<<j;
				return (i<<TAGSHIFT) + j;
			}
	}
	panic("no friggin tags left");
	return NOTAG;
}

void
freetag(int t)
{
	mntalloc.tagmask[t>>TAGSHIFT] &= ~(1<<(t&TAGMASK));
}

Mntrpc*
mntralloc(Chan *c, ulong msize)
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
		new->rpc = mallocz(msize, 0);
		if(new->rpc == nil){
			free(new);
			unlock(&mntalloc);
			exhausted("mount rpc buffer");
		}
		new->rpclen = msize;
		new->request.tag = alloctag();
	}
	else {
		mntalloc.rpcfree = new->list;
		mntalloc.nrpcfree--;
		if(new->rpclen < msize){
			free(new->rpc);
			new->rpc = mallocz(msize, 0);
			if(new->rpc == nil){
				free(new);
				mntalloc.nrpcused--;
				unlock(&mntalloc);
				exhausted("mount rpc buffer");
			}
			new->rpclen = msize;
		}
	}
	mntalloc.nrpcused++;
	unlock(&mntalloc);
	new->c = c;
	new->done = 0;
	new->flushed = nil;
	new->b = nil;
	return new;
}

void
mntfree(Mntrpc *r)
{
	if(r->b != nil)
		freeblist(r->b);
	lock(&mntalloc);
	if(mntalloc.nrpcfree >= 10){
		free(r->rpc);
		freetag(r->request.tag);
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

	/* This routine is mostly vestiges of prior lives; now it's just sanity checking */

	if(c->mchan == nil)
		panic("mntchk 1: nil mchan c %s\n", chanpath(c));

	m = c->mchan->mux;

	if(m == nil)
		print("mntchk 2: nil mux c %s c->mchan %s \n", chanpath(c), chanpath(c->mchan));

	/*
	 * Was it closed and reused (was error(Eshutdown); now, it cannot happen)
	 */
	if(m->id == 0 || m->id >= c->dev)
		panic("mntchk 3: can't happen");

	return m;
}

/*
 * Rewrite channel type and dev for in-flight data to
 * reflect local values.  These entries are known to be
 * the first two in the Dir encoding after the count.
 */
void
mntdirfix(uchar *dirbuf, Chan *c)
{
	uint r;

	r = devtab[c->type]->dc;
	dirbuf += BIT16SZ;	/* skip count */
	PBIT16(dirbuf, r);
	dirbuf += BIT16SZ;
	PBIT32(dirbuf, c->dev);
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
	devshutdown,
	mntattach,
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
