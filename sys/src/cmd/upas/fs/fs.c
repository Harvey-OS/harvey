#include "common.h"
#include <auth.h>
#include <fcall.h>
#include <libsec.h>
#include "dat.h"

enum
{
	OPERM	= 0x3,		// mask of all permission types in open mode
};

typedef struct Fid Fid;

struct Fid
{
	Qid	qid;
	short	busy;
	short	open;
	int	fid;
	Fid	*next;
	Mailbox	*mb;
	Message	*m;
	Message *mtop;		// top level message

	//finger pointers to speed up reads of large directories
	long	foff;	// offset/DIRLEN of finger
	Message	*fptr;	// pointer to message at off
	int	fvers;	// mailbox version when finger was saved
};

ulong	path;		// incremented for each new file
Fid	*fids;
int	mfd[2];
char	user[NAMELEN];
char	mdata[MAXMSG+MAXFDATA];
Fcall	rhdr;
Fcall	thdr;
int	fflg;
char	*mntpt;
int	biffing;
int	plumbing = 1;

QLock	mbllock;
Mailbox	*mbl;

Fid		*newfid(int);
void		error(char*);
void		io(void);
void		*erealloc(void*, ulong);
void		*emalloc(ulong);
void		usage(void);
void		reader(void);
int		readheader(Message*, char*, int, int);
int		cistrncmp(char*, char*, int);
int		tokenconvert(String*, char*, int);
void		post(char*, char*, int);

char	*rflush(Fid*), *rnop(Fid*), *rsession(Fid*),
	*rattach(Fid*), *rclone(Fid*), *rwalk(Fid*),
	*rclwalk(Fid*), *ropen(Fid*), *rcreate(Fid*),
	*rread(Fid*), *rwrite(Fid*), *rclunk(Fid*),
	*rremove(Fid*), *rstat(Fid*), *rwstat(Fid*);

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
char	Enoauth[] =	"no authentication in ramfs";
char	Enotexist[] =	"file does not exist";
char	Einuse[] =	"file in use";
char	Eexist[] =	"file exists";
char	Enotowner[] =	"not owner";
char	Eisopen[] = 	"file already open for I/O";
char	Excl[] = 	"exclusive use file already open";
char	Ename[] = 	"illegal name";
char	Ebadctl[] =	"unknown control message";

char *dirtab[] =
{
[Qdir]		".",
[Qbody]		"body",
[Qbcc]		"bcc",
[Qcc]		"cc",
[Qdate]		"date",
[Qdigest]	"digest",
[Qdisposition]	"disposition",
[Qfilename]	"filename",
[Qfrom]		"from",
[Qheader]	"header",
[Qinfo]		"info",
[Qinreplyto]	"inreplyto",
[Qlines]	"lines",
[Qmimeheader]	"mimeheader",
[Qmessageid]	"messageid",
[Qraw]		"raw",
[Qrawbody]	"rawbody",
[Qrawheader]	"rawheader",
[Qreplyto]	"replyto",
[Qsender]	"sender",
[Qsubject]	"subject",
[Qto]		"to",
[Qtype]		"type",
[Qunixheader]	"unixheader",
[Qctl]		"ctl",
[Qmboxctl]	"ctl",
};

enum
{
	Hsize=	1277,
};

Hash	*htab[Hsize];

int	debug;
int	fflag;
int	logging;

void
usage(void)
{
	fprint(2, "usage: %s [-b -m mountpoint]\n", argv0);
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
	int p[2], std, nodflt;
	char maildir[128];
	char mbox[128];
	char *mboxfile, *err;
	char srvfile[64];
	int srvpost;

	mntpt = nil;
	fflag = 0;
	mboxfile = nil;
	std = 0;
	nodflt = 0;
	srvpost = 0;

	ARGBEGIN{
	case 'b':
		biffing = 1;
		break;
	case 'f':
		fflag = 1;
		mboxfile = ARGF();
		break;
	case 'm':
		mntpt = ARGF();
		break;
	case 'd':
		debug = 1;
		break;
	case 'p':
		plumbing = 0;
		break;
	case 's':
		srvpost = 1;
		break;
	case 'l':
		logging = 1;
		break;
	case 'n':
		nodflt = 1;
		break;
	default:
		usage();
	}ARGEND

	if(pipe(p) < 0)
		error("pipe failed");
	mfd[0] = p[0];
	mfd[1] = p[0];

	notify(notifyf);
	strcpy(user, getuser());
	if(mntpt == nil){
		snprint(maildir, sizeof(maildir), "/mail/fs");
		mntpt = maildir;
	}
	if(mboxfile == nil && !nodflt){
		snprint(mbox, sizeof(mbox), "/mail/box/%s/mbox", user);
		mboxfile = mbox;
		std = 1;
	}

	if(debug)
		fmtinstall('F', fcallconv);

	if(mboxfile != nil){
		err = newmbox(mboxfile, "mbox", std);
		if(err != nil)
			sysfatal("opening mailbox: %s", err);
	}

	switch(rfork(RFFDG|RFPROC|RFNAMEG|RFNOTEG|RFREND)){
	case -1:
		error("fork");
	case 0:
		henter(CHDIR|PATH(0, Qtop), dirtab[Qctl],
			(Qid){PATH(0, Qctl), 0}, nil, nil);
		close(p[1]);
		io();
		postnote(PNGROUP, getpid(), "die yankee pig dog");
		break;
	default:
		close(p[0]);	/* don't deadlock if child fails */
		if(srvpost){
			sprint(srvfile, "/srv/upasfs.%s", user);
			post(srvfile, "upasfs", p[1]);
		} else {
			if(mount(p[1], mntpt, MREPL, "") < 0)
				error("mount failed");
		}
	}
	exits(0);
}

static int
fileinfo(Message *m, int t, char **pp)
{
	char *p;
	int len;

	p = "";
	len = 0;
	switch(t){
	case Qbody:
		p = m->body;
		len = m->bend - m->body;
		break;
	case Qbcc:
		if(m->bcc822){
			p = s_to_c(m->bcc822);
			len = strlen(p);
		}
		break;
	case Qcc:
		if(m->cc822){
			p = s_to_c(m->cc822);
			len = strlen(p);
		}
		break;
	case Qdisposition:
		switch(m->disposition){
		case Dinline:
			p = "inline";
			break;
		case Dfile:
			p = "file";
			break;
		}
		len = strlen(p);
		break;
	case Qdate:
		if(m->date822){
			p = s_to_c(m->date822);
			len = strlen(p);
		} else if(m->unixdate != nil){
			p = s_to_c(m->unixdate);
			len = strlen(p);
		}
		break;
	case Qfilename:
		if(m->filename){
			p = s_to_c(m->filename);
			len = strlen(p);
		}
		break;
	case Qinreplyto:
		if(m->inreplyto822){
			p = s_to_c(m->inreplyto822);
			len = strlen(p);
		}
		break;
	case Qmessageid:
		if(m->messageid822){
			p = s_to_c(m->messageid822);
			len = strlen(p);
		}
		break;
	case Qfrom:
		if(m->from822){
			p = s_to_c(m->from822);
			len = strlen(p);
		} else if(m->unixfrom != nil){
			p = s_to_c(m->unixfrom);
			len = strlen(p);
		}
		break;
	case Qheader:
		p = m->header;
		len = headerlen(m);
		break;
	case Qlines:
		p = m->lines;
		if(*p == 0)
			countlines(m);
		len = strlen(m->lines);
		break;
	case Qraw:
		p = m->start;
		if(strncmp(m->start, "From ", 5) == 0){
			p = strchr(p, '\n');
			if(p == nil)
				p = m->start;
			else
				p++;
		}
		len = m->end - p;
		break;
	case Qrawbody:
		p = m->rbody;
		len = m->rbend - p;
		break;
	case Qrawheader:
		p = m->header;
		len = m->hend - p;
		break;
	case Qmimeheader:
		p = m->mheader;
		len = m->mhend - p;
		break;
	case Qreplyto:
		p = nil;
		if(m->replyto822 != nil){
			p = s_to_c(m->replyto822);
			len = strlen(p);
		} else if(m->from822 != nil){
			p = s_to_c(m->from822);
			len = strlen(p);
		} else if(m->sender822 != nil){
			p = s_to_c(m->sender822);
			len = strlen(p);
		} else if(m->unixfrom != nil){
			p = s_to_c(m->unixfrom);
			len = strlen(p);
		}
		break;
	case Qsender:
		if(m->sender822){
			p = s_to_c(m->sender822);
			len = strlen(p);
		}
		break;
	case Qsubject:
		p = nil;
		if(m->subject822){
			p = s_to_c(m->subject822);
			len = strlen(p);
		}
		break;
	case Qto:
		if(m->to822){
			p = s_to_c(m->to822);
			len = strlen(p);
		}
		break;
	case Qtype:
		if(m->type){
			p = s_to_c(m->type);
			len = strlen(p);
		}
		break;
	case Qunixdate:
		if(m->unixdate){
			p = s_to_c(m->unixdate);
			len = strlen(p);
		}
		break;
	case Qunixheader:
		if(m->unixheader){
			p = s_to_c(m->unixheader);
			len = s_len(m->unixheader);
		}
		break;
	case Qdigest:
		if(m->sdigest){
			p = s_to_c(m->sdigest);
			len = strlen(p);
		}
		break;
	}
	*pp = p;
	return len;
}

int infofields[] = {
	Qfrom,
	Qto,
	Qcc,
	Qreplyto,
	Qunixdate,
	Qsubject,
	Qtype,
	Qdisposition,
	Qfilename,
	Qdigest,
	Qbcc,
	Qinreplyto,
	Qdate,
	Qsender,
	Qmessageid,
	Qlines,
	-1,
};

static int
readinfo(Message *m, char *buf, long off, int count)
{
	char *p;
	int len, i, n;
	String *s;

	s = s_new();
	len = 0;
	for(i = 0; len < count && infofields[i] >= 0; i++){
		n = fileinfo(m, infofields[i], &p);
		s_reset(s);
		if(tokenconvert(s, p, n) == 0){
			p = s_to_c(s);
			n = strlen(p);
		}
		if(off > 0){
			if(off >= n+1){
				off -= n+1;
				continue;
			}
			p += off;
			n -= off;
			off = 0;
		}
		if(n > count - len)
			n = count - len;
		if(buf)
			memmove(buf+len, p, n);
		len += n;
		if(buf)
			buf[len] = '\n';
		len++;
	}
	s_free(s);
	return len;
}

static void
mkstat(Dir *d, Mailbox *mb, Message *m, int t)
{
	char *p;

	strncpy(d->uid, user, NAMELEN);
	strncpy(d->gid, user, NAMELEN);
	d->mode = 0444;
	d->qid.vers = 0;
	d->type = 0;
	d->dev = 0;

	switch(t){
	case Qtop:
		strcpy(d->name, ".");
		d->mode = CHDIR|0555;
		d->atime = d->mtime = time(0);
		d->length = 0;
		d->qid.path = CHDIR|PATH(0, Qtop);
		break;
	case Qmbox:
		strcpy(d->name, mb->name);
		d->mode = CHDIR|0555;
		d->atime = mb->d.atime;
		d->mtime = mb->d.mtime;
		d->length = 0;
		d->qid.path = CHDIR|PATH(mb->id, Qmbox);
		d->qid.vers = mb->vers;
		break;
	case Qdir:
		strcpy(d->name, m->name);
		d->mode = CHDIR|0555;
		d->atime = mb->d.atime;
		d->mtime = mb->d.mtime;
		d->length = 0;
		d->qid.path = CHDIR|PATH(m->id, Qdir);
		break;
	case Qctl:
		strncpy(d->name, dirtab[t], NAMELEN);
		d->mode = 0666;
		d->atime = d->mtime = time(0);
		d->length = 0;
		d->qid.path = PATH(0, Qctl);
		break;
	case Qmboxctl:
		strncpy(d->name, dirtab[t], NAMELEN);
		d->mode = 0222;
		d->atime = d->mtime = time(0);
		d->length = 0;
		d->qid.path = PATH(mb->id, Qmboxctl);
		break;
	case Qinfo:
		strncpy(d->name, dirtab[t], NAMELEN);
		d->atime = mb->d.atime;
		d->mtime = mb->d.mtime;
		d->length = readinfo(m, nil, 0, 1<<30);
		d->qid.path = PATH(m->id, t);
		break;
	default:
		strncpy(d->name, dirtab[t], NAMELEN);
		d->atime = mb->d.atime;
		d->mtime = mb->d.mtime;
		d->length = fileinfo(m, t, &p);
		d->qid.path = PATH(m->id, t);
		break;
	}
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
	USED(f);
	return 0;
}

char*
rattach(Fid *f)
{
	f->busy = 1;
	f->m = nil;
	f->mb = nil;
	thdr.qid.path = f->qid.path = CHDIR|PATH(0, Qtop);
	f->qid.vers = 0;
	thdr.qid.vers = 0;
	if(strcmp(rhdr.uname, user) != 0)
		return Eperm;
	return 0;
}

char*
rclone(Fid *f)
{
	Fid *nf;

	if(f->open)
		return Eisopen;
	nf = newfid(rhdr.newfid);
	nf->busy = 1;
	nf->open = 0;
	nf->m = f->m;
	nf->mtop = f->mtop;
	nf->mb = f->mb;
	if(f->mb != nil)
		mboxincref(f->mb);
	if(f->mtop != nil){
		qlock(f->mb);
		msgincref(f->mtop);
		qunlock(f->mb);
	}
	nf->qid = f->qid;
	return 0;
}

char*
rwalk(Fid *f)
{
	char *name;
	int t;
	Mailbox *omb, *mb;
	char *rv, *p;
	Hash *h;

	name = rhdr.name;
	t = FILE(f->qid.path);

	rv = Enotexist;

	omb = f->mb;
	if(omb)
		qlock(omb);
	else
		qlock(&mbllock);

	// this must catch everything except . and ..
retry:
	h = hlook(f->qid.path, name);
	if(h != nil){
		f->mb = h->mb;
		f->m = h->m;
		switch(t){
		case Qtop:
			if(f->mb != nil)
				mboxincref(f->mb);
			break;
		case Qmbox:
			if(f->m){
				msgincref(f->m);
				f->mtop = f->m;
			}
			break;
		}
		f->qid = h->qid;
		thdr.qid = f->qid;
		rv = nil;
	} else if((p = strchr(name, '.')) != nil && *name != '.'){
		*p = 0;
		goto retry;
	}

	if(omb)
		qunlock(omb);
	else
		qunlock(&mbllock);
	if(rv == nil)
		return rv;


	if(strcmp(name, ".") == 0){
		thdr.qid = f->qid;
		return nil;
	}

	if((f->qid.path&CHDIR) != CHDIR)
		return Enotdir;

	if(strcmp(name, "..") == 0){
		switch(t){
		case Qtop:
			f->qid.path = CHDIR|PATH(0, Qtop);
			f->qid.vers = 0;
			break;
		case Qmbox:
			f->qid.path = CHDIR|PATH(0, Qtop);
			f->qid.vers = 0;
			qlock(&mbllock);
			mb = f->mb;
			f->mb = nil;
			mboxdecref(mb);
			qunlock(&mbllock);
			break;
		case Qdir:
			qlock(f->mb);
			if(f->m->whole == f->mb->root){
				f->qid.path = CHDIR|PATH(f->mb->id, Qmbox);
				f->qid.vers = f->mb->d.qid.vers;
				msgdecref(f->mb, f->mtop);
				f->m = f->mtop = nil;
			} else {
				f->m = f->m->whole;
				f->qid.path = CHDIR|PATH(f->m->id, Qdir);
			}
			qunlock(f->mb);
			break;
		}
		thdr.qid = f->qid;
		rv = nil;
	}

	return rv;
}

char *
rclwalk(Fid *f)
{
	Fid *nf;
	char *err;

	nf = newfid(rhdr.newfid);
	nf->busy = 1;
	nf->open = 0;
	if(f->mb != nil)
		mboxincref(f->mb);
	if(f->mtop != nil){
		qlock(f->mb);
		msgincref(f->m);
		qunlock(f->mb);
	}
	nf->mtop = f->mtop;
	nf->m = f->m;
	nf->mb = f->mb;
	if(err = rwalk(nf))
		rclunk(nf);
	return err;
}

char *
ropen(Fid *f)
{
	int file;

	if(f->open)
		return Eisopen;

	file = FILE(f->qid.path);
	if(rhdr.mode != OREAD)
		if(file != Qctl && file != Qmboxctl)
			return Eperm;

	// make sure we've decoded
	if(file == Qbody){
		if(f->m->decoded == 0)
			decode(f->m);
		if(f->m->converted == 0)
			convert(f->m);
	}

	thdr.qid = f->qid;
	f->open = 1;
	return 0;
}

char *
rcreate(Fid*)
{
	return Eperm;
}

int
readtopdir(Fid*, char *buf, long off, int cnt)
{
	Dir d;
	int n;
	Mailbox *mb;

	n = 0;
	if(cnt == 0)
		return 0;
	if(off == 0){
		mkstat(&d, nil, nil, Qctl);
		convD2M(&d, &buf[n]);
		n += DIRLEN;
		cnt--;
	} else
		off--;
		
	for(mb = mbl; cnt > 0 && mb != nil; mb = mb->next){
		if(off > 0){
			off--;
			continue;
		}
		mkstat(&d, mb, nil, Qmbox);
		convD2M(&d, &buf[n]);
		n += DIRLEN;
		cnt--;
	}
	return n;
}

int
readmboxdir(Fid *f, char *buf, long off, int cnt)
{
	Dir d;
	int n;
	Message *m;
	long pos;

	n = 0;
	if(f->mb->ctl){
		if(off == 0){
			mkstat(&d, f->mb, nil, Qmboxctl);
			convD2M(&d, &buf[n]);
			n += DIRLEN;
			cnt--;
		} else
			off--;
	}

	// to avoid n**2 reads of the directory, use a saved finger pointer
	if(f->mb->vers == f->fvers && off >= f->foff && f->foff > 0){
		m = f->fptr;
		pos = f->foff;
	} else {
		m = f->mb->root->part;
		pos = 0;
	} 

	for(; cnt > 0 && m != nil; m = m->next){
		// act like deleted files aren't there
		if(m->deleted)
			continue;

		// skip till we've passed the offset
		if(off > pos){
			pos++;
 			continue;
		}

		mkstat(&d, f->mb, m, Qdir);
		convD2M(&d, &buf[n]);
		n += DIRLEN;
		cnt--;
		pos++;
	}

	// save a finger pointer for next read of the mbox directory
	f->foff = pos;
	f->fptr = m;
	f->fvers = f->mb->vers;

	return n;
}

int
readmsgdir(Fid *f, char *buf, long off, int cnt)
{
	Dir d;
	int i, n;
	Message *m;

	n = 0;
	for(i = 0; cnt > 0 && i < Qmax; i++){
		if(off > 0){
			off--;
			continue;
		}
		mkstat(&d, f->mb, f->m, i);
		convD2M(&d, &buf[n]);
		n += DIRLEN;
		cnt--;
	}
	for(m = f->m->part; cnt > 0 && m != nil; m = m->next){
		if(off > 0){
			off--;
			continue;
		}
		mkstat(&d, f->mb, m, Qdir);
		convD2M(&d, &buf[n]);
		n += DIRLEN;
		cnt--;
	}

	return n;
}

char*
rread(Fid *f)
{
	char *buf;
	long off;
	int t, i, n, cnt;
	char *p;

	thdr.count = 0;
	off = rhdr.offset;
	buf = thdr.data;
	cnt = rhdr.count;

	t = FILE(f->qid.path);
	if(f->qid.path & CHDIR){
		cnt /= DIRLEN;
		if(off%DIRLEN)
			return "i/o error";
		off /= DIRLEN;

		if(t == Qtop) {
			qlock(&mbllock);
			n = readtopdir(f, buf, off, cnt);
			qunlock(&mbllock);
		} else if(t == Qmbox) {
			qlock(f->mb);
			if(off == 0)
				syncmbox(f->mb, 1);
			n = readmboxdir(f, buf, off, cnt);
			qunlock(f->mb);
		} else if(t == Qmboxctl) {
			n = 0;
		} else {
			n = readmsgdir(f, buf, off, cnt);
		}

		thdr.count = n;
		return nil;
	}

	if(FILE(f->qid.path) == Qheader){
		thdr.count = readheader(f->m, buf, off, cnt);
		return nil;
	}

	if(FILE(f->qid.path) == Qinfo){
		thdr.count = readinfo(f->m, buf, off, cnt);
		return nil;
	}

	i = fileinfo(f->m, FILE(f->qid.path), &p);
	if(off < i){
		if((off + cnt) > i)
			cnt = i - off;
		memmove(buf, p + off, cnt);
		thdr.count = cnt;
	}
	return nil;
}

char*
rwrite(Fid *f)
{
	char *err;
	char *token[1024];
	int t, n;
	String *file;

	t = FILE(f->qid.path);
	switch(t){
	case Qctl:
		if(rhdr.count == 0)
			return Ebadctl;
		if(rhdr.data[rhdr.count-1] == '\n')
			rhdr.data[rhdr.count-1] = 0;
		else
			rhdr.data[rhdr.count] = 0;
		n = tokenize(rhdr.data, token, nelem(token));
		if(n == 0)
			return Ebadctl;
		if(strcmp(token[0], "open") == 0){
			file = s_new();
			switch(n){
			case 1:
				err = Ebadctl;
				break;
			case 2:
				mboxpath(token[1], getlog(), file, 0);
				err = newmbox(s_to_c(file), nil, 0);
				break;
			default:
				mboxpath(token[1], getlog(), file, 0);
				if(strchr(token[2], '/') != nil)
					err = "/ not allowed in mailbox name";
				else
					err = newmbox(s_to_c(file), token[2], 0);
				break;
			}
			s_free(file);
			thdr.count = rhdr.count;
			return err;
		}
		if(strcmp(token[0], "close") == 0){
			if(n < 2)
				return nil;
			freembox(token[1]);
			return nil;
		}
		if(strcmp(token[0], "delete") == 0){
			if(n < 3)
				return nil;
			delmessages(n-1, &token[1]);
			thdr.count = rhdr.count;
			return nil;
		}
//		if(strcmp(token[0], "pooldump") == 0){
//			print("msgallocd %d, msgfreed %d\n", msgallocd, msgfreed);
//			pooldump("Main");
//			return nil;
//		}
		return Ebadctl;
	case Qmboxctl:
		if(f->mb && f->mb->ctl){
			if(rhdr.count == 0)
				return Ebadctl;
			if(rhdr.data[rhdr.count-1] == '\n')
				rhdr.data[rhdr.count-1] = 0;
			else
				rhdr.data[rhdr.count] = 0;
			n = tokenize(rhdr.data, token, nelem(token));
			if(n == 0)
				return Ebadctl;
			return (*f->mb->ctl)(f->mb, n, token);
		}
	}
	return Eperm;
}

char *
rclunk(Fid *f)
{
	Mailbox *mb;

	f->busy = 0;
	f->open = 0;
	if(f->mtop != nil){
		qlock(f->mb);
		msgdecref(f->mb, f->mtop);
		qunlock(f->mb);
	}
	f->m = f->mtop = nil;
	mb = f->mb;
	if(mb != nil){
		f->mb = nil;
		qlock(&mbllock);
		mboxdecref(mb);
		qunlock(&mbllock);
	}
	return 0;
}

char *
rremove(Fid *f)
{
	if(f->m->deleted == 0)
		mailplumb(f->mb, f->m, 1);
	f->m->deleted = 1;
	return rclunk(f);
}

char *
rstat(Fid *f)
{
	Dir d;

	if(FILE(f->qid.path) == Qmbox){
		qlock(f->mb);
		syncmbox(f->mb, 1);
		qunlock(f->mb);
	}
	mkstat(&d, f->mb, f->m, FILE(f->qid.path));
	convD2M(&d, thdr.stat);
	return 0;
}

char *
rwstat(Fid*)
{
	return Eperm;
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

int
fidmboxrefs(Mailbox *mb)
{
	Fid *f;
	int refs = 0;

	for(f = fids; f; f = f->next){
		if(f->mb == mb)
			refs++;
	}
	return refs;
}

void
io(void)
{
	char *err;
	int n;

	// start a process to watch the mailboxes
	if(plumbing){
		switch(rfork(RFPROC|RFMEM)){
		case -1:
			/* oh well */
			break;
		case 0:
			reader();
			exits(nil);
		default:
			break;
		}
	}

	for(;;){
		/*
		 * reading from a pipe or a network device
		 * will give an error after a few eof reads
		 * however, we cannot tell the difference
		 * between a zero-length read and an interrupt
		 * on the processes writing to us,
		 * so we wait for the error
		 */
		n = read(mfd[0], mdata, sizeof mdata);
		if(n == 0)
			continue;
		if(n < 0)
			return;
		if(convM2S(mdata, &rhdr, n) == 0)
			continue;

		if(debug)
			fprint(2, "%s:<-%F\n", argv0, &rhdr);

		thdr.data = mdata + MAXMSG;
		if(!fcalls[rhdr.type])
			err = "bad fcall type";
		else
			err = (*fcalls[rhdr.type])(newfid(rhdr.fid));
		if(err){
			thdr.type = Rerror;
			strncpy(thdr.ename, err, ERRLEN);
		}else{
			thdr.type = rhdr.type + 1;
			thdr.fid = rhdr.fid;
		}
		thdr.tag = rhdr.tag;
		if(debug)
			fprint(2, "%s:->%F\n", argv0, &thdr);/**/
		n = convS2M(&thdr, mdata);
		if(write(mfd[1], mdata, n) != n)
			error("mount write");
	}
}

void
reader(void)
{
	ulong t;
	Dir d;
	Mailbox *mb;

	sleep(15*1000);
	for(;;){
		t = time(0);
		qlock(&mbllock);
		for(mb = mbl; mb != nil; mb = mb->next){
			if(mb->waketime != 0 && t > mb->waketime){
				qlock(mb);
				mb->waketime = 0;
				break;
			}

			if(dirstat(mb->path, &d) < 0)
				continue;

			if(d.qid.path != mb->d.qid.path
			   || d.qid.vers != mb->d.qid.vers){
				qlock(mb);
				break;
			}
		}
		qunlock(&mbllock);
		if(mb != nil){
			syncmbox(mb, 1);
			qunlock(mb);
		} else
			sleep(15*1000);
	}
}

int
newid(void)
{
	int rv;
	static int id;
	static Lock idlock;

	lock(&idlock);
	rv = ++id;
	unlock(&idlock);

	return rv;
}

void
error(char *s)
{
	postnote(PNGROUP, getpid(), "die yankee pig dog");
	fprint(2, "%s: %s: %r\n", argv0, s);
	exits(s);
}


typedef struct Ignorance Ignorance;
struct Ignorance
{
	Ignorance *next;
	char	*str;		/* string */
	int	partial;	/* true if not exact match */
};
Ignorance *ignorance;

/*
 *  read the file of headers to ignore 
 */
void
readignore(void)
{
	char *p;
	Ignorance *i;
	Biobuf *b;

	if(ignorance != nil)
		return;

	b = Bopen("/mail/lib/ignore", OREAD);
	if(b == 0)
		return;
	while(p = Brdline(b, '\n')){
		p[Blinelen(b)-1] = 0;
		while(*p && (*p == ' ' || *p == '\t'))
			p++;
		if(*p == '#')
			continue;
		i = malloc(sizeof(Ignorance));
		if(i == 0)
			break;
		i->partial = strlen(p);
		i->str = strdup(p);
		if(i->str == 0){
			free(i);
			break;
		}
		i->next = ignorance;
		ignorance = i;
	}
	Bterm(b);
}

int
ignore(char *p)
{
	Ignorance *i;

	readignore();
	for(i = ignorance; i != nil; i = i->next)
		if(cistrncmp(i->str, p, i->partial) == 0)
			return 1;
	return 0;
}

int
hdrlen(char *p, char *e)
{
	char *ep;

	ep = p;
	do {
		ep = strchr(ep, '\n');
		if(ep == nil){
			ep = e;
			break;
		}
		ep++;
		if(ep >= e){
			ep = e;
			break;
		}
	} while(*ep == ' ' || *ep == '\t');
	return ep - p;
}

// rfc2047 non-ascii
typedef struct Charset Charset;
struct Charset {
	char *name;
	int len;
	int convert;
} charsets[4] =
{
	{ "us-ascii",		8,	1, },
	{ "utf-8",		5,	0, },
	{ "iso-8859-1",		10,	1, },
	{ "big5",		4,	2, },
};

// convert a single token
int
tokenconvert(String *s, char *token, int len)
{
	char decoded[1024];
	char utfbuf[2*1024];
	int i, havequote;
	char *e, *x;

	if(len == 0)
		return -1;

	havequote = 0;
	if(token[0] == '"' && token[len-1] == '"'){
		havequote = 1;
		token++;
		len -= 2;
	}

	if(token[0] != '=' || token[1] != '?' ||
	   token[len-2] != '?' || token[len-1] != '=')
		return -1;
	e = token+len-2;
	token += 2;

	// bail if we don't understand the character set
	for(i = 0; i < nelem(charsets); i++)
		if(cistrncmp(charsets[i].name, token, charsets[i].len) == 0)
		if(token[charsets[i].len] == '?'){
			token += charsets[i].len + 1;
			break;
		}
	if(i >= nelem(charsets))
		return -1;

	// bail if it doesn't fit 
	if(strlen(token) > sizeof(decoded)-1)
		return -1;

	// bail if we don't understand the encoding
	if(cistrncmp(token, "b?", 2) == 0){
		token += 2;
		len = dec64((uchar*)decoded, sizeof(decoded), token, e-token);
		decoded[len] = 0;
	} else if(cistrncmp(token, "q?", 2) == 0){
		token += 2;
		len = decquoted(decoded, token, e);
		if(len > 0 && decoded[len-1] == '\n')
			len--;
		decoded[len] = 0;
	} else
		return -1;

	if(havequote)
		s_append(s, "\"");
	switch(charsets[i].convert){
	case 0:
		s_append(s, decoded);
		break;
	case 1:
		latin1toutf(utfbuf, decoded, decoded+len);
		s_append(s, utfbuf);
		break;
	case 2:
		if(xtoutf(charsets[i].name, &x, decoded, decoded+len) <= 0){
			s_append(s, decoded);
		} else {
			s_append(s, x);
			free(x);
		}
		break;
	}
	if(havequote)
		s_append(s, "\"");

	return 0;
}

// convert a header line
String*
hdrconvert(String *s, char *line, int len)
{
	char *end;
	char token[1024];
	int i;

	end = line+len;
	s = s_reset(s);
	i = 0;
	if(memchr(line, '=', len) == 0){
		s_nappend(s, line, len);
		return s;
	}
	while(line < end){
		switch(*line){
		case ' ':
		case '\t':
		case '\n':
			token[i] = 0;
			if(tokenconvert(s, token, i) < 0){
				token[i++] = *line++;
				token[i] = 0;
				s_append(s, token);
				i = 0;
				continue;
			}
			s_putc(s, *line++);
			s_terminate(s);
			i = 0;
			break;

		default:
			if(i >= sizeof(token)-3){
				token[i++] = *line++;
				token[i] = 0;
				s_append(s, token);
				i = 0;
				continue;
			}
			token[i++] = *line++;
			break;
		}
	}
	return s;
}

int
readheader(Message *m, char *buf, int off, int cnt)
{
	char *p, *e;
	int n, ns;
	char *to = buf;
	String *s;

	p = m->header;
	e = m->hend;
	s = nil;

	// copy in good headers
	while(cnt > 0 && p < e){
		n = hdrlen(p, e);
		if(ignore(p)){
			p += n;
			continue;
		}

		// rfc2047 processing
		s = hdrconvert(s, p, n);
		ns = s_len(s);
		if(off > 0){
			if(ns <= off){
				off -= ns;
				p += n;
				continue;
			}
			ns -= off;
		}
		if(ns > cnt)
			ns = cnt;
		memmove(to, s_to_c(s)+off, ns);
		to += ns;
		p += n;
		cnt -= ns;
		off = 0;
	}

	s_free(s);
	return to - buf;
}

int
headerlen(Message *m)
{
	char buf[1024];
	int i, n;

	if(m->hlen >= 0)
		return m->hlen;
	for(n = 0; ; n += i){
		i = readheader(m, buf, n, sizeof(buf));
		if(i <= 0)
			break;
	}
	m->hlen = n;
	return n;
}

QLock hashlock;

uint
hash(ulong ppath, char *name)
{
	uchar *p;
	uint h;

	h = 0;
	for(p = (uchar*)name; *p; p++)
		h = h*7 + *p;
	h += ppath;

	return h % Hsize;
}

Hash*
hlook(ulong ppath, char *name)
{
	int h;
	Hash *hp;

	qlock(&hashlock);
	h = hash(ppath, name);
	for(hp = htab[h]; hp != nil; hp = hp->next)
		if(ppath == hp->ppath && strcmp(name, hp->name) == 0){
			qunlock(&hashlock);
			return hp;
		}
	qunlock(&hashlock);
	return nil;
}

void
henter(ulong ppath, char *name, Qid qid, Message *m, Mailbox *mb)
{
	int h;
	Hash *hp, **l;

	qlock(&hashlock);
	h = hash(ppath, name);
	for(l = &htab[h]; *l != nil; l = &(*l)->next){
		hp = *l;
		if(ppath == hp->ppath && strcmp(name, hp->name) == 0){
			hp->m = m;
			hp->mb = mb;
			hp->qid = qid;
			qunlock(&hashlock);
			return;
		}
	}

	*l = hp = emalloc(sizeof(*hp));
	hp->m = m;
	hp->mb = mb;
	hp->qid = qid;
	hp->name = name;
	hp->ppath = ppath;
	qunlock(&hashlock);
}

void
hfree(ulong ppath, char *name)
{
	int h;
	Hash *hp, **l;

	qlock(&hashlock);
	h = hash(ppath, name);
	for(l = &htab[h]; *l != nil; l = &(*l)->next){
		hp = *l;
		if(ppath == hp->ppath && strcmp(name, hp->name) == 0){
			*l = hp->next;
			free(hp);
			break;
		}
	}
	qunlock(&hashlock);
}

int
hashmboxrefs(Mailbox *mb)
{
	int h;
	Hash *hp;
	int refs = 0;

	qlock(&hashlock);
	for(h = 0; h < Hsize; h++){
		for(hp = htab[h]; hp != nil; hp = hp->next)
			if(hp->mb == mb)
				refs++;
	}
	qunlock(&hashlock);
	return refs;
}

void
post(char *name, char *envname, int srvfd)
{
	int fd;
	char buf[32];

	fd = create(name, OWRITE, 0600);
	if(fd < 0)
		return;
	sprint(buf, "%d",srvfd);
	if(write(fd, buf, strlen(buf)) != strlen(buf))
		error("srv write");
	close(fd);
	putenv(envname, name);
}
