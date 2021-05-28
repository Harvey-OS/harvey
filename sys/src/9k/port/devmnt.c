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

#define MAXRPC (IOHDRSZ+16*1024)	/* maybe a larger size will be faster */
/* use a known-good common size for initial negotiation */
#define MAXCMNRPC (IOHDRSZ+8192)

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
	uint	id;
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
usize
mntversion(Chan *c, u32int msize, char *version, usize returnlen)
{
	Fcall f;
	uchar *msg;
	Mnt *mnt;
	char *v;
	long l, n;
	usize k;
	vlong oo;
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

	///* validity */
	//if(msize < 0)	pointless if msize unsigned, but but should it be?
	//	error("bad iounit in version call");
	if(strncmp(v, VERSION9P, strlen(VERSION9P)) != 0)
		error("bad 9P version specification");

	mnt = c->mux;

	if(mnt != nil){
		qunlock(&c->umqlock);
		poperror();

		strecpy(buf, buf+sizeof buf, mnt->version);
		k = strlen(buf);
		if(strncmp(buf, v, k) != 0){
			snprint(buf, sizeof buf, "incompatible 9P versions %s %s", mnt->version, v);
			error(buf);
		}
		if(returnlen != 0){
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
	msg = malloc(MAXCMNRPC);
	if(msg == nil)
		exhausted("version memory");
	if(waserror()){
		free(msg);
		nexterror();
	}
	k = convS2M(&f, msg, MAXCMNRPC);
	if(k == 0)
		error("bad fversion conversion on send");

	lock(c);
	oo = c->offset;
	c->offset += k;
	unlock(c);

	l = c->dev->write(c, msg, k, oo);

	if(l < k){
		lock(c);
		c->offset -= k - l;
		unlock(c);
		error("short write in fversion");
	}

	/* message sent; receive and decode reply */
	n = c->dev->read(c, msg, MAXCMNRPC, c->offset);
	if(n <= 0)
		error("EOF receiving fversion reply");

	lock(c);
	c->offset += n;
	unlock(c);

	l = convM2S(msg, n, &f);
	if(l != n)
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
	mnt = mntalloc.mntfree;
	if(mnt != nil)
		mntalloc.mntfree = mnt->list;
	else {
		mnt = malloc(sizeof(Mnt));
		if(mnt == nil) {
			unlock(&mntalloc);
			exhausted("mount devices");
		}
	}
	mnt->list = mntalloc.list;
	mntalloc.list = mnt;
	mnt->version = nil;
	kstrdup(&mnt->version, f.version);
	mnt->id = mntalloc.id++;
	mnt->q = qopen(10*MAXRPC, 0, nil, nil);
	mnt->msize = f.msize;
	unlock(&mntalloc);

	if(returnlen != 0){
		if(returnlen < k)
			error(Eshort);
		memmove(version, f.version, k);
	}

	poperror();	/* msg */
	free(msg);

	lock(mnt);
	mnt->queue = 0;
	mnt->rip = 0;

	c->flag |= CMSG;
	c->mux = mnt;
	mnt->c = c;
	unlock(mnt);

	poperror();	/* c */
	qunlock(&c->umqlock);

	return k;
}

Chan*
mntauth(Chan *c, char *spec)
{
	Mnt *mnt;
	Mntrpc *r;

	mnt = c->mux;

	if(mnt == nil){
		mntversion(c, MAXRPC, VERSION9P, 0);
		mnt = c->mux;
		if(mnt == nil)
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

	r = mntralloc(0, mnt->msize);

	if(waserror()) {
		mntfree(r);
		nexterror();
	}

	r->request.type = Tauth;
	r->request.afid = c->fid;
	r->request.uname = up->user;
	r->request.aname = spec;
	mountrpc(mnt, r);

	c->qid = r->reply.aqid;
	c->mchan = mnt->c;
	incref(mnt->c);
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
	Mnt *mnt;
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

	mnt = c->mux;

	if(mnt == nil){
		mntversion(c, 0, nil, 0);
		mnt = c->mux;
		if(mnt == nil)
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

	r = mntralloc(0, mnt->msize);

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
	mountrpc(mnt, r);

	c->qid = r->reply.qid;
	c->mchan = mnt->c;
	incref(mnt->c);
	c->mqid = c->qid;

	poperror();	/* r */
	mntfree(r);

	poperror();	/* c */

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
	c->devno = mntalloc.id++;
	unlock(&mntalloc);

	if(c->mchan)
		panic("mntchan non-zero %#p", c->mchan);
	return c;
}

static Walkqid*
mntwalk(Chan *c, Chan *nc, char **name, int nname)
{
	int i, alloc;
	Mnt *mnt;
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
	mnt = mntchk(c);
	r = mntralloc(c, mnt->msize);
	if(nc == nil){
		nc = devclone(c);
		/*
		 * Until the other side accepts this fid,
		 * we can't mntclose it.
		 * nc->dev remains nil for now.
		 */
		alloc = 1;
	}
	wq->clone = nc;
	nc->flag |= c->flag&CCACHE;

	if(waserror()) {
		mntfree(r);
		nexterror();
	}
	r->request.type = Twalk;
	r->request.fid = c->fid;
	r->request.newfid = nc->fid;
	r->request.nwname = nname;
	memmove(r->request.wname, name, nname*sizeof(char*));

	mountrpc(mnt, r);

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
			wq->clone->dev = c->dev;
			//if(wq->clone->dev != nil)	//XDYNX
			//	devtabincr(wq->clone->dev);
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

static long
mntstat(Chan *c, uchar *dp, long n)
{
	Mnt *mnt;
	Mntrpc *r;
	usize nstat;

	if(n < BIT16SZ)
		error(Eshortstat);
	mnt = mntchk(c);
	r = mntralloc(c, mnt->msize);
	if(waserror()) {
		mntfree(r);
		nexterror();
	}
	r->request.type = Tstat;
	r->request.fid = c->fid;
	mountrpc(mnt, r);

	if(r->reply.nstat > n){
		nstat = BIT16SZ;
		PBIT16(dp, r->reply.nstat-2);
	}else{
		nstat = r->reply.nstat;
		memmove(dp, r->reply.stat, nstat);
		validstat(dp, nstat);
		mntdirfix(dp, c);
	}
	poperror();
	mntfree(r);

	return nstat;
}

static Chan*
mntopencreate(int type, Chan *c, char *name, int omode, int perm)
{
	Mnt *mnt;
	Mntrpc *r;

	mnt = mntchk(c);
	r = mntralloc(c, mnt->msize);
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
	mountrpc(mnt, r);

	c->qid = r->reply.qid;
	c->offset = 0;
	c->mode = openmode(omode);
	c->iounit = r->reply.iounit;
	if(c->iounit == 0 || c->iounit > mnt->msize-IOHDRSZ)
		c->iounit = mnt->msize-IOHDRSZ;
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
mntcreate(Chan *c, char *name, int omode, int perm)
{
	mntopencreate(Tcreate, c, name, omode, perm);
}

static void
mntclunk(Chan *c, int t)
{
	Mnt *mnt;
	Mntrpc *r;

	mnt = mntchk(c);
	r = mntralloc(c, mnt->msize);
	if(waserror()){
		mntfree(r);
		nexterror();
	}

	r->request.type = t;
	r->request.fid = c->fid;
	mountrpc(mnt, r);
	mntfree(r);
	poperror();
}

void
muxclose(Mnt *mnt)
{
	Mntrpc *q, *r;

	for(q = mnt->queue; q; q = r) {
		r = q->list;
		mntfree(q);
	}
	mnt->id = 0;
	free(mnt->version);
	mnt->version = nil;
	mntpntfree(mnt);
}

void
mntpntfree(Mnt *mnt)
{
	Mnt *f, **l;
	Queue *q;

	lock(&mntalloc);
	l = &mntalloc.list;
	for(f = *l; f; f = f->list) {
		if(f == mnt) {
			*l = mnt->list;
			break;
		}
		l = &f->list;
	}
	mnt->list = mntalloc.mntfree;
	mntalloc.mntfree = mnt;
	q = mnt->q;
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

static long
mntwstat(Chan *c, uchar *dp, long n)
{
	Mnt *mnt;
	Mntrpc *r;

	mnt = mntchk(c);
	r = mntralloc(c, mnt->msize);
	if(waserror()) {
		mntfree(r);
		nexterror();
	}
	r->request.type = Twstat;
	r->request.fid = c->fid;
	r->request.nstat = n;
	r->request.stat = dp;
	mountrpc(mnt, r);
	poperror();
	mntfree(r);
	return n;
}

static long
mntread(Chan *c, void *buf, long n, vlong off)
{
	uchar *p, *e;
	int nc, cache, isdir;
	usize dirlen;

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
	Mnt *mnt;
	Mntrpc *r;
	char *uba;
	int cache;
	ulong cnt, nr, nreq;

	mnt = mntchk(c);
	uba = buf;
	cnt = 0;
	cache = c->flag & CCACHE;
	if(c->qid.type & QTDIR)
		cache = 0;
	for(;;) {
		r = mntralloc(c, mnt->msize);
		if(waserror()) {
			mntfree(r);
			nexterror();
		}
		r->request.type = type;
		r->request.fid = c->fid;
		r->request.offset = off;
		r->request.data = uba;
		nr = n;
		if(nr > mnt->msize-IOHDRSZ)
			nr = mnt->msize-IOHDRSZ;
		r->request.count = nr;
		mountrpc(mnt, r);
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
mountrpc(Mnt *mnt, Mntrpc *r)
{
	char *sn, *cn;
	int t;

	r->reply.tag = 0;
	r->reply.type = Tmax;	/* can't ever be a valid message type */

	mountio(mnt, r);

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
		if(mnt->c->path != nil)
			sn = mnt->c->path->s;
		cn = "?";
		if(r->c != nil && r->c->path != nil)
			cn = r->c->path->s;
		print("mnt: proc %s %d: mismatch from %s %s rep %#p tag %d fid %d T%d R%d rp %d\n",
			up->text, up->pid, sn, cn,
			r, r->request.tag, r->request.fid, r->request.type,
			r->reply.type, r->reply.tag);
		error(Emountrpc);
	}
}

void
mountio(Mnt *mnt, Mntrpc *r)
{
	int n;

	while(waserror()) {
		if(mnt->rip == up)
			mntgate(mnt);
		if(strcmp(up->errstr, Eintr) != 0){
			mntflushfree(mnt, r);
			nexterror();
		}
		r = mntflushalloc(r, mnt->msize);
	}

	lock(mnt);
	r->m = mnt;
	r->list = mnt->queue;
	mnt->queue = r;
	unlock(mnt);

	/* Transmit a file system rpc */
	if(mnt->msize == 0)
		panic("msize");
	n = convS2M(&r->request, r->rpc, mnt->msize);
	if(n < 0)
		panic("bad message type in mountio");
	if(mnt->c->dev->write(mnt->c, r->rpc, n, 0) != n)
		error(Emountrpc);
	r->stime = fastticks(nil);
	r->reqlen = n;

	/* Gate readers onto the mount point one at a time */
	for(;;) {
		lock(mnt);
		if(mnt->rip == 0)
			break;
		unlock(mnt);
		sleep(&r->r, rpcattn, r);
		if(r->done){
			poperror();
			mntflushfree(mnt, r);
			return;
		}
	}
	mnt->rip = up;
	unlock(mnt);
	while(r->done == 0) {
		if(mntrpcread(mnt, r) < 0)
			error(Emountrpc);
		mountmux(mnt, r);
	}
	mntgate(mnt);
	poperror();
	mntflushfree(mnt, r);
}

static int
doread(Mnt *mnt, int len)
{
	Block *b;

	while(qlen(mnt->q) < len){
		b = mnt->c->dev->bread(mnt->c, mnt->msize, 0);
		if(b == nil)
			return -1;
		if(blocklen(b) == 0){
			freeblist(b);
			return -1;
		}
		qaddlist(mnt->q, b);
	}
	return 0;
}

int
mntrpcread(Mnt *mnt, Mntrpc *r)
{
	int i, t, len, hlen;
	Block *b, **l, *nb;

	r->reply.type = 0;
	r->reply.tag = 0;

	/* read at least length, type, and tag and pullup to a single block */
	if(doread(mnt, BIT32SZ+BIT8SZ+BIT16SZ) < 0)
		return -1;
	nb = pullupqueue(mnt->q, BIT32SZ+BIT8SZ+BIT16SZ);

	/* read in the rest of the message, avoid ridiculous (for now) message sizes */
	len = GBIT32(nb->rp);
	if(len > mnt->msize){
		qdiscard(mnt->q, qlen(mnt->q));
		return -1;
	}
	if(doread(mnt, len) < 0)
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
	nb = pullupqueue(mnt->q, hlen);

	if(convM2S(nb->rp, len, &r->reply) <= 0){
		/* bad message, dump it */
		print("mntrpcread: convM2S failed\n");
		qdiscard(mnt->q, len);
		return -1;
	}

	/* hang the data off of the fcall struct */
	l = &r->b;
	*l = nil;
	do {
		b = qremove(mnt->q);
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
			qputback(mnt->q, nb);
			*l = b;
			return 0;
		}
	}while(len > 0);

	return 0;
}

void
mntgate(Mnt *mnt)
{
	Mntrpc *q;

	lock(mnt);
	mnt->rip = 0;
	for(q = mnt->queue; q; q = q->list) {
		if(q->done == 0)
		if(wakeup(&q->r))
			break;
	}
	unlock(mnt);
}

void
mountmux(Mnt *mnt, Mntrpc *r)
{
	Mntrpc **l, *q;

	lock(mnt);
	l = &mnt->queue;
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
			unlock(mnt);
			if(mntstats != nil)
				(*mntstats)(q->request.type,
					mnt->c, q->stime,
					q->reqlen + r->replen);
			if(q != r)
				wakeup(&q->r);
			return;
		}
		l = &q->list;
	}
	unlock(mnt);
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
mntflushfree(Mnt *mnt, Mntrpc *r)
{
	Mntrpc *fr;

	while(r){
		fr = r->flushed;
		if(!r->done){
			r->reply.type = Rflush;
			mntqrm(mnt, r);
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
mntqrm(Mnt *mnt, Mntrpc *r)
{
	Mntrpc **l, *f;

	lock(mnt);
	r->done = 1;

	l = &mnt->queue;
	for(f = *l; f; f = f->list) {
		if(f == r) {
			*l = r->list;
			break;
		}
		l = &f->list;
	}
	unlock(mnt);
}

Mnt*
mntchk(Chan *c)
{
	Mnt *mnt;

	/* This routine is mostly vestiges of prior lives; now it's just sanity checking */

	if(c->mchan == nil)
		panic("mntchk 1: nil mchan c %s\n", chanpath(c));

	mnt = c->mchan->mux;

	if(mnt == nil)
		print("mntchk 2: nil mux c %s c->mchan %s \n", chanpath(c), chanpath(c->mchan));

	/*
	 * Was it closed and reused (was error(Eshutdown); now, it cannot happen)
	 */
	if(mnt->id == 0 || mnt->id >= c->devno)
		panic("mntchk 3: can't happen");

	return mnt;
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

	r = c->dev->dc;
	dirbuf += BIT16SZ;	/* skip count */
	PBIT16(dirbuf, r);
	dirbuf += BIT16SZ;
	PBIT32(dirbuf, c->devno);
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
