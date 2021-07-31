#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#define Extern	extern
#include "statfs.h"

char Ebadfid[]	= "Bad fid";
char Enotdir[]	="Not a directory";
char Edupfid[]	= "Fid already in use";
char	Eopen[]	= "Fid already opened";
char	Exmnt[]	= "Cannot .. past mount point";
char	Enoauth[]	= "iostats: Authentication failed";
char	Ebadver[]	= "Unrecognized 9P version";

int
okfile(char *s, int mode)
{
	if(strncmp(s, "/fd/", 3) == 0){
		/* 0, 1, and 2 we handle ourselves */
		if(s[4]=='/' || atoi(s+4) > 2)
			return 0;
		return 1;
	}
	if(strncmp(s, "/net/ssl", 8) == 0)
		return 0;
	if(strncmp(s, "/net/tls", 8) == 0)
		return 0;
	if(strncmp(s, "/srv/", 5) == 0 && ((mode&3) == OWRITE || (mode&3) == ORDWR))
		return 0;
	return 1;
}

void
Xversion(Fsrpc *r)
{
	Fcall thdr;
	Rpc *rpc;
	ulong t;

	t = msec();

	if(r->work.msize > IOHDRSZ+Maxfdata)
		thdr.msize = IOHDRSZ+Maxfdata;
	else
		thdr.msize = r->work.msize;
	myiounit = thdr.msize - IOHDRSZ;
	if(strncmp(r->work.version, "9P2000", 6) != 0){
		reply(&r->work, &thdr, Ebadver);
		r->busy = 0;
		return;
	}
	thdr.version = "9P2000";
	/* BUG: should clunk all fids */
	reply(&r->work, &thdr, 0);
	r->busy = 0;

	t = msec()-t;
	rpc = &stats->rpc[Tversion];
	rpc->time += t;
	if(t < rpc->loms)
		rpc->loms = t;
	if(t > rpc->hims)
		rpc->hims = t;
}

void
Xauth(Fsrpc *r)
{
	Fcall thdr;
	Rpc *rpc;
	ulong t;

	t = msec();

	reply(&r->work, &thdr, Enoauth);
	r->busy = 0;

	t = msec()-t;
	rpc = &stats->rpc[Tauth];
	rpc->time += t;
	if(t < rpc->loms)
		rpc->loms = t;
	if(t > rpc->hims)
		rpc->hims = t;
}

void
Xflush(Fsrpc *r)
{
	Fsrpc *t, *e;
	Fcall thdr;

	e = &Workq[Nr_workbufs];

	for(t = Workq; t < e; t++) {
		if(t->work.tag == r->work.oldtag) {
			DEBUG(2, "\tQ busy %d pid %d can %d\n", t->busy, t->pid, t->canint);
			if(t->busy && t->pid) {
				t->flushtag = r->work.tag;
				DEBUG(2, "\tset flushtag %d\n", r->work.tag);
				if(t->canint)
					postnote(PNPROC, t->pid, "flush");
				r->busy = 0;
				return;
			}
		}
	}

	reply(&r->work, &thdr, 0);
	DEBUG(2, "\tflush reply\n");
	r->busy = 0;
}

void
Xattach(Fsrpc *r)
{
	Fcall thdr;
	Fid *f;
	Rpc *rpc;
	ulong t;

	t = msec();

	f = newfid(r->work.fid);
	if(f == 0) {
		reply(&r->work, &thdr, Ebadfid);
		r->busy = 0;
		return;
	}

	f->f = root;
	thdr.qid = f->f->qid;
	reply(&r->work, &thdr, 0);
	r->busy = 0;

	t = msec()-t;
	rpc = &stats->rpc[Tattach];
	rpc->time += t;
	if(t < rpc->loms)
		rpc->loms = t;
	if(t > rpc->hims)
		rpc->hims = t;
}

void
Xwalk(Fsrpc *r)
{
	char errbuf[ERRMAX], *err;
	Fcall thdr;
	Fid *f, *n;
	File *nf;
	Rpc *rpc;
	ulong t;
	int i;

	t = msec();

	f = getfid(r->work.fid);
	if(f == 0) {
		reply(&r->work, &thdr, Ebadfid);
		r->busy = 0;
		return;
	}
	n = nil;
	if(r->work.newfid != r->work.fid){
		n = newfid(r->work.newfid);
		if(n == 0) {
			reply(&r->work, &thdr, Edupfid);
			r->busy = 0;
			return;
		}
		n->f = f->f;
		f = n;	/* walk new guy */
	}

	thdr.nwqid = 0;
	err = nil;
	for(i=0; i<r->work.nwname; i++){
		if(i >= MAXWELEM)
			break;
		if(strcmp(r->work.wname[i], "..") == 0) {
			if(f->f->parent == 0) {
				err = Exmnt;
				break;
			}
			f->f = f->f->parent;
			thdr.wqid[thdr.nwqid++] = f->f->qid;
			continue;
		}
	
		nf = file(f->f, r->work.wname[i]);
		if(nf == 0) {
			errstr(errbuf, sizeof errbuf);
			err = errbuf;
			break;
		}

		f->f = nf;
		thdr.wqid[thdr.nwqid++] = nf->qid;
		continue;
	}

	if(err == nil && thdr.nwqid == 0 && r->work.nwname > 0)
		err = "file does not exist";

	if(n != nil && (err != 0 || thdr.nwqid < r->work.nwname)){
		/* clunk the new fid, which is the one we walked */
		freefid(n->nr);
	}

	if(thdr.nwqid > 0)
		err = nil;
	reply(&r->work, &thdr, err);
	r->busy = 0;

	t = msec()-t;
	rpc = &stats->rpc[Twalk];
	rpc->time += t;
	if(t < rpc->loms)
		rpc->loms = t;
	if(t > rpc->hims)
		rpc->hims = t;
}

void
Xclunk(Fsrpc *r)
{
	Fcall thdr;
	Fid *f;
	Rpc *rpc;
	ulong t;
	int fid;

	t = msec();

	f = getfid(r->work.fid);
	if(f == 0) {
		reply(&r->work, &thdr, Ebadfid);
		r->busy = 0;
		return;
	}

	if(f->fid >= 0)
		close(f->fid);

	fid = r->work.fid;
	reply(&r->work, &thdr, 0);
	r->busy = 0;

	t = msec()-t;
	rpc = &stats->rpc[Tclunk];
	rpc->time += t;
	if(t < rpc->loms)
		rpc->loms = t;
	if(t > rpc->hims)
		rpc->hims = t;

	if(f->nread || f->nwrite)
		fidreport(f);

	freefid(fid);
}

void
Xstat(Fsrpc *r)
{
	char err[ERRMAX], path[128];
	uchar statbuf[STATMAX];
	Fcall thdr;
	Fid *f;
	int s;
	Rpc *rpc;
	ulong t;

	t = msec();

	f = getfid(r->work.fid);
	if(f == 0) {
		reply(&r->work, &thdr, Ebadfid);
		r->busy = 0;
		return;
	}
	makepath(path, f->f, "");
	if(!okfile(path, -1)){
		snprint(err, sizeof err, "iostats: can't simulate %s", path);
		reply(&r->work, &thdr, err);
		r->busy = 0;
		return;
	}

	if(f->fid >= 0)
		s = fstat(f->fid, statbuf, sizeof statbuf);
	else
		s = stat(path, statbuf, sizeof statbuf);

	if(s < 0) {
		errstr(err, sizeof err);
		reply(&r->work, &thdr, err);
		r->busy = 0;
		return;
	}
	thdr.stat = statbuf;
	thdr.nstat = s;
	reply(&r->work, &thdr, 0);
	r->busy = 0;

	t = msec()-t;
	rpc = &stats->rpc[Tstat];
	rpc->time += t;
	if(t < rpc->loms)
		rpc->loms = t;
	if(t > rpc->hims)
		rpc->hims = t;
}

void
Xcreate(Fsrpc *r)
{
	char err[ERRMAX], path[128];
	Fcall thdr;
	Fid *f;
	File *nf;
	Rpc *rpc;
	ulong t;

	t = msec();

	f = getfid(r->work.fid);
	if(f == 0) {
		reply(&r->work, &thdr, Ebadfid);
		r->busy = 0;
		return;
	}
	

	makepath(path, f->f, r->work.name);
	f->fid = create(path, r->work.mode, r->work.perm);
	if(f->fid < 0) {
		errstr(err, sizeof err);
		reply(&r->work, &thdr, err);
		r->busy = 0;
		return;
	}

	nf = file(f->f, r->work.name);
	if(nf == 0) {
		errstr(err, sizeof err);
		reply(&r->work, &thdr, err);
		r->busy = 0;
		return;
	}

	f->mode = r->work.mode;
	f->f = nf;
	thdr.iounit = myiounit;
	thdr.qid = f->f->qid;
	reply(&r->work, &thdr, 0);
	r->busy = 0;

	t = msec()-t;
	rpc = &stats->rpc[Tcreate];
	rpc->time += t;
	if(t < rpc->loms)
		rpc->loms = t;
	if(t > rpc->hims)
		rpc->hims = t;
}


void
Xremove(Fsrpc *r)
{
	char err[ERRMAX], path[128];
	Fcall thdr;
	Fid *f;
	ulong t;
	Rpc *rpc;

	t = msec();

	f = getfid(r->work.fid);
	if(f == 0) {
		reply(&r->work, &thdr, Ebadfid);
		r->busy = 0;
		return;
	}

	makepath(path, f->f, "");
	DEBUG(2, "\tremove: %s\n", path);
	if(remove(path) < 0) {
		errstr(err, sizeof err);
		reply(&r->work, &thdr, err);
		freefid(r->work.fid);
		r->busy = 0;
		return;
	}

	f->f->inval = 1;
	if(f->fid >= 0)
		close(f->fid);
	freefid(r->work.fid);

	reply(&r->work, &thdr, 0);
	r->busy = 0;

	t = msec()-t;
	rpc = &stats->rpc[Tremove];
	rpc->time += t;
	if(t < rpc->loms)
		rpc->loms = t;
	if(t > rpc->hims)
		rpc->hims = t;
}

void
Xwstat(Fsrpc *r)
{
	char err[ERRMAX], path[128];
	Fcall thdr;
	Fid *f;
	int s;
	ulong t;
	Rpc *rpc;

	t = msec();

	f = getfid(r->work.fid);
	if(f == 0) {
		reply(&r->work, &thdr, Ebadfid);
		r->busy = 0;
		return;
	}
	if(f->fid >= 0)
		s = fwstat(f->fid, r->work.stat, r->work.nstat);
	else {
		makepath(path, f->f, "");
		s = wstat(path, r->work.stat, r->work.nstat);
	}
	if(s < 0) {
		errstr(err, sizeof err);
		reply(&r->work, &thdr, err);
	}
	else
		reply(&r->work, &thdr, 0);

	r->busy = 0;
	t = msec()-t;
	rpc = &stats->rpc[Twstat];
	rpc->time += t;
	if(t < rpc->loms)
		rpc->loms = t;
	if(t > rpc->hims)
		rpc->hims = t;
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
				pid = rendezvous(p->pid, (ulong)f);
				if(pid != p->pid)
					fatal("rendezvous sync fail");
				return;
			}	
		}

		if(++nproc > MAXPROC)
			fatal("too many procs");

		pid = rfork(RFPROC|RFMEM);
		if(pid < 0)
			fatal("rfork");

		if(pid == 0)
			blockingslave();

		p = malloc(sizeof(Proc));
		if(p == 0)
			fatal("out of memory");

		p->busy = 0;
		p->pid = pid;
		p->next = Proclist;
		Proclist = p;

		rendezvous(pid, (ulong)p);
	}
}

void
blockingslave(void)
{
	Proc *m;
	int pid;
	Fsrpc *p;
	Fcall thdr;

	notify(flushaction);

	pid = getpid();

	m = (Proc*)rendezvous(pid, 0);
		
	for(;;) {
		p = (Fsrpc*)rendezvous(pid, pid);
		if((int)p == ~0)			/* Interrupted */
			continue;

		DEBUG(2, "\tslave: %d %F b %d p %d\n", pid, &p->work, p->busy, p->pid);
		if(p->flushtag != NOTAG)
			return;

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
			reply(&p->work, &thdr, "exportfs: slave type error");
		}
		if(p->flushtag != NOTAG) {
			p->work.type = Tflush;
			p->work.tag = p->flushtag;
			reply(&p->work, &thdr, 0);
		}
		p->busy = 0;	
		m->busy = 0;
	}
}

void
slaveopen(Fsrpc *p)
{
	char err[ERRMAX], path[128];
	Fcall *work, thdr;
	Fid *f;
	ulong t;
	Rpc *rpc;

	work = &p->work;

	t = msec();

	f = getfid(work->fid);
	if(f == 0) {
		reply(work, &thdr, Ebadfid);
		return;
	}
	if(f->fid >= 0) {
		close(f->fid);
		f->fid = -1;
	}
	
	makepath(path, f->f, "");
	DEBUG(2, "\topen: %s %d\n", path, work->mode);

	p->canint = 1;
	if(p->flushtag != NOTAG)
		return;

	if(!okfile(path, work->mode)){
		snprint(err, sizeof err, "iostats can't simulate %s", path);
		reply(work, &thdr, err);
		return;
	}

	/* There is a race here I ignore because there are no locks */
	f->fid = open(path, work->mode);
	p->canint = 0;
	if(f->fid < 0) {
		errstr(err, sizeof err);
		reply(work, &thdr, err);
		return;
	}

	DEBUG(2, "\topen: fd %d\n", f->fid);
	f->mode = work->mode;
	thdr.iounit = myiounit;
	thdr.qid = f->f->qid;
	reply(work, &thdr, 0);

	t = msec()-t;
	rpc = &stats->rpc[Topen];
	rpc->time += t;
	if(t < rpc->loms)
		rpc->loms = t;
	if(t > rpc->hims)
		rpc->hims = t;
}

void
slaveread(Fsrpc *p)
{
	char data[Maxfdata], err[ERRMAX];
	Fcall *work, thdr;
	Fid *f;
	int n, r;
	ulong t;
	Rpc *rpc;

	work = &p->work;

	t = msec();

	f = getfid(work->fid);
	if(f == 0) {
		reply(work, &thdr, Ebadfid);
		return;
	}

	n = (work->count > Maxfdata) ? Maxfdata : work->count;
	p->canint = 1;
	if(p->flushtag != NOTAG)
		return;
	/* can't just call pread, since directories must update the offset */
	if(f->f->qid.type&QTDIR){
		if(work->offset != f->offset){
			if(work->offset != 0){
				snprint(err, sizeof err, "can't seek in directory from %lld to %lld", f->offset, work->offset);
				reply(work, &thdr, err);
				return;
			}
			if(seek(f->fid, 0, 0) != 0){
				errstr(err, sizeof err);
				reply(work, &thdr, err);
				return;	
			}
			f->offset = 0;
		}
		r = read(f->fid, data, n);
		if(r > 0)
			f->offset += r;
	}else
		r = pread(f->fid, data, n, work->offset);
	p->canint = 0;
	if(r < 0) {
		errstr(err, sizeof err);
		reply(work, &thdr, err);
		return;
	}

	DEBUG(2, "\tread: fd=%d %d bytes\n", f->fid, r);

	thdr.data = data;
	thdr.count = r;
	stats->totread += r;
	f->nread++;
	f->bread += r;
	reply(work, &thdr, 0);

	t = msec()-t;
	rpc = &stats->rpc[Tread];
	rpc->time += t;
	if(t < rpc->loms)
		rpc->loms = t;
	if(t > rpc->hims)
		rpc->hims = t;
}

void
slavewrite(Fsrpc *p)
{
	char err[ERRMAX];
	Fcall *work, thdr;
	Fid *f;
	int n;
	ulong t;
	Rpc *rpc;

	work = &p->work;

	t = msec();

	f = getfid(work->fid);
	if(f == 0) {
		reply(work, &thdr, Ebadfid);
		return;
	}

	n = (work->count > Maxfdata) ? Maxfdata : work->count;
	p->canint = 1;
	if(p->flushtag != NOTAG)
		return;
	n = pwrite(f->fid, work->data, n, work->offset);
	p->canint = 0;
	if(n < 0) {
		errstr(err, sizeof err);
		reply(work, &thdr, err);
		return;
	}

	DEBUG(2, "\twrite: %d bytes fd=%d\n", n, f->fid);

	thdr.count = n;
	f->nwrite++;
	f->bwrite += n;
	stats->totwrite += n;
	reply(work, &thdr, 0);

	t = msec()-t;
	rpc = &stats->rpc[Twrite];
	rpc->time += t;
	if(t < rpc->loms)
		rpc->loms = t;
	if(t > rpc->hims)
		rpc->hims = t;
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
	if(strncmp(cause, "kill", 4) == 0)
		noted(NDFLT);

	noted(NCONT);
}
