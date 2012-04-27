#include	"u.h"
#include	"lib.h"
#include	"dat.h"
#include	"fns.h"
#include	"error.h"

typedef	struct Fid	Fid;
typedef	struct Export	Export;
typedef	struct Exq	Exq;

#define	nil	((void*)0)

enum
{
	Nfidhash	= 1,
	MAXRPC		= MAXMSG+MAXFDATA,
	MAXDIRREAD	= (MAXFDATA/DIRLEN)*DIRLEN
};

struct Export
{
	Ref	r;
	Exq*	work;
	Lock	fidlock;
	Fid*	fid[Nfidhash];
	Chan*	root;
	Chan*	io;
	Pgrp*	pgrp;
	int	npart;
	char	part[MAXRPC];
};

struct Fid
{
	Fid*	next;
	Fid**	last;
	Chan*	chan;
	long	offset;
	int	fid;
	int	ref;		/* fcalls using the fid; locked by Export.Lock */
	int	attached;	/* fid attached or cloned but not clunked */
};

struct Exq
{
	Lock	lk;
	int	nointr;
	int	noresponse;	/* don't respond to this one */
	Exq*	next;
	int	shut;		/* has been noted for shutdown */
	Export*	export;
	void*	slave;
	Fcall	rpc;
	char	buf[MAXRPC];
};

struct
{
	Lock	l;
	Qlock	qwait;
	Rendez	rwait;
	Exq	*head;		/* work waiting for a slave */
	Exq	*tail;
}exq;

static void	exshutdown(Export*);
static void	exflush(Export*, int, int);
static void	exslave(void*);
static void	exfree(Export*);
static void	exportproc(Export*);

static char*	Exauth(Export*, Fcall*);
static char*	Exattach(Export*, Fcall*);
static char*	Exclunk(Export*, Fcall*);
static char*	Excreate(Export*, Fcall*);
static char*	Exopen(Export*, Fcall*);
static char*	Exread(Export*, Fcall*);
static char*	Exremove(Export*, Fcall*);
static char*	Exstat(Export*, Fcall*);
static char*	Exwalk(Export*, Fcall*);
static char*	Exwrite(Export*, Fcall*);
static char*	Exwstat(Export*, Fcall*);
static char*	Exversion(Export*, Fcall*);

static char	*(*fcalls[Tmax])(Export*, Fcall*);

static char	Enofid[]   = "no such fid";
static char	Eseekdir[] = "can't seek on a directory";
static char	Ereaddir[] = "unaligned read of a directory";
static int	exdebug = 0;

int
sysexport(int fd)
{
	Chan *c;
	Export *fs;

	if(waserror())
		return -1;

	c = fdtochan(fd, ORDWR, 1, 1);
	poperror();
	c->flag |= CMSG;

	fs = mallocz(sizeof(Export));
	fs->r.ref = 1;
	fs->pgrp = up->pgrp;
	refinc(&fs->pgrp->r);
	refinc(&up->slash->r);
	fs->root = up->slash;
	refinc(&fs->root->r);
	fs->root = domount(fs->root);
	fs->io = c;

	exportproc(fs);

	return 0;
}

static void
exportinit(void)
{
	lock(&exq.l);
	if(fcalls[Tversion] != nil) {
		unlock(&exq.l);
		return;
	}

	fmtinstall('F', fcallfmt);
	fmtinstall('D', dirfmt);
	fmtinstall('M', dirmodefmt);
	fcalls[Tversion] = Exversion;
	fcalls[Tauth] = Exauth;
	fcalls[Tattach] = Exattach;
	fcalls[Twalk] = Exwalk;
	fcalls[Topen] = Exopen;
	fcalls[Tcreate] = Excreate;
	fcalls[Tread] = Exread;
	fcalls[Twrite] = Exwrite;
	fcalls[Tclunk] = Exclunk;
	fcalls[Tremove] = Exremove;
	fcalls[Tstat] = Exstat;
	fcalls[Twstat] = Exwstat;
	unlock(&exq.l);
}

void
exportproc(Export *fs)
{
	Exq *q;
	char *buf;
	int n, cn, len;

	exportinit();

	for(;;){
		q = mallocz(sizeof(Exq));
		if(q == 0)
			panic("no memory");

		q->rpc.data = q->buf + MAXMSG;

		buf = q->buf;
		len = MAXRPC;
		if(fs->npart) {
			memmove(buf, fs->part, fs->npart);
			buf += fs->npart;
			len -= fs->npart;
			goto chk;
		}
		for(;;) {
			if(waserror())
				goto bad;

			n = (*devtab[fs->io->type].read)(fs->io, buf, len, 0);
			poperror();

			if(n <= 0)
				goto bad;

			buf += n;
			len -= n;
	chk:
			n = buf - q->buf;

			/* convM2S returns size of correctly decoded message */
			cn = convM2S(q->buf, &q->rpc, n);
			if(cn < 0){
				iprint("bad message type in devmnt\n");
				goto bad;
			}
			if(cn > 0) {
				n -= cn;
				if(n < 0){
					iprint("negative size in devmnt");
					goto bad;
				}
				fs->npart = n;
				if(n != 0)
					memmove(fs->part, q->buf+cn, n);
				break;
			}
		}
		if(exdebug)
			iprint("export %d <- %F\n", getpid(), &q->rpc);

		if(q->rpc.type == Tflush){
			exflush(fs, q->rpc.tag, q->rpc.oldtag);
			free(q);
			continue;
		}

		q->export = fs;
		refinc(&fs->r);

		lock(&exq.l);
		if(exq.head == nil)
			exq.head = q;
		else
			exq.tail->next = q;
		q->next = nil;
		exq.tail = q;
		unlock(&exq.l);
		if(exq.qwait.first == nil) {
			n = thread("exportfs", exslave, nil);
/* iprint("launch export (pid=%ux)\n", n); */
		}
		rendwakeup(&exq.rwait);
	}
bad:
	free(q);
	exshutdown(fs);
	exfree(fs);
}

void
exflush(Export *fs, int flushtag, int tag)
{
	Exq *q, **last;
	int n;
	Fcall fc;
	char buf[MAXMSG];

	/* hasn't been started? */
	lock(&exq.l);
	last = &exq.head;
	for(q = exq.head; q != nil; q = q->next){
		if(q->export == fs && q->rpc.tag == tag){
			*last = q->next;
			unlock(&exq.l);
			exfree(fs);
			free(q);
			goto Respond;
		}
		last = &q->next;
	}
	unlock(&exq.l);

	/* in progress? */
	lock(&fs->r.l);
	for(q = fs->work; q != nil; q = q->next){
		if(q->rpc.tag == tag && !q->noresponse){
			lock(&q->lk);
			q->noresponse = 1;
			if(!q->nointr)
				intr(q->slave);
			unlock(&q->lk);
			unlock(&fs->r.l);
			goto Respond;
			return;
		}
	}
	unlock(&fs->r.l);

if(exdebug) iprint("exflush: did not find rpc: %d\n", tag);

Respond:
	fc.type = Rflush;
	fc.tag = flushtag;
	n = convS2M(&fc, buf);
if(exdebug) iprint("exflush -> %F\n", &fc);
	if(!waserror()){
		(*devtab[fs->io->type].write)(fs->io, buf, n, 0);
		poperror();
	}
}

void
exshutdown(Export *fs)
{
	Exq *q, **last;

	lock(&exq.l);
	last = &exq.head;
	for(q = exq.head; q != nil; q = *last){
		if(q->export == fs){
			*last = q->next;
			exfree(fs);
			free(q);
			continue;
		}
		last = &q->next;
	}
	unlock(&exq.l);

	lock(&fs->r.l);
	q = fs->work;
	while(q != nil){
		if(q->shut){
			q = q->next;
			continue;
		}
		q->shut = 1;
		unlock(&fs->r.l);
	/*	postnote(q->slave, 1, "interrupted", NUser);	*/
		iprint("postnote 2!\n");
		lock(&fs->r.l);
		q = fs->work;
	}
	unlock(&fs->r.l);
}

void
exfree(Export *fs)
{
	Fid *f, *n;
	int i;

	if(refdec(&fs->r) != 0)
		return;
	closepgrp(fs->pgrp);
	cclose(fs->root);
	cclose(fs->io);
	for(i = 0; i < Nfidhash; i++){
		for(f = fs->fid[i]; f != nil; f = n){
			if(f->chan != nil)
				cclose(f->chan);
			n = f->next;
			free(f);
		}
	}
	free(fs);
}

int
exwork(void *a)
{
	return exq.head != nil;
}

void
exslave(void *a)
{
	Export *fs;
	Exq *q, *t, **last;
	char *err;
	int n;
/*
	closepgrp(up->pgrp);
	up->pgrp = nil;
*/
	for(;;){
		qlock(&exq.qwait);
		rendsleep(&exq.rwait, exwork, nil);

		lock(&exq.l);
		q = exq.head;
		if(q == nil) {
			unlock(&exq.l);
			qunlock(&exq.qwait);
			continue;
		}
		exq.head = q->next;
		q->slave = curthread();
		unlock(&exq.l);

		qunlock(&exq.qwait);

		q->noresponse = 0;
		q->nointr = 0;
		fs = q->export;
		lock(&fs->r.l);
		q->next = fs->work;
		fs->work = q;
		unlock(&fs->r.l);

		up->pgrp = q->export->pgrp;

		if(exdebug > 1)
			iprint("exslave dispatch %d %F\n", getpid(), &q->rpc);

		if(waserror()){
			iprint("exslave err %r\n");
			err = up->errstr;
			goto Err;
		}
		if(q->rpc.type >= Tmax || !fcalls[q->rpc.type])
			err = "bad fcall type";
		else
			err = (*fcalls[q->rpc.type])(fs, &q->rpc);

		poperror();
		Err:;
		q->rpc.type++;
		if(err){
			q->rpc.type = Rerror;
			strncpy(q->rpc.ename, err, ERRLEN);
		}
		n = convS2M(&q->rpc, q->buf);

		if(exdebug)
			iprint("exslve %d -> %F\n", getpid(), &q->rpc);

		lock(&q->lk);
		if(q->noresponse == 0){
			q->nointr = 1;
			clearintr();
			if(!waserror()){
				(*devtab[fs->io->type].write)(fs->io, q->buf, n, 0);
				poperror();
			}
		}
		unlock(&q->lk);

		/*
		 * exflush might set noresponse at this point, but
		 * setting noresponse means don't send a response now;
		 * it's okay that we sent a response already.
		 */
		if(exdebug > 1)
			iprint("exslave %d written %d\n", getpid(), q->rpc.tag);

		lock(&fs->r.l);
		last = &fs->work;
		for(t = fs->work; t != nil; t = t->next){
			if(t == q){
				*last = q->next;
				break;
			}
			last = &t->next;
		}
		unlock(&fs->r.l);

		exfree(q->export);
		free(q);
	}
	iprint("exslave shut down");
	threadexit();
}

Fid*
Exmkfid(Export *fs, int fid)
{
	ulong h;
	Fid *f, *nf;

	nf = mallocz(sizeof(Fid));
	if(nf == nil)
		return nil;
	lock(&fs->fidlock);
	h = fid % Nfidhash;
	for(f = fs->fid[h]; f != nil; f = f->next){
		if(f->fid == fid){
			unlock(&fs->fidlock);
			free(nf);
			return nil;
		}
	}

	nf->next = fs->fid[h];
	if(nf->next != nil)
		nf->next->last = &nf->next;
	nf->last = &fs->fid[h];
	fs->fid[h] = nf;

	nf->fid = fid;
	nf->ref = 1;
	nf->attached = 1;
	nf->offset = 0;
	nf->chan = nil;
	unlock(&fs->fidlock);
	return nf;
}

Fid*
Exgetfid(Export *fs, int fid)
{
	Fid *f;
	ulong h;

	lock(&fs->fidlock);
	h = fid % Nfidhash;
	for(f = fs->fid[h]; f; f = f->next) {
		if(f->fid == fid){
			if(f->attached == 0)
				break;
			f->ref++;
			unlock(&fs->fidlock);
			return f;
		}
	}
	unlock(&fs->fidlock);
	return nil;
}

void
Exputfid(Export *fs, Fid *f)
{
	lock(&fs->fidlock);
	f->ref--;
	if(f->ref == 0 && f->attached == 0){
		if(f->chan != nil)
			cclose(f->chan);
		f->chan = nil;
		*f->last = f->next;
		if(f->next != nil)
			f->next->last = f->last;
		unlock(&fs->fidlock);
		free(f);
		return;
	}
	unlock(&fs->fidlock);
}

char*
Exsession(Export *e, Fcall *rpc)
{
	memset(rpc->authid, 0, sizeof(rpc->authid));
	memset(rpc->authdom, 0, sizeof(rpc->authdom));
	memset(rpc->chal, 0, sizeof(rpc->chal));
	return nil;
}

char*
Exauth(Export *e, Fcall *f)
{
	return "authentication not required";
}

char*
Exattach(Export *fs, Fcall *rpc)
{
	Fid *f;

	f = Exmkfid(fs, rpc->fid);
	if(f == nil)
		return Einuse;
	if(waserror()){
		f->attached = 0;
		Exputfid(fs, f);
		return up->errstr;
	}
	f->chan = clone(fs->root, nil);
	poperror();
	rpc->qid = f->chan->qid;
	Exputfid(fs, f);
	return nil;
}

char*
Exclone(Export *fs, Fcall *rpc)
{
	Fid *f, *nf;

	if(rpc->fid == rpc->newfid)
		return Einuse;
	f = Exgetfid(fs, rpc->fid);
	if(f == nil)
		return Enofid;
	nf = Exmkfid(fs, rpc->newfid);
	if(nf == nil){
		Exputfid(fs, f);
		return Einuse;
	}
	if(waserror()){
		Exputfid(fs, f);
		Exputfid(fs, nf);
		return up->errstr;
	}
	nf->chan = clone(f->chan, nil);
	poperror();
	Exputfid(fs, f);
	Exputfid(fs, nf);
	return nil;
}

char*
Exclunk(Export *fs, Fcall *rpc)
{
	Fid *f;

	f = Exgetfid(fs, rpc->fid);
	if(f != nil){
		f->attached = 0;
		Exputfid(fs, f);
	}
	return nil;
}

char*
Exwalk(Export *fs, Fcall *rpc)
{
	Fid *f;
	Chan *c;

	f = Exgetfid(fs, rpc->fid);
	if(f == nil)
		return Enofid;
	if(waserror()){
		Exputfid(fs, f);
		return up->errstr;
	}
	c = walk(f->chan, rpc->name, 1);
	if(c == nil)
		error(Enonexist);
	poperror();

	f->chan = c;
	rpc->qid = c->qid;
	Exputfid(fs, f);
	return nil;
}

char*
Exopen(Export *fs, Fcall *rpc)
{
	Fid *f;
	Chan *c;

	f = Exgetfid(fs, rpc->fid);
	if(f == nil)
		return Enofid;
	if(waserror()){
		Exputfid(fs, f);
		return up->errstr;
	}
	c = f->chan;
	c = (*devtab[c->type].open)(c, rpc->mode);
	poperror();

	f->chan = c;
	f->offset = 0;
	rpc->qid = f->chan->qid;
	Exputfid(fs, f);
	return nil;
}

char*
Excreate(Export *fs, Fcall *rpc)
{
	Fid *f;
	Chan *c;

	f = Exgetfid(fs, rpc->fid);
	if(f == nil)
		return Enofid;
	if(waserror()){
		Exputfid(fs, f);
		return up->errstr;
	}
	c = f->chan;
	if(c->mnt && !(c->flag&CCREATE))
		c = createdir(c);
	(*devtab[c->type].create)(c, rpc->name, rpc->mode, rpc->perm);
	poperror();

	f->chan = c;
	rpc->qid = f->chan->qid;
	Exputfid(fs, f);
	return nil;
}

char*
Exread(Export *fs, Fcall *rpc)
{
	Fid *f;
	Chan *c;
	long off;
	int dir, n, seek;

	f = Exgetfid(fs, rpc->fid);
	if(f == nil)
		return Enofid;

	c = f->chan;
	dir = c->qid.path & CHDIR;
	if(dir){
		rpc->count -= rpc->count%DIRLEN;
		if(rpc->offset%DIRLEN || rpc->count==0){
			Exputfid(fs, f);
			return Ereaddir;
		}
		if(f->offset > rpc->offset){
			Exputfid(fs, f);
			return Eseekdir;
		}
	}

	if(waserror()) {
		Exputfid(fs, f);
		return up->errstr;
	}

	for(;;){
		n = rpc->count;
		seek = 0;
		off = rpc->offset;
		if(dir && f->offset != off){
			off = f->offset;
			n = rpc->offset - off;
			if(n > MAXDIRREAD)
				n = MAXDIRREAD;
			seek = 1;
		}
		if(dir && c->mnt != nil)
			n = unionread(c, rpc->data, n);
		else{
			c->offset = off;
			n = (*devtab[c->type].read)(c, rpc->data, n, off);
		}
		if(n == 0 || !seek)
			break;
		f->offset = off + n;
		c->offset += n;
	}
	rpc->count = n;
	poperror();
	Exputfid(fs, f);
	return nil;
}

char*
Exwrite(Export *fs, Fcall *rpc)
{
	Fid *f;
	Chan *c;

	f = Exgetfid(fs, rpc->fid);
	if(f == nil)
		return Enofid;
	if(waserror()){
		Exputfid(fs, f);
		return up->errstr;
	}
	c = f->chan;
	if(c->qid.path & CHDIR)
		error(Eisdir);
	rpc->count = (*devtab[c->type].write)(c, rpc->data, rpc->count, rpc->offset);
	poperror();
	Exputfid(fs, f);
	return nil;
}

char*
Exstat(Export *fs, Fcall *rpc)
{
	Fid *f;
	Chan *c;

	f = Exgetfid(fs, rpc->fid);
	if(f == nil)
		return Enofid;
	if(waserror()){
		Exputfid(fs, f);
		return up->errstr;
	}
	c = f->chan;
	(*devtab[c->type].stat)(c, rpc->stat);
	poperror();
	Exputfid(fs, f);
	return nil;
}

char*
Exwstat(Export *fs, Fcall *rpc)
{
	Fid *f;
	Chan *c;

	f = Exgetfid(fs, rpc->fid);
	if(f == nil)
		return Enofid;
	if(waserror()){
		Exputfid(fs, f);
		return up->errstr;
	}
	c = f->chan;
	(*devtab[c->type].wstat)(c, rpc->stat);
	poperror();
	Exputfid(fs, f);
	return nil;
}

char*
Exremove(Export *fs, Fcall *rpc)
{
	Fid *f;
	Chan *c;

	f = Exgetfid(fs, rpc->fid);
	if(f == nil)
		return Enofid;
	if(waserror()){
		Exputfid(fs, f);
		return up->errstr;
	}
	c = f->chan;
	(*devtab[c->type].remove)(c);
	poperror();

	/*
	 * chan is already clunked by remove.
	 * however, we need to recover the chan,
	 * and follow sysremove's lead in making to point to root.
	 */
	c->type = 0;

	f->attached = 0;
	Exputfid(fs, f);
	return nil;
}
