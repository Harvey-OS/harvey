#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"
#include	"devtab.h"

typedef struct Mntrpc Mntrpc;
struct Mntrpc
{
	Mntrpc	*list;		/* Free/pending list */
	Fcall	request;	/* Outgoing file system protocol message */
	Fcall	reply;		/* Incoming reply */
	Mnt	*m;		/* Mount device during rpc */
	Rendez	r;		/* Place to hang out */
	char	*rpc;		/* I/O Data buffer */
	char	done;		/* Rpc completed */
	char	flushed;	/* Flush was sent */
	ushort	flushtag;	/* Tag flush sent on */
	char	flush[MAXMSG];	/* Somewhere to build flush */
};

struct Mnt
{
	Ref;			/* Count of attached channels */
	Chan	*c;		/* Channel to file service */
	Proc	*rip;		/* Reader in progress */
	Mntrpc	*queue;		/* Queue of pending requests on this channel */
	int	id;		/* Multiplexor id for channel check */
	Mnt	*list;		/* Free list */
	char	mux;		/* Set if the device aleady does the multiplexing */
	int	blocksize;	/* read/write block size */
	ushort	flushtag;	/* Tag to send flush on */
	ushort	flushbase;	/* Base tag of flush window for this buffer */
};

struct Mntalloc
{
	Lock;
	Mnt	*list;		/* Mount devices in used */
	Mnt	*mntfree;	/* Free list */
	Mntrpc	*rpcfree;
	int	id;
	int	rpctag;
}mntalloc;

#define MAXRPC		(MAXFDATA+MAXMSG)
#define limit(n, max)	(n > max ? max : n)

Chan*	mattach(Mnt*, char*, char*);
void	mntauth(Mnt *, Mntrpc *, char *, ushort);
Mnt*	mntchk(Chan*);
void	mntdirfix(uchar*, Chan*);
void	mntdoclunk(Mnt *, Mntrpc *);
int	mntflush(Mnt*, Mntrpc*);
void	mntfree(Mntrpc*);
void	mntgate(Mnt*);
void	mntpntfree(Mnt*);
void	mntqrm(Mnt*, Mntrpc*);
Mntrpc*	mntralloc(void);
long	mntrdwr(int , Chan*, void*,long , ulong);
void	mntrpcread(Mnt*, Mntrpc*);
void	mountio(Mnt*, Mntrpc*);
void	mountmux(Mnt*, Mntrpc*);
void	mountrpc(Mnt*, Mntrpc*);
int	rpcattn(Mntrpc*);

enum
{
	Tagspace = 1,
	Tagfls = 0x8000,
	Tagend = 0xfffe,

	ALIGN = 256,		/* Vme block mode alignment */
};

void
mntreset(void)
{
	mntalloc.id = 1;
	mntalloc.rpctag = Tagspace;
}

void
mntinit(void)
{
}

Chan*
mntattach(char *muxattach)
{
	Mnt *m;
	Chan *c;
	struct bogus{
		Chan	*chan;
		char	*spec;
		char	*serv;
	}bogus;

	bogus = *((struct bogus *)muxattach);
	c = bogus.chan;

	lock(&mntalloc);
	for(m = mntalloc.list; m; m = m->list) {
		if(m->c == c && m->id) {
			lock(m);
			if(m->id && m->ref > 0 && m->c == c) {
				unlock(&mntalloc);
				m->ref++;
				unlock(m);
				return mattach(m, bogus.spec, bogus.serv);
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
		m->flushbase = Tagfls;
		m->flushtag = Tagfls;
	}
	m->list = mntalloc.list;
	mntalloc.list = m;
	m->id = mntalloc.id++;
	lock(m);
	unlock(&mntalloc);

	m->ref = 1;
	m->queue = 0;
	m->rip = 0;
	m->c = c;
	m->c->flag |= CMSG;
	m->blocksize = MAXFDATA;/**/

	switch(devchar[m->c->type]) {
	default:
		m->mux = 0;
		break;
	case 'C':			/* Cyclone */
		m->mux = 1;
		break;
	}
	incref(m->c);
	unlock(m);

	if(waserror()) {
		close(m->c);
		if(decref(m) == 0)
			mntpntfree(m);
		nexterror();
	}

	c = mattach(m, bogus.spec, bogus.serv);
	poperror();
	return c;
}

Chan *
mattach(Mnt *m, char *spec, char *serv)
{
	Chan *c;
	Mntrpc *r;

	r = mntralloc();
	c = devattach('M', spec);
	lock(&mntalloc);
	c->dev = mntalloc.id++;
	unlock(&mntalloc);
	c->mntptr = m;

	if(waserror()){
		mntfree(r);
		/* Close must not be called since it will call mnt recursively */
		chanfree(c);
		nexterror();
	}

	memset(r->request.auth, 0, sizeof r->request.auth);
	if(*serv)
		mntauth(m, r, serv, c->fid);

	r->request.type = Tattach;
	r->request.fid = c->fid;
	memmove(r->request.uname, u->p->user, NAMELEN);
	strncpy(r->request.aname, spec, NAMELEN);
	mountrpc(m, r);

	c->qid = r->reply.qid;
	c->mchan = m->c;
	c->mqid = c->qid;
	poperror();
	mntfree(r);
	return c;
}

void
mntauth(Mnt *m, Mntrpc *f, char *serv, ushort fid)
{
	int i;
	Mntrpc *r;
	uchar chal[AUTHLEN];

	r = mntralloc();
	if(waserror()) {
		mntfree(r);
		return;
	}

	r->request.type = Tauth;
	r->request.fid = fid;
	memmove(r->request.uname, u->p->user, NAMELEN);
	chal[0] = FScchal;
	for(i = 1; i < AUTHLEN; i++)
		chal[i] = nrand(256);

	memmove(r->request.chal, chal, AUTHLEN);
	strncpy(r->request.chal+AUTHLEN, serv, NAMELEN);
	encrypt(u->p->pgrp->crypt->key, r->request.chal, AUTHLEN+NAMELEN);

	mountrpc(m, r);

	decrypt(u->p->pgrp->crypt->key, r->reply.chal, 2*AUTHLEN+2*DESKEYLEN);
	chal[0] = FSctick;
	poperror();
	if(memcmp(chal, r->reply.chal, AUTHLEN) != 0) {
		mntfree(r);
		error(Eperm);
	}
	memmove(f->request.auth, r->reply.chal+AUTHLEN+DESKEYLEN, AUTHLEN+DESKEYLEN);
	mntfree(r);
}

Chan*
mntclone(Chan *c, Chan *nc)
{
	Mnt *m;
	Mntrpc *r;
	int alloc = 0;

	m = mntchk(c);
	r = mntralloc();
	if(nc == 0) {
		nc = newchan();
		alloc = 1;
	}
	if(waserror()) {
		mntfree(r);
		if(alloc)
			close(nc);
		nexterror();
	}

	r->request.type = Tclone;
	r->request.fid = c->fid;
	r->request.newfid = nc->fid;
	mountrpc(m, r);

	nc->type = c->type;
	nc->dev = c->dev;
	nc->qid = c->qid;
	nc->mode = c->mode;
	nc->flag = c->flag;
	nc->offset = c->offset;
	nc->mnt = c->mnt;
	nc->mountid = c->mountid;
	nc->aux = c->aux;
	nc->mchan = c->mchan;
	nc->mqid = c->qid;
	incref(m);

	USED(alloc);
	poperror();
	mntfree(r);
	return nc;
}

int	 
mntwalk(Chan *c, char *name)
{
	Mnt *m;
	Mntrpc *r;

	m = mntchk(c);
	r = mntralloc();
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

void	 
mntstat(Chan *c, char *dp)
{
	Mnt *m;
	Mntrpc *r;

	m = mntchk(c);
	r = mntralloc();
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

Chan*
mntopen(Chan *c, int omode)
{
	Mnt *m;
	Mntrpc *r;

	m = mntchk(c);
	r = mntralloc();
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
	return c;
}

void	 
mntcreate(Chan *c, char *name, int omode, ulong perm)
{
	Mnt *m;
	Mntrpc *r;

	m = mntchk(c);
	r = mntralloc();
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
}

void	 
mntclunk(Chan *c, int t)
{
	Mnt *m;
	Mntrpc *r;
		
	m = mntchk(c);
	r = mntralloc();
	if(waserror()){
		mntdoclunk(m, r);
		nexterror();
	}

	r->request.type = t;
	r->request.fid = c->fid;
	mountrpc(m, r);
	mntdoclunk(m, r);
	poperror();
}

void
mntdoclunk(Mnt *m, Mntrpc *r)
{
	Mntrpc *q;

	mntfree(r);
	if(decref(m) == 0) {
		for(q = m->queue; q; q = r) {
			r = q->list;
			q->flushed = 0;
			mntfree(q);
		}
		m->id = 0;
		close(m->c);
		mntpntfree(m);
	}
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
	Mntrpc *r;

	m = mntchk(c);
	r = mntralloc();
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
mntread(Chan *c, void *buf, long n, ulong offset)
{
	uchar *p, *e;

	n = mntrdwr(Tread, c, buf, n, offset);
	if(c->qid.path & CHDIR) 
		for(p = (uchar*)buf, e = &p[n]; p < e; p += DIRLEN)
			mntdirfix(p, c);

	return n;
}

long	 
mntwrite(Chan *c, void *buf, long n, ulong offset)
{
	return mntrdwr(Twrite, c, buf, n, offset);	
}

long
mntrdwr(int type, Chan *c, void *buf, long n, ulong offset)
{
	Mnt *m;
	Mntrpc *r;
	char *uba;
	ulong cnt, nr;

	m = mntchk(c);
	uba = buf;
	for(cnt = 0; n; n -= nr) {
		r = mntralloc();
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
		if(type == Tread)
			memmove(uba, r->reply.data, nr);
		poperror();
		mntfree(r);
		offset += nr;
		uba += nr;
		cnt += nr;
		if(nr != r->request.count)
			break;
	}
	return cnt;
}

void
mountrpc(Mnt *m, Mntrpc *r)
{
	r->reply.tag = 0;		/* poison the old values */
	r->reply.type = 4;

	mountio(m, r);
	if(r->reply.type == Rerror)
		error(r->reply.ename);

	if(r->reply.type == Rflush)
		error(Eintr);

	if(r->reply.type != r->request.type+1) {
		print("mnt: mismatched reply 0x%lux T%d R%d tags req %d fls %d rep %d\n",
				r, r->request.type, r->reply.type, r->request.tag, 
				r->flushtag, r->reply.tag);

		error(Emountrpc);
	}
}

void
mountio(Mnt *m, Mntrpc *r)
{
	int n;

	lock(m);
	r->flushed = 0;
	r->m = m;
	r->list = m->queue;
	m->queue = r;
	unlock(m);

	/* Transmit a file system rpc */
	n = convS2M(&r->request, r->rpc);
	if(waserror()) {
		if(mntflush(m, r) == 0)
			nexterror();
	}
	else {
		if((*devtab[m->c->type].write)(m->c, r->rpc, n, 0) != n)
			error(Emountrpc);
		poperror();
	}
	if(m->mux) {
		mntqrm(m, r);
		mntrpcread(m, r);
		return;
	}

	/* Gate readers onto the mount point one at a time */
	for(;;) {
		lock(m);
		if(m->rip == 0)
			break;
		unlock(m);
		if(waserror()) {
			if(mntflush(m, r) == 0)
				nexterror();
			continue;
		}
		sleep(&r->r, rpcattn, r);
		poperror();
		if(r->done)
			return;
	}
	m->rip = u->p;
	unlock(m);
	while(r->done == 0) {
		mntrpcread(m, r);
		mountmux(m, r);
	}
	mntgate(m);
}

void
mntrpcread(Mnt *m, Mntrpc *r)
{
	int n;

	for(;;) {
		if(waserror()) {
			if(mntflush(m, r) == 0) {
				if(m->mux == 0)
					mntgate(m);
				nexterror();
			}
			continue;
		}
		r->reply.type = 0;
		r->reply.tag = 0;
		n = (*devtab[m->c->type].read)(m->c, r->rpc, MAXRPC, 0);
		poperror();
		if(n == 0)
			continue;

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
	for(q = m->queue; q; q = q->list)
		if(q->done == 0) {
			lock(&q->r);
			if(q->r.p) {
				unlock(&q->r);
				unlock(m);
				wakeup(&q->r);
				return;
			}
			unlock(&q->r);
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
		if(q->request.tag == r->reply.tag
		|| q->flushed && q->flushtag == r->reply.tag) {
			*l = q->list;
			unlock(m);
			if(q != r) {		/* Completed someone else */
				dp = q->rpc;
				q->rpc = r->rpc;
				r->rpc = dp;
				memmove(&q->reply, &r->reply, sizeof(Fcall));
				q->done = 1;
				wakeup(&q->r);
			}else
				q->done = 1;
			return;
		}
		l = &q->list;
	}
	unlock(m);
}

int
mntflush(Mnt *m, Mntrpc *r)
{
	int n;
	Fcall flush;

	lock(m);
	r->flushtag = m->flushtag++;
	if(m->flushtag == Tagend)
		m->flushtag = m->flushbase;
	r->flushed = 1;
	unlock(m);

	flush.type = Tflush;
	flush.tag = r->flushtag;
	flush.oldtag = r->request.tag;
	n = convS2M(&flush, r->flush);

	if(waserror()) {
		if(strcmp(u->error, Eintr) == 0)
			return 1;
		mntqrm(m, r);
		return 0;
	}
	(*devtab[m->c->type].write)(m->c, r->flush, n, 0);
	poperror();
	return 1;
}

Mntrpc *
mntralloc(void)
{
	Mntrpc *new;

	lock(&mntalloc);
	new = mntalloc.rpcfree;
	if(new != 0)
		mntalloc.rpcfree = new->list;
	else {
		new = xalloc(sizeof(Mntrpc)+MAXRPC);
		if(new == 0) {
			unlock(&mntalloc);
			exhausted("mount rpc buffer");
		}
		new->rpc = (char*)new+sizeof(Mntrpc);
		new->request.tag = mntalloc.rpctag++;
	}
	unlock(&mntalloc);
	new->done = 0;
	new->flushed = 0;
	return new;
}

void
mntfree(Mntrpc *r)
{
	lock(&mntalloc);
	r->list = mntalloc.rpcfree;
	mntalloc.rpcfree = r;
	unlock(&mntalloc);
}

void
mntqrm(Mnt *m, Mntrpc *r)
{
	Mntrpc **l, *f;

	lock(m);
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
	unlock(m);
}

Mnt *
mntchk(Chan *c)
{
	Mnt *m;

	m = c->mntptr;
	/* Was it closed and reused ? */
	if(m->id == 0 || m->id >= c->dev)	/* Sanity check */
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
rpcattn(Mntrpc *r)
{
	return r->done || r->m->rip == 0;
}
