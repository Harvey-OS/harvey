#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#include "dat.h"
#include "fns.h"

static int	openflags(int);
static void	rattach(void);
static void	rsession(void);
static void	rclone(void);
static void	rclunk(void);
static void	rcreate(void);
static void	rflush(void);
static void	rmservice(void);
static void	rnop(void);
static void	ropen(void);
static void	rread(void);
static void	rremove(void);
static void	rsession(void);
static void	rstat(void);
static void	rwalk(void);
static void	rwrite(void);
static void	rwstat(void);
static void	usage(void);

static Fcall	thdr;
static Fcall	rhdr;
static char	data[MAXMSG+MAXFDATA];
static char	fdata[MAXFDATA];
static char	srvfile[2*NAMELEN];

extern Xfsub	*xsublist[];

jmp_buf	err_lab[16];
int	nerr_lab;
char	err_msg[ERRLEN];

int	chatty;
int	nojoliet;
int	noplan9;
int norock;

void
main(int argc, char **argv)
{
	int srvfd, pipefd[2], n, nw, stdio;
	Xfsub **xs;

	stdio = 0;
	ARGBEGIN {
	case 'v':
		chatty = 1;
		break;
	case 'f':
		deffile = ARGF();
		break;
	case 's':
		stdio = 1;
		break;
	case '9':
		noplan9 = 1;
		break;
	case 'J':
		nojoliet = 1;
		break;
	case 'r':
		norock = 1;
		break;
	default:
		usage();
	} ARGEND

	switch(argc) {
	case 0:
		break;
	case 1:
		srvname = argv[0];
		break;
	default:
		usage();
	}

	iobuf_init();
	for(xs=xsublist; *xs; xs++)
		(*(*xs)->reset)();

	if(stdio) {
		pipefd[0] = 0;
		pipefd[1] = 1;
	} else {
		close(0);
		close(1);
		open("/dev/null", OREAD);
		open("/dev/null", OWRITE);
		if(pipe(pipefd) < 0)
			panic(1, "pipe");
		sprint(srvfile, "/srv/%s", srvname);
		srvfd = create(srvfile, OWRITE, 0666);
		if(srvfd < 0)
			panic(1, srvfile);
		atexit(rmservice);
		fprint(srvfd, "%d", pipefd[0]);
		close(pipefd[0]);
		close(srvfd);
		fprint(2, "%s %d: serving %s\n", argv0, getpid(), srvfile);
	}
	srvfd = pipefd[1];

	switch(rfork(RFNOWAIT|RFNOTEG|RFFDG|RFPROC)){
	case -1:
		panic(1, "fork");
	default:
		_exits(0);
	case 0:
		break;
	}

	while((n = read(srvfd, data, sizeof data)) > 0){
		if(convM2S(data, &thdr, n) <= 0)
			panic(0, "convM2S");
		if(!waserror()){
			switch(thdr.type){
			default:	panic(0, "type %d", thdr.type);
							break;
			case Tnop:	rnop();		break;
			case Tsession:	rsession();	break;
			case Tflush:	rflush();	break;
			case Tattach:	rattach();	break;
			case Tclone:	rclone();	break;
			case Twalk:	rwalk();	break;
			case Topen:	ropen();	break;
			case Tcreate:	rcreate();	break;
			case Tread:	rread();	break;
			case Twrite:	rwrite();	break;
			case Tclunk:	rclunk();	break;
			case Tremove:	rremove();	break;
			case Tstat:	rstat();	break;
			case Twstat:	rwstat();	break;
			}
			poperror();
			rhdr.type = thdr.type+1;
		}else{
			rhdr.type = Rerror;
			strncpy(rhdr.ename, err_msg, ERRLEN);
		}
		rhdr.fid = thdr.fid;
		rhdr.tag = thdr.tag;
		chat((rhdr.type != Rerror ? "OK\n" : "%s\n"), err_msg);
		if((n = convS2M(&rhdr, (char *)data)) <= 0)
			panic(0, "convS2M");
		nw = write(srvfd, data, n);
		if(nw != n)
			panic(1, "write");
		if(nerr_lab != 0)
			panic(0, "err stack %d: %lux %lux %lux %lux %lux %lux", nerr_lab,
			err_lab[0][JMPBUFPC], err_lab[1][JMPBUFPC],
			err_lab[2][JMPBUFPC], err_lab[3][JMPBUFPC],
			err_lab[4][JMPBUFPC], err_lab[5][JMPBUFPC]);
	}
	if(n < 0)
		panic(1, "read");
	chat("%s %d: exiting\n", argv0, getpid());
	exits(0);
}

static void
usage(void)
{
	fprint(2, "usage: %s [-v] [-9Jr] [-s] [-f devicefile] [srvname]\n", argv0);
	exits("usage");
}

void
error(char *p)
{
	strncpy(err_msg, p, ERRLEN);
	nexterror();
}

void
nexterror(void)
{
	longjmp(err_lab[--nerr_lab], 1);
}

void*
ealloc(long n)
{
	void *p;

	p = malloc(n);
	if(p == 0)
		error("no memory");
	return p;
}

static void
rmservice(void)
{
	remove(srvfile);
}

static void
rnop(void)
{
	chat("nop...");
}

static void
rauth(void)
{
	chat("auth...");
	error(Eauth);
}

static void
rsession(void)
{
	chat("session...");
	memset(rhdr.authid, 0, sizeof(rhdr.authid));
	memset(rhdr.authdom, 0, sizeof(rhdr.authdom));
	memset(rhdr.chal, 0, sizeof(rhdr.chal));
}

static void
rflush(void)
{
	chat("flush...");
}

static void
rattach(void)
{
	Xfile *root;
	Xfs *xf;
	Xfsub **xs;

	chat("attach(fid=%d,uname=\"%s\",aname=\"%s\",auth=\"%s\")...",
		thdr.fid, thdr.uname, thdr.aname, thdr.auth);

	if(waserror()){
		xfile(thdr.fid, Clunk);
		nexterror();
	}
	root = xfile(thdr.fid, Clean);
	root->qid = (Qid){CHDIR, 0};
	root->xf = xf = ealloc(sizeof(Xfs));
	memset(xf, 0, sizeof(Xfs));
	xf->ref = 1;
	xf->d = getxdata(thdr.aname);

	for(xs=xsublist; *xs; xs++)
		if((*(*xs)->attach)(root) >= 0){
			poperror();
			xf->s = *xs;
			xf->rootqid = root->qid;
			rhdr.qid = root->qid;
			return;
		}
	error("unknown format");
}

static void
rclone(void)
{
	Xfile *of, *nf, *next;

	chat("clone(fid=%d,newfid=%d)...", thdr.fid, thdr.newfid);
	of = xfile(thdr.fid, Asis);
	nf = xfile(thdr.newfid, Clean);
	if(waserror()){
		xfile(thdr.newfid, Clunk);
		nexterror();
	}
	next = nf->next;
	*nf = *of;
	nf->next = next;
	nf->fid = thdr.newfid;
	refxfs(nf->xf, 1);
	if(nf->len){
		nf->ptr = ealloc(nf->len);
		memmove(nf->ptr, of->ptr, nf->len);
	}else
		nf->ptr = of->ptr;
	(*of->xf->s->clone)(of, nf);
	poperror();
}

static void
rwalk(void)
{
	Xfile *f;

	chat("walk(fid=%d,name=\"%s\")...", thdr.fid, thdr.name);
	f=xfile(thdr.fid, Asis);
	if(!(f->qid.path & CHDIR)){
		chat("qid.path=0x%x...", f->qid.path);
		error("walk in non-directory");
	}
	if(strcmp(thdr.name, ".")==0)
		/* nop */;
	else if(strcmp(thdr.name, "..")==0){
		if(f->qid.path==f->xf->rootqid.path)
			error("walkup from root");
		(*f->xf->s->walkup)(f);
	}else
		(*f->xf->s->walk)(f, thdr.name);
	rhdr.qid = f->qid;
}

static void
ropen(void)
{
	Xfile *f;

	chat("open(fid=%d,mode=%d)...", thdr.fid, thdr.mode);
	f = xfile(thdr.fid, Asis);
	if(f->flags&Omodes)
		error("open on open file");
	(*f->xf->s->open)(f, thdr.mode);
	chat("f->qid=0x%8.8lux...", f->qid.path);
	f->flags = openflags(thdr.mode);
	rhdr.qid = f->qid;
}

static void
rcreate(void)
{
	Xfile *f;

	chat("create(fid=%d,name=\"%s\",perm=%uo,mode=%d)...",
		thdr.fid, thdr.name, thdr.perm, thdr.mode);
	if(strcmp(thdr.name, ".") == 0 || strcmp(thdr.name, "..") == 0)
		error("create . or ..");
	f = xfile(thdr.fid, Asis);
	if(f->flags&Omodes)
		error("create on open file");
	if(!(f->qid.path&CHDIR))
		error("create in non-directory");
	(*f->xf->s->create)(f, thdr.name, thdr.perm, thdr.mode);
	chat("f->qid=0x%8.8lux...", f->qid.path);
	f->flags = openflags(thdr.mode);
	rhdr.qid = f->qid;
}

static void
rread(void)
{
	Xfile *f;

	chat("read(fid=%d,offset=%d,count=%d)...",
		thdr.fid, thdr.offset, thdr.count);
	f=xfile(thdr.fid, Asis);
	if (!(f->flags&Oread))
		error("file not opened for reading");
	if(f->qid.path & CHDIR){
		if(thdr.count%DIRLEN || thdr.offset%DIRLEN){
			chat("count%%%d=%d,offset%%%d=%d...",
				DIRLEN, thdr.count%DIRLEN,
				DIRLEN, thdr.offset%DIRLEN);
			error("bad offset or count");
		}
		rhdr.count = (*f->xf->s->readdir)(f, fdata, thdr.offset, thdr.count);
	}else
		rhdr.count = (*f->xf->s->read)(f, fdata, thdr.offset, thdr.count);
	rhdr.data = fdata;
	chat("rcnt=%d...", rhdr.count);
}

static void
rwrite(void)
{
	Xfile *f;

	chat("write(fid=%d,offset=%d,count=%d)...",
		thdr.fid, thdr.offset, thdr.count);
	f=xfile(thdr.fid, Asis);
	if(!(f->flags&Owrite))
		error("file not opened for writing");
	rhdr.count = (*f->xf->s->write)(f, thdr.data, thdr.offset, thdr.count);
	chat("rcnt=%d...", rhdr.count);
}

static void
rclunk(void)
{
	Xfile *f;

	chat("clunk(fid=%d)...", thdr.fid);
	if(!waserror()){
		f = xfile(thdr.fid, Asis);
		if(f->flags&Orclose)
			(*f->xf->s->remove)(f);
		else
			(*f->xf->s->clunk)(f);
		poperror();
	}
	xfile(thdr.fid, Clunk);
}

static void
rremove(void)
{
	Xfile *f;

	chat("remove(fid=%d)...", thdr.fid);
	if(waserror()){
		xfile(thdr.fid, Clunk);
		nexterror();
	}
	f=xfile(thdr.fid, Asis);
	(*f->xf->s->remove)(f);
	poperror();
	xfile(thdr.fid, Clunk);
}

static void
rstat(void)
{
	Xfile *f;
	Dir dir;

	chat("stat(fid=%d)...", thdr.fid);
	f=xfile(thdr.fid, Asis);
	(*f->xf->s->stat)(f, &dir);
	if(chatty)
		showdir(2, &dir);
	convD2M(&dir, rhdr.stat);
}

static void
rwstat(void)
{
	Xfile *f;
	Dir dir;

	chat("wstat(fid=%d)...", thdr.fid);
	f=xfile(thdr.fid, Asis);
	convM2D(rhdr.stat, &dir);
	(*f->xf->s->wstat)(f, &dir);
}

static int
openflags(int mode)
{
	int flags = 0;

	switch(mode & ~(OTRUNC|OCEXEC|ORCLOSE)){
	case OREAD:
	case OEXEC:
		flags = Oread; break;
	case OWRITE:
		flags = Owrite; break;
	case ORDWR:
		flags = Oread|Owrite; break;
	}
	if(mode & ORCLOSE)
		flags |= Orclose;
	return flags;
}

void
showdir(int fd, Dir *s)
{
	char a_time[32], m_time[32];
	char *p;

	strcpy(a_time, ctime(s->atime));
	if(p=strchr(a_time, '\n'))	/* assign = */
		*p = 0;
	strcpy(m_time, ctime(s->mtime));
	if(p=strchr(m_time, '\n'))	/* assign = */
		*p = 0;
	fprint(fd, "name=\"%s\" qid=(0x%8.8lux,%lud) type=%d dev=%d \
mode=0x%8.8lux=0%luo atime=%s mtime=%s length=%lld uid=\"%s\" gid=\"%s\"...",
		s->name, s->qid.path, s->qid.vers, s->type, s->dev,
		s->mode, s->mode,
		a_time, m_time, s->length, s->uid, s->gid);
}

#define	SIZE	1024

void
chat(char *fmt, ...)
{
	va_list arg;
	char buf[SIZE], *out;

	if(chatty){
		va_start(arg, fmt);
		out = doprint(buf, buf+SIZE, fmt, arg);
		va_end(arg);
		write(2, buf, out-buf);
	}
}

void
panic(int rflag, char *fmt, ...)
{
	va_list arg;
	char buf[SIZE]; int n;

	n = sprint(buf, "%s %d: ", argv0, getpid());
	va_start(arg, fmt);
	doprint(buf+n, buf+SIZE, fmt, arg);
	va_end(arg);
	fprint(2, (rflag ? "%s: %r\n" : "%s\n"), buf);
	if(chatty){
		fprint(2, "abort\n");
		abort();
	}
	exits("panic");
}
