#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#define Extern	extern
#include "exportfs.h"

char *e[] =
{
	[Ebadfid]	"Bad fid",
	[Enotdir]	"Not a directory",
	[Edupfid]	"Fid already in use",
	[Eopen]		"Fid already opened",
	[Exmnt]		"Cannot .. past mount point",
	[Enoauth]	"Authentication failed",
	[Emip]		"Mount in progress",
	[Enopsmt]	"Out of pseudo mount points",
};

void
Xnop(Fsrpc *r)
{
	Fcall thdr;

	reply(&r->work, &thdr, 0);
	r->busy = 0;
}

void
Xsession(Fsrpc *r)
{
	Fcall thdr;

	memset(thdr.authid, 0, sizeof(thdr.authid));
	memset(thdr.authdom, 0, sizeof(thdr.authdom));
	memset(thdr.chal, 0, sizeof(thdr.chal));
	reply(&r->work, &thdr, 0);
	r->busy = 0;
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

	f = newfid(r->work.fid);
	if(f == 0) {
		reply(&r->work, &thdr, e[Ebadfid]);
		r->busy = 0;
		return;
	}

	f->f = root;
	thdr.qid = f->f->qid;
	reply(&r->work, &thdr, 0);
	r->busy = 0;
}

void
Xclone(Fsrpc *r)
{
	Fcall thdr;
	Fid *f, *n;

	f = getfid(r->work.fid);
	if(f == 0) {
		reply(&r->work, &thdr, e[Ebadfid]);
		r->busy = 0;
		return;
	}
	n = newfid(r->work.newfid);
	if(n == 0) {
		n = getfid(r->work.newfid);
		if(n == 0)
			fatal("inconsistent fids");
		if(n->fid >= 0)
			close(n->fid);
		freefid(r->work.newfid);
		n = newfid(r->work.newfid);
		if(n == 0)
			fatal("inconsistent fids2");
	}
	n->f = f->f;
	reply(&r->work, &thdr, 0);
	r->busy = 0;
}

void
Xwalk(Fsrpc *r)
{
	char err[ERRLEN];
	Fcall thdr;
	Fid *f;
	File *nf;

	f = getfid(r->work.fid);
	if(f == 0) {
		reply(&r->work, &thdr, e[Ebadfid]);
		r->busy = 0;
		return;
	}

	if(strcmp(r->work.name, "..") == 0) {
		if(f->f->parent == 0) {
			reply(&r->work, &thdr, e[Exmnt]);
			r->busy = 0;
			return;
		}
		f->f = f->f->parent;
		thdr.qid = f->f->qid;
		reply(&r->work, &thdr, 0);
		r->busy = 0;
		return;
	}

	nf = file(f->f, r->work.name);
	if(nf == 0) {
		errstr(err);
		reply(&r->work, &thdr, err);
		r->busy = 0;
		return;
	}
		
	f->f = nf;
	thdr.qid = nf->qid;
	reply(&r->work, &thdr, 0);
	r->busy = 0;
}

void
Xclunk(Fsrpc *r)
{
	Fcall thdr;
	Fid *f;

	f = getfid(r->work.fid);
	if(f == 0) {
		reply(&r->work, &thdr, e[Ebadfid]);
		r->busy = 0;
		return;
	}

	if(f->fid >= 0)
		close(f->fid);

	freefid(r->work.fid);
	reply(&r->work, &thdr, 0);
	r->busy = 0;
}

void
Xstat(Fsrpc *r)
{
	char err[ERRLEN], path[128];
	Fcall thdr;
	Fid *f;
	int s;

	f = getfid(r->work.fid);
	if(f == 0) {
		reply(&r->work, &thdr, e[Ebadfid]);
		r->busy = 0;
		return;
	}
	if(f->fid >= 0)
		s = fstat(f->fid, thdr.stat);
	else {
		makepath(path, f->f, "");
		s = stat(path, thdr.stat);
	}

	if(s < 0) {
		errstr(err);
		reply(&r->work, &thdr, err);
		r->busy = 0;
		return;
	}
	reply(&r->work, &thdr, 0);
	r->busy = 0;
}

void
Xcreate(Fsrpc *r)
{
	char err[ERRLEN], path[128];
	Fcall thdr;
	Fid *f;
	File *nf;

	f = getfid(r->work.fid);
	if(f == 0) {
		reply(&r->work, &thdr, e[Ebadfid]);
		r->busy = 0;
		return;
	}
	

	makepath(path, f->f, r->work.name);
	f->fid = create(path, r->work.mode, r->work.perm);
	if(f->fid < 0) {
		errstr(err);
		reply(&r->work, &thdr, err);
		r->busy = 0;
		return;
	}

	nf = file(f->f, r->work.name);
	if(nf == 0) {
		errstr(err);
		reply(&r->work, &thdr, err);
		r->busy = 0;
		return;
	}

	f->mode = r->work.mode;
	f->f = nf;
	f->offset = 0;
	thdr.qid = f->f->qid;
	reply(&r->work, &thdr, 0);
	r->busy = 0;
}


void
Xremove(Fsrpc *r)
{
	char err[ERRLEN], path[128];
	Fcall thdr;
	Fid *f;

	f = getfid(r->work.fid);
	if(f == 0) {
		reply(&r->work, &thdr, e[Ebadfid]);
		r->busy = 0;
		return;
	}

	makepath(path, f->f, "");
	DEBUG(2, "\tremove: %s\n", path);
	if(remove(path) < 0) {
		errstr(err);
		reply(&r->work, &thdr, err);
		r->busy = 0;
		return;
	}

	if(f->fid >= 0)
		close(f->fid);
	freefid(r->work.fid);

	reply(&r->work, &thdr, 0);
	r->busy = 0;
}

void
Xwstat(Fsrpc *r)
{
	char err[ERRLEN], path[128];
	Fcall thdr;
	Fid *f;
	int s;

	f = getfid(r->work.fid);
	if(f == 0) {
		reply(&r->work, &thdr, e[Ebadfid]);
		r->busy = 0;
		return;
	}
	if(f->fid >= 0)
		s = fwstat(f->fid, r->work.stat);
	else {
		makepath(path, f->f, "");
		s = wstat(path, r->work.stat);
	}
	if(s < 0) {
		errstr(err);
		reply(&r->work, &thdr, err);
	}
	else {
		/* wstat may really be rename */
		strncpy(f->f->name, r->work.stat, NAMELEN);
		reply(&r->work, &thdr, 0);
	}

	r->busy = 0;
}
 
void
Xclwalk(Fsrpc *r)
{
	Fcall thdr;
	Fid *f, *n;

	f = getfid(r->work.fid);
	if(f == 0) {
		reply(&r->work, &thdr, e[Ebadfid]);
		r->busy = 0;
		return;
	}
	n = newfid(r->work.newfid);
	if(n == 0) {
		reply(&r->work, &thdr, e[Edupfid]);
		r->busy = 0;
		return;
	}
	n->f = f->f;
	r->work.fid = r->work.newfid;
	Xwalk(r);
}

int
wrmount(Fsrpc *p)
{
	Dir d;
	Fid *f;
	Fcall thdr, *work;

	work = &p->work;

	f = getfid(work->fid);
	if(f == 0)
		return 0;
	if(f->fid < 0)
		return 0;
	if(dirfstat(f->fid, &d) < 0)
		return 0;
	if((d.mode&CHMOUNT) == 0)
		return 0;

	/* This may need to be a list matched by tag */
	if(f->mpend) {
		reply(work, &thdr, e[Emip]);
		p->busy = 0;
		return 1;
	}

	f->mpend = p;

	thdr.count = work->count;
	reply(work, &thdr, 0);
	return 1;
}

void
slave(Fsrpc *f)
{
	Proc *p;
	int pid, n;
	Fcall mcall;
	static int nproc;

	/*
	 * Look for a write to a message channel from the mount
	 * driver and attempt to multiplex to a local mount
	 */
	if(f->work.type == Twrite && f->work.data[0] == Tattach) {
		n = convM2S(f->work.data, &mcall, f->work.count);
		if(n != 0 && wrmount(f))
			return;
	}

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
		switch(pid) {
		case -1:
			fatal("rfork");

		case 0:
			blockingslave();
			fatal("slave");

		default:
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
}

void
blockingslave(void)
{
	Fsrpc *p;
	Fcall thdr;
	Proc *m;
	int pid;

	notify(flushaction);

	pid = getpid();

	m = (Proc*)rendezvous(pid, 0);
	
	for(;;) {
		p = (Fsrpc*)rendezvous(pid, pid);
		if((int)p == ~0)			/* Interrupted */
			continue;

		DEBUG(2, "\tslave: %d %F b %d p %d\n", pid, &p->work, p->busy, p->pid);
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
			reply(&p->work, &thdr, "exportfs: slave type error");
		}
		if(p->flushtag != NOTAG) {
flushme:
			p->work.type = Tflush;
			p->work.tag = p->flushtag;
			reply(&p->work, &thdr, 0);
		}
		p->busy = 0;
		m->busy = 0;
	}
}

File*
mkmpt(char *buf, Fid *f)
{
	int i;
	File *fl;
	char nr[10];

	if(psmpt == 0)
		return 0;

	for(i = 1; i < Npsmpt; i++)
		if(psmap[i] == 0)
			break;

	if(i >= Npsmpt-1)
		return 0;

	sprint(nr, "%d", i);
	fl = file(psmpt, nr);
	if(fl == 0)
		return 0;

	sprint(buf, "/mnt/exportfs/%d", i);
	psmap[i] = 1;
	f->mid = i;

	return fl;
}

void
rdmount(Fid *f, Fsrpc *p)
{
	File *nf;
	int n, fd;
	Fid *mfid;
	Fsrpc *mp;
	char mpath[256];
	Fcall thdr, *work, mcall;

	work = &p->work;

	mp = f->mpend;
	convM2S(mp->work.data, &mcall, mp->work.count);

	mfid = newfid(mcall.fid);
	if(mfid == 0) {
		mcall.type = Rerror;
		strcpy(mcall.ename, e[Ebadfid]);
		goto repl;
	}
	
	nf = mkmpt(mpath, mfid);
	if(nf == 0) {
		mcall.type = Rerror;
		strcpy(mcall.ename, e[Enopsmt]);
		goto repl;
	}

	fd = dup(f->fid, -1);
	p->canint = 1;
	n = amount(fd, mpath, MREPL, mcall.aname);
	p->canint = 0;
	if(n < 0) {
		close(fd);
		freefid(mcall.fid);
		mcall.type = Rerror;
		mcall.ename[0] = 0;
		errstr(mcall.ename);
	}
	else {
		mfid->f = nf;
		mcall.type = Rattach;
		mcall.qid = nf->qid;
		mcall.qid.path &= ~CHDIR;
	}
repl:
	thdr.count = convS2M(&mcall, mp->buf);
	thdr.data = mp->buf;
	reply(work, &thdr, 0);
	mp->busy = 0;
	f->mpend = 0;
}

void
slaveopen(Fsrpc *p)
{
	char err[ERRLEN], path[128];
	Fcall *work, thdr;
	Fid *f;

	work = &p->work;

	f = getfid(work->fid);
	if(f == 0) {
		reply(work, &thdr, e[Ebadfid]);
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
	/* There is a race here I ignore because there are no locks */
	f->fid = open(path, work->mode);
	p->canint = 0;
	if(f->fid < 0) {
		errstr(err);
		reply(work, &thdr, err);
		return;
	}

	DEBUG(2, "\topen: fd %d\n", f->fid);
	f->mode = work->mode;
	f->offset = 0;
	thdr.qid = f->f->qid;
	reply(work, &thdr, 0);
}

void
slaveread(Fsrpc *p)
{
	Fid *f;
	int n, r;
	Fcall *work, thdr;
	char data[MAXFDATA], err[ERRLEN];

	work = &p->work;

	f = getfid(work->fid);
	if(f == 0) {
		reply(work, &thdr, e[Ebadfid]);
		return;
	}

	/* Do the work half of a split transaction mount */
	if(f->mpend) {
		rdmount(f, p);
		return;
	}

	if(work->offset != f->offset)
		fileseek(f, work->offset);

	n = (work->count > MAXFDATA) ? MAXFDATA : work->count;
	p->canint = 1;
	if(p->flushtag != NOTAG)
		return;
	r = read(f->fid, data, n);
	p->canint = 0;
	if(r < 0) {
		errstr(err);
		reply(work, &thdr, err);
		return;
	}

	DEBUG(2, "\tread: fd=%d %d bytes\n", f->fid, r);

	f->offset += r;
	thdr.data = data;
	thdr.count = r;
	reply(work, &thdr, 0);
}

void
slavewrite(Fsrpc *p)
{
	char err[ERRLEN];
	Fcall *work, thdr;
	Fid *f;
	int n;

	work = &p->work;

	f = getfid(work->fid);
	if(f == 0) {
		reply(work, &thdr, e[Ebadfid]);
		return;
	}

	if(work->offset != f->offset)
		fileseek(f, work->offset);

	n = (work->count > MAXFDATA) ? MAXFDATA : work->count;
	p->canint = 1;
	if(p->flushtag != NOTAG)
		return;
	n = write(f->fid, work->data, n);
	p->canint = 0;
	if(n < 0) {
		errstr(err);
		reply(work, &thdr, err);
		return;
	}

	DEBUG(2, "\twrite: %d bytes fd=%d\n", n, f->fid);

	f->offset += n;
	thdr.count = n;
	reply(work, &thdr, 0);
}

void
fileseek(Fid *f, ulong offset)
{
	char chunk[DIRCHUNK];
	int n, nbytes, r;

	if(f->f->qid.path&CHDIR) {
		if(offset < f->offset)
			reopen(f);

		for(nbytes = offset - f->offset; nbytes; nbytes -= r) {
			n = (nbytes > DIRCHUNK) ? DIRCHUNK : nbytes;
			r = read(f->fid, chunk, n);
			if(r <= 0) {
				DEBUG(2,"\tdir seek error\n");
				return;
			}
			f->offset += r;
		}
	}
	else
		f->offset = seek(f->fid, offset, 0);
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
