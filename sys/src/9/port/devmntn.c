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

#define MAXRPC (IOHDRSZ+128*1024)

struct Mntrpc
{
	Chan*	c;		/* Channel for whom we are working */
	Mntrpc*	list;		/* Free/pending list */
	Fcall	request;	/* Outgoing file system protocol message */
	Fcall 	reply;		/* Incoming reply */
	Mnt*	m;		/* Mount device during rpc */
	Rendez	r;		/* Place to hang out */
	uint8_t*	rpc;		/* I/O Data buffer */
	uint	rpclen;		/* len of buffer */
	Block	*b;		/* reply blocks */
	char	done;		/* Rpc completed */
	uint64_t	stime;		/* start time for mnt statistics */
	uint32_t	reqlen;		/* request length for mnt statistics */
	uint32_t	replen;		/* reply length for mnt statistics */
	Mntrpc*	flushed;	/* message this one flushes */
};

enum
{
	TAGSHIFT = 5,			/* uint32_t has to be 32 bits */
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
	uint32_t	tagmask[NMASK];
}mntalloc;

Mnt*	mntchk(Chan*);
void	mntdirfix(uint8_t*, Chan*);
Mntrpc*	mntflushalloc(Mntrpc*, uint32_t);
void	mntflushfree(Mnt*, Mntrpc*);
void	mntfree(Mntrpc*);
void	mntgate(Mnt*);
void	mntpntfree(Mnt*);
void	mntqrm(Mnt*, Mntrpc*);
Mntrpc*	mntralloc(Chan*, uint32_t);
int32_t	mntrdwr(int, Chan*, void*, int32_t, int64_t);
int	mntrpcread(Mnt*, Mntrpc*);
void	mountio(Mnt*, Mntrpc*);
void	mountmux(Mnt*, Mntrpc*);
void	mountrpc(Mnt*, Mntrpc*);
int	rpcattn(void*);
Chan*	mntchann(void);

extern char	Esbadstat[];
extern char	Enoversion[];


void (*mntstats)(int, Chan*, uint64_t, uint32_t);

static void
mntreset(void)
{
	mntalloc.id = 1;
	mntalloc.tagmask[0] = 1;			/* don't allow 0 as a tag */
	mntalloc.tagmask[NMASK-1] = 0x80000000UL;	/* don't allow NOTAG */
	fmtinstall('F', fcallfmt);
	fmtinstall('D', dirfmt);
/* We can't install %M since eipfmt does and is used in the kernel [sape] */

	if(mfcinit != nil)
		mfcinit();
}

static Chan*
mntattach(char *muxattach)
{
	Proc *up = externup();
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

	c = mntchann();
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
	incref(&mnt->c->r);
	c->mqid = c->qid;

	poperror();	/* r */
	mntfree(r);

	poperror();	/* c */

	if((bogus.flags & MCACHE) && mfcinit != nil)
		c->flag |= CCACHE;
	return c;
}

static Walkqid*
mntwalk(Chan *c, Chan *nc, char **name, int nname)
{
	Proc *up = externup();
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
			incref(&c->mchan->r);
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

static int32_t
mntstat(Chan *c, uint8_t *dp, int32_t n)
{
	Proc *up = externup();
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
	Proc *up = externup();
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
	c->writeoffset = 0;
	c->buffend = 0;
	c->writebuff = nil;
	c->buffsize = mnt->msize;
	if(c->iounit == 0 || c->iounit > mnt->msize-IOHDRSZ)
		c->iounit = mnt->msize-IOHDRSZ;
	c->flag |= COPEN;
	poperror();
	mntfree(r);

	if(c->flag & CCACHE)
		mfcopen(c);

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
	Proc *up = externup();
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

static void
mntclose(Chan *c)
{
	if (c->buffend > 0) {
		mntrdwr(Twrite, c, c->writebuff, c->buffend, c->writeoffset);
	}
	c->buffend = 0;
	free(c->writebuff);
	c->writebuff = nil;
	mntclunk(c, Tclunk);
}

static void
mntremove(Chan *c)
{
	mntclunk(c, Tremove);
}

static int32_t
mntwstat(Chan *c, uint8_t *dp, int32_t n)
{
	Proc *up = externup();
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

static int32_t
mntread(Chan *c, void *buf, int32_t n, int64_t off)
{
	uint8_t *p, *e;
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
		nc = mfcread(c, buf, n, off);
		if(nc > 0) {
			n -= nc;
			if(n == 0)
				return nc;
			p += nc;
			off += nc;
		}
		n = mntrdwr(Tread, c, p, n, off);
		mfcupdate(c, p, n, off);
		return n + nc;
	}

	// Flush if we're reading this file.  Would be nice to see if
	// read could be satisfied from buffer.
	if (c->buffend > 0) {
		mntrdwr(Twrite, c, c->writebuff, c->buffend, c->writeoffset);
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

static int32_t
mntwrite(Chan *c, void *buf, int32_t n, int64_t off)
{
	int result = n;
	int offset = 0;
	if (c->writebuff == nil){
		c->writebuff = (unsigned char*)malloc(c->buffsize);
		if(c->writebuff == nil){
			print("devmntn: write buffer allocation of %d bytes failed\n", c->buffsize);
			return mntrdwr(Twrite, c, buf, n, off);
		}
	}
	if(off != c->writeoffset + c->buffend){
		// non-continuous write - flush cached data
		mntrdwr(Twrite, c, c->writebuff, c->buffend, c->writeoffset);
		c->buffend = 0;
		c->writeoffset = off;
	}
	while (n + c->buffend >= c->buffsize){
		offset = c->mux->msize - c->buffend;
		n -= offset;
		memmove(&c->writebuff[c->buffend], buf, offset);
		mntrdwr(Twrite, c, c->writebuff, c->mux->msize, c->writeoffset);
		c->writeoffset += offset;
	}
	memmove(&c->writebuff[c->buffend], buf + offset, n);
	c->buffend += n;
	return result;
}

Chan*
mntchann(void)
{
        Chan *c;

        c = devattach('N', 0);
        lock(&mntalloc);
        c->devno = mntalloc.id++;
        unlock(&mntalloc);

        if(c->mchan)
                panic("mntchan non-zero %#p", c->mchan);
        return c;
}

Dev mntndevtab = {
	.dc = 'N',
	.name = "mntn",

	.reset = mntreset,
	.init = devinit,
	.shutdown = devshutdown,
	.attach = mntattach,
	.walk = mntwalk,
	.stat = mntstat,
	.open = mntopen,
	.create = mntcreate,
	.close = mntclose,
	.read = mntread,
	.bread = devbread,
	.write = mntwrite,
	.bwrite = devbwrite,
	.remove = mntremove,
	.wstat = mntwstat,
};
