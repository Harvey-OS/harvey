#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>

#define LOGFILE "telco"

/*
 * Rather than reading /adm/users, which is a lot of work for
 * a toy progdev, we assume all groups have the form
 *	NNN:user:user:
 * meaning that each user is the leader of his own group.
 */

enum
{
	OPERM	= 0x3,		/* mask of all permission types in open mode */
	Ndev	= 8,
	Nreq	= (Ndev*3)/2,
	Nrbuf	= 32*1024,
};

typedef struct Fid Fid;
typedef struct Dev Dev;
typedef struct Request Request;
typedef struct Type Type;

struct Fid
{
	Qid	qid;
	short	busy;
	short	open;
	int	fid;
	Fid	*next;
	char	*user;
};

struct Request
{
	Request	*next;

	Fid	*fid;
	ulong	tag;
	int	count;
	int	flushed;
};

struct Dev
{
	Lock;

	/* device state */
	int	ctl;		/* control fd */
	int	data;		/* data fd */
	char	*path;		/* to device */
	Type	*t;
	Type	*baset;
	int	speed;
	int	fclass;

	/* fs emulation */
	int	open;
	long	perm;
	char	name[NAMELEN];
	char	user[NAMELEN];
	char	msgbuf[128];
	Request	*r;
	Request *rlast;

	/* input reader */
	int	monitoring;	/* monitor pid */
	char	rbuf[Nrbuf];
	char	*rp;
	char	*wp;
	long	pid;
};

enum
{
	Devmask=	(Ndev-1)<<8,
	
	Qlvl1=		0,
	Qlvl2=		1,
	Qclone=		2,
	Qlvl3=		3,
	Qdata=		4,
	Qctl=		5,

	Pexec =		1,
	Pwrite = 	2,
	Pread = 	4,
	Pother = 	1,
	Pgroup = 	8,
	Powner =	64,
};

char *names[] =
{
[Qlvl1]		"/",
[Qlvl2]		"telco",
[Qclone]	"clone",
[Qlvl3]		"",
[Qdata]		"data",
[Qctl]		"ctl",
};

#define DEV(q) (((q).path&Devmask)>>8)
#define TYPE(q) ((q).path&((1<<8)-1))
#define MKQID(t, i) ((((i)<<8)&Devmask) | (t))

enum
{
	/*
	 *  modem specific commands
	 */
	Cerrorcorrection	= 0,	/* error correction */
	Ccompression,			/* compression */
	Cflowctl,			/* CTS/RTS */
	Crateadjust,			/* follow line speed */
	Cfclass2,			/* set up for fax */
	Cfclass0,			/* set up for data */
	Ncommand,
};

struct Type
{
	char	*name;
	char	*ident;		/* inquire request */
	char	*response;	/* inquire response (strstr) */
	char	*basetype;	/* name of base type */

	char	*commands[Ncommand];
};

/*
 *  Fax setup summary
 *
 *	FCLASS=2	- set to service class 2, i.e., one where the fax handles timing 
 *	FTBC=0		- ???
 *	FREL=1		- ???
 *	FCQ=1		- receive copy quality checking enabled
 *	FBOR=1		- set reversed bit order for phase C data
 *	FCR=1		- the DCE can receive message data, bit 10 in the DIS or
 *			  DTC frame will be set
 *	FDIS=,3		- limit com speed to 9600 baud
 */

Type typetab[] =
{
 {	"Rockwell",		0,	0,	0,
	"AT\\N7",	/* auto reliable (V.42, fall back to MNP, to none) */
	"AT%C1\\J0",	/* negotiate for compression, don't change port baud rate */
	"AT\\Q3",	/* CTS/RTS flow control */
 	"AT\\J1",
	"AT+FCLASS=2\rAT+FCR=1\r",
	"AT+FCLASS=0",
 },

 {	"ATT2400",	"ATI9",	"E2400",	"Rockwell",
	"AT\\N3",	/* auto reliable (MNP, fall back to none) */
	0,	
	0,
	0,
	0,
 	0,
 },

 {	"ATT14400",	"ATI9",	"E14400",	"Rockwell",
	0,
	0,	
	0,
	0,
	0,
 	0,
 },

 {	"MT1432",	"ATI2",	"MT1432",	0,
	"AT&E1",	/* auto reliable (V.42, fall back to none) */
	"AT&E15$BA0",	/* negotiate for compression */
	"AT&E4",	/* CTS/RTS flow control */
	"AT$BA1",	/* don't change port baud rate */
	"AT+FCLASS=2\rAT+FTBC=0\rAT+FREL=1\rAT+FCQ=1\rAT+FBOR=1\rAT+FCR=1\rAT+FDIS=,3",
	"AT+FCLASS=0",
 },

 {	"MT2834",	"ATI2",	"MT2834",	"MT1432",
	0,
	0,
	0,
	0,
 	"AT+FCLASS=2\rAT+FTBC=0\rAT+FREL=1\rAT+FCQ=1\rAT+FBOR=1\rAT+FCR=1",
	0,
 },

 {	"VOCAL",	"ATI6",	"144DPL+FAX",	"Rockwell",
	"AT\\N3",	/* auto reliable (V.42, fall back to MNP, fall back to none) */
	"AT%C3\\J0",	/* negotiate for compression, don't change port baud rate */	
	0,
	0,
 	"AT+FCLASS=2\rAT+FTBC=0\rAT+FREL=1\rAT+FCQ=1\rAT+FBOR=1\rAT+FCR=1",
	"AT+FCLASS=0",
 },

 { 0, },
};

/*
 *  modem return codes
 */
enum
{
	Ok,
	Success,
	Failure,
	Noise,
	Found,
};

/*
 *  modem return messages
 */
typedef struct Msg	Msg;
struct Msg
{
	char	*text;
	int	type;
};

Msg msgs[] =
{
	{ "OK",			Ok, },
	{ "NO CARRIER", 	Failure, },
	{ "ERROR",		Failure, },
	{ "NO DIALTONE",	Failure, },
	{ "BUSY",		Failure, },
	{ "NO ANSWER",		Failure, },
	{ "CONNECT",		Success, },
	{ 0,			0 },
};

Fid	*fids;
Dev	*dev;
int	ndev;
int	mfd[2];
char	user[NAMELEN];
char	mdata[MAXMSG+MAXFDATA];
Fcall	rhdr;
Fcall	thdr;
char	errbuf[ERRLEN+1];
int	pulsed;
int	verbose;
int	maxspeed = 56000;
char	*srcid = "plan9";
int	answer = 1;

Fid	*newfid(int);
void	devstat(Dir*, char*);
int	devgen(Qid, int, Dir*, char*);
void	error(char*);
void	io(void);
void	*erealloc(void*, ulong);
void	*emalloc(ulong);
void	usage(void);
int	perm(Fid*, Dev*, int);
void	setspeed(Dev*, int);
int	getspeed(char*, int);
char	*dialout(Dev*, char*);
void	onhook(Dev*);
int	readmsg(Dev*, int, char*);
void	monitor(Dev*);
int	getinput(Dev*, char*, int);
void	serve(Dev*);
void	receiver(Dev*);
char*	modemtype(Dev*, int, int);


char	*rflush(Fid*), *rnop(Fid*), *rsession(Fid*),
	*rattach(Fid*), *rclone(Fid*), *rwalk(Fid*),
	*rclwalk(Fid*), *ropen(Fid*), *rcreate(Fid*),
	*rread(Fid*), *rwrite(Fid*), *rclunk(Fid*),
	*rremove(Fid*), *rstat(Fid*), *rwstat(Fid*),
	*rauth(Fid*);

char 	*(*fcalls[])(Fid*) = {
	[Tflush]	rflush,
	[Tsession]	rsession,
	[Tnop]		rnop,
	[Tattach]	rattach,
	[Tclone]	rclone,
	[Twalk]		rwalk,
	[Tclwalk]	rclwalk,
	[Topen]		ropen,
	[Tcreate]	rcreate,
	[Tread]		rread,
	[Twrite]	rwrite,
	[Tclunk]	rclunk,
	[Tremove]	rremove,
	[Tstat]		rstat,
	[Twstat]	rwstat,
};

char	Eperm[] =	"permission denied";
char	Enotdir[] =	"not a directory";
char	Enotexist[] =	"file does not exist";
char	Eio[] = 	"i/o error";
char	Ebadaddr[] = 	"bad address";
char	Eattn[] = 	"can't get modem's attention";
char	Edial[] = 	"can't dial";
char	Enoauth[] =	"authentication not implimented";
char	Eisopen[] = 	"file already open for I/O";
char	Enodev[] = 	"no free modems";
char	Enostream[] =	"stream closed prematurely";

void
usage(void)
{
	fprint(2, "usage: %s [-vp] [-i srcid] dev ...\n", argv0);
	exits("usage");
}

void
notifyf(void *a, char *s)
{
	USED(a);
	if(strncmp(s, "interrupt", 9) == 0)
		noted(NCONT);
	noted(NDFLT);
}

void
main(int argc, char *argv[])
{
	int p[2];
	int fd;
	char buf[10];
	Dev *d;

	ARGBEGIN{
	case 'p':
		pulsed = 1;
		break;
	case 'v':
		verbose = 1;
		break;
	case 'i':
		srcid = ARGF();
		break;
	case 's':
		maxspeed = atoi(ARGF());
		break;
	case 'n':
		answer = 0;
		break;
	default:
		usage();
	}ARGEND

	if(argc == 0)
		usage();
	if(argc > Ndev)
		argc = Ndev;

	if(pipe(p) < 0)
		error("pipe failed");

	notify(notifyf);
	fmtinstall('F', fcallconv);
	strcpy(user, getuser());

	switch(rfork(RFFDG|RFPROC|RFREND|RFNOTEG)){
	case -1:
		error("fork");
	case 0:
		close(p[1]);
		mfd[0] = mfd[1] = p[0];
		break;
	default:
		close(p[0]);
		fd = create("/srv/telco", OWRITE, 0666);
		if(fd < 0)
			error("create of /srv/telco failed");
		sprint(buf, "%d", p[1]);
		if(write(fd, buf, strlen(buf)) < 0)
			error("writing /srv/telco");
		close(fd);
		if(amount(p[1], "/net", MBEFORE, "") < 0)
			error("mount failed");
		exits(0);
	}

	dev = mallocz(argc*sizeof(Dev), 1);
	for(ndev = 0; ndev < argc; ndev++){
		d = &dev[ndev];
		d->path = argv[ndev];
		d->rp = d->wp = d->rbuf;
		monitor(d);
		d->open++;
		onhook(d);
		d->open--;
	}

	io();
}

/*
 *  generate a stat structure for a qid
 */
void
devstat(Dir *dir, char *buf)
{
	Dev *d;
	int t;

	memset(dir->name, 0, sizeof(dir->name));
	memset(dir->uid, 0, sizeof(dir->uid));
	memset(dir->gid, 0, sizeof(dir->gid));
	t = TYPE(dir->qid);
	if(t != Qlvl3)
		strcpy(dir->name, names[t]);
	else
		sprint(dir->name, "%lud", DEV(dir->qid));
	dir->mode = 0755;
	strcpy(dir->uid, user);
	if(t >= Qlvl3){
		d = &dev[DEV(dir->qid)];
		if(d->open){
			dir->mode = d->perm;
			strcpy(dir->uid, d->user);
		}
	}
	if(dir->qid.path & CHDIR)
		dir->mode |= CHDIR;
	strcpy(dir->gid, user);
	if(t == Qdata){
		d = &dev[DEV(dir->qid)];
		dir->length = d->wp - d->rp;
		if(dir->length < 0)
			dir->length += Nrbuf;
	} else
		dir->length = 0;
	dir->atime = time(0);
	dir->mtime = dir->atime;
	if(buf)
		convD2M(dir, buf);
}

/*
 *  enumerate file's we can walk to from q
 */
int
devgen(Qid q, int i, Dir *d, char *buf)
{
	static ulong v;

	d->qid.vers = v++;
	switch(TYPE(q)){
	case Qlvl1:
		if(i != 0)
			return -1;
		d->qid.path = CHDIR|Qlvl2;
		break;
	case Qlvl2:
		switch(i){
		case -1:
			d->qid.path = CHDIR|Qlvl1;
			break;
		case 0:
			d->qid.path = Qclone;
			break;
		default:
			if(i > ndev)
				return -1;
			d->qid.path = MKQID(CHDIR|Qlvl3, i-1);
			break;
		}
		break;
	case Qlvl3:
		switch(i){
		case -1:
			d->qid.path = CHDIR|Qlvl2;
			break;
		case 0:
			d->qid.path = MKQID(Qdata, DEV(q));
			break;
		case 1:
			d->qid.path = MKQID(Qctl, DEV(q));
			break;
		default:
			return -1;
		}
		break;
	default:
		return -1;
	}
	devstat(d, buf);
	return 0;
}

char*
rnop(Fid *f)
{
	USED(f);
	return 0;
}

char*
rsession(Fid *unused)
{
	Fid *f;

	USED(unused);

	for(f = fids; f; f = f->next)
		if(f->busy)
			rclunk(f);
	memset(thdr.authid, 0, sizeof(thdr.authid));
	memset(thdr.authdom, 0, sizeof(thdr.authdom));
	memset(thdr.chal, 0, sizeof(thdr.chal));
	return 0;
}

char*
rflush(Fid *f)
{
	Request *r, **l;
	Dev *d;

	USED(f);
	for(d = dev; d < &dev[ndev]; d++){
		lock(d);
		for(l = &d->r; r = *l; l = &r->next)
			if(r->tag == rhdr.oldtag){
				*l = r->next;
				free(r);
				break;
			}
		unlock(d);
	}
	return 0;
}

char *
rauth(Fid *f)
{
	USED(f);
	return Enoauth;
}

char*
rattach(Fid *f)
{
	f->busy = 1;
	f->qid.path = CHDIR | Qlvl1;
	f->qid.vers = 0;
	thdr.qid = f->qid;
	if(rhdr.uname[0])
		f->user = strdup(rhdr.uname);
	else
		f->user = "none";
	return 0;
}

char*
rclone(Fid *f)
{
	Fid *nf;

	if(f->open)
		return Eisopen;
	if(f->busy == 0)
		return Enotexist;
	nf = newfid(rhdr.newfid);
	nf->busy = 1;
	nf->open = 0;
	nf->qid = f->qid;
	nf->user = strdup(f->user);
	return 0;
}

char*
rwalk(Fid *f)
{
	int i;
	char *name;
	Dir dir;

	if((f->qid.path & CHDIR) == 0)
		return Enotdir;
	name = rhdr.name;
	if(strcmp(name, ".") == 0)
		dir.qid = f->qid;
	else if(strcmp(name, "..") == 0){
		if(devgen(f->qid, -1, &dir, 0) < 0)
			return Enotexist;
	} else for(i = 0;; i++){
		if(devgen(f->qid, i, &dir, 0) < 0)
			return Enotexist;
		if(strcmp(name, dir.name) == 0)
			break;
	}
	f->qid = dir.qid;
	thdr.qid = f->qid;
	return 0;
}

char *
rclwalk(Fid *f)
{
	Fid *nf;
	char *err;

	nf = newfid(rhdr.newfid);
	nf->busy = 1;
	nf->qid = f->qid;
	nf->user = strdup(f->user);
	if(err = rwalk(nf))
		rclunk(nf);
	return err;
}

char *
ropen(Fid *f)
{
	Dev *d;
	int mode, t;

	if(f->open)
		return Eisopen;
	mode = rhdr.mode;
	mode &= OPERM;
	if(f->qid.path & CHDIR){
		if(mode != OREAD)
			return Eperm;
		thdr.qid = f->qid;
		return 0;
	}
	if(mode==OEXEC)
		return Eperm;
	t = TYPE(f->qid);
	if(t == Qclone){
		for(d = dev; d < &dev[ndev]; d++)
			if(d->open == 0)
				break;
		if(d == &dev[ndev])
			return Enodev;
		f->qid.path = MKQID(Qctl, d-dev);
		t = Qctl;
	}
	switch(t){
	case Qdata:
	case Qctl:
		d = &dev[DEV(f->qid)];
		if(d->open == 0){
			strcpy(d->user, f->user);
			d->perm = 0660;
		}else {
			if(mode==OWRITE || mode==ORDWR)
				if(!perm(f, d, Pwrite))
					return Eperm;
			if(mode==OREAD || mode==ORDWR)
				if(!perm(f, d, Pread))
					return Eperm;
		}
		d->open++;
		break;
	}
	thdr.qid = f->qid;
	f->open = 1;
	return 0;
}

char *
rcreate(Fid *f)
{
	USED(f);
	return Eperm;
}

/*
 *  intercept a note
 */
void
takeanote(void *u, char *note)
{
	USED(u);
	if(strstr(note, "flushed"))
		noted(NCONT);
	noted(NDFLT);
}

char*
rread(Fid *f)
{
	char *buf;
	long off;
	int i, n, cnt, t;
	Dir dir;
	char num[32];
	Dev *d;
	Request *r;

	n = 0;
	thdr.count = 0;
	off = rhdr.offset;
	cnt = rhdr.count;
	buf = thdr.data;
	t = TYPE(f->qid);
	switch(t){
	default:
		cnt = (cnt/DIRLEN)*DIRLEN;
		if(off%DIRLEN)
			return Eio;
		for(i = off/DIRLEN; n < cnt; i++){
			if(devgen(f->qid, i, &dir, buf+n) < 0)
				break;
			n += DIRLEN;
		}
		break;
	case Qctl:
		i = sprint(num, "%lud", DEV(f->qid));
		if(off < i){
			n = cnt;
			if(off + n > i)
				n = i - off;
			memmove(buf, num + off, n);
		} else
			n = 0;
		break;
	case Qdata:
		d = &dev[DEV(f->qid)];
		r = mallocz(sizeof(Request), 1);
		r->tag = rhdr.tag;
		r->count = rhdr.count;
		r->fid = f;
		r->flushed = 0;
		lock(d);
		if(d->r)
			d->rlast->next = r;
		else
			d->r = r;
		d->rlast = r;
		serve(d);
		unlock(d);
		return "";
	}
	thdr.count = n;
	return 0;
}

char *cmsg = "connect ";
int clen;

char*
rwrite(Fid *f)
{
	Dev *d;
	ulong off;
	int cnt;
	char *cp;
	char buf[64];

	off = rhdr.offset;
	cnt = rhdr.count;
	switch(TYPE(f->qid)){
	default:
		return "file is a directory";
	case Qctl:
		d = &dev[DEV(f->qid)];
		clen = strlen(cmsg);
		if(cnt < clen || strncmp(rhdr.data, cmsg, clen) != 0){
			/*
			 *  send control message to real control file
			 */
			if(seek(d->ctl, off, 0) < 0 || write(d->ctl, rhdr.data, cnt) < 0){
				errstr(errbuf);
				return errbuf;
			}
		} else {
			/*
			 *  connect
			 */
			cnt -= clen;
			if(cnt >= sizeof(buf))
				cnt = sizeof(buf) - 1;
			if(cnt < 0)
				return Ebadaddr;
			strncpy(buf, &rhdr.data[clen], cnt);
			buf[cnt] = 0;
			cp = dialout(d, buf);
			if(cp)
				return cp;
		}
		thdr.count = cnt;
		break;
	case Qdata:
		d = &dev[DEV(f->qid)];
		if(write(d->data, rhdr.data, cnt) < 0){
			errstr(errbuf);
			return errbuf;
		}
		thdr.count = cnt;
		break;
	}
	return 0;
}

char *
rclunk(Fid *f)
{
	Dev *d;

	if(f->open)
		switch(TYPE(f->qid)){
		case Qdata:
		case Qctl:
			d = &dev[DEV(f->qid)];
			if(d->open == 1)
				onhook(d);
			d->open--;
			break;
		}
	free(f->user);
	f->busy = 0;
	f->open = 0;
	return 0;
}

char *
rremove(Fid *f)
{
	USED(f);
	return Eperm;
}

char *
rstat(Fid *f)
{
	Dir d;

	d.qid = f->qid;
	devstat(&d, thdr.stat);
	return 0;
}

char *
rwstat(Fid *f)
{
	Dev *d;
	Dir dir;

	if(TYPE(f->qid) < Qlvl3)
		return Eperm;

	convM2D(rhdr.stat, &dir);
	d = &dev[DEV(f->qid)];

	/*
	 * To change mode, must be owner
	 */
	if(d->perm != dir.mode){
		if(strcmp(f->user, d->user) != 0)
		if(strcmp(f->user, user) != 0)
			return Eperm;
	}

	/* all ok; do it */
	d->perm = dir.mode & ~CHDIR;
	return 0;
}

Fid *
newfid(int fid)
{
	Fid *f, *ff;

	ff = 0;
	for(f = fids; f; f = f->next)
		if(f->fid == fid)
			return f;
		else if(!ff && !f->busy)
			ff = f;
	if(ff){
		ff->fid = fid;
		return ff;
	}
	f = emalloc(sizeof *f);
	f->fid = fid;
	f->next = fids;
	fids = f;
	return f;
}

/*
 *  read fs requests and dispatch them
 */
void
io(void)
{
	char *err;
	int n;

	for(;;){
		/*
		 * reading from a pipe or a network device
		 * will give an error after a few eof reads
		 * however, we cannot tell the difference
		 * between a zero-length read and an interrupt
		 * on the processes writing to us,
		 * so we wait for the error
		 */
		n = read9p(mfd[0], mdata, sizeof mdata);
		if(n == 0)
			continue;
		if(n < 0)
			error("mount read");
		if(convM2S(mdata, &rhdr, n) == 0)
			continue;


		thdr.data = mdata + MAXMSG;
		if(!fcalls[rhdr.type])
			err = "bad fcall type";
		else
			err = (*fcalls[rhdr.type])(newfid(rhdr.fid));
		if(err){
			if(*err == 0)
				continue;	/* assigned to a slave */
			thdr.type = Rerror;
			strncpy(thdr.ename, err, ERRLEN);
		}else{
			thdr.type = rhdr.type + 1;
			thdr.fid = rhdr.fid;
		}
		thdr.tag = rhdr.tag;
		n = convS2M(&thdr, mdata);
		if(write9p(mfd[1], mdata, n) != n)
			error("mount write");
	}
}


int
perm(Fid *f, Dev *d, int p)
{
	if((p*Pother) & d->perm)
		return 1;
	if(strcmp(f->user, user)==0 && ((p*Pgroup) & d->perm))
		return 1;
	if(strcmp(f->user, d->user)==0 && ((p*Powner) & d->perm))
		return 1;
	return 0;
}

void
error(char *s)
{
	fprint(2, "%s: %s: %r\n", argv0, s);
	syslog(0, LOGFILE, "%s: %r", s);
	remove("/srv/telco");
	postnote(PNGROUP, getpid(), "exit");
	exits(s);
}

void *
emalloc(ulong n)
{
	void *p;

	p = mallocz(n, 1);
	if(!p)
		error("out of memory");
	return p;
}

void *
erealloc(void *p, ulong n)
{
	p = realloc(p, n);
	if(!p)
		error("out of memory");
	return p;
}

/*
 *  send bytes to modem
 */
int
send(Dev *d, char *x)
{
	if(verbose)
		syslog(0, LOGFILE, "->%s", x);
	return write(d->data, x, strlen(x));
}

/*
 *  apply a string of commands to modem
 */
int
apply(Dev *d, char *s, char *substr, int secs)
{
	char buf[128];
	char *p;
	int c, m;

	p = buf;
	m = Ok;
	while(*s){
		c = *p++ = *s++;
		if(c == '\r' || *s == 0){
			if(c != '\r')
				*p++ = '\r';
			*p = 0;
			if(send(d, buf) < 0)
				return Failure;
			m = readmsg(d, secs, substr);
			p = buf;
		}
	}
	return m;
}

/*
 *  apply a command type
 */
int
applyspecial(Dev *d, int index)
{
	char *cmd;

	cmd = d->t->commands[index];
	if(cmd == 0 && d->baset)
		cmd = d->baset->commands[index];
	if(cmd == 0)
		return Failure;

	return apply(d, cmd, 0, 2);
}

/*
 *  get modem into command mode if it isn't already
 */
int
attention(Dev *d)
{
	int i;

	for(i = 0; i < 2; i++){
		sleep(250);
		if(send(d, "+") < 0)
			continue;
		sleep(250);
		if(send(d, "+") < 0)
			continue;
		sleep(250);
		if(send(d, "+") < 0)
			continue;
		sleep(250);
		readmsg(d, 0, 0);
		if(apply(d, "ATZH0", 0, 2) == Ok)
			return Ok;
	}
	return Failure;
}

int portspeed[] = { 56000, 38400, 19200, 14400, 9600, 4800, 2400, 1200, 600, 300, 0 };

/*
 *  get the modem's type and speed
 */
char*
modemtype(Dev *d, int limit,  int fax)
{
	int *p;
	Type *t, *bt;
	char buf[28];

	d->t = typetab;
	d->baset = 0;

	/* assume we're at a good speed, try getting attention a few times */
	attention(d);

	/* find a common port rate */
	for(p = portspeed; *p; p++){
		if(*p > limit)
			continue;
		setspeed(d, *p);
		if(attention(d) == Ok)
			break;
	}
	if(*p == 0)
		return Eattn;
	d->speed = *p;
	if(verbose)
		syslog(0, LOGFILE, "port speed %d", *p);

	/*
	 *  basic Hayes commands everyone implements (we hope)
	 *	Q0 = report result codes
	 * 	V1 = full word result codes
	 *	E0 = don't echo commands
	 *	M1 = speaker on until on-line
	 *	S0=0 = autoanswer off
	 */
	if(apply(d, "ATQ0V1E0M1S0=0", 0, 2) != Ok)
		return Eattn;

	/* find modem type */
	for(t = typetab; t->name; t++){
		if(t->ident == 0 || t->response == 0)
			continue;
		if(apply(d, t->ident, t->response, 2) == Found)
			break;
		readmsg(d, 0, 0);
	}
	readmsg(d, 0, 0);
	if(t->name){
		d->t = t;
		if(t->basetype){
			for(bt = typetab; bt->name; bt++)
				if(strcmp(bt->name, t->basetype) == 0)
					break;
			if(bt->name)
				d->baset = bt;
		}
	}
	if(verbose)
		syslog(0, LOGFILE, "modem %s", d->t->name);

	/* try setting fax modes */
	d->fclass = 0;
	if(fax){
		/* set up fax parameters */
		if(applyspecial(d, Cfclass2) != Failure)
			d->fclass = 2;

		/* setup a source id */
		if(srcid){
			sprint(buf, "AT+FLID=\"%s\"", srcid);
			apply(d, buf, 0, 2);
		}

		/* allow both data and fax calls in */
		apply(d, "AT+FAA=1", 0, 2);
	} else
		applyspecial(d, Cfclass0);
	return 0;
}

/*
 *  a process to read input from a modem.
 */
void
monitor(Dev *d)
{
	int n;
	char *p;
	char file[256];
	int background;

	background = 0;
	d->ctl = d->data = -1;

	for(;;){
		lock(d);
		sprint(file, "%sctl", d->path);
		d->ctl = open(file, ORDWR);
		if(d->ctl < 0)
			error("opening ctl");
		d->data = open(d->path, ORDWR);
		if(d->data < 0)
			error("opening data");
		d->wp = d->rp = d->rbuf;
		unlock(d);

		if(!background){
			background = 1;
			switch(d->pid = rfork(RFPROC|RFMEM)){
			case -1:
				error("out of processes");
			case 0:
				break;
			default:
				return;
			}
		}

		/* wait for ring or off hook */
		while(d->open == 0){
			d->rp = d->rbuf;
			p = d->wp;
			n = read(d->data, p, 1);
			if(n < 1)
				continue;
			if(p < &d->rbuf[Nrbuf] - 2)
				d->wp++;
			if(*p == '\r' || *p == '\n'){
				*(p+1) = 0;
				if(verbose)
					syslog(0, LOGFILE, "<:-%s", d->rp);
				if(answer && strncmp(d->rp, "RING", 4) == 0){
					receiver(d);
					continue;
				}
				if(d->open == 0)
					d->wp = d->rbuf;
			}
		}

		/* shuttle bytes till on hook */
		while(d->open){
			if(d->wp >= d->rp)
				n = &d->rbuf[Nrbuf] - d->wp;
			else
				n = d->rp - d->wp - 1;
			if(n > 0)
				n = read(d->data, d->wp, n);
			else {
				read(d->data, file, sizeof(file));
				continue;
			}
			if(n < 0)
				break;
			lock(d);
			if(d->wp + n >= &d->rbuf[Nrbuf])
				d->wp = d->rbuf;
			else
				d->wp += n;
			serve(d);
			unlock(d);
		}

		close(d->ctl);
		close(d->data);
	}
}

/*
 *  get bytes input by monitor() (only routine that changes d->rp)
 */
int
getinput(Dev *d, char *buf, int n)
{
	char *p;
	int i;

	p = buf;
	while(n > 0){
		if(d->wp == d->rp)
			break;
		if(d->wp < d->rp)
			i = &d->rbuf[Nrbuf] - d->rp;
		else
			i = d->wp - d->rp;
		if(i > n)
			i = n;
		memmove(p, d->rp, i);
		if(d->rp + i == &d->rbuf[Nrbuf])
			d->rp = d->rbuf;
		else
			d->rp += i;
		n -= i;
		p += i;
	}
	return p - buf;
}

/*
 *  fulfill a read request (we assume d is locked)
 */
void
serve(Dev *d)
{
	Request *r;
	int n;
	Fcall thdr;
	char mdata[MAXMSG+MAXFDATA];
	char buf[MAXFDATA];

	for(;;){
		if(d->r == 0 || d->rp == d->wp)
			return;
		r = d->r;
		if(r->count > sizeof(buf))
			r->count = sizeof(buf);

		n = getinput(d, buf, r->count);
		if(n == 0)
			return;
		d->r = r->next;

		thdr.type = Rread;
		thdr.fid = r->fid->fid;
		thdr.tag = r->tag;
		thdr.data = buf;
		thdr.count = n;
		n = convS2M(&thdr, mdata);
		if(write9p(mfd[1], mdata, n) != n)
			fprint(2, "telco: error writing\n");
		free(r);
	}
}

/*
 *  dial a number
 */
char*
dialout(Dev *d, char *number)
{
	int i, m, compress, rateadjust, speed, fax;
	char *err;
	char *field[5];
	char dialstr[2*NAMELEN];

	compress = Ok;
	rateadjust = Failure;
	speed = maxspeed;
	fax = Failure;

	m = getfields(number, field, 5, 1, "!");
	for(i = 1; i < m; i++){
		if(field[i][0] >= '0' && field[i][0] <= '9')
			speed = atoi(field[i]);
		else if(strcmp(field[i], "nocompress") == 0)
			compress = Failure;
		else if(strcmp(field[i], "fax") == 0)
			fax = Ok;
	}

	syslog(0, LOGFILE, "dialing %s speed=%d %s", number, speed, fax==Ok?"fax":"");
	
	err = modemtype(d, speed, fax == Ok);
	if(err)
		return err;

	/*
	 *  extented Hayes commands, meaning depends on modem (VGA all over again)
	 */
	if(fax != Ok){
		if(d->fclass != 0)
			applyspecial(d, Cfclass0);
		applyspecial(d, Cerrorcorrection);
		if(compress == Ok)
			compress = applyspecial(d, Ccompression);
		if(compress != Ok)
			rateadjust = applyspecial(d, Crateadjust);
	}
	applyspecial(d, Cflowctl);

	/* dialout */
	sprint(dialstr, "ATD%c%s\r", pulsed ? 'P' : 'T', number);
	if(send(d, dialstr) < 0)
		return Edial;

	if(fax == Ok)
		return 0;		/* fax sender worries about the rest */

	switch(readmsg(d, 120, 0)){
	case Success:
		break;
	default:
		return d->msgbuf;
	}

	/* change line rate if not compressing */
	if(rateadjust == Ok)
		setspeed(d, getspeed(d->msgbuf, d->speed));

	return 0;
}

/*
 *  start a receiving process
 */
void
receiver(Dev *d)
{
	int fd;
	char file[256];
	char *argv[8];
	int argc;
	int pfd[2];
	char *prog;

	pipe(pfd);
	switch(rfork(RFPROC|RFMEM|RFFDG|RFNAMEG)){
	case -1:
		return;
	case 0:
		fd = open("/srv/telco", ORDWR);
		if(fd < 0){
			syslog(0, LOGFILE, "can't open telco: %r");
			exits(0);
		}
		if(mount(fd, "/net", MAFTER, "") < 0){
			syslog(0, LOGFILE, "can't mount: %r");
			exits(0);
		}
		close(fd);

		/* open connection through the file system interface */
		sprint(file, "/net/telco/%ld/data", d - dev);
		fd = open(file, ORDWR);
		if(fd < 0){
			syslog(0, LOGFILE, "can't open %s: %r", file);
			exits(0);
		}

		/* let parent continue */
		close(pfd[0]);
		close(pfd[1]);

		/* answer the phone and see what flavor call this is */
		prog = "/bin/service/telcodata";
		switch(apply(d, "ATA", "+FCON", 30)){
		case Success:
			break;
		case Found:
			prog = "/bin/service/telcofax";
			break;
		default:
			syslog(0, LOGFILE, "bad ATA response");
			exits(0);
		}

		/* fork a receiving process */
		dup(fd, 0);
		dup(fd, 1);
		close(fd);
		argc = 0;
		argv[argc++] = strrchr(prog, '/')+1;
		argv[argc++] = file;
		argv[argc++] = dev->t->name;
		argv[argc] = 0;
		exec(prog, argv);
		syslog(0, LOGFILE, "can't exec %s: %r\n", prog);
		exits(0);
	default:
		/* wait till child gets the device open */
		close(pfd[1]);
		read(pfd[0], file, 1);
		close(pfd[0]);
		break;
	}
}

/*
 *  hang up an connections in progress
 */
void
onhook(Dev *d)
{
	write(d->ctl, "d0", 2);
	write(d->ctl, "r0", 2);
	sleep(250);
	write(d->ctl, "r1", 2);
	write(d->ctl, "d1", 2);
	modemtype(d, maxspeed, 1);
}

/*
 *  read till we see a message or we time out
 */
int
readmsg(Dev *d, int secs, char *substr)
{
	ulong start;
	char *p;
	int i, len;
	Msg *pp;
	int found = 0;

	p = d->msgbuf;
	len = sizeof(d->msgbuf) - 1;
	for(start = time(0); time(0) <= start+secs;){
		if(len && d->rp == d->wp){
			sleep(100);
			continue;
		}
		i = getinput(d, p, 1);
		if(i == 0)
			continue;
		if(*p == '\n' || *p == '\r' || len == 0){
			*p = 0;
			if(verbose && p != d->msgbuf)
				syslog(0, LOGFILE, "<-%s", d->msgbuf);
			if(substr && strstr(d->msgbuf, substr))
				found = 1;
			for(pp = msgs; pp->text; pp++)
				if(strncmp(pp->text, d->msgbuf, strlen(pp->text))==0)
					return found ? Found : pp->type;
			start = time(0);
			p = d->msgbuf;
			len = sizeof(d->msgbuf) - 1;
			continue;
		}
		len--;
		p++;
	}
	strcpy(d->msgbuf, "No response from modem");
	return found ? Found : Noise;
}

/*
 *  get baud rate from a connect message
 */
int
getspeed(char *msg, int speed)
{
	char *p;
	int s;

	p = msg + sizeof("CONNECT") - 1;
	while(*p == ' ' || *p == '\t')
		p++;
	s = atoi(p);
	if(s <= 0)
		return speed;
	else
		return s;
}

/*
 *  set speed and RTS/CTS modem flow control
 */
void
setspeed(Dev *d, int baud)
{
	char buf[32];

	if(d->ctl < 0)
		return;
	sprint(buf, "b%d", baud);
	write(d->ctl, buf, strlen(buf));
	write(d->ctl, "m1", 2);
}
