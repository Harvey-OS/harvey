#include	<u.h>
#include	<libc.h>
#include	<fcall.h>
#include	"compat.h"
#include	"error.h"

typedef	struct Fid	Fid;
typedef	struct Export	Export;
typedef	struct Exq	Exq;
typedef	struct Exwork	Exwork;

enum
{
	Nfidhash	= 32,
	Maxfdata	= 8192,
	Maxrpc		= IOHDRSZ + Maxfdata,
};

struct Export
{
	Ref	r;
	Exq*	work;
	Lock	fidlock;
	Fid*	fid[Nfidhash];
	int	io;		/* fd to read/write */
	int	iounit;
	int	nroots;
	Chan	**roots;
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
	int	responding;	/* writing out reply message */
	int	noresponse;	/* don't respond to this one */
	Exq*	next;
	int	shut;		/* has been noted for shutdown */
	Export*	export;
	void*	slave;
	Fcall	rpc;
	uchar	buf[Maxrpc];
};

struct Exwork
{
	Lock	l;

	int	ref;

	int	nwaiters;	/* queue of slaves waiting for work */
	QLock	qwait;
	Rendez	rwait;

	Exq	*head;		/* work waiting for a slave */
	Exq	*tail;
};

Exwork exq;

static void	exshutdown(Export*);
static void	exflush(Export*, int, int);
static void	exslave(void*);
static void	exfree(Export*);
static void	exportproc(Export*);

static char*	Exattach(Export*, Fcall*, uchar*);
static char*	Exauth(Export*, Fcall*, uchar*);
static char*	Exclunk(Export*, Fcall*, uchar*);
static char*	Excreate(Export*, Fcall*, uchar*);
static char*	Exversion(Export*, Fcall*, uchar*);
static char*	Exopen(Export*, Fcall*, uchar*);
static char*	Exread(Export*, Fcall*, uchar*);
static char*	Exremove(Export*, Fcall*, uchar*);
static char*	Exsession(Export*, Fcall*, uchar*);
static char*	Exstat(Export*, Fcall*, uchar*);
static char*	Exwalk(Export*, Fcall*, uchar*);
static char*	Exwrite(Export*, Fcall*, uchar*);
static char*	Exwstat(Export*, Fcall*, uchar*);

static char	*(*fcalls[Tmax])(Export*, Fcall*, uchar*);

static char	Enofid[]   = "no such fid";
static char	Eseekdir[] = "can't seek on a directory";
static char	Ereaddir[] = "unaligned read of a directory";
static int	exdebug = 0;

int
sysexport(int fd, Chan **roots, int nroots)
{
	Export *fs;

	fs = smalloc(sizeof(Export));
	fs->r.ref = 1;
	fs->io = fd;
	fs->roots = roots;
	fs->nroots = nroots;

	exportproc(fs);

	return 0;
}

static void
exportinit(void)
{
	lock(&exq.l);
	exq.ref++;
	if(fcalls[Tversion] != nil){
		unlock(&exq.l);
		return;
	}

	fmtinstall('F', fcallfmt);
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

static void
exportproc(Export *fs)
{
	Exq *q;
	int n, ed;

	exportinit();
	ed = errdepth(-1);
	for(;;){
		errdepth(ed);
		q = smalloc(sizeof(Exq));

		n = read9pmsg(fs->io, q->buf, Maxrpc);
		if(n <= 0 || convM2S(q->buf, n, &q->rpc) != n)
			goto bad;

		if(exdebug)
			print("export %d <- %F\n", getpid(), &q->rpc);

		if(q->rpc.type == Tflush){
			exflush(fs, q->rpc.tag, q->rpc.oldtag);
			free(q);
			continue;
		}

		q->export = fs;
		incref(&fs->r);

		lock(&exq.l);
		if(exq.head == nil)
			exq.head = q;
		else
			exq.tail->next = q;
		q->next = nil;
		exq.tail = q;
		n = exq.nwaiters;
		if(n)
			exq.nwaiters = n - 1;
		unlock(&exq.l);
		if(!n)
			kproc("exportfs", exslave, nil);
		rendwakeup(&exq.rwait);
	}
bad:
	free(q);
	if(exdebug)
		fprint(2, "export proc shutting down: %r\n");
	exshutdown(fs);
	exfree(fs);
}

static void
exflush(Export *fs, int flushtag, int tag)
{
	Exq *q, **last;
	Fcall fc;
	uchar buf[Maxrpc];
	int n;

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
	lock(&fs->r);
	for(q = fs->work; q != nil; q = q->next){
		if(q->rpc.tag == tag){
			lock(&q->lk);
			q->noresponse = 1;
			if(!q->responding)
				rendintr(q->slave);
			unlock(&q->lk);
			break;
		}
	}
	unlock(&fs->r);

Respond:
	fc.type = Rflush;
	fc.tag = flushtag;

	n = convS2M(&fc, buf, Maxrpc);
	if(n == 0)
		panic("convS2M error on write");
	if(write(fs->io, buf, n) != n)
		panic("mount write");
}

static void
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

	/*
	 * cleanly shut down the slaves if this is the last fs around
	 */
	exq.ref--;
	if(!exq.ref)
		rendwakeup(&exq.rwait);
	unlock(&exq.l);

	/*
	 * kick any sleepers
	 */
	lock(&fs->r);
	for(q = fs->work; q != nil; q = q->next){
		lock(&q->lk);
		q->noresponse = 1;
		if(!q->responding)
			rendintr(q->slave);
		unlock(&q->lk);
	}
	unlock(&fs->r);
}

static void
exfree(Export *fs)
{
	Fid *f, *n;
	int i;

	if(decref(&fs->r) != 0)
		return;
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

static int
exwork(void *)
{
	int work;

	lock(&exq.l);
	work = exq.head != nil || !exq.ref;
	unlock(&exq.l);
	return work;
}

static void
exslave(void *)
{
	Export *fs;
	Exq *q, *t, **last;
	char *volatile err;
	int n, ed;

	while(waserror())
		fprint(2, "exslave %d errored out of loop -- heading back in!\n", getpid());
	ed = errdepth(-1);
	for(;;){
		errdepth(ed);
		qlock(&exq.qwait);
		if(waserror()){
			qunlock(&exq.qwait);
			nexterror();
		}
		rendsleep(&exq.rwait, exwork, nil);

		lock(&exq.l);
		if(!exq.ref){
			unlock(&exq.l);
			poperror();
			qunlock(&exq.qwait);
			break;
		}
		q = exq.head;
		if(q == nil){
			unlock(&exq.l);
			poperror();
			qunlock(&exq.qwait);
			continue;
		}
		exq.head = q->next;
		if(exq.head == nil)
			exq.tail = nil;
		poperror();
		qunlock(&exq.qwait);

		/*
		 * put the job on the work queue before it's
		 * visible as off of the head queue, so it's always
		 * findable for flushes and shutdown
		 */
		q->slave = up;
		q->noresponse = 0;
		q->responding = 0;
		rendclearintr();
		fs = q->export;
		lock(&fs->r);
		q->next = fs->work;
		fs->work = q;
		unlock(&fs->r);

		unlock(&exq.l);

		if(exdebug > 1)
			print("exslave dispatch %d %F\n", getpid(), &q->rpc);

		if(waserror()){
			print("exslave err %r\n");
			err = up->error;
		}else{
			if(q->rpc.type >= Tmax || !fcalls[q->rpc.type])
				err = "bad fcall type";
			else
				err = (*fcalls[q->rpc.type])(fs, &q->rpc, &q->buf[IOHDRSZ]);
			poperror();
		}

		q->rpc.type++;
		if(err){
			q->rpc.type = Rerror;
			q->rpc.ename = err;
		}
		n = convS2M(&q->rpc, q->buf, Maxrpc);

		if(exdebug)
			print("exslave %d -> %F\n", getpid(), &q->rpc);

		lock(&q->lk);
		if(!q->noresponse){
			q->responding = 1;
			unlock(&q->lk);
			write(fs->io, q->buf, n);
		}else
			unlock(&q->lk);

		/*
		 * exflush might set noresponse at this point, but
		 * setting noresponse means don't send a response now;
		 * it's okay that we sent a response already.
		 */
		if(exdebug > 1)
			print("exslave %d written %d\n", getpid(), q->rpc.tag);

		lock(&fs->r);
		last = &fs->work;
		for(t = fs->work; t != nil; t = t->next){
			if(t == q){
				*last = q->next;
				break;
			}
			last = &t->next;
		}
		unlock(&fs->r);

		exfree(q->export);
		free(q);

		rendclearintr();
		lock(&exq.l);
		exq.nwaiters++;
		unlock(&exq.l);
	}
	if(exdebug)
		fprint(2, "export slaveshutting down\n");
	kexit();
}

Fid*
Exmkfid(Export *fs, int fid)
{
	ulong h;
	Fid *f, *nf;

	nf = mallocz(sizeof(Fid), 1);
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
	for(f = fs->fid[h]; f; f = f->next){
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

static char*
Exversion(Export *fs, Fcall *rpc, uchar *)
{
	if(rpc->msize > Maxrpc)
		rpc->msize = Maxrpc;
	if(strncmp(rpc->version, "9P", 2) != 0){
		rpc->version = "unknown";
		return nil;
	}

	fs->iounit = rpc->msize - IOHDRSZ;
	rpc->version = "9P2000";
	return nil;
}

static char*
Exauth(Export *, Fcall *, uchar *)
{
	return "vnc: authentication not required";
}

static char*
Exattach(Export *fs, Fcall *rpc, uchar *)
{
	Fid *f;
	int w;

	w = 0;
	if(rpc->aname != nil)
		w = strtol(rpc->aname, nil, 10);
	if(w < 0 || w > fs->nroots)
		error(Ebadspec);
	f = Exmkfid(fs, rpc->fid);
	if(f == nil)
		return Einuse;
	if(waserror()){
		f->attached = 0;
		Exputfid(fs, f);
		return up->error;
	}
	f->chan = cclone(fs->roots[w]);
	poperror();
	rpc->qid = f->chan->qid;
	Exputfid(fs, f);
	return nil;
}

static char*
Exclunk(Export *fs, Fcall *rpc, uchar *)
{
	Fid *f;

	f = Exgetfid(fs, rpc->fid);
	if(f != nil){
		f->attached = 0;
		Exputfid(fs, f);
	}
	return nil;
}

static char*
Exwalk(Export *fs, Fcall *rpc, uchar *)
{
	Fid *volatile f, *volatile nf;
	Walkqid *wq;
	Chan *c;
	int i, nwname;
	int volatile isnew;

	f = Exgetfid(fs, rpc->fid);
	if(f == nil)
		return Enofid;
	nf = nil;
	if(waserror()){
		Exputfid(fs, f);
		if(nf != nil)
			Exputfid(fs, nf);
		return up->error;
	}

	/*
	 * optional clone, but don't attach it until the walk succeeds.
	 */
	if(rpc->fid != rpc->newfid){
		nf = Exmkfid(fs, rpc->newfid);
		if(nf == nil)
			error(Einuse);
		nf->attached = 0;
		isnew = 1;
	}else{
		nf = Exgetfid(fs, rpc->fid);
		isnew = 0;
	}

	/*
	 * let the device do the work
	 */
	c = f->chan;
	nwname = rpc->nwname;
	wq = (*devtab[c->type]->walk)(c, nf->chan, rpc->wname, nwname);
	if(wq == nil)
		error(Enonexist);

	poperror();

	/*
	 * copy qid array
	 */
	for(i = 0; i < wq->nqid; i++)
		rpc->wqid[i] = wq->qid[i];
	rpc->nwqid = wq->nqid;

	/*
	 * update the channel if everything walked correctly.
	 */
	if(isnew && wq->nqid == nwname){
		nf->chan = wq->clone;
		nf->attached = 1;
	}

	free(wq);
	Exputfid(fs, f);
	Exputfid(fs, nf);
	return nil;
}

static char*
Exopen(Export *fs, Fcall *rpc, uchar *)
{
	Fid *volatile f;
	Chan *c;
	int iou;

	f = Exgetfid(fs, rpc->fid);
	if(f == nil)
		return Enofid;
	if(waserror()){
		Exputfid(fs, f);
		return up->error;
	}
	c = f->chan;
	c = (*devtab[c->type]->open)(c, rpc->mode);
	poperror();

	f->chan = c;
	f->offset = 0;
	rpc->qid = f->chan->qid;
	iou = f->chan->iounit;
	if(iou > fs->iounit)
		iou = fs->iounit;
	rpc->iounit = iou;
	Exputfid(fs, f);
	return nil;
}

static char*
Excreate(Export *fs, Fcall *rpc, uchar *)
{
	Fid *f;
	Chan *c;
	int iou;

	f = Exgetfid(fs, rpc->fid);
	if(f == nil)
		return Enofid;
	if(waserror()){
		Exputfid(fs, f);
		return up->error;
	}
	c = f->chan;
	(*devtab[c->type]->create)(c, rpc->name, rpc->mode, rpc->perm);
	poperror();

	f->chan = c;
	rpc->qid = f->chan->qid;
	iou = f->chan->iounit;
	if(iou > fs->iounit)
		iou = fs->iounit;
	rpc->iounit = iou;
	Exputfid(fs, f);
	return nil;
}

static char*
Exread(Export *fs, Fcall *rpc, uchar *buf)
{
	Fid *f;
	Chan *c;
	long off;

	f = Exgetfid(fs, rpc->fid);
	if(f == nil)
		return Enofid;

	c = f->chan;

	if(waserror()){
		Exputfid(fs, f);
		return up->error;
	}

	rpc->data = (char*)buf;
	off = rpc->offset;
	c->offset = off;
	rpc->count = (*devtab[c->type]->read)(c, rpc->data, rpc->count, off);
	poperror();
	Exputfid(fs, f);
	return nil;
}

static char*
Exwrite(Export *fs, Fcall *rpc, uchar *)
{
	Fid *f;
	Chan *c;

	f = Exgetfid(fs, rpc->fid);
	if(f == nil)
		return Enofid;
	if(waserror()){
		Exputfid(fs, f);
		return up->error;
	}
	c = f->chan;
	if(c->qid.type & QTDIR)
		error(Eisdir);
	rpc->count = (*devtab[c->type]->write)(c, rpc->data, rpc->count, rpc->offset);
	poperror();
	Exputfid(fs, f);
	return nil;
}

static char*
Exstat(Export *fs, Fcall *rpc, uchar *buf)
{
	Fid *f;
	Chan *c;

	f = Exgetfid(fs, rpc->fid);
	if(f == nil)
		return Enofid;
	if(waserror()){
		Exputfid(fs, f);
		return up->error;
	}
	c = f->chan;
	rpc->stat = buf;
	rpc->nstat = (*devtab[c->type]->stat)(c, rpc->stat, Maxrpc);
	poperror();
	Exputfid(fs, f);
	return nil;
}

static char*
Exwstat(Export *fs, Fcall *rpc, uchar *)
{
	Fid *f;
	Chan *c;

	f = Exgetfid(fs, rpc->fid);
	if(f == nil)
		return Enofid;
	if(waserror()){
		Exputfid(fs, f);
		return up->error;
	}
	c = f->chan;
	(*devtab[c->type]->wstat)(c, rpc->stat, rpc->nstat);
	poperror();
	Exputfid(fs, f);
	return nil;
}

static char*
Exremove(Export *fs, Fcall *rpc, uchar *)
{
	Fid *f;
	Chan *c;

	f = Exgetfid(fs, rpc->fid);
	if(f == nil)
		return Enofid;
	if(waserror()){
		Exputfid(fs, f);
		return up->error;
	}
	c = f->chan;
	(*devtab[c->type]->remove)(c);
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
