/*
 * Remote debugging file system
 */

#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#include <bio.h>

/*
 * Like the library routine, but does postnote to the group.
 */
void
sysfatal(char *fmt, ...)
{
	char buf[1024];
	va_list arg;

	va_start(arg, fmt);
	doprint(buf, buf+sizeof(buf), fmt, arg);
	va_end(arg);
	fprint(2, "%s: %s\n", argv0, buf);
	postnote(PNGROUP, getpid(), "die yankee pig dog");
	exits(buf);
}

int dbg = 0;
#define DBG	if(dbg)fprint

enum {
	NHASH = 4096,
	Readlen = 4,
	Pagequantum = 1024,
};

/* caching memory pages: a lot of space to avoid serial communications */
Lock pglock;
typedef struct	Page	Page;
struct Page {	/* cached memory contents */
	Page *link;
	ulong len;
	ulong addr;
	uchar val[Readlen];
};

Page *pgtab[NHASH];

Page *freelist;

/* called with pglock locked */
Page*
newpg(void)
{
	int i;
	Page *p, *q;

	if(freelist == nil){
		p = malloc(sizeof(Page)*Pagequantum);
		if(p == nil)
			sysfatal("out of memory");

		for(i=0, q=p; i<Pagequantum-1; i++, q++)
			q->link = q+1;
		q->link = nil;

		freelist = p;
	}
	p = freelist;
	freelist = freelist->link;
	return p;
}

#define PHIINV 0.61803398874989484820
uint
ahash(ulong addr)
{
	return (uint)floor(NHASH*fmod(addr*PHIINV, 1.0));
}

int
lookup(ulong addr, uchar *val)
{
	Page *p;

	lock(&pglock);
	for(p=pgtab[ahash(addr)]; p; p=p->link){
		if(p->addr == addr){
			memmove(val, p->val, Readlen);
			unlock(&pglock);
			return 1;
		}
	}
	unlock(&pglock);
	return 0;
}

void
insert(ulong addr, uchar *val)
{
	Page *p;
	uint h;

	lock(&pglock);
	p = newpg();
	p->addr = addr;
	memmove(p->val, val, Readlen);
	h = ahash(addr);
	p->link = pgtab[h];
	p->len = pgtab[h] ? pgtab[h]->len+1 : 1;
	pgtab[h] = p;
	unlock(&pglock);
}

void
flushcache(void)
{
	int i;
	Page *p;

	lock(&pglock);
	for(i=0; i<NHASH; i++){
		if(p=pgtab[i]){
			for(;p->link; p=p->link)
				;
			p->link = freelist;
			freelist = p;
		}
		pgtab[i] = nil;
	}
	unlock(&pglock);
}

enum
{
	NFHASH	= 32,
	MSGSIZE	= 10,
	NPROCS = 2,	/* number of slave processes */

	/*
	 * Qid 0 is root
	 * Subdirectory named "procname" ("1" is default)
	 * Files: mem, proc, and text
	 */
	Qroot	= CHDIR,
	Q1	= CHDIR|0x01,
	Qctl	= 1,
	Qfpregs,
	Qkregs,
	Qmem,
	Qproc,
	Qregs,
	Qtext,
	Qstatus,
	NFILES,

	STACKSIZE = 4096,
};

typedef struct Fid	Fid;
struct Fid
{
	int	fid;
	int	open;
	Fid	*hash;
	Qid	qid;
};

/*
 *	we only serve files needed by the debuggers
 */
char*	fnames[] =
{
	[Qctl]		"ctl",
	[Qfpregs]	"fpregs",
	[Qkregs]	"kregs",
	[Qmem]		"mem",
	[Qproc]		"proc",
	[Qregs]		"regs",
	[Qtext]		"text",
	[Qstatus]	"status",
};

int	textfd;
int	srvfd;
int	rfd;
Biobuf rfb;
int	pids[NPROCS];
int	nopen;
Fid*	hash[NFHASH];
char	dirbuf[DIRLEN*NFILES];
char 	iob[MSGSIZE];
char*	portname = "/dev/eia0";
char*	textfile = "/386/9pc";
char*	procname = "1";
char	dbuf[MAXFDATA];

char*	Eexist = "file does not exist";

void	mflush(Fcall*);
void	msession(Fcall*);
void	mnop(Fcall*);
void	mattach(Fcall*);
void	mclone(Fcall*);
void	mwalk(Fcall*);
void	mopen(Fcall*);
void	mcreate(Fcall*);
void	mread(Fcall*);
void	mwrite(Fcall*);
void	mclunk(Fcall*);
void	mremove(Fcall*);
void	mstat(Fcall*);
void	mwstat(Fcall*);

void 	(*fcall[])(Fcall *f) =
{
	[Tflush]	mflush,
	[Tsession]	msession,
	[Tnop]		mnop,
	[Tattach]	mattach,
	[Tclone]	mclone,
	[Twalk]		mwalk,
	[Topen]		mopen,
	[Tcreate]	mcreate,
	[Tread]		mread,
	[Twrite]	mwrite,
	[Tclunk]	mclunk,
	[Tremove]	mremove,
	[Tstat]		mstat,
	[Twstat]	mwstat
};

Fid*	newfid(int);
Fid*	lookfid(int);
int	delfid(Fcall*);
void	attachremote(char*);
void	reply(Fcall*, char*);
void	eiaread(void);
void	fsysread(void);
char* progname = "rdbfs";

enum {
	Rendez	= 0x89ABCDEF
};

void
usage(void)
{
	fprint(2, "Usage: rdbfs [-p procnum] [-t textfile] [serialport]\n");
	exits("usage");
}

int
forkproc(void (*fn)(void))
{
	int pid;
	switch(pid=rfork(RFNAMEG|RFMEM|RFPROC)){
	case -1:
		sysfatal("fork: %r");
	case 0:
		fn();
		_exits(0);
	default:
		return pid;
	}
	return -1;	/* not reached */
}

void
main(int argc, char **argv)
{
	int p[2];

	rfork(RFNOTEG|RFREND);

	ARGBEGIN{
	case 'd':
		dbg = 1;
		break;
	case 'p':
		if((procname=ARGF()) == nil)
			usage();
		break;
	case 't':
		if((textfile=ARGF()) == nil)
			usage();
		break;
	default:
		usage();
	}ARGEND;

	switch(argc){
	case 0:
		break;
	case 1:
		portname = argv[0];
		break;
	default:
		usage();
	}

	attachremote(portname);
	if(pipe(p) < 0)
		sysfatal("pipe: %r");

	fmtinstall('F', fcallconv);
	srvfd = p[1];
	forkproc(eiaread);
	forkproc(fsysread);

	if(mount(p[0], "/proc", MBEFORE, "") < 0)
		sysfatal("mount failed: %r");
	exits(0);
}

/*
 *	read protocol messages
 */
void
fsysread(void)
{
	int n;
	Fcall f;
	char buf[MAXMSG+MAXFDATA];

	pids[0] = getpid();
	for(;;) {
		n = read(srvfd, buf, sizeof(buf));
		if(n < 0)
			sysfatal("read mntpt: %r");
		n = convM2S(buf, &f, n);
		if(n < 0)
			continue;
		fcall[f.type](&f);
	}
}

/*
 *	write a protocol reply to the debugger
 */
void
reply(Fcall *r, char *error)
{
	int n;
	char rbuf[MAXMSG+MAXFDATA];

	if(error == nil)
		r->type++;
	else {
		r->type = Rerror;
		strncpy(r->ename, error, sizeof(r->ename));
	}
	n = convS2M(r, rbuf);
	if(write(srvfd, rbuf, n) < 0)
		sysfatal("reply: %r");

	DBG(2, "%F\n", r);
}

void
noalarm(void*, char *msg)
{
	if(strstr(msg, "alarm"))
		noted(NCONT);
	noted(NDFLT);
}

/*
 * 	send and receive responses on the serial line
 */
void
eiaread(void)
{
	Fcall*	r;
	char *p;
	uchar *data;
	char err[ERRLEN];
	char buf[1000];
	int i, tries;

	notify(noalarm);
	for(; r = (Fcall*)rendezvous(Rendez, 0); free(r)){
		DBG(2, "got %F: here goes...", r);
		if(r->count > Readlen)
			r->count = Readlen;
		if(lookup(r->offset, (uchar*)r->data)){
			reply(r, nil);
			continue;
		}
		for(tries=0; tries<5; tries++){
			if(r->type == Twrite){
				DBG(2, "w%.8lux %.8lux...", (ulong)r->offset, *(ulong*)r->data);
				fprint(rfd, "w%.8lux %.8lux\n", (ulong)r->offset, *(ulong*)r->data);
			}else if(r->type == Tread){
				DBG(2, "r%.8lux...", (ulong)r->offset);
				fprint(rfd, "r%.8lux\n", (ulong)r->offset);
			}else{
				reply(r, "oops");
				break;
			}
			for(;;){
				werrstr("");
				alarm(500);
				p=Brdline(&rfb, '\n');
				alarm(0);
				if(p == nil){
					err[0] = 0;
					errstr(err);
					DBG(2, "error %s\n", err);
					if(strstr(err, "alarm") || strstr(err, "interrupted"))
						break;
					if(Blinelen(&rfb) == 0) // true eof
						sysfatal("eof on serial line?");
					Bread(&rfb, buf, Blinelen(&rfb)<sizeof buf ? Blinelen(&rfb) : sizeof buf);
					continue;
				}
				p[Blinelen(&rfb)-1] = 0;
				if(p[0] == '\r')
					p++;
				DBG(2, "serial %s\n", p);
				if(p[0] == 'R'){
					if(strtoul(p+1, 0, 16) == (ulong)r->offset){
						data = (uchar*)r->data;
						for(i=0; i<Readlen; i++)
							data[i] = strtol(p+1+8+1+3*i, 0, 16);
						insert(r->offset, (uchar*)r->data);
						reply(r, nil);
						goto Break2;
					}else
						DBG(2, "%.8lux ≠ %.8lux\n", strtoul(p+1, 0, 16), (ulong)r->offset);
				}else if(p[0] == 'W'){
					r->count = 4;
					reply(r, nil);
					goto Break2;
				}else{
					DBG(2, "unknown message\n");
				}
			}
		}
	Break2:;
	}
}

void
attachremote(char* name)
{
	int fd;
	char buf[128];

	print("attach %s\n", name);
	rfd = open(name, ORDWR);
	if(rfd < 0)
		sysfatal("can't open remote %s", name);

	sprint(buf, "%sctl", name);
	fd = open(buf, OWRITE);
	if(fd < 0)
		sysfatal("can't set baud rate on %s", buf);
	write(fd, "B9600", 6);
	close(fd);
	Binit(&rfb, rfd, OREAD);
}

int
sendremote(Fcall *r)
{
	Fcall *nr;

	if(r->type == Tread || r->type == Twrite)
		nr = malloc(sizeof(Fcall)+Readlen);
	else
		nr = malloc(sizeof(Fcall));

	if(nr == nil)
		return -1;

	*nr = *r;
	if(r->type == Tread)
		nr->data = (char*)(nr+1);
	else if(r->type == Twrite){
		nr->data = (char*)(nr+1);
		*(ulong*)nr->data = *(ulong*)r->data;
	}
	rendezvous(Rendez, (ulong)nr);
	return 0;
}

void
mflush(Fcall *f)
{
	reply(f, nil);
}

void
msession(Fcall *f)
{
	memset(f->authid, 0, sizeof(f->authid));
	memset(f->authdom, 0, sizeof(f->authdom));
	memset(f->chal, 0, sizeof(f->chal));
	reply(f, nil);
}

void
mnop(Fcall *f)
{
	reply(f, nil);
}

void
mattach(Fcall *f)
{
	Fid *nf;

	nf = newfid(f->fid);
	if(nf == nil) {
		reply(f, "fid busy");
		return;
	}
	nf->qid = (Qid){Qroot, 0};
	f->qid = nf->qid;
	memset(f->rauth, 0, sizeof(f->rauth));
	nopen++;
	reply(f, nil);
}

void
mclone(Fcall *f)
{
	Fid *of, *nf;

	of = lookfid(f->fid);
	if(of == nil) {
		reply(f, "bad fid");
		return;
	}
	nf = newfid(f->newfid);
	if(nf == nil) {
		reply(f, "fid busy");
		return;
	}
	nf->qid = of->qid;
	f->qid = nf->qid;
	nopen++;
	reply(f, nil);
}

void
mwalk(Fcall *f)
{
	Fid *nf;
	int i;

	nf = lookfid(f->fid);
	if(nf == nil) {
		reply(f, "bad fid");
		return;
	}
	if(strcmp(f->name, ".") == 0) {
		f->qid = nf->qid;
		reply(f, nil);
		return;
	}

	switch(nf->qid.path) {
	case Qroot:
		if (strcmp(f->name, procname) == 0) {
			nf->qid.path = Q1;
			f->qid = nf->qid;
			reply(f, nil);
			return;
		}
		break;
	case Q1:
		if(strcmp(f->name, "..") == 0) {
			f->qid.path = Qroot;
			reply(f, nil);
			return;
		}
		for(i = 0; i < NFILES; i++) {
			if(fnames[i] != nil && strcmp(f->name, fnames[i]) == 0) {
				nf->qid.path = i;
				f->qid = nf->qid;
				reply(f, nil);
				return;
			}
		}
		break;
	}
	reply(f, Eexist);
}

void
statgen(Qid *q, char *buf)
{
	Dir dir;

	strcpy(dir.uid, "none");
	strcpy(dir.gid, "none");
	dir.qid = *q;
	dir.length = 0;
	dir.atime = time(0);
	dir.mtime = dir.atime;
	memset(dir.name, 0, NAMELEN);

	switch(q->path) {
	case Qroot:
		strcpy(dir.name, ".");
		dir.mode = CHDIR|0555;
		break;
	case Q1:
		strcpy(dir.name, procname);
		dir.mode = CHDIR|0555;
		break;
	case Qctl:
	case Qfpregs:
	case Qkregs:
	case Qmem:
	case Qproc:
	case Qregs:
	case Qtext:
	case Qstatus:
		strcpy(dir.name, fnames[q->path]);
		dir.mode = 0644;
		break;
	default:
		sysfatal("stat: bad qid %lux", q->path);
	}
	convD2M(&dir, buf);
}

void
mopen(Fcall *f)
{
	Fid *of;
	char buf[ERRLEN];

	of = lookfid(f->fid);
	if(of == nil) {
		reply(f, "bad fid");
		return;
	}

	switch(of->qid.path) {
	case Qroot:
	case Q1:
		break;
	case Qctl:
	case Qfpregs:
	case Qkregs:
	case Qmem:
	case Qproc:
	case Qregs:
	case Qstatus:
		f->qid = of->qid;
		break;
	case Qtext:
		close(textfd);
		textfd = open(textfile, OREAD);
		if(textfd < 0) {
			sprint(buf, "text: %r");
			reply(f, buf);
			return;
		}
		break;
	default:
		reply(f, "bad qid");
		return;
	}		
	of->open = 1;
	reply(f, nil);
}

void
mread(Fcall *f)
{
	Fid *of;
	Qid qid;
	int i, n;
	char *p, buf[512];

	of = lookfid(f->fid);
	if(of == nil) {
		reply(f, "bad fid");
		return;
	}

	switch(of->qid.path) {
	case Qroot:
		qid = (Qid){Q1, 0};
		statgen(&qid, dirbuf);
		if(f->offset >= DIRLEN)
			f->count = 0;
		else if(f->offset+f->count > DIRLEN)
			f->count = DIRLEN-f->offset;
		f->data = dirbuf+f->offset;
		reply(f, nil);
		break;
	case Q1:
		if(!of->open) {
			reply(f, "not open");
			return;
		}
		p = dirbuf;
		for(i = 0; i < NFILES; i++) {
			if(fnames[i]) {
				qid = (Qid){i, 0};
				statgen(&qid, p);
				p += DIRLEN;
			}
		}
		n = p-dirbuf;
		if(f->offset >= n)
			f->count = 0;
		else if(f->offset+f->count > n)
			f->count = n-f->offset;
		f->data = dirbuf+f->offset;
		reply(f, nil);
		break;
	case Qctl:
	case Qfpregs:
	case Qkregs:
	case Qmem:
	case Qproc:
	case Qregs:
		if(!of->open) {
			reply(f, "not open");
			return;
		}
		if(sendremote(f) < 0) {
			snprint(buf, sizeof buf, "rdbfs %r");
			reply(f, buf);
			return;
		}
		break;
	case Qtext:
		n = seek(textfd, f->offset, 0);
		if(n < 0) {
			errstr(buf);
			reply(f, buf);
			break;
		}
		n = read(textfd, dbuf, f->count);
		if(n < 0) {
			errstr(buf);
			reply(f, buf);
			break;
		}
		f->count = n;
		f->data = dbuf;
		reply(f, nil);
		break;
	case Qstatus:
		n = sprint(buf, "%-28s%-28s%-28s", "remote", "system", "New");
		for(i = 0; i < 9; i++)
			n += sprint(buf+n, "%-12d", 0);
		if(f->offset+f->count > n)
			f->count = n - f->offset;
		if(f->count < 0)
			f->count = 0;
		f->data = buf+f->offset;
		reply(f, nil);
		break;
	default:
		sysfatal("unknown read");
	}
}

/*
 *	disentangle our file system from the namespace
 */
void
umount(void)
{
	bind("#p", "/proc", MREPL);
	exits(nil);
}

void
mwrite(Fcall *f)
{
	Fid *of;
	char buf[ERRLEN];

	of = lookfid(f->fid);
	if(of == nil) {
		reply(f, "bad fid");
		return;
	}

	switch(of->qid.path) {
	case Qroot:
	case Q1:
		reply(f, "permission denied");
		break;
	case Qctl:
		if(strncmp(f->data, "kill", f->count-1) == 0 ||
		   strncmp(f->data, "exit", f->count-1) == 0) {
			reply(f, nil);
			umount();
		}else if(strncmp(f->data, "refresh", 7) == 0){
			flushcache();
			reply(f, nil);
		}else if(strncmp(f->data, "hashstats", 9) == 0){
			int i;
			lock(&pglock);
			for(i=0; i<NHASH; i++)
				if(pgtab[i])
					print("%lud ", pgtab[i]->len);
			print("\n");
			unlock(&pglock);
			reply(f, nil);
		}else
			reply(f, "permission denied");
		break;
	case Qfpregs:
	case Qkregs:
	case Qmem:
	case Qproc:
	case Qregs:
		if(!of->open) {
			reply(f, "not open");
			return;
		}
		if(sendremote(f) < 0) {
			snprint(buf, sizeof buf, "rdbfs %r");
			reply(f, buf);
			return;
		}
		break;
	case Qtext:
		reply(f, "text write protected");
	default:
		reply(f, "bad path");
		break;
	}
}

void
mclunk(Fcall *f)
{
	Fid *of;

	of = lookfid(f->fid);
	if(of == nil) {
		reply(f, "bad fid");
		return;
	}
	delfid(f);
	reply(f, nil);
	nopen--;
	if(nopen <= 0) {
		postnote(PNGROUP, getpid(), "kill");
		exits(nil);
	}
}

void
mstat(Fcall *f)
{
	Fid *of;

	of = lookfid(f->fid);
	if(of == nil) {
		reply(f, "bad fid");
		return;
	}

	statgen(&of->qid, f->stat);
	reply(f, nil);
}

void
mwstat(Fcall *f)
{
	reply(f, "permission denied");
}

void
mcreate(Fcall *f)
{
	reply(f, "permission denied");
}

void
mremove(Fcall *f)
{
	reply(f, "permission denied");
}

Fid*
newfid(int fid)
{
	Fid *nf, **l;

	if(lookfid(fid))
		return nil;

	nf = mallocz(sizeof(*nf), 1);
	assert(nf != nil);

	nf->fid = fid;
	nf->open = 0;
	nf->qid = (Qid){0, 0};

	l = &hash[fid%NFHASH];
	nf->hash = *l;
	*l = nf;

	return nf;
}

Fid*
lookfid(int fid)
{
	Fid *l;

	for(l = hash[fid%NFHASH]; l; l = l->hash)
		if(l->fid == fid)
			return l;

	return nil;
}

int
delfid(Fcall *f)
{
	Fid **l, *nf;

	l = &hash[f->fid%NFHASH];
	for(nf = *l; nf; nf = nf->hash) {
		if(nf->fid == f->fid) {
			*l = nf->hash;
			free(nf);
			return 1;
		}
	}
	return 0;
}
