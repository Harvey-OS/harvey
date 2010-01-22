/*
 * Framework for USB devices that provide a file tree.
 * The main process (usbd or the driver's main proc)
 * calls fsinit() to start FS operation.
 *
 * One or more drivers call fsstart/fsend to register
 * or unregister their operations for their subtrees.
 *
 * root dir has qids with 0 in high 32 bits.
 * for other files we keep the device id in there.
 * The low 32 bits for directories at / must be 0.
 */
#include <u.h>
#include <libc.h>
#include <thread.h>
#include <fcall.h>
#include "usb.h"
#include "usbfs.h"

#undef dprint
#define dprint if(usbfsdebug)fprint

typedef struct Rpc Rpc;

enum
{
	Nproc = 3,		/* max nb. of cached FS procs */

	Nofid = ~0,		/* null value for fid number */
	Notag = ~0,		/* null value for tags */
	Dietag = 0xdead,		/* fake tag to ask outproc to die */

	Stack = 16 * 1024,

	/* Fsproc requests */
	Run = 0,		/* call f(r) */
	Exit,			/* terminate */

};

struct Rpc
{
	Fcall	t;
	Fcall	r;
	Fid*	fid;
	int	flushed;
	Rpc*	next;
	char	data[Bufsize];
};

int usbfsdebug;

char Enotfound[] = "file not found";
char Etoosmall[] = "parameter too small";
char Eio[] = "i/o error";
char Eperm[] = "permission denied";
char Ebadcall[] = "unknown fs call";
char Ebadfid[] = "fid not found";
char Einuse[] = "fid already in use";
char Eisopen[] = "it is already open";
char Ebadctl[] = "unknown control request";

static char *user;
static ulong epoch;
static ulong msgsize = Msgsize;
static int fsfd = -1;
static Channel *outc;	/* of Rpc* */

static QLock rpclck;	/* protect vars in this block */
static Fid *freefids;
static Fid *fids;
static Rpc *freerpcs;
static Rpc *rpcs;

static Channel*procc;
static Channel*endc;

static Usbfs* fsops;

static void fsioproc(void*);

static void
schedproc(void*)
{
	Channel *proc[Nproc];
	int nproc;
	Channel *p;

	Alt a[] =
	{
		{procc, &proc[0], CHANSND},
		{endc, &p, CHANRCV},
		{nil, nil, CHANEND}
	};
	memset(proc, 0, sizeof(proc));
	nproc = 0;
	for(;;){
		if(nproc == 0){
			proc[0] = chancreate(sizeof(Rpc*), 0);
			proccreate(fsioproc, proc[0], Stack);
			nproc++;
		}
		switch(alt(a)){
		case 0:
			proc[0] = nil;
			if(nproc > 1){
				proc[0] = proc[nproc-1];
				proc[nproc-1] = nil;
			}
			nproc--;
			break;
		case 1:
			if(nproc < nelem(proc))
				proc[nproc++] = p;
			else
				sendp(p, nil);
			break;
		default:
			sysfatal("alt");
		}
	}
}

static void
dump(void)
{
	Rpc *rpc;
	Fid *fid;

	qlock(&rpclck);
	fprint(2, "dump:\n");
	for(rpc = rpcs; rpc != nil; rpc = rpc->next)
		fprint(2, "rpc %#p %F next %#p\n", rpc, &rpc->t, rpc->next);
	for(fid = fids; fid != nil; fid = fid->next)
		fprint(2, "fid %d qid %#llux omode %d aux %#p\n",
			fid->fid, fid->qid.path, fid->omode, fid->aux);
	fprint(2, "\n");
	qunlock(&rpclck);
}

static Rpc*
newrpc(void)
{
	Rpc *r;

	qlock(&rpclck);
	r = freerpcs;
	if(r != nil)
		freerpcs = r->next;
	else
		r = emallocz(sizeof(Rpc), 0);
	r->next = rpcs;
	rpcs = r;
	r->t.tag = r->r.tag = Notag;
	r->t.fid = r->r.fid = Nofid;
	r->t.type = r->r.type = 0;
	r->flushed = 0;
	r->fid = nil;
	r->r.data = (char*)r->data;
	qunlock(&rpclck);
	return r;
}

static void
freerpc(Rpc *r)
{
	Rpc **l;
	if(r == nil)
		return;
	qlock(&rpclck);
	for(l = &rpcs; *l != nil && *l != r; l = &(*l)->next)
		;
	assert(*l == r);
	*l = r->next;
	r->next = freerpcs;
	freerpcs = r;
	r->t.type = 0;
	r->t.tag = 0x77777777;
	qunlock(&rpclck);
}

static void
flushrpc(int tag)
{
	Rpc *r;

	qlock(&rpclck);
	for(r = rpcs; r != nil; r = r->next)
		if(r->t.tag == tag){
			r->flushed = 1;
			break;
		}
	qunlock(&rpclck);
}

static Fid*
getfid(int fid, int alloc)
{
	Fid *f;

	qlock(&rpclck);
	for(f = fids; f != nil && f->fid != fid; f = f->next)
		;
	if(f != nil && alloc != 0){	/* fid in use */
		qunlock(&rpclck);
		return nil;
	}
	if(f == nil && alloc != 0){
		if(freefids != nil){
			f = freefids;
			freefids = freefids->next;
		}else
			f = emallocz(sizeof(Fid), 1);
		f->fid = fid;
		f->aux = nil;
		f->omode = ONONE;
		f->next = fids;
		fids = f;
	}
	qunlock(&rpclck);
	return f;
}

static void
freefid(Fid *f)
{
	Fid **l;

	if(f == nil)
		return;
	if(fsops->clunk != nil)
		fsops->clunk(fsops, f);
	qlock(&rpclck);
	for(l = &fids; *l != nil && *l != f; l = &(*l)->next)
		;
	assert(*l == f);
	*l = f->next;
	f->next = freefids;
	freefids = f;
	qunlock(&rpclck);
}

static Rpc*
fserror(Rpc *rpc, char* fmt, ...)
{
	va_list arg;
	char *c;

	va_start(arg, fmt);
	c = (char*)rpc->data;
	vseprint(c, c+sizeof(rpc->data), fmt, arg);
	va_end(arg);
	rpc->r.type = Rerror;
	rpc->r.ename = (char*)rpc->data;
	return rpc;
}

static Rpc*
fsversion(Rpc *r)
{
	if(r->t.msize < 256)
		return fserror(r, Etoosmall);
	if(strncmp(r->t.version, "9P2000", 6) != 0)
		return fserror(r, "wrong version");
	if(r->t.msize < msgsize)
		msgsize = r->t.msize;
	r->r.msize = msgsize;
	r->r.version = "9P2000";
	return r;
}

static Rpc*
fsattach(Rpc *r)
{
	static int already;

	/* Reload user because at boot it could be still none */
	user=getuser();
	if(already++ > 0 && strcmp(r->t.uname, user) != 0)
		return fserror(r, Eperm);
	if(r->fid == nil)
		return fserror(r, Einuse);

	r->r.qid.type = QTDIR;
	r->r.qid.path = fsops->qid;
	r->r.qid.vers = 0;
	r->fid->qid = r->r.qid;
	return r;
}

static Rpc*
fswalk(Rpc *r)
{
	int i;
	Fid *nfid, *ofid;

	if(r->fid->omode != ONONE)
		return fserror(r, Eisopen);

	nfid = nil;
	ofid = r->fid;
	if(r->t.newfid != r->t.fid){
		nfid = getfid(r->t.newfid, 1);
		if(nfid == nil)
			return fserror(r, Einuse);
		nfid->qid = r->fid->qid;
		if(fsops->clone != nil)
			fsops->clone(fsops, ofid, nfid);
		else
			nfid->aux = r->fid->aux;
		r->fid = nfid;
	}
	r->r.nwqid = 0;
	for(i = 0; i < r->t.nwname; i++)
		if(fsops->walk(fsops, r->fid, r->t.wname[i]) < 0)
			break;
		else
			r->r.wqid[i] = r->fid->qid;
	r->r.nwqid = i;
	if(i != r->t.nwname && r->t.nwname > 0){
		if(nfid != nil)
			freefid(nfid);
		r->fid = ofid;
	}
	if(i == 0 && r->t.nwname > 0)
		return fserror(r, "%r");
	return r;
}

static void
fsioproc(void* a)
{
	long rc;
	Channel *p = a;
	Rpc *rpc;
	Fcall *t, *r;
	Fid *fid;

	dprint(2, "%s: fsioproc pid %d\n", argv0, getpid());
	while((rpc = recvp(p)) != nil){
		t = &rpc->t;
		r = &rpc->r;
		fid = rpc->fid;
		rc = -1;
		dprint(2, "%s: fsioproc pid %d: req %d\n", argv0, getpid(), t->type);
		switch(t->type){
		case Topen:
			rc = fsops->open(fsops, fid, t->mode);
			if(rc >= 0){
				r->iounit = 0;
				r->qid = fid->qid;
				fid->omode = t->mode & 3;
			}
			break;
		case Tread:
			rc = fsops->read(fsops, fid, r->data, t->count, t->offset);
			if(rc >= 0){
				if(rc > t->count)
					print("%s: bug: read %ld bytes > %ud wanted\n",
						argv0, rc, t->count);
				r->count = rc;
			}
			/*
			 * TODO: if we encounter a long run of continuous read
			 * errors, we should do something more drastic so that
			 * our caller doesn't just spin its wheels forever.
			 */
			break;
		case Twrite:
			rc = fsops->write(fsops, fid, t->data, t->count, t->offset);
			r->count = rc;
			break;
		default:
			sysfatal("fsioproc: bad type");
		}
		if(rc < 0)
			sendp(outc, fserror(rpc, "%r"));
		else
			sendp(outc, rpc);
		sendp(endc, p);
	}
	chanfree(p);
	dprint(2, "%s: fsioproc %d exiting\n", argv0, getpid());
	threadexits(nil);
}

static Rpc*
fsopen(Rpc *r)
{
	Channel *p;

	if(r->fid->omode != ONONE)
		return fserror(r, Eisopen);
	if((r->t.mode & 3) != OREAD && (r->fid->qid.type & QTDIR) != 0)
		return fserror(r, Eperm);
	p = recvp(procc);
	sendp(p, r);
	return nil;
}

int
usbdirread(Usbfs*f, Qid q, char *data, long cnt, vlong off, Dirgen gen, void *arg)
{
	int i, n, nd;
	char name[Namesz];
	Dir d;

	memset(&d, 0, sizeof(d));
	d.name = name;
	d.uid = d.gid = d.muid = user;
	d.atime = time(nil);
	d.mtime = epoch;
	d.length = 0;
	for(i = n = 0; gen(f, q, i, &d, arg) >= 0; i++){
		if(usbfsdebug > 1)
			fprint(2, "%s: dir %d q %#llux: %D\n", argv0, i, q.path, &d);
		nd = convD2M(&d, (uchar*)data+n, cnt-n);
		if(nd <= BIT16SZ)
			break;
		if(off > 0)
			off -= nd;
		else
			n += nd;
		d.name = name;
		d.uid = d.gid = d.muid = user;
		d.atime = time(nil);
		d.mtime = epoch;
		d.length = 0;
	}
	return n;
}

long
usbreadbuf(void *data, long count, vlong offset, void *buf, long n)
{
	if(offset >= n)
		return 0;
	if(offset + count > n)
		count = n - offset;
	memmove(data, (char*)buf + offset, count);
	return count;
}

static Rpc*
fsread(Rpc *r)
{
	Channel *p;

	if(r->fid->omode != OREAD && r->fid->omode != ORDWR)
		return fserror(r, Eperm);
	p = recvp(procc);
	sendp(p, r);
	return nil;
}

static Rpc*
fswrite(Rpc *r)
{
	Channel *p;

	if(r->fid->omode != OWRITE && r->fid->omode != ORDWR)
		return fserror(r, Eperm);
	p = recvp(procc);
	sendp(p, r);
	return nil;
}

static Rpc*
fsclunk(Rpc *r)
{
	freefid(r->fid);
	return r;
}

static Rpc*
fsno(Rpc *r)
{
	return fserror(r, Eperm);
}

static Rpc*
fsstat(Rpc *r)
{
	Dir d;
	char name[Namesz];

	memset(&d, 0, sizeof(d));
	d.name = name;
	d.uid = d.gid = d.muid = user;
	d.atime = time(nil);
	d.mtime = epoch;
	d.length = 0;
	if(fsops->stat(fsops, r->fid->qid, &d) < 0)
		return fserror(r, "%r");
	r->r.stat = (uchar*)r->data;
	r->r.nstat = convD2M(&d, (uchar*)r->data, msgsize);
	return r;
}

static Rpc*
fsflush(Rpc *r)
{
	/*
	 * Flag it as flushed and respond.
	 * Either outproc will reply to the flushed request
	 * before responding to flush, or it will never reply to it.
	 * Note that we do NOT abort the ongoing I/O.
	 * That might leave the affected endpoints in a failed
	 * state. Instead, we pretend the request is aborted.
	 *
	 * Only open, read, and write are processed
	 * by auxiliary processes and other requests wil never be
	 * flushed in practice.
	 */
	flushrpc(r->t.oldtag);
	return r;
}

Rpc* (*fscalls[])(Rpc*) = {
	[Tversion]	fsversion,
	[Tauth]		fsno,
	[Tattach]	fsattach,
	[Twalk]		fswalk,
	[Topen]		fsopen,
	[Tcreate]	fsno,
	[Tread]		fsread,
	[Twrite]	fswrite,
	[Tclunk]	fsclunk,
	[Tremove]	fsno,
	[Tstat]		fsstat,
	[Twstat]	fsno,
	[Tflush]	fsflush,
};

static void
outproc(void*)
{
	static uchar buf[Bufsize];
	Rpc *rpc;
	int nw;
	static int once = 0;

	if(once++ != 0)
		sysfatal("more than one outproc");
	for(;;){
		do
			rpc = recvp(outc);
		while(rpc == nil);		/* a delayed reply */
		if(rpc->t.tag == Dietag)
			break;
		if(rpc->flushed){
			dprint(2, "outproc: tag %d flushed\n", rpc->t.tag);
			freerpc(rpc);
			continue;
		}
		dprint(2, "-> %F\n", &rpc->r);
		nw = convS2M(&rpc->r, buf, sizeof(buf));
		if(nw == sizeof(buf))
			fprint(2, "%s: outproc: buffer is too small\n", argv0);
		if(nw <= BIT16SZ)
			fprint(2, "%s: conS2M failed\n", argv0);
		else if(write(fsfd, buf, nw) != nw){
			fprint(2, "%s: outproc: write: %r", argv0);
			/* continue and let the reader abort us */
		}
		if(usbfsdebug > 1)
			dump();
		freerpc(rpc);
	}
	dprint(2, "%s: outproc: exiting\n", argv0);
}

static void
usbfs(void*)
{
	Rpc *rpc;
	int nr;
	static int once = 0;

	if(once++ != 0)
		sysfatal("more than one usbfs proc");

	outc = chancreate(sizeof(Rpc*), 1);
	procc = chancreate(sizeof(Channel*), 0);
	endc = chancreate(sizeof(Channel*), 0);
	if(outc == nil || procc == nil || endc == nil)
		sysfatal("chancreate: %r");
	threadcreate(schedproc, nil, Stack);
	proccreate(outproc, nil, Stack);
	for(;;){
		rpc = newrpc();
		do{
			nr = read9pmsg(fsfd, rpc->data, sizeof(rpc->data));
		}while(nr == 0);
		if(nr < 0){
			dprint(2, "%s: usbfs: read: '%r'", argv0);
			if(fsops->end != nil)
				fsops->end(fsops);
			else
				closedev(fsops->dev);
			rpc->t.tag = Dietag;
			sendp(outc, rpc);
			break;
		}
		if(convM2S((uchar*)rpc->data, nr, &rpc->t) <=0){
			dprint(2, "%s: convM2S failed\n", argv0);
			freerpc(rpc);
			continue;
		}
		dprint(2, "<- %F\n", &rpc->t);
		rpc->r.tag = rpc->t.tag;
		rpc->r.type = rpc->t.type + 1;
		rpc->r.fid = rpc->t.fid;
		if(fscalls[rpc->t.type] == nil){
			sendp(outc, fserror(rpc, Ebadcall));
			continue;
		}
		if(rpc->t.fid != Nofid){
			if(rpc->t.type == Tattach)
				rpc->fid = getfid(rpc->t.fid, 1);
			else
				rpc->fid = getfid(rpc->t.fid, 0);
			if(rpc->fid == nil){
				sendp(outc, fserror(rpc, Ebadfid));
				continue;
			}
		}
		sendp(outc, fscalls[rpc->t.type](rpc));
	}
	dprint(2, "%s: ubfs: eof: exiting\n", argv0);
}

void
usbfsinit(char* srv, char *mnt, Usbfs *f, int flag)
{
	int fd[2];
	int sfd;
	int afd;
	char sfile[40];

	fsops = f;
	if(pipe(fd) < 0)
		sysfatal("pipe: %r");
	user = getuser();
	epoch = time(nil);

	fmtinstall('D', dirfmt);
	fmtinstall('M', dirmodefmt);
	fmtinstall('F', fcallfmt);
	fsfd = fd[1];
	procrfork(usbfs, nil, Stack, RFNAMEG);	/* no RFFDG */
	if(srv != nil){
		snprint(sfile, sizeof(sfile), "#s/%s", srv);
		remove(sfile);
		sfd = create(sfile, OWRITE, 0660);
		if(sfd < 0)
			sysfatal("post: %r");
		snprint(sfile, sizeof(sfile), "%d", fd[0]);
		if(write(sfd, sfile, strlen(sfile)) != strlen(sfile))
			sysfatal("post: %r");
		close(sfd);
	}
	if(mnt != nil){
		sfd = dup(fd[0], -1);	/* debug */
		afd = fauth(sfd, "");
		if(afd >= 0)
			sysfatal("authentication required??");
		if(mount(sfd, -1, mnt, flag, "") < 0)
			sysfatal("mount: %r");
	}
	close(fd[0]);
}

