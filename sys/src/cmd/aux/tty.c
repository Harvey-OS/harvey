#include <u.h>
#include <libc.h>

#include <auth.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>

/* Uses fs code from cinap's realemu */

int logfd;
	int frchld[2];
	int tochld[2];

static char Ebusy[] = "device is busy";
static char Eintr[] = "interrupted";
static char Eperm[] = "permission denied";
static char Eio[] = "i/o error";
static char Enonexist[] = "file does not exist";
static char Ebadspec[] = "bad attach specifier";
static char Ewalk[] = "walk in non directory";

static struct
{
	QLock;

	int	raw;		/* true if we shouldn't process input */
	Ref	ctl;		/* number of opens to the control file */
	int	x;		/* index into line */
	char	line[1024];	/* current input line */

	int	count;
	int	ctlpoff;
} kbd = {
};

int echo = 1;
int raw;
int ctrlp;

enum {
	Qroot,
	Qcons,
	Qconsctl,
	Nqid,
};

static struct Qtab {
	char *name;
	int mode;
	int type;
	int length;
} qtab[Nqid] = {
	"/",
		DMDIR|0555,
		QTDIR,
		0,

	"cons",
		0666,
		0,
		0,

	"consctl",
		0666,	
		0,
		0,
};

static int
fillstat(unsigned long qid, Dir *d)
{
	struct Qtab *t;

	memset(d, 0, sizeof(Dir));
	d->uid = "tty";
	d->gid = "tty";
	d->muid = "";
	d->qid = (Qid){qid, 0, 0};
	d->atime = time(0);
	t = qtab + qid;
	d->name = t->name;
	d->qid.type = t->type;
	d->mode = t->mode;
	d->length = t->length;
	return 1;
}

static void
fsattach(Req *r)
{
	char *spec;

	spec = r->ifcall.aname;
	if(spec && spec[0]){
		respond(r, Ebadspec);
		return;
	}
	r->fid->qid = (Qid){Qroot, 0, QTDIR};
	r->ofcall.qid = r->fid->qid;
	respond(r, nil);
}

static void
fsstat(Req *r)
{
	fillstat((unsigned long)r->fid->qid.path, &r->d);
	r->d.name = estrdup9p(r->d.name);
	r->d.uid = estrdup9p(r->d.uid);
	r->d.gid = estrdup9p(r->d.gid);
	r->d.muid = estrdup9p(r->d.muid);
	respond(r, nil);
}

static char*
fswalk1(Fid *fid, char *name, Qid *qid)
{
	int i;
	unsigned long path;

	path = fid->qid.path;
	switch(path){
	case Qroot:
		if (strcmp(name, "..") == 0) {
			*qid = (Qid){Qroot, 0, QTDIR};
			fid->qid = *qid;
			return nil;
		}
		for(i = fid->qid.path; i<Nqid; i++){
			if(strcmp(name, qtab[i].name) != 0)
				continue;
			*qid = (Qid){i, 0, 0};
			fid->qid = *qid;
			return nil;
		}
		return Enonexist;
		
	default:
		return Ewalk;
	}
}

static void
fsopen(Req *r)
{
	static int need[4] = { 4, 2, 6, 1 };
	struct Qtab *t;
	int n;

	t = qtab + r->fid->qid.path;
	n = need[r->ifcall.mode & 3];
	if((n & t->mode) != n)
		respond(r, Eperm);
	else
		respond(r, nil);
}

static int
readtopdir(Fid *fid, unsigned char *buf, long off, int cnt, int blen)
{
	int i, m, n;
	long pos;
	Dir d;

	n = 0;
	pos = 0;
	for (i = 1; i < Nqid; i++){
		fillstat(i, &d);
		m = convD2M(&d, &buf[n], blen-n);
		if(off <= pos){
			if(m <= BIT16SZ || m > cnt)
				break;
			n += m;
			cnt -= m;
		}
		pos += m;
	}
	return n;
}

static Channel *reqchan;

static Channel *flushchan;

static int
flushed(void *r)
{
	return nbrecvp(flushchan) == r;
}

static void
cpuproc(void *data)
{
	unsigned long path;
	long long o;
	long l, bp;
	unsigned long n;
	char *p;
	char ch;
	int send;
	Req *r;

	threadsetname("cpuproc");
	fprint(logfd, "cpuproc started\n");

	while(r = recvp(reqchan)){
		fprint(logfd, "got a request\n");
		if(flushed(r)){
			respond(r, Eintr);
			continue;
		}

		path = r->fid->qid.path;

		p = r->ifcall.data;
		n = r->ifcall.count;
		o = r->ifcall.offset;

		switch(((int)r->ifcall.type<<8)|path){
		case (Tread<<8) | Qcons:
			while (read(0, &ch, 1)) {
				send = 0;
				if (ch == 0) {
					send = 1;
				} else if (kbd.raw) {
					kbd.line[kbd.x++] = ch;
					send = 1;
				} else {
					switch (ch) {
					case '\b':
						if(kbd.x > 0)
							kbd.x--;
						break;
					case 0x15:	/* ^U */
						kbd.x = 0;
						break;
					case '\n':
					case 0x04:	/* ^D */
						send = 1;
					default:
						if(ch != 0x04)
							kbd.line[kbd.x++] = ch;
						break;
					}
				}
				if (echo)
					write(1, ch, 1);
				if (send || kbd.x == sizeof kbd.line) {
					kbd.x = 0;
					break;
				}
			}
			readbuf(r, kbd.line, kbd.x);
			respond(r, nil);
			break;
/*
		case (Tread<<8) | Qcall:
			readbuf(r, &rmu, sizeof rmu);
			respond(r, nil);
			break;
*/
		case (Twrite<<8) | Qcons:
			write(1, p, n);
			r->ofcall.count = n;
			respond(r, nil);
			break;
/*
		case (Twrite<<8) | Qcall:
			if(n != sizeof rmu){
				respond(r, Ebadureg);
				break;
			}
			memmove(&rmu, p, n);
			if(p = realmode(&cpu, &rmu, r)){
				respond(r, p);
				break;
			}
			r->ofcall.count = n;
			respond(r, nil);
			break;
*/
		}
	}
}

static void
fsflush(Req *r)
{
	nbsendp(flushchan, r->oldreq);
	respond(r, nil);
}

static void
dispatch(Req *r)
{
	if(!nbsendp(reqchan, r)) {
		fprint(logfd, "reqchan was busy\n");
		respond(r, Ebusy);
	}
}

static void
fsread(Req *r)
{
	fprint(logfd, "got read\n");
	switch((unsigned long)r->fid->qid.path){
	case Qroot:
		r->ofcall.count = readtopdir(r->fid, (void*)r->ofcall.data, r->ifcall.offset,
			r->ifcall.count, r->ifcall.count);
		respond(r, nil);
		break;
	default:
		fprint(logfd, "calling dispatch\n");
		dispatch(r);
	}
}

static void
fsend(Srv* srv)
{
	threadexitsall(nil);
}

static Srv fs = {
	.attach=		fsattach,
	.walk1=			fswalk1,
	.open=			fsopen,
	.read=			fsread,
	.write=			dispatch,
	.stat=			fsstat,
	.flush=			fsflush,
	.end=			fsend,
};

void
usage(void)
{
	fprint(2, "usage: aux/tty [-p] [-f comfile] cmd args...\n");
	exits("usage");
}

void
threadmain(int argc, char *argv[])
{
	int pid;
	int i, j;
	char *file;

	file = nil;

	ARGBEGIN{
	case 'p':
		ctrlp++;
		break;
	case 'f':
		file = EARGF(usage());
		break;
	}ARGEND

	if(file != nil){
		int fd;
		if((fd = open(file, ORDWR)) == -1)
			sysfatal("open %s", file);
		dup(fd, 0);
		dup(fd, 1);
		dup(fd, 2);
		close(fd);
	}

	logfd = open("#c/cons", ORDWR);
	fprint(logfd, "starting tty\n");

	reqchan = chancreate(sizeof(Req*), 8);
	flushchan = chancreate(sizeof(Req*), 8);
	procrfork(cpuproc, nil, 16*1024, RFNAMEG|RFNOTEG);
	threadpostmountsrv(&fs, nil, "/dev", MBEFORE);
fprint(logfd, "done threadpostmountsrv\n");
	int fd;
	switch(pid = rfork(RFPROC|RFFDG|RFNOTEG)){
	case -1:
		sysfatal("fork");
	case 0:
		if ((fd = open("/dev/cons", ORDWR)) == -1)
			sysfatal("open %s", file);
		dup(fd, 0);
		dup(logfd, 1);
		dup(logfd, 2);
		exec(argv[0], argv);
		sysfatal("exec");
	default:
		break;
	}
/*
	static char buf[512];
	static char obuf[512];
	int nfr, nto;
	int wpid;
	pipe(frchld);
	pipe(tochld);

	switch(wpid = rfork(RFPROC)){
	case -1:
		sysfatal("rfork");
	case 0:
		close(0);
		while((nfr = read(frchld[1], buf, sizeof buf)) > 0){
			int i, j;
			j = 0;
			for(i = 0; i < nfr; i++){
				if(buf[i] == '\n'){
					if(j > 0){
						write(1, buf, j);
						j = 0;
					}
					write(1, "\r\n", 2);
				} else {
					buf[j++] = buf[i];
				}
			}

			if(write(1, buf, j) != j)
				exits("write");
		}
		fprint(1, "aux/tty: got eof\n");
		postnote(PNPROC, getppid(), "interrupt");
		exits(nil);
	default:
		close(frchld[1]);
		j = 0;
		while((nto = read(0, buf, sizeof buf)) > 0){
			int oldj;
			oldj = j;
			for(i = 0; i < nto; i++){
				if(buf[i] == '\r' || buf[i] == '\n'){
					obuf[j++] = '\n';
					write(tochld[0], obuf, j);
					if(echo){
						obuf[j-1] = '\r';
						obuf[j++] = '\n';
						write(1, obuf+oldj, j-oldj);
					}
					j = 0;
				} else if(ctrlp && buf[i] == '\x10'){ // ctrl-p
					if(j > 0){
						if(echo)write(1, obuf+oldj, j-oldj);
						write(tochld[0], obuf, j);
						j = 0;
					}
					int fd;
					fd = open("/dev/reboot", OWRITE);
					if(fd != -1){
						fprint(1, "aux/tty: rebooting the system\n");
						sleep(2000);
						write(fd, "reboot", 6);
						close(fd);
					} else {
						fprint(1, "aux/tty: open /dev/reboot: %r\n");
					}
				} else if(buf[i] == '\003'){ // ctrl-c
					if(j > 0){
						if(echo)write(1, obuf+oldj, j-oldj);
						write(tochld[0], obuf, j);
						j = 0;
					}
					fprint(1, "aux/tty: sent interrupt to %d\n", pid);
					postnote(PNGROUP, pid, "interrupt");
					continue;
				} else if(buf[i] == '\x1d' ){ // ctrl-]
					if(j > 0){
						if(echo)write(1, obuf+oldj, j-oldj);
						write(tochld[0], obuf, j);
						j = 0;
					}
					fprint(1, "aux/tty: sent interrupt to %d\n", pid);
					postnote(PNGROUP, pid, "term");
					continue;
				} else if(buf[i] == '\004'){ // ctrl-d
					if(j > 0){
						if(echo)write(1, obuf+oldj, j-oldj);
						write(tochld[0], obuf, j);
						j = 0;
					}
					fprint(1, "aux/tty: sent eof to %d\n", pid);
					write(tochld[0], obuf, 0); //eof
					continue;
				} else if(buf[i] == 0x15){ // ctrl-u
					if(!raw){
						while(j > 0){
							j--;
							if(echo)write(1, "\b \b", 3); // bs
							else write(tochld[0], "\x15", 1); // bs
						}
					} else {
						obuf[j++] = buf[i];
					}
					continue;
				} else if(buf[i] == 0x7f || buf[i] == '\b'){ // backspace
					if(!raw){
						if(j > 0){
							j--;
							if(echo)write(1, "\b \b", 3); // bs
							else write(tochld[0], "\b", 1); // bs
						}
					} else {
						obuf[j++] = '\b';
					}
					continue;
				} else {
					obuf[j++] = buf[i];
				}
			}
			if(j > 0){
				if(raw){
					if(echo)write(1, obuf, j);
					write(tochld[0], obuf, j);
					j = 0;
				} else if(echo && j > oldj){
					write(1, obuf+oldj, j-oldj);
				}

			}
		}
		close(0);
		close(1);
		close(2);
		close(tochld[0]);
		postnote(PNGROUP, pid, "hangup");
		waitpid();
		waitpid();
	}
	exits(nil);
*/
}

