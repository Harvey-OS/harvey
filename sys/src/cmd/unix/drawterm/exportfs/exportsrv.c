#include <u.h>
#include <libc.h>
#include <fcall.h>
#define Extern	extern
#include "exportfs.h"

char Ebadfid[] = "Bad fid";
char Enotdir[] = "Not a directory";
char Edupfid[] = "Fid already in use";
char Eopen[] = "Fid already opened";
char Exmnt[] = "Cannot .. past mount point";
char Emip[] = "Mount in progress";
char Enopsmt[] = "Out of pseudo mount points";
char Enomem[] = "No memory";
char Eversion[] = "Bad 9P2000 version";

int iounit(int x)
{
	return 8192+IOHDRSZ;
}

void*
emallocz(ulong n)
{
	void *v;

	v = mallocz(n, 1);
	if(v == nil)
		panic("out of memory");
	return v;
}


void
Xversion(Fsrpc *t)
{
	Fcall rhdr;

	if(t->work.msize > messagesize)
		t->work.msize = messagesize;
	messagesize = t->work.msize;
	if(strncmp(t->work.version, "9P2000", 6) != 0){
		reply(&t->work, &rhdr, Eversion);
		return;
	}
	rhdr.version = "9P2000";
	rhdr.msize = t->work.msize;
	reply(&t->work, &rhdr, 0);
	t->busy = 0;
}

void
Xauth(Fsrpc *t)
{
	Fcall rhdr;

	reply(&t->work, &rhdr, "exportfs: authentication not required");
	t->busy = 0;
}

void
Xflush(Fsrpc *t)
{
	Fsrpc *w, *e;
	Fcall rhdr;

	e = &Workq[Nr_workbufs];

	for(w = Workq; w < e; w++) {
		if(w->work.tag == t->work.oldtag) {
			DEBUG(DFD, "\tQ busy %d pid %d can %d\n", w->busy, w->pid, w->canint);
			if(w->busy && w->pid) {
				w->flushtag = t->work.tag;
				DEBUG(DFD, "\tset flushtag %d\n", t->work.tag);
			//	if(w->canint)
			//		postnote(PNPROC, w->pid, "flush");
				t->busy = 0;
				return;
			}
		}
	}

	reply(&t->work, &rhdr, 0);
	DEBUG(DFD, "\tflush reply\n");
	t->busy = 0;
}

void
Xattach(Fsrpc *t)
{
	Fcall rhdr;
	Fid *f;

	f = newfid(t->work.fid);
	if(f == 0) {
		reply(&t->work, &rhdr, Ebadfid);
		t->busy = 0;
		return;
	}

	if(srvfd >= 0){
/*
		if(psmpt == 0){
		Nomount:
			reply(&t->work, &rhdr, Enopsmt);
			t->busy = 0;
			freefid(t->work.fid);
			return;
		}
		for(i=0; i<Npsmpt; i++)
			if(psmap[i] == 0)
				break;
		if(i >= Npsmpt)
			goto Nomount;
		sprint(buf, "%d", i);
		f->f = file(psmpt, buf);
		if(f->f == nil)
			goto Nomount;
		sprint(buf, "/mnt/exportfs/%d", i);
		nfd = dup(srvfd, -1);
		if(amount(nfd, buf, MREPL|MCREATE, t->work.aname) < 0){
			errstr(buf, sizeof buf);
			reply(&t->work, &rhdr, buf);
			t->busy = 0;
			freefid(t->work.fid);
			close(nfd);
			return;
		}
		psmap[i] = 1;
		f->mid = i;
*/
	}else{
		f->f = root;
		f->f->ref++;
	}

	rhdr.qid = f->f->qid;
	reply(&t->work, &rhdr, 0);
	t->busy = 0;
}

Fid*
clonefid(Fid *f, int new)
{
	Fid *n;

	n = newfid(new);
	if(n == 0) {
		n = getfid(new);
		if(n == 0)
			fatal("inconsistent fids");
		if(n->fid >= 0)
			close(n->fid);
		freefid(new);
		n = newfid(new);
		if(n == 0)
			fatal("inconsistent fids2");
	}
	n->f = f->f;
	n->f->ref++;
	return n;
}

void
Xwalk(Fsrpc *t)
{
	char err[ERRMAX], *e;
	Fcall rhdr;
	Fid *f, *nf;
	File *wf;
	int i;

	f = getfid(t->work.fid);
	if(f == 0) {
		reply(&t->work, &rhdr, Ebadfid);
		t->busy = 0;
		return;
	}

	nf = nil;
	if(t->work.newfid != t->work.fid){
		nf = clonefid(f, t->work.newfid);
		f = nf;
	}

	rhdr.nwqid = 0;
	e = nil;
	for(i=0; i<t->work.nwname; i++){
		if(i == MAXWELEM){
			e = "Too many path elements";
			break;
		}

		if(strcmp(t->work.wname[i], "..") == 0) {
			if(f->f->parent == nil) {
				e = Exmnt;
				break;
			}
			wf = f->f->parent;
			wf->ref++;
			goto Accept;
		}
	
		wf = file(f->f, t->work.wname[i]);
		if(wf == 0){
			errstr(err, sizeof err);
			e = err;
			break;
		}
    Accept:
		freefile(f->f);
		rhdr.wqid[rhdr.nwqid++] = wf->qid;
		f->f = wf;
		continue;
	}

	if(nf!=nil && (e!=nil || rhdr.nwqid!=t->work.nwname))
		freefid(t->work.newfid);
	if(rhdr.nwqid > 0)
		e = nil;
	reply(&t->work, &rhdr, e);
	t->busy = 0;
}

void
Xclunk(Fsrpc *t)
{
	Fcall rhdr;
	Fid *f;

	f = getfid(t->work.fid);
	if(f == 0) {
		reply(&t->work, &rhdr, Ebadfid);
		t->busy = 0;
		return;
	}

	if(f->fid >= 0)
		close(f->fid);

	freefid(t->work.fid);
	reply(&t->work, &rhdr, 0);
	t->busy = 0;
}

void
Xstat(Fsrpc *t)
{
	char err[ERRMAX], *path;
	Fcall rhdr;
	Fid *f;
	Dir *d;
	int s;
	uchar *statbuf;

	f = getfid(t->work.fid);
	if(f == 0) {
		reply(&t->work, &rhdr, Ebadfid);
		t->busy = 0;
		return;
	}
	if(f->fid >= 0)
		d = dirfstat(f->fid);
	else {
		path = makepath(f->f, "");
		d = dirstat(path);
		free(path);
	}

	if(d == nil) {
		errstr(err, sizeof err);
		reply(&t->work, &rhdr, err);
		t->busy = 0;
		return;
	}

	d->qid.path = f->f->qidt->uniqpath;
	s = sizeD2M(d);
	statbuf = emallocz(s);
	s = convD2M(d, statbuf, s);
	free(d);
	rhdr.nstat = s;
	rhdr.stat = statbuf;
	reply(&t->work, &rhdr, 0);
	free(statbuf);
	t->busy = 0;
}

static int
getiounit(int fd)
{
	int n;

	n = iounit(fd);
	if(n > messagesize-IOHDRSZ)
		n = messagesize-IOHDRSZ;
	return n;
}

void
Xcreate(Fsrpc *t)
{
	char err[ERRMAX], *path;
	Fcall rhdr;
	Fid *f;
	File *nf;

	f = getfid(t->work.fid);
	if(f == 0) {
		reply(&t->work, &rhdr, Ebadfid);
		t->busy = 0;
		return;
	}
	

	path = makepath(f->f, t->work.name);
	f->fid = create(path, t->work.mode, t->work.perm);
	free(path);
	if(f->fid < 0) {
		errstr(err, sizeof err);
		reply(&t->work, &rhdr, err);
		t->busy = 0;
		return;
	}

	nf = file(f->f, t->work.name);
	if(nf == 0) {
		errstr(err, sizeof err);
		reply(&t->work, &rhdr, err);
		t->busy = 0;
		return;
	}

	f->mode = t->work.mode;
	freefile(f->f);
	f->f = nf;
	rhdr.qid = f->f->qid;
	rhdr.iounit = getiounit(f->fid);
	reply(&t->work, &rhdr, 0);
	t->busy = 0;
}

void
Xremove(Fsrpc *t)
{
	char err[ERRMAX], *path;
	Fcall rhdr;
	Fid *f;

	f = getfid(t->work.fid);
	if(f == 0) {
		reply(&t->work, &rhdr, Ebadfid);
		t->busy = 0;
		return;
	}

	path = makepath(f->f, "");
	DEBUG(DFD, "\tremove: %s\n", path);
	if(remove(path) < 0) {
		free(path);
		errstr(err, sizeof err);
		reply(&t->work, &rhdr, err);
		t->busy = 0;
		return;
	}
	free(path);

	f->f->inval = 1;
	if(f->fid >= 0)
		close(f->fid);
	freefid(t->work.fid);

	reply(&t->work, &rhdr, 0);
	t->busy = 0;
}

void
Xwstat(Fsrpc *t)
{
	char err[ERRMAX], *path;
	Fcall rhdr;
	Fid *f;
	int s;
	char *strings;
	Dir d;

	f = getfid(t->work.fid);
	if(f == 0) {
		reply(&t->work, &rhdr, Ebadfid);
		t->busy = 0;
		return;
	}
	strings = emallocz(t->work.nstat);	/* ample */
	if(convM2D(t->work.stat, t->work.nstat, &d, strings) <= BIT16SZ){
		rerrstr(err, sizeof err);
		reply(&t->work, &rhdr, err);
		t->busy = 0;
		free(strings);
		return;
	}

	if(f->fid >= 0)
		s = dirfwstat(f->fid, &d);
	else {
		path = makepath(f->f, "");
		s = dirwstat(path, &d);
		free(path);
	}
	if(s < 0) {
		rerrstr(err, sizeof err);
		reply(&t->work, &rhdr, err);
	}
	else {
		/* wstat may really be rename */
		if(strcmp(d.name, f->f->name)!=0 && strcmp(d.name, "")!=0){
			free(f->f->name);
			f->f->name = estrdup(d.name);
		}
		reply(&t->work, &rhdr, 0);
	}
	free(strings);
	t->busy = 0;
}

void
slave(Fsrpc *f)
{
	Proc *p;
	int pid;
	static int nproc;

	for(;;) {
		for(p = Proclist; p; p = p->next) {
			if(p->busy == 0) {
				f->pid = p->pid;
				p->busy = 1;
				pid = (uintptr)rendezvous((void*)(uintptr)p->pid, f);
				if(pid != p->pid)
					fatal("rendezvous sync fail");
				return;
			}	
		}

		if(++nproc > MAXPROC)
			fatal("too many procs");

		pid = kproc("slave", blockingslave, nil);
		DEBUG(DFD, "slave pid %d\n", pid);
		if(pid == -1)
			fatal("kproc");

		p = malloc(sizeof(Proc));
		if(p == 0)
			fatal("out of memory");

		p->busy = 0;
		p->pid = pid;
		p->next = Proclist;
		Proclist = p;

DEBUG(DFD, "parent %d rendez\n", pid);
		rendezvous((void*)(uintptr)pid, p);
DEBUG(DFD, "parent %d went\n", pid);
	}
}

void
blockingslave(void *x)
{
	Fsrpc *p;
	Fcall rhdr;
	Proc *m;
	int pid;

	USED(x);

	notify(flushaction);

	pid = getpid();

DEBUG(DFD, "blockingslave %d rendez\n", pid);
	m = (Proc*)rendezvous((void*)(uintptr)pid, 0);
DEBUG(DFD, "blockingslave %d rendez got %p\n", pid, m);
	
	for(;;) {
		p = rendezvous((void*)(uintptr)pid, (void*)(uintptr)pid);
		if((uintptr)p == ~(uintptr)0)			/* Interrupted */
			continue;

		DEBUG(DFD, "\tslave: %d %F b %d p %d\n", pid, &p->work, p->busy, p->pid);
		if(p->flushtag != NOTAG)
			goto flushme;

		switch(p->work.type) {
		case Tread:
			slaveread(p);
			break;

		case Twrite:
			slavewrite(p);
			break;

		case Topen:
			slaveopen(p);
			break;

		default:
			reply(&p->work, &rhdr, "exportfs: slave type error");
		}
		if(p->flushtag != NOTAG) {
flushme:
			p->work.type = Tflush;
			p->work.tag = p->flushtag;
			reply(&p->work, &rhdr, 0);
		}
		p->busy = 0;
		m->busy = 0;
	}
}

int
openmount(int sfd)
{
	werrstr("openmount not implemented");
	return -1;
}

void
slaveopen(Fsrpc *p)
{
	char err[ERRMAX], *path;
	Fcall *work, rhdr;
	Fid *f;
	Dir *d;

	work = &p->work;

	f = getfid(work->fid);
	if(f == 0) {
		reply(work, &rhdr, Ebadfid);
		return;
	}
	if(f->fid >= 0) {
		close(f->fid);
		f->fid = -1;
	}
	
	path = makepath(f->f, "");
	DEBUG(DFD, "\topen: %s %d\n", path, work->mode);

	p->canint = 1;
	if(p->flushtag != NOTAG){
		free(path);
		return;
	}
	/* There is a race here I ignore because there are no locks */
	f->fid = open(path, work->mode);
	free(path);
	p->canint = 0;
	if(f->fid < 0 || (d = dirfstat(f->fid)) == nil) {
	Error:
		errstr(err, sizeof err);
		reply(work, &rhdr, err);
		return;
	}
	f->f->qid = d->qid;
	free(d);
	if(f->f->qid.type & QTMOUNT){	/* fork new exportfs for this */
		f->fid = openmount(f->fid);
		if(f->fid < 0)
			goto Error;
	}

	DEBUG(DFD, "\topen: fd %d\n", f->fid);
	f->mode = work->mode;
	rhdr.iounit = getiounit(f->fid);
	rhdr.qid = f->f->qid;
	reply(work, &rhdr, 0);
}

void
slaveread(Fsrpc *p)
{
	Fid *f;
	int n, r;
	Fcall *work, rhdr;
	char *data, err[ERRMAX];

	work = &p->work;

	f = getfid(work->fid);
	if(f == 0) {
		reply(work, &rhdr, Ebadfid);
		return;
	}

	n = (work->count > messagesize-IOHDRSZ) ? messagesize-IOHDRSZ : work->count;
	p->canint = 1;
	if(p->flushtag != NOTAG)
		return;
	data = malloc(n);
	if(data == nil)
		fatal(Enomem);

	/* can't just call pread, since directories must update the offset */
	r = pread(f->fid, data, n, work->offset);
	p->canint = 0;
	if(r < 0) {
		free(data);
		errstr(err, sizeof err);
		reply(work, &rhdr, err);
		return;
	}

	DEBUG(DFD, "\tread: fd=%d %d bytes\n", f->fid, r);

	rhdr.data = data;
	rhdr.count = r;
	reply(work, &rhdr, 0);
	free(data);
}

void
slavewrite(Fsrpc *p)
{
	char err[ERRMAX];
	Fcall *work, rhdr;
	Fid *f;
	int n;

	work = &p->work;

	f = getfid(work->fid);
	if(f == 0) {
		reply(work, &rhdr, Ebadfid);
		return;
	}

	n = (work->count > messagesize-IOHDRSZ) ? messagesize-IOHDRSZ : work->count;
	p->canint = 1;
	if(p->flushtag != NOTAG)
		return;
	n = pwrite(f->fid, work->data, n, work->offset);
	p->canint = 0;
	if(n < 0) {
		errstr(err, sizeof err);
		reply(work, &rhdr, err);
		return;
	}

	DEBUG(DFD, "\twrite: %d bytes fd=%d\n", n, f->fid);

	rhdr.count = n;
	reply(work, &rhdr, 0);
}

void
reopen(Fid *f)
{
	USED(f);
	fatal("reopen");
}

void
flushaction(void *a, char *cause)
{
	USED(a);
	if(strncmp(cause, "sys:", 4) == 0 && !strstr(cause, "pipe")) {
		fprint(2, "exportsrv: note: %s\n", cause);
		exits("noted");
	}
	if(strncmp(cause, "kill", 4) == 0)
		noted(NDFLT);

	noted(NCONT);
}
