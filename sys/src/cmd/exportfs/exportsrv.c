#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#define Extern	extern
#include "exportfs.h"

extern char *netdir, *local, *remote;

char Ebadfid[] = "bad fid";
char Enotdir[] = "not a directory";
char Edupfid[] = "fid already in use";
char Eopen[] = "fid already opened";
char Emip[] = "mount in progress";
char Enopsmt[] = "out of pseudo mount points";
char Enomem[] = "no memory";
char Eversion[] = "bad 9P2000 version";
char Ereadonly[] = "file system read only";
char Enoprocs[] = "out of processes";

ulong messagesize;
int readonly;

void
Xversion(Fsrpc *t)
{
	Fcall rhdr;

	if(t->work.msize < 256){
		reply(&t->work, &rhdr, "version: message size too small");
		putsbuf(t);
		return;
	}
	if(t->work.msize > messagesize)
		t->work.msize = messagesize;
	messagesize = t->work.msize;
	if(strncmp(t->work.version, "9P2000", 6) != 0){
		reply(&t->work, &rhdr, Eversion);
		putsbuf(t);
		return;
	}
	rhdr.version = "9P2000";
	rhdr.msize = t->work.msize;
	reply(&t->work, &rhdr, 0);
	putsbuf(t);
}

void
Xauth(Fsrpc *t)
{
	Fcall rhdr;

	reply(&t->work, &rhdr, "exportfs: authentication not required");
	putsbuf(t);
}

void
Xflush(Fsrpc *t)
{
	Fcall rhdr;
	Fsrpc *w;
	Proc *m;

	for(m = Proclist; m != nil; m = m->next){
		w = m->busy;
		if(w == nil || w->work.tag != t->work.oldtag)
			continue;

		lock(m);
		w = m->busy;
		if(w != nil && w->work.tag == t->work.oldtag) {
			w->flushtag = t->work.tag;
			DEBUG(DFD, "\tset flushtag %d\n", t->work.tag);
			postnote(PNPROC, m->pid, "flush");
			unlock(m);
			putsbuf(t);
			return;
		}
		unlock(m);
	}

	reply(&t->work, &rhdr, 0);
	DEBUG(DFD, "\tflush reply\n");
	putsbuf(t);
}

void
Xattach(Fsrpc *t)
{
	int i, nfd;
	Fcall rhdr;
	Fid *f;
	char buf[128];

	f = newfid(t->work.fid);
	if(f == nil) {
		reply(&t->work, &rhdr, Ebadfid);
		putsbuf(t);
		return;
	}

	if(srvfd >= 0){
		if(psmpt == nil)
			goto Nomount;
		for(i=0; i<Npsmpt; i++)
			if(psmap[i] == 0)
				break;
		if(i >= Npsmpt)
			goto Nomount;
		snprint(buf, sizeof(buf), "%d", i);
		f->f = file(psmpt, buf);
		if(f->f == nil)
			goto Nomount;
		snprint(buf, sizeof(buf), "/mnt/exportfs/%d", i);
		nfd = dup(srvfd, -1);
		if(amount(nfd, buf, MREPL|MCREATE, t->work.aname) < 0){
			errstr(buf, sizeof buf);
			reply(&t->work, &rhdr, buf);
			freefid(t->work.fid);
			putsbuf(t);
			close(nfd);
			return;
		}
		psmap[i] = 1;
		f->mid = i;
	}else{
		f->f = root;
		f->f->ref++;
	}

	rhdr.qid = f->f->qid;
	reply(&t->work, &rhdr, 0);
	putsbuf(t);
	return;

Nomount:
	reply(&t->work, &rhdr, Enopsmt);
	freefid(t->work.fid);
	putsbuf(t);
}

Fid*
clonefid(Fid *f, int new)
{
	Fid *n;

	n = newfid(new);
	if(n == nil) {
		n = getfid(new);
		if(n == nil)
			fatal("inconsistent fids");
		if(n->fid >= 0)
			close(n->fid);
		freefid(new);
		n = newfid(new);
		if(n == nil)
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
	if(f == nil) {
		reply(&t->work, &rhdr, Ebadfid);
		putsbuf(t);
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

		if(strcmp(t->work.wname[i], "..") == 0) {
			if(f->f->parent != nil)
				wf = f->f->parent;
			else
				wf = f->f;
			wf->ref++;
		}else{
			wf = file(f->f, t->work.wname[i]);
			if(wf == nil){
				errstr(err, sizeof err);
				e = err;
				break;
			}
		}

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
	putsbuf(t);
}

void
Xclunk(Fsrpc *t)
{
	Fcall rhdr;
	Fid *f;

	f = getfid(t->work.fid);
	if(f == nil) {
		reply(&t->work, &rhdr, Ebadfid);
		putsbuf(t);
		return;
	}

	if(f->fid >= 0)
		close(f->fid);

	freefid(t->work.fid);
	reply(&t->work, &rhdr, 0);
	putsbuf(t);
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
	if(f == nil) {
		reply(&t->work, &rhdr, Ebadfid);
		putsbuf(t);
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
		putsbuf(t);
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
	putsbuf(t);
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

	if(readonly) {
		reply(&t->work, &rhdr, Ereadonly);
		putsbuf(t);
		return;
	}
	f = getfid(t->work.fid);
	if(f == nil) {
		reply(&t->work, &rhdr, Ebadfid);
		putsbuf(t);
		return;
	}
	

	path = makepath(f->f, t->work.name);
	f->fid = create(path, t->work.mode, t->work.perm);
	free(path);
	if(f->fid < 0) {
		errstr(err, sizeof err);
		reply(&t->work, &rhdr, err);
		putsbuf(t);
		return;
	}

	nf = file(f->f, t->work.name);
	if(nf == nil) {
		errstr(err, sizeof err);
		reply(&t->work, &rhdr, err);
		putsbuf(t);
		return;
	}

	f->mode = t->work.mode;
	freefile(f->f);
	f->f = nf;
	rhdr.qid = f->f->qid;
	rhdr.iounit = getiounit(f->fid);
	reply(&t->work, &rhdr, 0);
	putsbuf(t);
}

void
Xremove(Fsrpc *t)
{
	char err[ERRMAX], *path;
	Fcall rhdr;
	Fid *f;

	if(readonly) {
		reply(&t->work, &rhdr, Ereadonly);
		putsbuf(t);
		return;
	}
	f = getfid(t->work.fid);
	if(f == nil) {
		reply(&t->work, &rhdr, Ebadfid);
		putsbuf(t);
		return;
	}

	path = makepath(f->f, "");
	DEBUG(DFD, "\tremove: %s\n", path);
	if(remove(path) < 0) {
		free(path);
		errstr(err, sizeof err);
		reply(&t->work, &rhdr, err);
		putsbuf(t);
		return;
	}
	free(path);

	f->f->inval = 1;
	if(f->fid >= 0)
		close(f->fid);
	freefid(t->work.fid);

	reply(&t->work, &rhdr, 0);
	putsbuf(t);
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

	if(readonly) {
		reply(&t->work, &rhdr, Ereadonly);
		putsbuf(t);
		return;
	}
	f = getfid(t->work.fid);
	if(f == nil) {
		reply(&t->work, &rhdr, Ebadfid);
		putsbuf(t);
		return;
	}
	strings = emallocz(t->work.nstat);	/* ample */
	if(convM2D(t->work.stat, t->work.nstat, &d, strings) <= BIT16SZ){
		rerrstr(err, sizeof err);
		reply(&t->work, &rhdr, err);
		putsbuf(t);
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
		if(strcmp(d.name, f->f->name)!=0 && d.name[0]!=0){
			free(f->f->name);
			f->f->name = estrdup(d.name);
		}
		reply(&t->work, &rhdr, 0);
	}
	free(strings);
	putsbuf(t);
}

/*
 * based on libthread's threadsetname, but drags in less library code.
 * actually just sets the arguments displayed.
 */
void
procsetname(char *fmt, ...)
{
	int fd;
	char *cmdname;
	char buf[128];
	va_list arg;

	va_start(arg, fmt);
	cmdname = vsmprint(fmt, arg);
	va_end(arg);
	if (cmdname == nil)
		return;
	snprint(buf, sizeof buf, "#p/%d/args", getpid());
	if((fd = open(buf, OWRITE)) >= 0){
		write(fd, cmdname, strlen(cmdname)+1);
		close(fd);
	}
	free(cmdname);
}

void
slave(Fsrpc *f)
{
	static int nproc;
	Proc *m, **l;
	Fcall rhdr;
	int omode;
	int pid;

	if(readonly){
		switch(f->work.type){
		case Twrite:
			reply(&f->work, &rhdr, Ereadonly);
			putsbuf(f);
			return;
		case Topen:
			omode = f->work.mode & 3;
			if(omode != OREAD && omode != OEXEC || (f->work.mode&(OTRUNC|ORCLOSE)) != 0){
				reply(&f->work, &rhdr, Ereadonly);
				putsbuf(f);
				return;
			}
		}
	}
	for(;;) {
		for(l = &Proclist; (m = *l) != nil; l = &m->next) {
			if(m->busy != nil)
				continue;

			m->busy = f;
			while(rendezvous(m, f) == (void*)~0)
				;

			/* swept a slave proc */
			if(f == nil){
				*l = m->next;
				free(m);
				nproc--;
				break;
			}
			f = nil;

			/*
			 * as long as the number of slave procs
			 * is small, dont bother sweeping.
			 */
			if(nproc < 16)
				break;
		}
		if(f == nil)
			return;

		m = emallocz(sizeof(Proc));
		pid = rfork(RFPROC|RFMEM|RFNOWAIT);
		switch(pid) {
		case -1:
			reply(&f->work, &rhdr, Enoprocs);
			putsbuf(f);
			free(m);
			return;

		case 0:
			if(local[0] != '\0'){
				if(netdir[0] != '\0')
					procsetname("%s: %s -> %s", netdir, local, remote);
				else
					procsetname("%s -> %s", local, remote);
			}
			blockingslave(m);
			_exits(0);

		default:
			m->pid = pid;
			m->next = Proclist;
			Proclist = m;
			nproc++;
		}
	}
}

void
blockingslave(Proc *m)
{
	Fsrpc *p;
	Fcall rhdr;

	notify(flushaction);

	for(;;) {
		p = rendezvous(m, nil);
		if(p == (void*)~0)	/* Interrupted */
			continue;
		if(p == nil)		/* Swept */
			break;

		DEBUG(DFD, "\tslave: %d %F\n", m->pid, &p->work);
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
flushme:
		lock(m);
		m->busy = nil;
		unlock(m);

		/* no more flushes can come in now */
		if(p->flushtag != NOTAG) {
			p->work.type = Tflush;
			p->work.tag = p->flushtag;
			reply(&p->work, &rhdr, 0);
		}
		putsbuf(p);
	}
}

int
openmount(int sfd)
{
	int p[2], fd, i, n;
	char *arg[10], fdbuf[20], mbuf[20];
	Dir *dir;

	if(pipe(p) < 0)
		return -1;

	switch(rfork(RFPROC|RFMEM|RFNOWAIT|RFNAMEG|RFFDG|RFREND)){
	case -1:
		close(p[0]);
		close(p[1]);
		return -1;

	default:
		close(sfd);
		close(p[0]);
		return p[1];

	case 0:
		break;
	}

	dup(p[0], 0);
	dup(p[0], 1);

	/* close all remaining file descriptors except sfd */
	if((fd = open("/fd", OREAD)) < 0)
		_exits("open /fd failed");
	n = dirreadall(fd, &dir);
	for(i=0; i<n; i++){
		if(strstr(dir[i].name, "ctl"))
			continue;
		fd = atoi(dir[i].name);
		if(fd > 2 && fd != sfd)
			close(fd);
	}
	free(dir);

	arg[0] = argv0; /* "/bin/exportfs" */
	snprint(fdbuf, sizeof fdbuf, "-S/fd/%d", sfd);
	arg[1] = fdbuf;
	snprint(mbuf, sizeof mbuf, "-m%lud", messagesize-IOHDRSZ);
	arg[2] = mbuf;
	arg[3] = nil;

	exec(arg[0], arg);
	_exits("whoops: exec failed");	
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
	if(f == nil) {
		reply(work, &rhdr, Ebadfid);
		return;
	}
	if(f->fid >= 0) {
		close(f->fid);
		f->fid = -1;
	}
	
	path = makepath(f->f, "");
	DEBUG(DFD, "\topen: %s %d\n", path, work->mode);
	f->fid = open(path, work->mode);
	free(path);
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
	f->offset = 0;
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
	if(f == nil) {
		reply(work, &rhdr, Ebadfid);
		return;
	}

	n = (work->count > messagesize-IOHDRSZ) ? messagesize-IOHDRSZ : work->count;
	data = malloc(n);
	if(data == nil) {
		reply(work, &rhdr, Enomem);
		return;
	}

	/* can't just call pread, since directories must update the offset */
	if(patternfile != nil && (f->f->qid.type&QTDIR))
		r = preaddir(f, (uchar*)data, n, work->offset);
	else
		r = pread(f->fid, data, n, work->offset);
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
	if(f == nil) {
		reply(work, &rhdr, Ebadfid);
		return;
	}

	n = (work->count > messagesize-IOHDRSZ) ? messagesize-IOHDRSZ : work->count;
	n = pwrite(f->fid, work->data, n, work->offset);
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
