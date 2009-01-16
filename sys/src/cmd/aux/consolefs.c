#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#include <bio.h>
#include <ndb.h>
#include <thread.h>

/*
 *  This fs presents a 1 level file system.  It contains
 *  up to three files per console (xxx and xxxctl and xxxstat)
 */

typedef struct Console Console;
typedef struct Fid Fid;
typedef struct Request Request;
typedef struct Reqlist Reqlist;
typedef struct Fs Fs;

enum
{
	/* last 5 bits of qid.path */
	Textern=	0,		/* fake parent of top level */
	Ttopdir,			/* top level directory */
	Qctl,
	Qstat,
	Qdata,

	Bufsize=	32*1024,	/* chars buffered per reader */
	Maxcons=	64,		/* maximum consoles */
	Nhash=		64,		/* Fid hash buckets */
};

#define TYPE(x)		(((ulong)x.path) & 0xf)
#define CONS(x)		((((ulong)x.path) >> 4)&0xfff)
#define QID(c, x)	(((c)<<4) | (x))

struct Request
{
	Request	*next;
	Fid	*fid;
	Fs	*fs;
	Fcall	f;
	uchar	buf[1];
};

struct Reqlist
{
	Lock;
	Request	*first;
	Request *last;
};

struct Fid
{
	Lock;
	Fid	*next;			/* hash list */
	Fid	*cnext;			/* list of Fid's on a console */
	int	fid;
	int	ref;

	int	attached;
	int	open;
	char	*user;
	char	mbuf[Bufsize];		/* message */
	int	bufn;
	int	used;
	Qid	qid;

	Console	*c;

	char	buf[Bufsize];
	char	*rp;
	char	*wp;

	Reqlist	r;			/* active read requests */
};

struct Console
{
	Lock;

	char	*name;
	char	*dev;
	int	speed;
	int	cronly;
	int	ondemand;		/* open only on demand */
	int	chat;			/* chat consoles are special */

	int	pid;			/* pid of reader */

	int	fd;
	int	cfd;
	int	sfd;

	Fid	*flist;			/* open fids to broadcast to */
};

struct Fs
{
	Lock;

	int	fd;			/* to kernel mount point */
	int	messagesize;
	Fid	*hash[Nhash];
	Console	*cons[Maxcons];
	int	ncons;
};

extern	void	console(Fs*, char*, char*, int, int, int);
extern	Fs*	fsmount(char*);

extern	void	fsreader(void*);
extern	void	fsrun(void*);
extern	Fid*	fsgetfid(Fs*, int);
extern	void	fsputfid(Fs*, Fid*);
extern	int	fsdirgen(Fs*, Qid, int, Dir*, uchar*, int);
extern	void	fsreply(Fs*, Request*, char*);
extern	void	fskick(Fs*, Fid*);
extern	int	fsreopen(Fs*, Console*);

extern	void	fsversion(Fs*, Request*, Fid*);
extern	void	fsflush(Fs*, Request*, Fid*);
extern	void	fsauth(Fs*, Request*, Fid*);
extern	void	fsattach(Fs*, Request*, Fid*);
extern	void	fswalk(Fs*, Request*, Fid*);
extern	void	fsclwalk(Fs*, Request*, Fid*);
extern	void	fsopen(Fs*, Request*, Fid*);
extern	void	fscreate(Fs*, Request*, Fid*);
extern	void	fsread(Fs*, Request*, Fid*);
extern	void	fswrite(Fs*, Request*, Fid*);
extern	void	fsclunk(Fs*, Request*, Fid*);
extern	void	fsremove(Fs*, Request*, Fid*);
extern	void	fsstat(Fs*, Request*, Fid*);
extern	void	fswstat(Fs*, Request*, Fid*);


void 	(*fcall[])(Fs*, Request*, Fid*) =
{
	[Tflush]	fsflush,
	[Tversion]	fsversion,
	[Tauth]	fsauth,
	[Tattach]	fsattach,
	[Twalk]		fswalk,
	[Topen]		fsopen,
	[Tcreate]	fscreate,
	[Tread]		fsread,
	[Twrite]	fswrite,
	[Tclunk]	fsclunk,
	[Tremove]	fsremove,
	[Tstat]		fsstat,
	[Twstat]	fswstat
};

char Eperm[] = "permission denied";
char Eexist[] = "file does not exist";
char Enotdir[] = "not a directory";
char Eisopen[] = "file already open";
char Ebadcount[] = "bad read/write count";
char Enofid[] = "no such fid";

char *consoledb = "/lib/ndb/consoledb";
char *mntpt = "/mnt/consoles";

int messagesize = 8192+IOHDRSZ;

void
fatal(char *fmt, ...)
{
	va_list arg;
	char buf[1024];

	write(2, "consolefs: ", 10);
	va_start(arg, fmt);
	vseprint(buf, buf+1024, fmt, arg);
	va_end(arg);
	write(2, buf, strlen(buf));
	write(2, "\n", 1);
	threadexitsall(fmt);
}


void*
emalloc(uint n)
{
	void *p;

	p = malloc(n);
	if(p == nil)
		fatal("malloc failed: %r");
	memset(p, 0, n);
	return p;
}

int debug;
Ndb *db;

/*
 *  any request that can get queued for a delayed reply
 */
Request*
allocreq(Fs *fs, int bufsize)
{
	Request *r;

	r = emalloc(sizeof(Request)+bufsize);
	r->fs = fs;
	r->next = nil;
	return r;
}

/*
 *  for maintaining lists of requests
 */
void
addreq(Reqlist *l, Request *r)
{
	lock(l);
	if(l->first == nil)
		l->first = r;
	else
		l->last->next = r;
	l->last = r;
	r->next = nil;
	unlock(l);
}

/*
 *  remove the first request from a list of requests
 */
Request*
remreq(Reqlist *l)
{
	Request *r;

	lock(l);
	r = l->first;
	if(r != nil)
		l->first = r->next;
	unlock(l);
	return r;
}

/*
 *  remove a request with the given tag from a list of requests
 */
Request*
remtag(Reqlist *l, int tag)
{
	Request *or, **ll;

	lock(l);
	ll = &l->first;
	for(or = *ll; or; or = or->next){
		if(or->f.tag == tag){
			*ll = or->next;
			unlock(l);
			return or;
		}
		ll = &or->next;
	}
	unlock(l);
	return nil;
}

Qid
parentqid(Qid q)
{
	if(q.type & QTDIR)
		return (Qid){QID(0, Textern), 0, QTDIR};
	else
		return (Qid){QID(0, Ttopdir), 0, QTDIR};
}

int
fsdirgen(Fs *fs, Qid parent, int i, Dir *d, uchar *buf, int nbuf)
{
	static char name[64];
	char *p;
	int xcons;

	d->uid = d->gid = d->muid = "network";
	d->length = 0;
	d->atime = time(nil);
	d->mtime = d->atime;
	d->type = 'C';
	d->dev = '0';

	switch(TYPE(parent)){
	case Textern:
		if(i != 0)
			return -1;
		p = "consoles";
		d->mode = DMDIR|0555;
		d->qid.type = QTDIR;
		d->qid.path = QID(0, Ttopdir);
		d->qid.vers = 0;
		break;
	case Ttopdir:
		xcons = i/3;
		if(xcons >= fs->ncons)
			return -1;
		p = fs->cons[xcons]->name;
		switch(i%3){
		case 0:
			if(fs->cons[xcons]->cfd < 0)
				return 0;
			snprint(name, sizeof name, "%sctl", p);
			p = name;
			d->qid.type = QTFILE;
			d->qid.path = QID(xcons, Qctl);
			d->qid.vers = 0;
			break;
		case 1:
			if(fs->cons[xcons]->sfd < 0)
				return 0;
			snprint(name, sizeof name, "%sstat", p);
			p = name;
			d->qid.type = QTFILE;
			d->qid.path = QID(xcons, Qstat);
			d->qid.vers = 0;
			break;
		case 2:
			d->qid.type = QTFILE;
			d->qid.path = QID(xcons, Qdata);
			d->qid.vers = 0;
			break;
		}
		d->mode = 0666;
		break;
	default:
		return -1;
	}
	d->name = p;
	if(buf != nil)
		return convD2M(d, buf, nbuf);
	return 1;
}

/*
 *  mount the user interface and start a request processor
 */
Fs*
fsmount(char *mntpt)
{
	Fs *fs;
	int pfd[2], srv;
	char buf[32];
	int n;
	static void *v[2];

	fs = emalloc(sizeof(Fs));

	if(pipe(pfd) < 0)
		fatal("opening pipe: %r");

	/* start up the file system process */
	v[0] = fs;
	v[1] = pfd;
	proccreate(fsrun, v, 16*1024);

	/* Typically mounted before /srv exists */
	if(access("#s/consoles", AEXIST) < 0){
		srv = create("#s/consoles", OWRITE, 0666);
		if(srv < 0)
			fatal("post: %r");

		n = sprint(buf, "%d", pfd[1]);
		if(write(srv, buf, n) < 0)
			fatal("write srv: %r");

		close(srv);
	}

	mount(pfd[1], -1, mntpt, MBEFORE, "");
	close(pfd[1]);
	return fs;
}

/*
 *  reopen a console
 */
int
fsreopen(Fs* fs, Console *c)
{
	char buf[128];
	static void *v[2];

	if(c->pid){
		if(postnote(PNPROC, c->pid, "reopen") != 0)
			fprint(2, "postnote failed: %r\n");
		c->pid = 0;
	}

	if(c->fd >= 0){
		close(c->fd);
		close(c->cfd);
		close(c->sfd);
		c->cfd = -1;
		c->fd = -1;
		c->sfd = -1;
	}

	if(c->flist == nil && c->ondemand)
		return 0;

	c->fd = open(c->dev, ORDWR);
	if(c->fd < 0)
		return -1;

	snprint(buf, sizeof(buf), "%sctl", c->dev);
	c->cfd = open(buf, ORDWR);
	fprint(c->cfd, "b%d", c->speed);

	snprint(buf, sizeof(buf), "%sstat", c->dev);
	c->sfd = open(buf, OREAD);

	v[0] = fs;
	v[1] = c;
	proccreate(fsreader, v, 16*1024);

	return 0;
}

void
change(Fs *fs, Console *c, int doreopen, int speed, int cronly, int ondemand)
{
	lock(c);

	if(speed != c->speed){
		c->speed = speed;
		doreopen = 1;
	}
	if(ondemand != c->ondemand){
		c->ondemand = ondemand;
		doreopen = 1;
	}
	c->cronly = cronly;
	if(doreopen)
		fsreopen(fs, c);

	unlock(c);
}

/*
 *  create a console interface
 */
void
console(Fs* fs, char *name, char *dev, int speed, int cronly, int ondemand)
{
	Console *c;
	char *x;
	int i, doreopen;

	if(fs->ncons >= Maxcons)
		fatal("too many consoles, too little time");

	doreopen = 0;
	for(i = 0; i < fs->ncons; i++){
		c = fs->cons[i];
		if(strcmp(name, c->name) == 0){
			if(strcmp(dev, c->dev) != 0){
				/* new device */
				x = c->dev;
				c->dev = strdup(dev);
				free(x);
				doreopen = 1;
			}
			change(fs, c, doreopen, speed, cronly, ondemand);
			return;
		}
	}
#ifdef sapedoesntlikethis
	/*
	 * The code below prevents this from working.  I can't
	 * think of scenarios where the code below actually helps
	 *	Sape
	 *
	 * console=borneo dev=/dev/eia1
	 * 	speed=9600
	 * 	openondemand=1
	 * console=tottie dev=/dev/eia1
	 * 	speed=115200
	 * 	openondemand=1
	 */
	for(i = 0; i < fs->ncons; i++){
		c = fs->cons[i];
		if(strcmp(dev, c->dev) == 0){
			/* at least a rename */
			x = c->name;
			c->name = strdup(name);
			free(x);
			change(fs, c, doreopen, speed, cronly, ondemand);
			return;
		}
	}
#endif
	c = emalloc(sizeof(Console));
	fs->cons[fs->ncons] = c;
	fs->ncons++;
	c->name = strdup(name);
	c->dev = strdup(dev);
	if(strcmp(c->dev, "/dev/null") == 0) 
		c->chat = 1;
	else 
		c->chat = 0;
	c->fd = -1;
	c->cfd = -1;
	c->sfd = -1;
	change(fs, c, 1, speed, cronly, ondemand);
}

/*
 *  buffer data from console to a client.
 *  circular q with writer able to catch up to reader.
 *  the reader may miss data but always sees an in order sequence.
 */
void
fromconsole(Fid *f, char *p, int n)
{
	char *rp, *wp, *ep;
	int pass;

	lock(f);
	rp = f->rp;
	wp = f->wp;
	ep = f->buf + sizeof(f->buf);
	pass = 0;
	while(n--){
		*wp++ = *p++;
		if(wp >= ep)
			wp = f->buf;
		if(rp == wp)
			pass = 1;
	}
	f->wp = wp;

	/*  we overtook the read pointer, push it up so readers always
	 *  see the tail of what was written
	 */
	if(pass){
		wp++;
		if(wp >= ep)
			f->rp = f->buf;
		else
			f->rp = wp;
	}
	unlock(f);
}

/*
 *  broadcast a list of members to all listeners
 */
void
bcastmembers(Fs *fs, Console *c, char *msg, Fid *f)
{
	int n;
	Fid *fl;
	char buf[512];

	sprint(buf, "[%s%s", msg, f->user);
	for(fl = c->flist; fl != nil && strlen(buf) + 64 < sizeof(buf); fl = fl->cnext){
		if(f == fl)
			continue;
		strcat(buf, ", ");
		strcat(buf, fl->user);
	}
	strcat(buf, "]\n");

	n = strlen(buf);
	for(fl = c->flist; fl; fl = fl->cnext){
		fromconsole(fl, buf, n);
		fskick(fs, fl);
	}
}

void
handler(void*, char *msg)
{
	if(strstr(msg, "reopen") != nil ||
	   strstr(msg, "write on closed pipe") != nil)
		noted(NCONT);
	noted(NDFLT);
}

/*
 *  a process to read console output and broadcast it (one per console)
 */
void
fsreader(void *v)
{
	int n;
	Fid *fl;
	char buf[1024];
	Fs *fs;
	Console *c;
	void **a;

	a = v;
	fs = a[0];
	c = a[1];
	c->pid = getpid();
	notify(handler);
	if(c->chat)
		threadexits(nil);
	for(;;){
		n = read(c->fd, buf, sizeof(buf));
		if(n < 0)
			break;
		lock(c);
		for(fl = c->flist; fl; fl = fl->cnext){
			fromconsole(fl, buf, n);
			fskick(fs, fl);
		}
		unlock(c);
	}
}

void
readdb(Fs *fs)
{
	Ndbtuple *t, *nt;
	char *dev, *cons;
	int cronly, speed, ondemand;

	ndbreopen(db);

	/* start a listener for each console */
	for(;;){
		t = ndbparse(db);
		if(t == nil)
			break;
		dev = nil;
		cons = nil;
		speed = 9600;
		cronly = 0;
		ondemand = 0;
		for(nt = t; nt; nt = nt->entry){
			if(strcmp(nt->attr, "console") == 0)
				cons = nt->val;
			else if(strcmp(nt->attr, "dev") == 0)
				dev = nt->val;
			else if(strcmp(nt->attr, "speed") == 0)
				speed = atoi(nt->val);
			else if(strcmp(nt->attr, "cronly") == 0)
				cronly = 1;
			else if(strcmp(nt->attr, "openondemand") == 0)
				ondemand = 1;
		}
		if(dev != nil && cons != nil)
			console(fs, cons, dev, speed, cronly, ondemand);
		ndbfree(t);
	}
}

int dbmtime;

/*
 *  a request processor (one per Fs)
 */
void
fsrun(void *v)
{
	int n, t;
	Request *r;
	Fid *f;
	Dir *d;
	void **a = v;
	Fs* fs;
	int *pfd;

	fs = a[0];
	pfd = a[1];
	fs->fd = pfd[0];
	notify(handler);
	for(;;){
		d = dirstat(consoledb);
		if(d != nil && d->mtime != dbmtime){
			dbmtime = d->mtime;
			readdb(fs);
		}
		free(d);
		r = allocreq(fs, messagesize);
		n = read9pmsg(fs->fd, r->buf, messagesize);
		if(n <= 0)
			fatal("unmounted");

		if(convM2S(r->buf, n, &r->f) == 0){
			fprint(2, "can't convert %ux %ux %ux\n", r->buf[0],
				r->buf[1], r->buf[2]);
			free(r);
			continue;
		}


		f = fsgetfid(fs, r->f.fid);
		r->fid = f;
		if(debug)
			fprint(2, "%F path %llux\n", &r->f, f->qid.path);

		t = r->f.type;
		r->f.type++;
		(*fcall[t])(fs, r, f);
	}
}

Fid*
fsgetfid(Fs *fs, int fid)
{
	Fid *f, *nf;

	lock(fs);
	for(f = fs->hash[fid%Nhash]; f; f = f->next){
		if(f->fid == fid){
			f->ref++;
			unlock(fs);
			return f;
		}
	}

	nf = emalloc(sizeof(Fid));
	nf->next = fs->hash[fid%Nhash];
	fs->hash[fid%Nhash] = nf;
	nf->fid = fid;
	nf->ref = 1;
	nf->wp = nf->buf;
	nf->rp = nf->wp;
	unlock(fs);
	return nf;
}

void
fsputfid(Fs *fs, Fid *f)
{
	Fid **l, *nf;

	lock(fs);
	if(--f->ref > 0){
		unlock(fs);
		return;
	}
	for(l = &fs->hash[f->fid%Nhash]; nf = *l; l = &nf->next)
		if(nf == f){
			*l = f->next;
			break;
		}
	unlock(fs);
	free(f->user);
	free(f);
}

void
fsauth(Fs *fs, Request *r, Fid*)
{
	fsreply(fs, r, "consolefs: authentication not required");
}

void
fsversion(Fs *fs, Request *r, Fid*)
{

	if(r->f.msize < 256){
		fsreply(fs, r, "message size too small");
		return;
	}
	messagesize = r->f.msize;
	if(messagesize > 8192+IOHDRSZ)
		messagesize = 8192+IOHDRSZ;
	r->f.msize = messagesize;
	if(strncmp(r->f.version, "9P2000", 6) != 0){
		fsreply(fs, r, "unrecognized 9P version");
		return;
	}
	r->f.version = "9P2000";

	fsreply(fs, r, nil);
}

void
fsflush(Fs *fs, Request *r, Fid *f)
{
	Request *or;

	or = remtag(&f->r, r->f.oldtag);
	if(or != nil){
		fsputfid(fs, or->fid);
		free(or);
	}
	fsreply(fs, r, nil);
}

void
fsattach(Fs *fs, Request *r, Fid *f)
{
	f->qid.type = QTDIR;
	f->qid.path = QID(0, Ttopdir);
	f->qid.vers = 0;

	if(r->f.uname[0])
		f->user = strdup(r->f.uname);
	else
		f->user = strdup("none");

	/* hold down the fid till the clunk */
	f->attached = 1;
	lock(fs);
	f->ref++;
	unlock(fs);

	r->f.qid = f->qid;
	fsreply(fs, r, nil);
}

void
fswalk(Fs *fs, Request *r, Fid *f)
{
	char *name;
	Dir d;
	int i, n, nqid, nwname;
	Qid qid, wqid[MAXWELEM];
	Fid *nf;
	char *err;

	if(f->attached == 0){
		fsreply(fs, r, Enofid);
		return;
	}

	nf = nil;
	if(r->f.fid != r->f.newfid){
		nf = fsgetfid(fs, r->f.newfid);
		nf->attached = f->attached;
		nf->open = f->open;
		nf->qid = f->qid;
		nf->user = strdup(f->user);
		nf->c = f->c;
		nf->wp = nf->buf;
		nf->rp = nf->wp;
		f = nf;
	}

	qid = f->qid;
	err = nil;
	nwname = r->f.nwname;
	nqid = 0;
	if(nwname > 0){
		for(; err == nil && nqid < nwname; nqid++){
			if(nqid >= MAXWELEM){
				err = "too many name elements";
				break;
			}
			name = r->f.wname[nqid];
			if(strcmp(name, "..") == 0)
				qid = parentqid(qid);
			else if(strcmp(name, ".") != 0){
				for(i = 0; ; i++){
					n = fsdirgen(fs, qid, i, &d, nil, 0);
					if(n < 0){
						err = Eexist;
						break;
					}
					if(n > 0 && strcmp(name, d.name) == 0){
						qid = d.qid;
						break;
					}
				}
			}
			wqid[nqid] = qid;
		}
		if(nf != nil && nqid < nwname)
			fsputfid(fs, nf);
		if(nqid == nwname)
			f->qid = qid;
	}

	memmove(r->f.wqid, wqid, nqid*sizeof(Qid));
	r->f.nwqid = nqid;
	fsreply(fs, r, err);
}

int
ingroup(char *user, char *group)
{
	Ndbtuple *t, *nt;
	Ndbs s;

	t = ndbsearch(db, &s, "group", group);
	if(t == nil)
		return 0;
	for(nt = t; nt; nt = nt->entry){
		if(strcmp(nt->attr, "uid") == 0)
		if(strcmp(nt->val, user) == 0)
			break;
	}
	ndbfree(t);
	return nt != nil;
}

int
userok(char *u, char *cname)
{
	Ndbtuple *t, *nt;
	Ndbs s;

	t = ndbsearch(db, &s, "console", cname);
	if(t == nil)
		return 0;

	for(nt = t; nt; nt = nt->entry){
		if(strcmp(nt->attr, "uid") == 0)
		if(strcmp(nt->val, u) == 0)
			break;
		if(strcmp(nt->attr, "gid") == 0)
		if(ingroup(u, nt->val))
			break;
	}
	ndbfree(t);

	return nt != nil;
}

int m2p[] ={
	[OREAD]		4,
	[OWRITE]	2,
	[ORDWR]		6
};

/*
 *  broadcast a message to all listeners
 */
void
bcastmsg(Fs *fs, Console *c, char *msg, int n)
{
	Fid *fl;

	for(fl = c->flist; fl; fl = fl->cnext){
		fromconsole(fl, msg, n);
		fskick(fs, fl);
	}
}

void
fsopen(Fs *fs, Request *r, Fid *f)
{
	int mode;
	Console *c;

	if(f->attached == 0){
		fsreply(fs, r, Enofid);
		return;
	}

	if(f->open){
		fsreply(fs, r, Eisopen);
		return;
	}

	mode = r->f.mode & 3;

	if((QTDIR & f->qid.type) && mode != OREAD){
		fsreply(fs, r, Eperm);
		return;
	}

	switch(TYPE(f->qid)){
	case Qdata:
		c = fs->cons[CONS(f->qid)];
		if(!userok(f->user, c->name)){
			fsreply(fs, r, Eperm);
			return;
		}
		f->rp = f->buf;
		f->wp = f->buf;
		f->c = c;
		lock(c);
		sprint(f->mbuf, "[%s] ", f->user);
		f->bufn = strlen(f->mbuf);
		f->used = 0;
		f->cnext = c->flist;
		c->flist = f;
		bcastmembers(fs, c, "+", f);
		if(c->pid == 0)
			fsreopen(fs, c);
		unlock(c);
		break;
	case Qctl:
		c = fs->cons[CONS(f->qid)];
		if(!userok(f->user, c->name)){
			fsreply(fs, r, Eperm);
			return;
		}
		f->c = c;
		break;
	case Qstat:
		c = fs->cons[CONS(f->qid)];
		if(!userok(f->user, c->name)){
			fsreply(fs, r, Eperm);
			return;
		}
		f->c = c;
		break;
	}

	f->open = 1;
	r->f.iounit = messagesize-IOHDRSZ;
	r->f.qid = f->qid;
	fsreply(fs, r, nil);
}

void
fscreate(Fs *fs, Request *r, Fid*)
{
	fsreply(fs, r, Eperm);
}

void
fsread(Fs *fs, Request *r, Fid *f)
{
	uchar *p, *e;
	int i, m, off;
	vlong offset;
	Dir d;
	char sbuf[ERRMAX];

	if(f->attached == 0){
		fsreply(fs, r, Enofid);
		return;
	}

	if((int)r->f.count < 0){
		fsreply(fs, r, Ebadcount);
		return;
	}

	if(QTDIR & f->qid.type){
		p = r->buf + IOHDRSZ;
		e = p + r->f.count;
		offset = r->f.offset;
		off = 0;
		for(i=0; p<e; i++, off+=m){
			m = fsdirgen(fs, f->qid, i, &d, p, e-p);
			if(m < 0)
				break;
			if(m > BIT16SZ && off >= offset)
				p += m;
		}
		r->f.data = (char*)r->buf + IOHDRSZ;
		r->f.count = (char*)p - r->f.data;
	} else {
		switch(TYPE(f->qid)){
		case Qdata:
			addreq(&f->r, r);
			fskick(fs, f);
			return;
		case Qctl:
			r->f.data = (char*)r->buf+IOHDRSZ;
			r->f.count = 0;
			break;
		case Qstat:
			if(r->f.count > sizeof(sbuf))
				r->f.count = sizeof(sbuf);
			i = pread(f->c->sfd, sbuf, r->f.count, r->f.offset);
			if(i < 0){
				errstr(sbuf, sizeof sbuf);
				fsreply(fs, r, sbuf);
				return;
			}
			r->f.data = sbuf;
			r->f.count = i;
			break;
		default:
			fsreply(fs, r, Eexist);
			return;
		}
	}
	fsreply(fs, r, nil);
}

void
fswrite(Fs *fs, Request *r, Fid *f)
{
	int i, eol = 0;

	if(f->attached == 0){
		fsreply(fs, r, Enofid);
		return;
	}

	if((int)r->f.count < 0){
		fsreply(fs, r, Ebadcount);
		return;
	}

	if(QTDIR & f->qid.type){
		fsreply(fs, r, Eperm);
		return;
	}

	switch(TYPE(f->qid)){
	default:
		fsreply(fs, r, Eperm);
		return;
	case Qctl:
		write(f->c->cfd, r->f.data, r->f.count);
		break;
	case Qdata:
		for(i = 0; i < r->f.count; i++){
			if(r->f.data[i] == '\n'){
				if(f->c->chat && f->used)
					eol = 1;
				if(f->c->cronly)
					r->f.data[i] = '\r';
			}
			else
				f->used = 1;
		}
		if(f->c->chat){
			fskick(fs, f);
			if(!f->used)
				break;
	
			if(f->bufn + r->f.count > Bufsize){
				r->f.count -= (f->bufn + r->f.count) % Bufsize;
				eol = 1;
			}
			strncat(f->mbuf, r->f.data, r->f.count);
			f->bufn += r->f.count;
			if(eol){
				bcastmsg(fs, f->c, f->mbuf, f->bufn);
				sprint(f->mbuf, "[%s] ", f->user);
				f->bufn = strlen(f->mbuf);
				f->used = 0;
			}
		}
		else
			write(f->c->fd, r->f.data, r->f.count);
		break;
	}
	fsreply(fs, r, nil);
}

void
fsclunk(Fs *fs, Request *r, Fid *f)
{
	Fid **l, *fl;
	Request *nr;

	if(f->open && TYPE(f->qid) == Qdata){
		while((nr = remreq(&f->r)) != nil){
			fsputfid(fs, f);
			free(nr);
		}

		lock(f->c);
		for(l = &f->c->flist; *l; l = &fl->cnext){
			fl = *l;
			if(fl == f){
				*l = fl->cnext;
				break;
			}
		}
		bcastmembers(fs, f->c, "-", f);
		if(f->c->ondemand && f->c->flist == nil)
			fsreopen(fs, f->c);
		unlock(f->c);
	}
	fsreply(fs, r, nil);
	fsputfid(fs, f);
}

void
fsremove(Fs *fs, Request *r, Fid*)
{
	fsreply(fs, r, Eperm);
}

void
fsstat(Fs *fs, Request *r, Fid *f)
{
	int i, n;
	Qid q;
	Dir d;

	q = parentqid(f->qid);
	for(i = 0; ; i++){
		r->f.stat = r->buf+IOHDRSZ;
		n = fsdirgen(fs, q, i, &d, r->f.stat, messagesize-IOHDRSZ);
		if(n < 0){
			fsreply(fs, r, Eexist);
			return;
		}
		r->f.nstat = n;
		if(r->f.nstat > BIT16SZ && d.qid.path == f->qid.path)
			break;
	}
	fsreply(fs, r, nil);
}

void
fswstat(Fs *fs, Request *r, Fid*)
{
	fsreply(fs, r, Eperm);
}

void
fsreply(Fs *fs, Request *r, char *err)
{
	int n;
	uchar buf[8192+IOHDRSZ];

	if(err){
		r->f.type = Rerror;
		r->f.ename = err;
	}
	n = convS2M(&r->f, buf, messagesize);
	if(debug)
		fprint(2, "%F path %llux n=%d\n", &r->f, r->fid->qid.path, n);
	fsputfid(fs, r->fid);
	if(write(fs->fd, buf, n) != n)
		fatal("unmounted");
	free(r);
}

/*
 *  called whenever input or a read request has been received
 */
void
fskick(Fs *fs, Fid *f)
{
	Request *r;
	char *p, *rp, *wp, *ep;
	int i;

	lock(f);
	while(f->rp != f->wp){
		r = remreq(&f->r);
		if(r == nil)
			break;
		p = (char*)r->buf;
		rp = f->rp;
		wp = f->wp;
		ep = &f->buf[Bufsize];
		for(i = 0; i < r->f.count && rp != wp; i++){
			*p++ = *rp++;
			if(rp >= ep)
				rp = f->buf;
		}
		f->rp = rp;
		r->f.data = (char*)r->buf;
		r->f.count = p - (char*)r->buf;
		fsreply(fs, r, nil);
	}
	unlock(f);
}

void
usage(void)
{
	fprint(2, "usage: consolefs [-d] [-m mount-point] [-c console-db]\n");
	threadexitsall("usage");
}

void
threadmain(int argc, char **argv)
{
	fmtinstall('F', fcallfmt);

	ARGBEGIN{
	case 'd':
		debug++;
		break;
	case 'c':
		consoledb = ARGF();
		if(consoledb == nil)
			usage();
		break;
	case 'm':
		mntpt = ARGF();
		if(mntpt == nil)
			usage();
		break;
	}ARGEND;

	db = ndbopen(consoledb);
	if(db == nil)
 		fatal("can't open %s: %r", consoledb);

	fsmount(mntpt);
}
