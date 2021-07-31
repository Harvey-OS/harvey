/*
 * Pseudo tty file system for POSIX emulation
 */

#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#include <tty.h>

#define nil	((void*)0)
#define DBG	if(0)print

typedef struct Fid Fid;
typedef struct Tty Tty;
typedef struct Queue Queue;
typedef struct Req Req;

struct Queue
{
	int	nc;
	int	sz;
	uchar	*ip;
	uchar	*op;
	uchar	*b;
	uchar	*t;
};
Queue*	newq(int size);
int	getq(Queue*);
int	putq(Queue*, uchar c);

struct Req
{
	ushort	tag;
	ushort	fid;
	int	bsz;
	int	count;
	char	*base;
	char	*dp;
	Req	*next;
};

struct Fid
{
	int	fid;
	int	open;
	Fid	*hash;
	Qid	qid;
};
#define TTY(s)		(&tty[((s)->qid.path>>1)-1])
#define SLAVE(s)	(((s)->qid.path&1) == 0)

struct Tty
{
	int	opens;
	int	busy;
	int	eol;
	int	escaped;
	int	col;
	int	mode;
	int	pgrp;
	int	state;

	Queue	*iq;
	Queue	*oq;

	Req	*mrd;
	Req	*mwr;
	Req	*swq;
	Req	*srq;

	Termios	t;
};

void	ttyinit(Tty*);

enum
{
	Stopped,
	Running,

	Alloc,
	Used,

	NFHASH	= 32,
	NTTY	= 32,

	/*
	 * Qid 0 is root
	 * Qid 1 is clone file
	 * Qid for tty are 2/3 for slave/master tty 0 etc.
	 */
	Qroot	= CHDIR,
	Qclone	= 1,
};

int	svfd;
Fid*	hash[NFHASH];
Tty	tty[NTTY];

char svnm[] = "/srv/ptyfs";
char Eexist[] = "file does not exist";

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
void	fatal(char*, ...);
int	fcallconv(void *, Fconv*);
void	slavew(Tty*, Fcall*);
void	slaver(Tty*, Fcall*);
void	mastrd(Tty*, Fcall*);
void	mastwr(Tty*, Fcall*);
void	masread(Tty*);
void	minput(Tty*);
int	rioc(Tty*, Fcall*);
int	wioc(Tty*, Fcall*);
void	RDreply(Req*);
void	WRreply(Req*);
void	note(int, char*);

char	dirbuf[2*NTTY*DIRLEN];

void
main(void)
{
	Fcall f;
	char buf[MAXMSG+MAXFDATA];
	int i, fd, n, sv[2];
	
	remove(svnm);
	if(pipe(sv) < 0)
		fatal("pipe: %r");

	fd = create(svnm, OWRITE, 0666);
	if(fd < 0)
		fatal("create: %r");
	n = sprint(buf, "%d", sv[0]);
	if(write(fd, buf, n) < 0)
		fatal("write: %r");
	close(fd);
	close(sv[0]);
	svfd = sv[1];

	switch(fork()) {
	case 0:
		rfork(RFNOTEG);
		break;
	case -1:
		fatal("fork: %r");
	default:
		exits(0);
	}

	fmtinstall('F', fcallconv);

	for(i = 0; i < 30; i++)
		if(i != svfd)
			close(i);

	for(;;) {
		n = read(svfd, buf, sizeof(buf));
		if(n < 0)
			fatal("read mntpt: %r");
		n = convM2S(buf, &f, n);
		if(n < 0)
			continue;
		DBG("%F\n", &f);
		(*fcall[f.type])(&f);
	}
}

void
reply(Fcall *r, char *error)
{
	int n;
	char rbuf[MAXMSG+MAXFDATA];

	if(error == 0)
		r->type++;
	else {
		r->type = Rerror;
		strncpy(r->ename, error, sizeof(r->ename));
	}
	n = convS2M(r, rbuf);
	if(write(svfd, rbuf, n) < 0)
		fatal("reply: %r");

	DBG("%F\n", r);
}

int
delreq(Req **l, ushort tag)
{
	Req *f;

	for(f = *l; f; f = f->next) {
		if(f->tag == tag) {
			*l = f->next;
			free(f);
			return 1;
		}
		l = &f->next;
	}
	return 0;
}

void
mflush(Fcall *f)
{
	int i;
	Tty *t;

	for(i = 0; i < NTTY; i++) {
		t = &tty[i];
		if(t->opens == 0)
			continue;

		if(delreq(&t->mrd, f->oldtag))
			break;
		if(delreq(&t->mwr, f->oldtag))
			break;
		if(delreq(&t->swq, f->oldtag))
			break;
		if(delreq(&t->srq, f->oldtag))
			break;
	}
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
	if(nf == 0) {
		reply(f, "fid busy");
		return;
	}
	nf->qid = (Qid){Qroot, 0};
	f->qid = nf->qid;
	memset(f->rauth, 0, sizeof(f->rauth));
	reply(f, nil);
}

void
mclone(Fcall *f)
{
	Fid *of, *nf;

	of = lookfid(f->fid);
	if(of == 0) {
		reply(f, "bad fid");
		return;
	}
	nf = newfid(f->newfid);
	if(nf == 0) {
		reply(f, "fid busy");
		return;
	}
	nf->qid = of->qid;
	f->qid = nf->qid;
	reply(f, nil);
}

void
mwalk(Fcall *f)
{
	int nr;
	Fid *nf;

	nf = lookfid(f->fid);
	if(nf == 0) {
		reply(f, "bad fid");
		return;
	}

	if(nf->qid.path != Qroot) {
		reply(f, Eexist);
		return;
	}
	if(strcmp(f->name, ".") == 0) {
		f->qid = nf->qid;
		reply(f, nil);
		return;
	}

	if(strcmp(f->name, "ptyclone") == 0) {
		nf->qid.path = Qclone;
		f->qid = nf->qid;
		reply(f, nil);
		return;		
	}

	nr = atoi(f->name+4);
	if(nr < 0 || nr > NTTY) {
		reply(f, Eexist);
		return;
	}

	if(strncmp(f->name, "ptty", 4) == 0)
		nf->qid.path = (nr*2)+2;
	else
	if(strncmp(f->name, "ttym", 4) == 0)
		nf->qid.path = (nr*2)+3;
	else
		reply(f, Eexist);

	f->qid = nf->qid;
	reply(f, nil);
}

void
mopen(Fcall *f)
{
	int i;
	Fid *of;
	Tty *t;

	of = lookfid(f->fid);
	if(of == 0) {
		reply(f, "bad fid");
		return;
	}
	switch(of->qid.path) {
	case Qroot:
		break;		
	case Qclone:
		for(i = 0; i < NTTY; i++)
			if(tty[i].opens == 0)
				break;
		if(i >= NTTY) {
			reply(f, "no free ptty devices");
			return;		
		}
		of->open = 1;
		of->qid.path = (2*i)+3;
		t = &tty[i];
		t->busy = 1;
		t->opens = 1;
		ttyinit(t);
		f->qid = of->qid;
		break;
	default:
		t = TTY(of);
		if(SLAVE(of)) {
			if(t->busy == 0) {
				reply(f, "master closed");
				return;
			}
			t->state = Used;
		}
		else {
			if(t->busy != 0) {
				reply(f, "device in use");
				return;
			}
			t->busy = 1;
			ttyinit(t);
		}
		of->open = 1;
		t->opens++;
		f->qid = of->qid;
	}
	reply(f, nil);
}

void
statgen(Qid *q, char *buf)
{
	int n;
	Tty *t;
	Dir dir;

	strcpy(dir.uid, "none");
	strcpy(dir.gid, "none");
	dir.qid = *q;
	dir.length = 0;
	dir.hlength = 0;
	dir.atime = time(nil);
	dir.mtime = dir.atime;
	memset(dir.name, 0, NAMELEN);

	switch(q->path) {
	case Qroot:
		strcpy(dir.name, ".");
		dir.mode = CHDIR|0555;
		break;
	case Qclone:
		strcpy(dir.name, "ptyclone");
		dir.mode = 0666;
		break;
	default:
		n = (q->path-2)/2;
		t = &tty[n];
		dir.mode = 0666;
		dir.dev = n;
		if(q->path&1) {
			sprint(dir.name, "ttym%d", n);
			dir.length = t->oq->nc;
		}
		else {
			sprint(dir.name, "ptty%d", n);
			dir.length =  t->iq->nc;
		}
	}
	convD2M(&dir, buf);
}

void
mread(Fcall *f)
{
	Fid *of;
	Tty *t;
	Qid qid;
	int n, o, i;
	char *p;

	of = lookfid(f->fid);
	if(of == 0) {
		reply(f, "bad fid");
		return;
	}

	switch(of->qid.path) {
	case Qroot:
		qid = (Qid){Qclone, 0};
		statgen(&qid, dirbuf);
		p = dirbuf+DIRLEN;
		o = DIRLEN;
		for(i = 0; i < NTTY; i++) {
			if(tty[i].opens) {
				qid = (Qid){i+2, 0};
				statgen(&qid, p);
				p += DIRLEN;
				o += DIRLEN;
				qid = (Qid){i+3, 0};
				statgen(&qid, p);
				p += DIRLEN;
				o += DIRLEN;
			}
		}
		n = f->count;
		if(n+f->offset > o)
			n = o-f->offset;
		if(n < 0)
			n = 0;
		f->data = dirbuf+f->offset;
		f->count = n;
		reply(f, nil);
		break;
	case Qclone:
		fatal("clone read");
	default:
		t = TTY(of);
		if(f->offset == -2) {
			if(rioc(t, f))
				return;
		}

		if(SLAVE(of))
			slaver(t, f);
		else
			mastrd(t, f);
		break;
	}
}

void
mwrite(Fcall *f)
{
	Fid *of;
	Tty *t;

	of = lookfid(f->fid);
	if(of == 0) {
		reply(f, "bad fid");
		return;
	}

	switch(of->qid.path) {
	case Qclone:
	case Qroot:
		fatal("write");
	default:
		t = TTY(of);
		if(f->offset == -2) {
			if(wioc(t, f))
				return;
		}
		if(SLAVE(of))
			slavew(t, f);
		else
			mastwr(t, f);
		break;
	}
}

void
freereq(Req **l, void (*f)(Req*))
{
	Req *r, *next;
	
	for(r = *l; r; r = next) {
		next = r->next;
		f(r);
	}
	*l = 0;
}

void
mclunk(Fcall *f)
{
	Tty *t;
	Fid *of;

	of = lookfid(f->fid);
	if(of == 0) {
		reply(f, "bad fid");
		return;
	}
	if(of->open) {
		t = TTY(of);
		if(!SLAVE(of)) {
			t->busy = 0;
			freereq(&t->srq, RDreply);
			freereq(&t->swq, WRreply);
		}
		t->opens--;
		if(t->busy && t->opens == 1) {
			freereq(&t->mrd, RDreply);
			freereq(&t->mwr, WRreply);
		}
		if(t->opens == 0) {
			free(t->iq);
			free(t->oq);
		}
	}
	delfid(f);
	reply(f, nil);
}

void
mstat(Fcall *f)
{
	Fid *of;

	of = lookfid(f->fid);
	if(of == 0) {
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
		return 0;

	nf = malloc(sizeof(Fid));
	if(nf == 0)
		fatal("no memory");
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

	return 0;
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

void
fatal(char *fmt, ...)
{
	char buf[128];

	doprint(buf, buf+sizeof(buf), fmt, (&fmt+1));
	fprint(2, "ptyfs: (fatal problem) %s\n", buf);
	exits(buf);
}

void
WRreply(Req *r)
{
	int n;
	Fcall f;
	char buf[MAXMSG];

	f.tag = r->tag;
	f.fid = r->fid;
	f.type = Rwrite;
	f.count = r->count;

	n = convS2M(&f, buf);
	if(write(svfd, buf, n) < 0)
		fatal("write: %r");

	DBG("WRreply: %F\n", &f);
	free(r);
}

void
RDreply(Req *r)
{
	int n;
	Fcall f;
	char buf[MAXMSG+MAXFDATA];

	f.tag = r->tag;
	f.fid = r->fid;
	f.type = Rread;
	f.count = r->count;
	f.data = r->base;

	n = convS2M(&f, buf);
	if(write(svfd, buf, n) < 0)
		fatal("write: %r");

	DBG("RDreply: %F\n", &f);
	free(r);
}

Req*
append(Fcall *f, Req **h)
{
	Req *r, *l;

	r = malloc(sizeof(Req)+f->count);
	r->base = (char*)r+sizeof(Req);
	r->dp = r->base;
	r->count = 0;
	r->bsz = f->count;
	r->tag = f->tag;
	r->fid = f->fid;
	r->next = 0;

	if(*h == 0)
		*h = r;
	else {
		for(l = *h; l->next; l = l->next)
			;
		l->next = r;
	}
	return r;
}

int
rrq(Req **h)
{
	int c;
	Req *r;

	for(;;) {
		r = *h;
		if(r == 0)
			return -1;
		if(r->bsz == 0) {
			*h = r->next;
			WRreply(r);
			continue;
		}
		c = *r->dp++;
		r->count++;
		r->bsz--;
		if(r->bsz == 0) {
			*h = r->next;
			WRreply(r);
		}
		return c;		
	}
}

void
sinput(Tty *t)
{
	int c, tab;

	for(;;) {
		c = rrq(&t->swq);
		if(c < 0)
			break;
		if((t->t.oflag & OPOST) == 0) {
			if(putq(t->oq, c) < 0)
				break;
			continue;
		}
		switch(c) {
		case '\n':
			if(t->t.oflag & ONLCR) {
				if(putq(t->oq, '\r') < 0)
					goto done;
			}
			if(putq(t->oq, c) < 0)
				return;
			break;
			
		case '\t':
			if((t->t.oflag & TAB3) == TAB3) {
				tab = 8 - (t->col%8);
				while(tab--)
					putq(t->oq, ' ');
				break;
			}
			/* Fall Through */
		default:
			if(t->t.oflag & OLCUC)
				if(c >= 'a' && c <= 'z')
					c = 'A' + (c-'a');
			if(putq(t->oq, c) < 0)
				goto done;
		}
	}
done:
	if(t->mrd)
		masread(t);
}

void
masread(Tty *t)
{
	int c;
	Req *r;

	r = t->mrd;
	while(r) {
		c = getq(t->oq);
		if(c < 0)
			break;
		*r->dp++ = c;
		r->count++;
		r->bsz--;
		if(r->bsz == 0) {
			t->mrd = r->next;
			RDreply(r);
			r = t->mrd;
		}		
	}
	if(r && r->count) {
		t->mrd = r->next;
		RDreply(r);
	}
	if(t->swq)
		sinput(t);
}

void
echo(Tty *t, uchar c)
{
	int tab;

	if(t->t.lflag & ECHO) {
		switch(c) {
		case '\r':
			if(t->t.oflag & OCRNL) {
				putq(t->oq, '\n');
				break;
			}
			putq(t->oq, c);
			break;
		case '\n':
			if(t->t.oflag & ONLCR)
				putq(t->oq, '\r');
			putq(t->oq, '\n');
			break;
		case '\t':
			if((t->t.oflag & TAB3) == TAB3) {
				tab = 8 - (t->col%8);
				while(tab--)
					putq(t->oq, ' ');
				break;
			}
			/* Fall Through */
		default:
			putq(t->oq, c);
			break;
		}
	}
	else
	if(c == '\n' && (t->t.lflag&(ECHONL|ICANON)) == (ECHONL|ICANON)) {
		if(t->t.oflag & ONLCR)
			putq(t->oq, '\r');
		putq(t->oq, '\n');
	}
}

void
soutput(Tty *t)
{
	Req *r;
	int c, canon;

	canon = t->t.lflag & ICANON;
	if(canon && t->eol == 0)
		return;

	r = t->srq;
	while(r) {
		c = getq(t->iq);
		if(c < 0)
			break;
		if(c == 0 || c == '\n') {
			t->eol--;
			if(canon) {
				if(c == '\n') {
					*r->dp = c;
					r->count++;
				}
				t->srq = r->next;
				RDreply(r);
				r = t->srq;
				continue;
			}
		}
		*r->dp++ = c;
		r->count++;
		r->bsz--;
		if(r->bsz == 0) {
			t->srq = r->next;
			RDreply(r);
			r = t->srq;
		}
	}
	masread(t);
	if(t->mwr)
		minput(t);
}

void
flushiput(Tty *t)
{
	Queue *q;

	t->eol = 0;
	t->escaped = 0;

	q = t->iq;
	q->nc = 0;
	q->op = q->b;
	q->ip = q->b;
}

int
bs(Tty *t)
{
	char c;
	Queue *q;

	q = t->iq;
	if(q->nc == 0)
		return 0;

	if(q->ip == q->b)
		c = q->b[q->sz-1];
	else
		c = q->ip[-1];

	if(c == 0 || c == '\n')
		return 0;

	q->nc--;
	
	if(q->ip == q->b)
		q->ip = &q->b[q->sz-1];
	else
		q->ip--;

	echo(t, '\b');
	if(t->t.lflag & ECHOE) {
		echo(t, ' ');
		echo(t, '\b');
	}
	return 1;
}

void
minput(Tty *t)
{
	int c;
	char *sig;

	for(;;) {
		c = rrq(&t->mwr);
		if(c < 0)
			break;
		if(c == 0)
			continue;   

		if(t->t.iflag & ISTRIP)
			c &= 0177;

		if((t->t.iflag & IXON) && c == t->t.cc[CSTOP]) {
			t->mode = Stopped;
			break;
		}

		switch(c) {
		case '\r':
			if(t->t.iflag & IGNCR)
				continue;
			if(t->t.iflag & ICRNL)
				c = '\n';
			break;
		case '\n':
			if(t->t.iflag&INLCR)
				c = '\r';
			break;
		}

		if(t->t.lflag & ISIG) {
			sig = 0;
			if(c == t->t.cc[VINTR])
				sig = "interrupt";
			if(c == t->t.cc[VQUIT])
				sig = "quit";
			if(sig) {
				t->mode = Running;
				if((t->t.lflag & NOFLSH) == 0)
					flushiput(t);
				if(t->pgrp)
					note(t->pgrp, sig);

				continue;
			}
		}

		if((t->t.lflag & ICANON) && t->escaped == 0) {
			if(c == t->t.cc[VERASE]) {
				bs(t);
				continue;
			}
			if(c == t->t.cc[VKILL]) {
				while(bs(t))
					;
				if(t->t.lflag & ECHOK)
					echo(t, '\n');
				continue;
			}
		}

		if(t->escaped == 0 && (c == t->t.cc[VEOF] || c == '\n'))
			t->eol++;

		if((t->t.lflag & ICANON) == 0) {
			echo(t, c);
			putq(t->iq, c);
			continue;
		}

		if(t->escaped) 
			echo(t, '\b');

		if(c != t->t.cc[VEOF])
			echo(t, c);

		if(c != '\\') {
			if(c == t->t.cc[VEOF]) 
				c = 0;
			putq(t->iq, c);
			t->escaped = 0;
			continue;
		}
		if(t->escaped) {
			putq(t->iq, '\\');
			t->escaped = 0;
		}
		else
			t->escaped = 1;
	}
	masread(t);
	if(t->srq)
		soutput(t);
}

void
mastwr(Tty *t, Fcall *f)
{
	Req *r;

	if(t->opens == 1) {
		reply(f, "write on closed ptty");
		return;
	}
	r = append(f, &t->mwr);
	memmove(r->base, f->data, r->bsz);
	minput(t);
}

void
slaver(Tty *t, Fcall *f)
{
	if(t->busy == 0) {
		f->count = 0;
		reply(f, nil);
		return;
	}

	append(f, &t->srq);
	soutput(t);
}

void
mastrd(Tty *t, Fcall *f)
{
	if(t->opens == 1 && t->state == Used) {
		reply(f, "read on closed ptty");
		return;
	}
	append(f, &t->mrd);
	masread(t);
}

void
slavew(Tty *t, Fcall *f)
{
	Req *r;

	if(t->busy == 0) {
		f->count = 0;
		reply(f, nil);
		return;
	}

	r = append(f, &t->swq);
	memmove(r->base, f->data, r->bsz);
	sinput(t);
}

void
ttyinit(Tty *t)
{
	t->t.iflag = ISTRIP|ICRNL|IXON|IXOFF;
	t->t.oflag = OPOST|TAB3|ONLCR;
	t->t.cflag = B9600;
	t->t.lflag = ISIG|ICANON|ECHO|ECHOE|ECHOK;
	t->t.cc[VINTR] = CINTR;
	t->t.cc[VQUIT] = CQUIT;
	t->t.cc[VERASE] = CERASE;
	t->t.cc[VKILL] = CKILL;
	t->t.cc[VEOF] = CEOF;
	t->t.cc[VEOL] = CEOL;
	t->t.cc[VSTART] = CSTART;
	t->t.cc[VSTOP] = CSTOP;

	t->eol = 0;
	t->escaped = 0;
	t->col = 0;
	t->mode = Running;
	t->pgrp = 0;

	t->iq = newq(MAXFDATA);
	t->oq = newq(MAXFDATA);

	t->state = Alloc;
}

Queue*
newq(int size)
{
	Queue *q;

	q = malloc(sizeof(Queue)+size);
	if(q == 0)
		fatal("no memory");

	q->b = (uchar*)q + sizeof(Queue);
	q->t = q->b+size;
	q->sz = size;
	q->ip = q->b;
	q->op = q->b;
	q->nc = 0;

	return q;
}

int
getq(Queue *q)
{
	uchar c;

	if(q->nc == 0)
		return -1;

	q->nc++;
	c = *q->op++;
	if(q->op >= q->t)
		q->op = q->b;
	return c;
}

int
putq(Queue *q, uchar c)
{
	if(q->nc >= q->sz)
		return -1;

	q->nc--;
	*q->ip++ = c;
	if(q->ip >= q->t)
		q->ip = q->b;

	return 0;
}

int
rioc(Tty *t, Fcall *f)
{
	int n, i;
	char buf[256];

	n = sprint(buf, "IOR %4.4lux %4.4lux %4.4lux %4.4lux ",
		t->t.iflag, t->t.oflag, t->t.cflag, t->t.lflag);
	for(i = 0; i < NCCS; i++)
		n += sprint(buf+n, "%2.2ux ", t->t.cc[i]);

	sprint(buf+n, "%d ", t->pgrp);

	f->data = buf;
	reply(f, nil);
	return 1;
}

int
wioc(Tty *t, Fcall *f)
{
	int n;

	if(strncmp(f->data, "IOW note", 8) == 0) {
		f->data[f->count] = '\0';
		t->pgrp = atoi(f->data+8);
		reply(f, nil);
		return 1;
	}

	if(f->count != 57 || strncmp(f->data, "IOW", 3) != 0)
		return 0;

	t->t.iflag = strtoul(f->data+4, 0, 16);
	t->t.oflag = strtoul(f->data+9, 0, 16);
	t->t.cflag = strtoul(f->data+14, 0, 16);
	t->t.lflag = strtoul(f->data+19, 0, 16);

	for(n = 0; n < NCCS; n++)
		t->t.cc[n] = strtoul(f->data+24+(n*3), 0, 16);

	reply(f, nil);
	return 1;
}

void
ignore(void *a, char *msg)
{
	USED(a, msg);
	noted(NCONT);
}

void
note(int pgrp, char *sig)
{
	int n;
	char buf[128];
	static int mypid, nfd = -1;

	if(nfd < 0) {
		mypid = getpid();
		sprint(buf, "/proc/%d/noteid", mypid);
		nfd = open(buf, OWRITE);
		if(nfd < 0)
			fatal("open %s: %r", buf);
		notify(ignore);
	}
	n = sprint(buf, "%d", pgrp);
	if(write(nfd, buf, n) < 0)
		return;

	postnote(PNGROUP, mypid, sig);
	n = sprint(buf, "%d", mypid);
	write(nfd, buf, n);
}

