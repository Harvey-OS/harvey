/*
 * Mount a Plan 9 server before other names in /dev.
 * Served files:
 *	cons, consctl:	like those in #c (slave end console)
 *			if consctl has had "rawon" written to it
 *			turn off input canonicizing until "rawoff"
 *			is written to it, or consctl is closed
 *	tty:		synonym for cons
 *	mcons:		master: writes provide chars to satisfy reads
 *			of cons; reads get chars written
 *			to cons
 *	pttyctl:	control file: reads and writes communicate
 *			termios structs, in this format:
 *			"stty %8ux %8ux %8ux %8ux "
 *			for c_iflag, c_oflag, c_cflag, c_lflag,
 *			followed by NCCS bytes for c_cc[]
 *			reading the file gets back the same thing,
 *			with current values filled in.
 *			Also understands message "pid %11d ", which
 *			is the pid whose pgrpg gets interrupt and die
 *			messages.  And, understands message "note xxx"
 *			to cause the xxx string to be sent to the
 *			registered process group.
 *
 * Stat on cons or mcons will give a lower bound on the number of
 * characters that will be returned (without blocking) on reading.
 *
 * If mcons refcount goes to zero, or cons/tty refcount goes to
 * zero and mcons refcount is not zero, that is taken as a signal
 * to clean up and exit.
 *
 * Bugs: no parity stuff.
 */

#define _PCONS_EXTENSION

#include "lib.h"
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <termios.h>
#include <stdio.h>
#include <pcons.h>
#include <signal.h>
#include "sys9.h"
#include "dir.h"
#include "fcall.h"

typedef unsigned char uchar;
typedef unsigned long ulong;

enum {
	Qcons=1,
	Qconsctl,
	Qmcons,
	Qpttyctl,
	Qtty,
	Qroot=0x80000000
};

enum {
	Nfid = 8192
};

typedef struct Dbuffer {
	char	*cur;		/* next place to get data */
	char	*end;		/* next place to put data */
	int	size;		/* current size of data */
	int	eof;		/* eof at end of current buffer */
	int	readn;		/* number of chars in a pending read */
	int	readtag;	/* tag for pending read */
	int	readfid;	/* tag for pending read */
	char	*data;
} Dbuffer;

typedef struct Fid Fid;
struct Fid
{
	int	busy;
	int	fid;
	Fid	*next;
	long	qidpath;
};

static int	sfd, cfd;		/* server and client fds */
static char	data[10000];		/* messages to server */
static char	rdata[10000];		/* messages from server */
static char	statbufs[6*DIRLEN];	/* for satisifying dir reads and stats */
static Fid	*fids;			/* list of Fid structs, to hold qid.path */
static int	consref;		/* number of fids for cons */
static int	mconsref;		/* number of fids for mcons */
static Fcall	rfc, tfc;		/* struct form of messages to, from server */
static Dbuffer	mtoc, ctom;		/* data from mcons to cons, and vice versa */
static struct termios tio;		/* current terminal characteristics */
static int	sigpid = 0;		/* pid whose pgrp gets signals */
static int	sigfd = -1;		/* open on /proc/pid/notepg */
static int	noteprocpid;		/* pid of process in notifyproc() */
static uchar	iprocind[256];		/* index of input handling proc */
static ulong	cmask;			/* mask input chars with this */
static char	eol;			/* alternate end of line char */
static int	suspendout;		/* true if suspending output */
static char	*pending = "pending";

static int	mountinit(char *);
static void	notifyproc(void);
static void	statinit(void);
static void	terminit(void);
static void	setiprocs(void);
static void	error(char *);
static void	io(void);
static void	doneio(void);
static void	iosighandler(int);
static void	sighandler(int);
static char	*rwalk(long, Fid *);
static char	*rread(long);
static char	*rwrite(long);
static char	*rstat(long);
static void	checkpend(Dbuffer *, int);
static void	mtocadd(char);
static void	ctomadd(char);
static void	addchars(Dbuffer *, char *, int);
static void	inorm(char);
static void	inocanon(char);
static void	ierase(char);
static void	ikill(char);
static void	ieof(char);
static void	ieol(char);
static void	icr(char);
static void	inl(char);
static void	isig(char);
static void	iflow(char);

enum {
	Pnorm,
	Pnocanon,
	Perase,
	Pkill,
	Peof,
	Peol,
	Pcr,
	Pnl,
	Psig,
	Pflow
};

static void (*iproc[])(char) = {
	[Pnorm]		inorm,
	[Pnocanon]	inocanon,
	[Perase]	ierase,
	[Pkill]		ikill,
	[Peof]		ieof,
	[Peol]		ieol,
	[Pcr]		icr,
	[Pnl]		inl,
	[Psig]		isig,
	[Pflow]		iflow
};

static int
_pcons(void)
{
	int p[2], f, mpid;
	char buf[25];

	notifyproc();

	/*
	 * arrange that shell interrupts get delivered
	 * to notifyproc and no other
	 */
	_RFORK(FORKNTG|FORKNSG);

	if(pipe(p) < 0)
		error("making server pipe");
	cfd = p[0];
	sfd = p[1];
	mpid = mountinit("/dev");
	statinit();
	terminit();
	switch(fork()){
	case 0:
		io();
		_exit(0);
	case -1:
		error("fork");
	default:
		break;
	}
	close(sfd);
	waitpid(mpid, 0, 0);
	f = open("/dev/pttyctl", O_WRONLY);
	if(f < 0)
		return -1;
	sprintf(buf, "pid %11d ", getpid());
	if(write(f, buf, strlen(buf)) < 0)
		return -1;
	close(f);
	return 0;
}

int
pcons(void)
{
	if(_pcons() < 0)
		return -1;

	/*
 	 * Now ensure that notes to this
	 * (main) process's process group will not
	 * interrupt the server
	 */
	_RFORK(FORKNTG);
	return 0;
}

static int
mountinit(char *place)
{
	int pid;

	switch((pid=fork())){
	case 0:
		if(_MOUNT(cfd, place, MBEFORE, "", "") < 0)
			error("mount failed");
		_exit(0);
	case -1:
		error("fork failed\n");
	}
	return pid;
}

/*
 * Put encoded stat of root at the beginning of xstatbuf,
 * followed by the encodings of the directory elements
 */
static void
statinit(void)
{
	Dir d;

	memset(&d, 0, sizeof d);
	strcpy(d.name, ".");
	d.qid.path = Qroot;
	d.mode = 0x80000000 | 0777;
	convD2M(&d, statbufs);
	strcpy(d.name, "cons");
	d.qid.path = Qcons;
	d.mode = 0666;
	convD2M(&d, statbufs + DIRLEN);
	strcpy(d.name, "consctl");
	d.qid.path = Qconsctl;
	convD2M(&d, statbufs + 2*DIRLEN);
	strcpy(d.name, "mcons");
	d.qid.path = Qmcons;
	convD2M(&d, statbufs + 3*DIRLEN);
	strcpy(d.name, "pttyctl");
	d.qid.path = Qpttyctl;
	convD2M(&d, statbufs + 4*DIRLEN);
	strcpy(d.name, "tty");
	d.qid.path = Qtty;
	convD2M(&d, statbufs + 5*DIRLEN);
}

static void
terminit(void)
{
	mtoc.data = malloc(8192);
	mtoc.cur = mtoc.end = mtoc.data;
	mtoc.size = 8192;
	mtoc.eof = mtoc.readn = mtoc.readtag = mtoc.readfid = 0;
	ctom.data = malloc(8192);
	ctom.cur = ctom.end = ctom.data;
	ctom.size = 8192;
	ctom.eof = ctom.readn = ctom.readtag = ctom.readfid = 0;
	if(!mtoc.data || !ctom.data)
		error("can't allocate bufs");

	tio.c_iflag = ICRNL;
	tio.c_oflag = 0;
	tio.c_cflag = CREAD | CS8;
	tio.c_lflag = ICANON | ISIG | ECHO | ECHOE | ECHOK;
	tio.c_cc[VEOF] = 0x04;		/* ^D */
	tio.c_cc[VEOL] = 0;		/* null */
	tio.c_cc[VERASE] = 0x08;	/* backspace */
	tio.c_cc[VINTR] = 0x7f;		/* delete */
	tio.c_cc[VKILL] = 0x15;		/* ^U */
	tio.c_cc[VMIN] = 1;
	tio.c_cc[VQUIT] = 0x1c;		/* ^\ */
	tio.c_cc[VSUSP] = 0;
	tio.c_cc[VTIME] = 0;
	tio.c_cc[VSTART] = 0x11;	/* ^Q */
	tio.c_cc[VSTOP] = 0x13;		/* ^S */

	cmask = 0xFF;
	eol = '\n';
	setiprocs();
}

static void
setiprocs(void)
{
	memset(iprocind, tio.c_lflag&ICANON ? Pnorm : Pnocanon, sizeof iprocind);
	if(tio.c_lflag & ICANON){
		iprocind[tio.c_cc[VERASE]] = Perase;
		iprocind[tio.c_cc[VKILL]] = Pkill;
		iprocind[tio.c_cc[VEOF]] = Peof;
		iprocind[tio.c_cc[VEOL]] = Peol;
	}
	if(tio.c_lflag & ISIG){
		iprocind[tio.c_cc[VINTR]] = Psig;
		iprocind[tio.c_cc[VQUIT]] = Psig;
	}
	if(tio.c_iflag & IXON){
		iprocind[tio.c_cc[VSTART]] = Pflow;
		iprocind[tio.c_cc[VSTOP]] = Pflow;
	}
	iprocind['\n'] = Pnl;
	iprocind['\r'] = Pcr;
}

static Fid *
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
	f = malloc(sizeof *f);
	f->qidpath = 0;
	f->fid = fid;
	f->next = fids;
	fids = f;
	return f;
}

static void
io(void)
{
	long n, q;
	char *err;
	Fid *f, *nf;
int i;

	for(n=0; n<OPEN_MAX; n++)
		if(n != sfd)
			close(n);
	signal(SIGINT, iosighandler);
    loop:
	n = read(sfd, data, sizeof data);
	if(n == 0)
		goto loop;
	if(n < 0)
		_EXITS("mount read");
	if(convM2S(data, &rfc, n) == 0){
		_EXITS("format error");
	}

	f = newfid(rfc.fid);
	q = f->qidpath;
	err = 0;
/*
fprintf(stderr, "io type %d fid %d tag %d q %x\n", rfc.type, rfc.tag, rfc.fid, q);
for(i=0;i<n;i++)fprintf(stderr, "%x ", data[i]&0xFF);
fprintf(stderr, "\n");
*/
	switch(rfc.type){
	default:
		_EXITS("type");
		break;
	case Tsession:
	case Tnop:
	case Tflush:
		break;
	case Tcreate:
	case Tremove:
	case Twstat:
		err = "permission denied";
		break;
	case Tauth:
		err = "no authentication";
		break;
	case Tattach:
		f->qidpath = Qroot;
		f->busy = 1;
		tfc.qid.path = Qroot;
		tfc.qid.vers = 0;
		break;
	case Tclone:
		nf = newfid(rfc.newfid);
/* fprintf(stderr,"clone newfid %d, nf %x, qid %x\n", rfc.newfid, nf, q); /**/
		nf->busy = 1;
		nf->qidpath = q;
		tfc.qid.path = q;
		tfc.qid.vers = 0;
		break;
	case Twalk:
		err = rwalk(q, f);
		break;
	case Tclwalk:
		nf = newfid(rfc.newfid);
		nf->busy = 1;
		nf->qidpath = q;
		tfc.qid.path = q;
		tfc.qid.vers = 0;
		rfc.fid = rfc.newfid;
		err = rwalk(q, nf);
		if (err)
			nf->busy = 0;
		break;
	case Topen:
		tfc.qid.path = q;
		tfc.qid.vers = 0;
		break;
	case Tread:
/* fprintf(stderr, "read fid %d tag %d\n", rfc.fid, rfc.tag); /**/
		err = rread(q);
		if(err == pending) {
/* fprintf(stderr, "nothing to satisfy read, so deferring\n"); /**/
			goto loop;
		}
		break;
	case Twrite:
/* fprintf(stderr, "write fid %d tag %d\n", rfc.fid, rfc.tag); /**/
		err = rwrite(q);
		break;
	case Tclunk:
/* fprintf(stderr, "clunk fid %d\n", rfc.fid); /**/
		switch(q){
		case Qconsctl:
			/* bug: should ref count and remember old value */
			tio.c_iflag |= ICANON;
			setiprocs();
			break;
		case Qmcons:
			if(--mconsref == 0)
				doneio();
			break;
		case Qcons:
		case Qtty:
			if(--consref == 0)
				if(mconsref)
					doneio();
			break;
		}
		f->busy = 0;
		f->qidpath = 0;
		break;
	case Tstat:
/* fprintf(stderr, "stat fid %d\n", rfc.fid); /**/
		rstat(q);
		break;
	}
	if(err){
		tfc.type = Rerror;
		strncpy(tfc.ename, err, ERRLEN);
	}else
		tfc.type = rfc.type+1;
	tfc.tag = rfc.tag;
	tfc.fid = rfc.fid;
	n = convS2M(&tfc, data);
	if(write(sfd, data, n) != n)
		error("mount write");
	goto loop;
}

static char*
rwalk(long q, Fid *f)
{
	long nq;
	char *name;

	if(q != Qroot)
		return "not a directory";
	nq = 0;
	name = rfc.name;
/* fprintf(stderr,"rwalk trying to open %s\n", name); /**/
	if(strcmp(name, "cons") == 0) {
		nq = Qcons;
		consref++;
	} else if(strcmp(name, "consctl") == 0)
		nq = Qconsctl;
	else if(strcmp(name, "mcons") == 0) {
		nq = Qmcons;
		mconsref++;
	} else if(strcmp(name, "pttyctl") == 0)
		nq = Qpttyctl;
	else if(strcmp(name, "tty") == 0) {
		nq = Qtty;
		consref++;
	} else if(strcmp(name, ".") == 0)
		nq = Qroot;
	if(!nq)
		return "file does not exist";
	f->qidpath = nq;
	tfc.qid.path = nq;
	tfc.qid.vers = 0;
	return 0;
}

static char*
rread(long q)
{
	long n;
	char buf[100];

	n = 0;
	switch(q){
	case Qroot:
		if(rfc.offset%DIRLEN == 0 && rfc.offset < 5*DIRLEN){
			tfc.data = statbufs + DIRLEN*(1 + rfc.offset/DIRLEN);
			n = DIRLEN * (rfc.count/DIRLEN);
			if(n > 5*DIRLEN - rfc.offset)
				n = 5*DIRLEN - rfc.offset;
		}
		break;
	case Qcons:
	case Qtty:
/* fprintf(stderr,"cons read (mtoc), old n %d fid %d tag %d\n", mtoc.readn, mtoc.readfid, mtoc.readtag); /**/
		mtoc.readn = rfc.count;
		mtoc.readfid = rfc.fid;
		mtoc.readtag = rfc.tag;
/* fprintf(stderr,"   new n %d fid %d tag %d\n", mtoc.readn, mtoc.readfid, mtoc.readtag); /**/
		checkpend(&mtoc, tio.c_iflag&ICANON);
		return pending;
		break;
	case Qmcons:
/* fprintf(stderr,"mcons read (ctom), old n %d fid %d tag %d\n", ctom.readn, ctom.readfid, ctom.readtag); /**/
		ctom.readn = rfc.count;
		ctom.readfid = rfc.fid;
		ctom.readtag = rfc.tag;
/* fprintf(stderr,"   new n %d fid %d tag %d\n", ctom.readn, ctom.readfid, ctom.readtag); /**/
		checkpend(&ctom, 0);
		return pending;
		break;
	case Qpttyctl:
		if(rfc.count >= 4+4*9+NCCS){
			sprintf(buf, "stty %8x %8x %8x %8x ",
				tio.c_iflag, tio.c_oflag, tio.c_cflag,
				tio.c_lflag);
			memcpy(buf+5+36, tio.c_cc, NCCS);
			tfc.data = buf;
			n = 4+4*9+NCCS;
		}
		break;
	case Qconsctl:
		return "io error";
	}
	tfc.count = n;
	return 0;
}

static void
doneio(void)
{
	char buf[100];
	int f;

	if(mconsref) {
		ctom.eof = 1;
		checkpend(&ctom, 0);
	}
	sprintf(buf, "/proc/%d/ctl", noteprocpid);
	f = open(buf, O_WRONLY);
	if(f >= 0)
		write(f, "kill", 4);
	close(f);
	_exit(0);
}

static char*
rwrite(long q)
{
	long n;
	int i;
	uchar c;
	char *p;

	n = rfc.count;
	switch(q){
	case Qroot:
		return "write to directory";
	case Qcons:
	case Qtty:
/* fprintf(stderr, "cons write %d chars\n", rfc.count); /**/
		for(i = 0; i < rfc.count; i++)
			ctomadd(rfc.data[i]);
		checkpend(&ctom, 0);
		break;
	case Qmcons:
/* fprintf(stderr, "mcons write %d chars\n", rfc.count); /**/
		for(i = 0; i < rfc.count; i++){
			c = rfc.data[i] & cmask;
			(*iproc[iprocind[c]])((char)c);
		}
		if(tio.c_lflag & ECHO)
			checkpend(&ctom, 0);
		break;
	case Qconsctl:
		p = rfc.data;
		if(n >= 5 && strncmp(p, "rawon", 5) == 0){
			tio.c_iflag &= ~ICANON;
			setiprocs();
			n = 5;
		} else if(n >= 6 && strncmp(p, "rawoff", 6) == 0){
			tio.c_iflag |= ICANON;
			setiprocs();
			n = 6;
		} else
			return "bad control";
		break;
	case Qpttyctl:
		p = rfc.data;
		if(n >= 4+4*9+NCCS && strncmp(p, "stty ", 5) == 0){
			tio.c_iflag = strtol(p+5, 0, 16);
			tio.c_oflag = strtol(p+5+9, 0, 16);
			tio.c_cflag = strtol(p+5+18, 0, 16);
			tio.c_lflag = strtol(p+5+27, 0, 16);
			memcpy(tio.c_cc, p+5+36, NCCS);
			setiprocs();
			eol = tio.c_cc[VEOL];
			if(tio.c_iflag & ISTRIP)
				cmask = 0x7F;
			else
				cmask = 0xFF;
			n = 4+4*9+NCCS;
		}else if(n >= 4+12 && strncmp(p, "pid ", 4) == 0){
			sigpid = strtol(p+4, 0, 10);
			n = 4+12;
		}else if(n >= 5 && strncmp(p, "note ", 5) == 0){
			if(n >= 5+9 && strncmp(p+5, "interrupt", 9) == 0)
				isig(tio.c_cc[VINTR]);
			else
				isig(tio.c_cc[VQUIT]);
		}else
			return "bad control";
	}
	tfc.count = n;
	return 0;
}

static char*
rstat(long q)
{
	if(q == Qroot)
		q = 0;
	memcpy(tfc.stat, statbufs + q*DIRLEN, DIRLEN);
	return 0;
}

static void
mtocadd(char c)
{
	addchars(&mtoc, &c, 1);
}

static void
ctomadd(char c)
{
	static char cr = '\r';

	if(c == '\n' && (tio.c_oflag & (ONLCR|OPOST)) == (ONLCR|OPOST))
		addchars(&ctom, &cr, 1);
	addchars(&ctom, &c, 1);
}

static void
inorm(char c)
{
	if(tio.c_lflag & ECHO)
		ctomadd(c);
	mtocadd(c);
}

static void
inocanon(char c)
{
	if(tio.c_lflag & ECHO)
		ctomadd(c);
	mtocadd(c);
	checkpend(&mtoc, 0);
}

static void
ierase(char c)
{
	char *p;

	p = mtoc.end - 1;
	if(p >= mtoc.cur && *p != '\n' && *p != eol){
		mtoc.end = p;
		if(tio.c_lflag & ECHOE){
			ctomadd('\b');
			ctomadd(' ');
			ctomadd('\b');
		}
	}
}

static void
ikill(char c)
{
	char *p;

	if(tio.c_lflag & ECHOK){
		ctomadd(c);
		ctomadd('\n');
	}
	p = mtoc.end - 1;
	while(p >= mtoc.cur && *p != '\n' && *p != eol){
		mtoc.end = p;
		p--;
	}
}

static void
ieof(char c)
{
	if(tio.c_lflag & ECHO)
		ctomadd(c);
/* fprintf(stderr, "ieof, so mtoc.eof == 1\n"); /**/
	mtoc.eof = 1;
	checkpend(&mtoc, 1);
}

static void
ieol(char c)
{
	if(tio.c_lflag & ECHO)
		ctomadd(c);
	mtocadd(c);
	checkpend(&mtoc, tio.c_lflag&ICANON);
}

static void
isig(char c)
{
	char nbuf[30];

	if(sigpid){
		if(sigfd < 0){
			sprintf(nbuf, "/proc/%d/notepg", sigpid);
			sigfd = open(nbuf, OWRITE);
		}
		if(sigfd >= 0){
			if(c == tio.c_cc[VINTR])
				write(sigfd, "interrupt", 9);
			else
				write(sigfd, "die", 3);
		}
	}
}

static void
iflow(char c)
{
	if(c == tio.c_cc[VSTART]){
		suspendout = 0;
		checkpend(&ctom, 0);
	}else
		suspendout = 1;
}

/*
 * The Plan9 kernel interchanges reports '\n' when the 'return'
 * key is pressed, and '\r' when the 'line feed' key is pressed.
 */
static void
icr(char c)	/* user hit 'line feed' key */
{
	if(!(tio.c_iflag & INLCR))
		c = '\n';
	if(tio.c_lflag & ECHO)
		ctomadd(c);
	mtocadd(c);
	if(c == '\n' || !(tio.c_lflag & ICANON))
		checkpend(&mtoc, tio.c_lflag&ICANON);
}

static void
inl(char c)	/* user hit 'return' key */
{
	if(tio.c_iflag & IGNCR)
		return;
	if(!(tio.c_iflag & ICRNL))
		c = '\r';
	if(tio.c_lflag & ECHO)
		ctomadd(c);
	mtocadd(c);
	if(c == '\n' || !(tio.c_lflag & ICANON))
		checkpend(&mtoc, tio.c_lflag&ICANON);
}


static void
addchars(Dbuffer *b, char *from, int n)
{
	int v;

	while(b->size - (b->end - b->data) < n){
		v = b->cur - b->data;
		if(v > 0){
			memcpy(b->data, b->cur, v);
			b->cur = b->data;
			b->end -= v;
		}else{
			v = b->end - b->data;
			b->data = realloc(b->data, 2*b->size);
			if(!b->data)
				error("can't grow buffer");
			b->cur = b->data;
			b->end = b->data + v;
			b->size *= 2;
		}
		
	}
	memcpy(b->end, from, n);
	b->end += n;
}

static void
checkpend(Dbuffer *b, int canon)
{
	char *p;
	long n;
	Fcall fc;

/* fprintf(stderr, "checkpend %s, readn=%d\n", b==&ctom ? "ctom" :"mtoc", b->readn); /**/
	n = 0;
	if(b->readn == 0)
		return;
	if(canon){
		for(p = b->cur; p < b->end; p++)
			if(*p == '\n' || *p == eol){
				n = p - b->cur + 1;
				break;
			}
		if(n == 0 && b->eof)
			n = p - b->cur;
	}else
		n = b->end - b->cur;
/* fprintf(stderr, "n %d, eof %d\n", n, b->eof); /**/
	if(n > 0 || b->eof || (!canon && tio.c_cc[VMIN] == 0)){
		if(n > b->readn)
			n = b->readn;
		fc.tag = b->readtag;
		fc.fid = b->readfid;
		fc.type = Rread;
		fc.count = n;
		fc.data = b->cur;
/* fprintf(stderr, "sending response to tag %d fid %d, %d chars\n", fc.tag, fc.fid, n); /**/
		b->cur += n;
		n = convS2M(&fc, rdata);
		if(write(sfd, rdata, n) != n)
			error("read reply write");
		b->readtag = 0;
		b->readfid = 0;
		b->readn = 0;
		b->eof = 0;
	}
}

static void
error(char *s)
{
	fprintf(stderr, "pcons: %s", s);
	perror("");
	exit(1);
}

static int connect(int, int);

/*
 * If isatty(0), start a pcons and plumb as follows:
 *            +------------------------+
 * fd 0   <-  |                        | <-  underlying consctl
 * fd 1   ->  | cons   (pcons)   mcons | ->  old fd 1
 * fd 2   ->  |                        |
 *            +------------------------+
 */

int
pushtty(void)
{
	int consctl, cons, mcons, f, i;

	if(!isatty(0))
		return -1;
	if(access("/dev/pttyctl", 0) == 0)
		return 0;	/* already there */
	consctl = open("/dev/consctl", O_WRONLY);
	if(write(consctl, "rawon", 5) != 5)
		return -1;
	cons = open("/dev/cons", O_RDONLY);
	if(cons < 0)
		return -1;
	if(_pcons() < 0)
		return -1;

	/* feed mcons from old cons */
	if((mcons = open("/dev/mcons", O_WRONLY)) < 0)
		return -1;
	connect(cons, mcons);
	close(cons);
	close(mcons);

	/* make fd 0 be new cons */
	close(0);
	if(open("/dev/cons", O_RDONLY) != 0)
		return -1;

	f = dup(1);

	/* feed fd 1 into new cons, feed mcons to old fd 1 */
	close(1);
	if(open("/dev/cons", O_WRONLY) != 1)
		return -1;
	if((mcons = open("/dev/mcons", O_RDONLY)) < 0)
		return -1;
	connect(mcons, f);
	close(f);
	close(mcons);

	/* fd fd 2 into new cons */
	close(2);
	if(open("/dev/cons", O_WRONLY) != 2)
		return -1;

	/*
 	 * forkpgrp(0) to protect pcons server and connect processes
	 */
	setsid();
	return 0;
}

static int
connect(int from, int to)
{
	int pid, n, i;
	char buf[8192];

	if((pid = fork()) == 0){
		for(i = 0; i < OPEN_MAX; i++)
			if(i != from && i != to)
				close(i);
		while((n = read(from, buf, sizeof(buf))) >= 0)
			write(to, buf, n);
		exit(0);
	}
	return pid;
}

static void
iosighandler(int sig)
{
	_exit(0);
}

static void
sighandler(int sig)
{
	static int fd = -1;

	if(fd < 0)
		fd = open("/dev/pttyctl", OWRITE);
	if(fd >= 0)
		write(fd, "note interrupt", strlen("note interrupt"));
}

static void
notifyproc(void)
{
	int i;

	switch((noteprocpid = _RFORK(FORKPCS|FORKCFDG))){
	case -1:
		error("can't fork");
	case 0:
		break;
	default:
		return;
	}

	signal(SIGINT, sighandler);
	for(;;)
		sleep(10000);
}
