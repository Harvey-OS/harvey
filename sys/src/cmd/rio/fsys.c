#include <u.h>
#include <libc.h>
#include <draw.h>
#include <thread.h>
#include <cursor.h>
#include <mouse.h>
#include <keyboard.h>
#include <frame.h>
#include <fcall.h>
#include "dat.h"
#include "fns.h"

char Eperm[] = "permission denied";
char Eexist[] = "file does not exist";
char Enotdir[] = "not a directory";
char	Ebadfcall[] = "bad fcall type";
char	Eoffset[] = "illegal offset";

int	messagesize = 8192+IOHDRSZ;	/* good start */

enum{
	DEBUG = 0
};

Dirtab dirtab[]=
{
	{ ".",			QTDIR,	Qdir,			0500|DMDIR },
	{ "cons",		QTFILE,	Qcons,		0600 },
	{ "cursor",		QTFILE,	Qcursor,		0600 },
	{ "consctl",	QTFILE,	Qconsctl,		0200 },
	{ "winid",		QTFILE,	Qwinid,		0400 },
	{ "winname",	QTFILE,	Qwinname,	0400 },
	{ "kbdin",		QTFILE,	Qkbdin,		0200 },
	{ "label",		QTFILE,	Qlabel,		0600 },
	{ "mouse",	QTFILE,	Qmouse,		0600 },
	{ "screen",		QTFILE,	Qscreen,		0400 },
	{ "snarf",		QTFILE,	Qsnarf,		0600 },
	{ "text",		QTFILE,	Qtext,		0400 },
	{ "wdir",		QTFILE,	Qwdir,		0600 },
	{ "wctl",		QTFILE,	Qwctl,		0600 },
	{ "window",	QTFILE,	Qwindow,		0400 },
	{ "wsys",		QTDIR,	Qwsys,		0500|DMDIR },
	{ nil, }
};

static uint		getclock(void);
static void		filsysproc(void*);
static Fid*		newfid(Filsys*, int);
static int		dostat(Filsys*, int, Dirtab*, uchar*, int, uint);

int	clockfd;
int	firstmessage = 1;

char	srvpipe[64];
char	srvwctl[64];

static	Xfid*	filsysflush(Filsys*, Xfid*, Fid*);
static	Xfid*	filsysversion(Filsys*, Xfid*, Fid*);
static	Xfid*	filsysauth(Filsys*, Xfid*, Fid*);
static	Xfid*	filsysnop(Filsys*, Xfid*, Fid*);
static	Xfid*	filsysattach(Filsys*, Xfid*, Fid*);
static	Xfid*	filsyswalk(Filsys*, Xfid*, Fid*);
static	Xfid*	filsysopen(Filsys*, Xfid*, Fid*);
static	Xfid*	filsyscreate(Filsys*, Xfid*, Fid*);
static	Xfid*	filsysread(Filsys*, Xfid*, Fid*);
static	Xfid*	filsyswrite(Filsys*, Xfid*, Fid*);
static	Xfid*	filsysclunk(Filsys*, Xfid*, Fid*);
static	Xfid*	filsysremove(Filsys*, Xfid*, Fid*);
static	Xfid*	filsysstat(Filsys*, Xfid*, Fid*);
static	Xfid*	filsyswstat(Filsys*, Xfid*, Fid*);

Xfid* 	(*fcall[Tmax])(Filsys*, Xfid*, Fid*) =
{
	[Tflush]	= filsysflush,
	[Tversion]	= filsysversion,
	[Tauth]	= filsysauth,
	[Tattach]	= filsysattach,
	[Twalk]	= filsyswalk,
	[Topen]	= filsysopen,
	[Tcreate]	= filsyscreate,
	[Tread]	= filsysread,
	[Twrite]	= filsyswrite,
	[Tclunk]	= filsysclunk,
	[Tremove]= filsysremove,
	[Tstat]	= filsysstat,
	[Twstat]	= filsyswstat,
};

void
post(char *name, char *envname, int srvfd)
{
	int fd;
	char buf[32];

	fd = create(name, OWRITE|ORCLOSE|OCEXEC, 0600);
	if(fd < 0)
		error(name);
	sprint(buf, "%d",srvfd);
	if(write(fd, buf, strlen(buf)) != strlen(buf))
		error("srv write");
	putenv(envname, name);
}

/*
 * Build pipe with OCEXEC set on second fd.
 * Can't put it on both because we want to post one in /srv.
 */
int
cexecpipe(int *p0, int *p1)
{
	/* pipe the hard way to get close on exec */
	if(bind("#|", "/mnt/temp", MREPL) < 0)
		return -1;
	*p0 = open("/mnt/temp/data", ORDWR);
	*p1 = open("/mnt/temp/data1", ORDWR|OCEXEC);
	unmount(nil, "/mnt/temp");
	if(*p0<0 || *p1<0)
		return -1;
	return 0;
}

Filsys*
filsysinit(Channel *cxfidalloc)
{
	int n, fd, pid, p0;
	Filsys *fs;
	Channel *c;
	char buf[128];

	fs = emalloc(sizeof(Filsys));
	if(cexecpipe(&fs->cfd, &fs->sfd) < 0)
		goto Rescue;
	fmtinstall('F', fcallfmt);
	clockfd = open("/dev/time", OREAD|OCEXEC);
	fd = open("/dev/user", OREAD);
	strcpy(buf, "Jean-Paul_Belmondo");
	if(fd >= 0){
		n = read(fd, buf, sizeof buf-1);
		if(n > 0)
			buf[n] = 0;
		close(fd);
	}
	fs->user = estrdup(buf);
	fs->cxfidalloc = cxfidalloc;
	pid = getpid();

	/*
	 * Create and post wctl pipe
	 */
	if(cexecpipe(&p0, &wctlfd) < 0)
		goto Rescue;
	sprint(srvwctl, "/srv/riowctl.%s.%d", fs->user, pid);
	post(srvwctl, "wctl", p0);
	close(p0);

	/*
	 * Start server processes
	 */
	c = chancreate(sizeof(char*), 0);
	if(c == nil)
		error("wctl channel");
	proccreate(wctlproc, c, 4096);
	threadcreate(wctlthread, c, 4096);
	proccreate(filsysproc, fs, 10000);

	/*
	 * Post srv pipe
	 */
	sprint(srvpipe, "/srv/rio.%s.%d", fs->user, pid);
	post(srvpipe, "wsys", fs->cfd);

	return fs;

Rescue:
	free(fs);
	return nil;
}

static
void
filsysproc(void *arg)
{
	int n;
	Xfid *x;
	Fid *f;
	Fcall t;
	uchar *buf;
	Filsys *fs;

	threadsetname("FILSYSPROC");
	fs = arg;
	fs->pid = getpid();
	x = nil;
	for(;;){
		buf = emalloc(messagesize+UTFmax);	/* UTFmax for appending partial rune in xfidwrite */
		n = read9pmsg(fs->sfd, buf, messagesize);
		if(n <= 0){
			yield();	/* if threadexitsall'ing, will not return */
			fprint(2, "rio: %d: read9pmsg: %d %r\n", getpid(), n);
			errorshouldabort = 0;
			error("eof or i/o error on server channel");
		}
		if(x == nil){
			send(fs->cxfidalloc, nil);
			recv(fs->cxfidalloc, &x);
			x->fs = fs;
		}
		x->buf = buf;
		if(convM2S(buf, n, x) != n)
			error("convert error in convM2S");
		if(DEBUG)
			fprint(2, "rio:<-%F\n", &x->Fcall);
		if(fcall[x->type] == nil)
			x = filsysrespond(fs, x, &t, Ebadfcall);
		else{
			if(x->type==Tversion || x->type==Tauth)
				f = nil;
			else
				f = newfid(fs, x->fid);
			x->f = f;
			x  = (*fcall[x->type])(fs, x, f);
		}
		firstmessage = 0;
	}
}

/*
 * Called only from a different FD group
 */
int
filsysmount(Filsys *fs, int id)
{
	char buf[32];

	close(fs->sfd);	/* close server end so mount won't hang if exiting */
	sprint(buf, "%d", id);
	if(mount(fs->cfd, -1, "/mnt/wsys", MREPL, buf) < 0){
		fprint(2, "mount failed: %r\n");
		return -1;
	}
	if(bind("/mnt/wsys", "/dev", MBEFORE) < 0){
		fprint(2, "bind failed: %r\n");
		return -1;
	}
	return 0;
}

Xfid*
filsysrespond(Filsys *fs, Xfid *x, Fcall *t, char *err)
{
	int n;

	if(err){
		t->type = Rerror;
		t->ename = err;
	}else
		t->type = x->type+1;
	t->fid = x->fid;
	t->tag = x->tag;
	if(x->buf == nil)
		x->buf = malloc(messagesize);
	n = convS2M(t, x->buf, messagesize);
	if(n <= 0)
		error("convert error in convS2M");
	if(write(fs->sfd, x->buf, n) != n)
		error("write error in respond");
	if(DEBUG)
		fprint(2, "rio:->%F\n", t);
	free(x->buf);
	x->buf = nil;
	return x;
}

void
filsyscancel(Xfid *x)
{
	if(x->buf){
		free(x->buf);
		x->buf = nil;
	}
}

static
Xfid*
filsysversion(Filsys *fs, Xfid *x, Fid*)
{
	Fcall t;

	if(!firstmessage)
		return filsysrespond(x->fs, x, &t, "version request not first message");
	if(x->msize < 256)
		return filsysrespond(x->fs, x, &t, "version: message size too small");
	messagesize = x->msize;
	t.msize = messagesize;
	if(strncmp(x->version, "9P2000", 6) != 0)
		return filsysrespond(x->fs, x, &t, "unrecognized 9P version");
	t.version = "9P2000";
	return filsysrespond(fs, x, &t, nil);
}

static
Xfid*
filsysauth(Filsys *fs, Xfid *x, Fid*)
{
	Fcall t;

		return filsysrespond(fs, x, &t, "rio: authentication not required");
}

static
Xfid*
filsysflush(Filsys*, Xfid *x, Fid*)
{
	sendp(x->c, xfidflush);
	return nil;
}

static
Xfid*
filsysattach(Filsys *, Xfid *x, Fid *f)
{
	Fcall t;

	if(strcmp(x->uname, x->fs->user) != 0)
		return filsysrespond(x->fs, x, &t, Eperm);
	f->busy = TRUE;
	f->open = FALSE;
	f->qid.path = Qdir;
	f->qid.type = QTDIR;
	f->qid.vers = 0;
	f->dir = dirtab;
	f->nrpart = 0;
	sendp(x->c, xfidattach);
	return nil;
}

static
int
numeric(char *s)
{
	for(; *s!='\0'; s++)
		if(*s<'0' || '9'<*s)
			return 0;
	return 1;
}

static
Xfid*
filsyswalk(Filsys *fs, Xfid *x, Fid *f)
{
	Fcall t;
	Fid *nf;
	int i, id;
	uchar type;
	ulong path;
	Dirtab *d, *dir;
	Window *w;
	char *err;
	Qid qid;

	if(f->open)
		return filsysrespond(fs, x, &t, "walk of open file");
	nf = nil;
	if(x->fid  != x->newfid){
		/* BUG: check exists */
		nf = newfid(fs, x->newfid);
		if(nf->busy)
			return filsysrespond(fs, x, &t, "clone to busy fid");
		nf->busy = TRUE;
		nf->open = FALSE;
		nf->dir = f->dir;
		nf->qid = f->qid;
		nf->w = f->w;
		incref(f->w);
		nf->nrpart = 0;	/* not open, so must be zero */
		f = nf;	/* walk f */
	}

	t.nwqid = 0;
	err = nil;

	/* update f->qid, f->dir only if walk completes */
	qid = f->qid;
	dir = f->dir;

	if(x->nwname > 0){
		for(i=0; i<x->nwname; i++){
			if((qid.type & QTDIR) == 0){
				err = Enotdir;
				break;
			}
			if(strcmp(x->wname[i], "..") == 0){
				type = QTDIR;
				path = Qdir;
				dir = dirtab;
				if(FILE(qid) == Qwsysdir)
					path = Qwsys;
				id = 0;
    Accept:
				if(i == MAXWELEM){
					err = "name too long";
					break;
				}
				qid.type = type;
				qid.vers = 0;
				qid.path = QID(id, path);
				t.wqid[t.nwqid++] = qid;
				continue;
			}

			if(qid.path == Qwsys){
				/* is it a numeric name? */
				if(!numeric(x->wname[i]))
					break;
				/* yes: it's a directory */
				id = atoi(x->wname[i]);
				qlock(&all);
				w = wlookid(id);
				if(w == nil){
					qunlock(&all);
					break;
				}
				path = Qwsysdir;
				type = QTDIR;
				qunlock(&all);
				incref(w);
				sendp(winclosechan, f->w);
				f->w = w;
				dir = dirtab;
				goto Accept;
			}
		
			if(snarffd>=0 && strcmp(x->wname[i], "snarf")==0)
				break;	/* don't serve /dev/snarf if it's provided in the environment */
			id = WIN(f->qid);
			d = dirtab;
			d++;	/* skip '.' */
			for(; d->name; d++)
				if(strcmp(x->wname[i], d->name) == 0){
					path = d->qid;
					type = d->type;
					dir = d;
					goto Accept;
				}

			break;	/* file not found */
		}

		if(i==0 && err==nil)
			err = Eexist;
	}

	if(err!=nil || t.nwqid<x->nwname){
		if(nf){
			if(nf->w)
				sendp(winclosechan, nf->w);
			nf->open = FALSE;
			nf->busy = FALSE;
		}
	}else if(t.nwqid == x->nwname){
		f->dir = dir;
		f->qid = qid;
	}

	return filsysrespond(fs, x, &t, err);
}

static
Xfid*
filsysopen(Filsys *fs, Xfid *x, Fid *f)
{
	Fcall t;
	int m;

	/* can't truncate anything, so just disregard */
	x->mode &= ~(OTRUNC|OCEXEC);
	/* can't execute or remove anything */
	if(x->mode==OEXEC || (x->mode&ORCLOSE))
		goto Deny;
	switch(x->mode){
	default:
		goto Deny;
	case OREAD:
		m = 0400;
		break;
	case OWRITE:
		m = 0200;
		break;
	case ORDWR:
		m = 0600;
		break;
	}
	if(((f->dir->perm&~(DMDIR|DMAPPEND))&m) != m)
		goto Deny;
		
	sendp(x->c, xfidopen);
	return nil;

    Deny:
	return filsysrespond(fs, x, &t, Eperm);
}

static
Xfid*
filsyscreate(Filsys *fs, Xfid *x, Fid*)
{
	Fcall t;

	return filsysrespond(fs, x, &t, Eperm);
}

static
int
idcmp(void *a, void *b)
{
	return *(int*)a - *(int*)b;
}

static
Xfid*
filsysread(Filsys *fs, Xfid *x, Fid *f)
{
	Fcall t;
	uchar *b;
	int i, n, o, e, len, j, k, *ids;
	Dirtab *d, dt;
	uint clock;
	char buf[16];

	if((f->qid.type & QTDIR) == 0){
		sendp(x->c, xfidread);
		return nil;
	}
	o = x->offset;
	e = x->offset+x->count;
	clock = getclock();
	b = malloc(messagesize-IOHDRSZ);	/* avoid memset of emalloc */
	if(b == nil)
		return filsysrespond(fs, x, &t, "out of memory");
	n = 0;
	switch(FILE(f->qid)){
	case Qdir:
	case Qwsysdir:
		d = dirtab;
		d++;	/* first entry is '.' */
		for(i=0; d->name!=nil && i<e; i+=len){
			len = dostat(fs, WIN(x->f->qid), d, b+n, x->count-n, clock);
			if(len <= BIT16SZ)
				break;
			if(i >= o)
				n += len;
			d++;
		}
		break;
	case Qwsys:
		qlock(&all);
		ids = emalloc(nwindow*sizeof(int));
		for(j=0; j<nwindow; j++)
			ids[j] = window[j]->id;
		qunlock(&all);
		qsort(ids, nwindow, sizeof ids[0], idcmp);
		dt.name = buf;
		for(i=0, j=0; j<nwindow && i<e; i+=len){
			k = ids[j];
			sprint(dt.name, "%d", k);
			dt.qid = QID(k, Qdir);
			dt.type = QTDIR;
			dt.perm = DMDIR|0700;
			len = dostat(fs, k, &dt, b+n, x->count-n, clock);
			if(len == 0)
				break;
			if(i >= o)
				n += len;
			j++;
		}
		free(ids);
		break;
	}
	t.data = (char*)b;
	t.count = n;
	filsysrespond(fs, x, &t, nil);
	free(b);
	return x;
}

static
Xfid*
filsyswrite(Filsys*, Xfid *x, Fid*)
{
	sendp(x->c, xfidwrite);
	return nil;
}

static
Xfid*
filsysclunk(Filsys *fs, Xfid *x, Fid *f)
{
	Fcall t;

	if(f->open){
		f->busy = FALSE;
		f->open = FALSE;
		sendp(x->c, xfidclose);
		return nil;
	}
	if(f->w)
		sendp(winclosechan, f->w);
	f->busy = FALSE;
	f->open = FALSE;
	return filsysrespond(fs, x, &t, nil);
}

static
Xfid*
filsysremove(Filsys *fs, Xfid *x, Fid*)
{
	Fcall t;

	return filsysrespond(fs, x, &t, Eperm);
}

static
Xfid*
filsysstat(Filsys *fs, Xfid *x, Fid *f)
{
	Fcall t;

	t.stat = emalloc(messagesize-IOHDRSZ);
	t.nstat = dostat(fs, WIN(x->f->qid), f->dir, t.stat, messagesize-IOHDRSZ, getclock());
	x = filsysrespond(fs, x, &t, nil);
	free(t.stat);
	return x;
}

static
Xfid*
filsyswstat(Filsys *fs, Xfid *x, Fid*)
{
	Fcall t;

	return filsysrespond(fs, x, &t, Eperm);
}

static
Fid*
newfid(Filsys *fs, int fid)
{
	Fid *f, *ff, **fh;

	ff = nil;
	fh = &fs->fids[fid&(Nhash-1)];
	for(f=*fh; f; f=f->next)
		if(f->fid == fid)
			return f;
		else if(ff==nil && f->busy==FALSE)
			ff = f;
	if(ff){
		ff->fid = fid;
		return ff;
	}
	f = emalloc(sizeof *f);
	f->fid = fid;
	f->next = *fh;
	*fh = f;
	return f;
}

static
uint
getclock(void)
{
	char buf[32];

	seek(clockfd, 0, 0);
	read(clockfd, buf, sizeof buf);
	return atoi(buf);
}

static
int
dostat(Filsys *fs, int id, Dirtab *dir, uchar *buf, int nbuf, uint clock)
{
	Dir d;

	d.qid.path = QID(id, dir->qid);
	if(dir->qid == Qsnarf)
		d.qid.vers = snarfversion;
	else
		d.qid.vers = 0;
	d.qid.type = dir->type;
	d.mode = dir->perm;
	d.length = 0;	/* would be nice to do better */
	d.name = dir->name;
	d.uid = fs->user;
	d.gid = fs->user;
	d.muid = fs->user;
	d.atime = clock;
	d.mtime = clock;
	return convD2M(&d, buf, nbuf);
}
