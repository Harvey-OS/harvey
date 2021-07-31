#include <u.h>
#include <libc.h>
#include <draw.h>
#include <thread.h>
#include <cursor.h>
#include <mouse.h>
#include <keyboard.h>
#include <frame.h>
#include <auth.h>
#include <fcall.h>
#include "dat.h"
#include "fns.h"

char Eperm[] = "permission denied";
char Eexist[] = "file does not exist";
char Enotdir[] = "not a directory";
char	Ebadfcall[] = "bad fcall type";
char	Eoffset[] = "illegal offset";

Dirtab dirtab[]=
{
	{ ".",			Qdir|CHDIR,	0500|CHDIR },
	{ "cons",		Qcons,		0600 },
	{ "cursor",		Qcursor,		0600 },
	{ "consctl",	Qconsctl,		0200 },
	{ "winid",		Qwinid,		0400 },
	{ "winname",	Qwinname,	0400 },
	{ "kbdin",		Qkbdin,		0200 },
	{ "label",		Qlabel,		0600 },
	{ "mouse",	Qmouse,		0600 },
	{ "screen",		Qscreen,		0400 },
	{ "snarf",		Qsnarf,		0600 },
	{ "text",		Qtext,		0400 },
	{ "wdir",		Qwdir,		0600 },
	{ "wctl",		Qwctl,		0600 },
	{ "window",	Qwindow,		0400 },
	{ "wsys",		Qwsys|CHDIR,	0500|CHDIR },
	{ nil, }
};

static uint		getclock(void);
static void		filsysproc(void*);
static Fid*		newfid(Filsys*, int);
static void		dostat(Filsys*, int, Dirtab*, char*, uint);

int	clockfd;

char	srvpipe[64];
char	srvwctl[64];

static	Xfid*	filsysflush(Filsys*, Xfid*, Fid*);
static	Xfid*	filsyssession(Filsys*, Xfid*, Fid*);
static	Xfid*	filsysnop(Filsys*, Xfid*, Fid*);
static	Xfid*	filsysattach(Filsys*, Xfid*, Fid*);
static	Xfid*	filsysclone(Filsys*, Xfid*, Fid*);
static	Xfid*	filsyswalk(Filsys*, Xfid*, Fid*);
static	Xfid*	filsysclwalk(Filsys*, Xfid*, Fid*);
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
	[Tsession]	= filsyssession,
	[Tnop]	= filsysnop,
	[Tattach]	= filsysattach,
	[Tclone]	= filsysclone,
	[Twalk]	= filsyswalk,
	[Tclwalk]	= filsysclwalk,
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

	fd = create(name, OWRITE, 0600);
	if(fd < 0)
		error(name);
	sprint(buf, "%d",srvfd);
	if(write(fd, buf, strlen(buf)) != strlen(buf))
		error("srv write");
	close(fd);
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
	if(bind("#|", "/mnt", MREPL) < 0)
		return -1;
	*p0 = open("/mnt/data", ORDWR);
	*p1 = open("/mnt/data1", ORDWR|OCEXEC);
	unmount(nil, "/mnt");
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

	fs = emalloc(sizeof(Filsys));
	if(cexecpipe(&fs->cfd, &fs->sfd) < 0)
		goto Rescue;
	fmtinstall('F', fcallconv);
	clockfd = open("/dev/time", OREAD|OCEXEC);
	fd = open("/dev/user", OREAD);
	strncpy(fs->user, "Jean-Paul_Belmondo", NAMELEN);
	if(fd >= 0){
		n = read(fd, fs->user, NAMELEN);
		if(n > 0)
			fs->user[n] = 0;
		close(fd);
	}
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
	 * Post srv pipe
	 */
	sprint(srvpipe, "/srv/rio.%s.%d", fs->user, pid);
	post(srvpipe, "wsys", fs->cfd);

	/*
	 * Start server processes
	 */
	c = chancreate(sizeof(char*), 0);
	if(c == nil)
		error("wctl channel");
	proccreate(wctlproc, c, 4096);
	threadcreate(wctlthread, c, 4096);
	proccreate(filsysproc, fs, 10000);

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
	char *buf;
	Filsys *fs;

	threadsetname("FILSYSPROC");
	fs = arg;
	fs->pid = getpid();
	x = nil;
	for(;;){
		buf = malloc(MAXFDATA+MAXMSG);
		n = read(fs->sfd, buf, MAXFDATA+MAXMSG);
		if(n <= 0){
			errorshouldabort = 0;
			error("eof or i/o error on server channel");
		}
		if(x == nil){
			send(fs->cxfidalloc, nil);
			recv(fs->cxfidalloc, &x);
			x->fs = fs;
		}
		x->buf = buf;
		if(convM2S(buf, x, n) != n)
			error("convert error in convM2S");
		if(fcall[x->type] == nil)
			x = filsysrespond(fs, x, &t, Ebadfcall);
		else{
			if(x->type==Tnop || x->type==Tsession)
				f = nil;
			else
				f = newfid(fs, x->fid);
			x->f = f;
			x  = (*fcall[x->type])(fs, x, f);
		}
	}
}

void
filsysclose(Filsys *fs)
{
	if(fs == nil)
		return;
	close(fs->sfd);
	fs->sfd = -1;
	close(fs->cfd);
	fs->cfd = -1;
	remove(srvpipe);
	remove(srvwctl);
}

/*
 * Called only from a different FD group
 */
int
filsysmount(Filsys *fs, int id)
{
	char buf[3*NAMELEN+CHALLEN+DOMLEN+1];

	close(fs->sfd);	/* close server end so mount won't hang if exiting */
	if(fsession(fs->cfd, buf) < 0){
		threadprint(2, "fsession failed: %r\n");
		return -1;
	}
	sprint(buf, "%d", id);
	if(mount(fs->cfd, "/mnt/wsys", MREPL, buf) < 0){
		threadprint(2, "mount failed: %r\n");
		return -1;
	}
	if(bind("/mnt/wsys", "/dev", MBEFORE) < 0){
		threadprint(2, "bind failed: %r\n");
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
		strncpy(t->ename, err, ERRLEN);
	}else
		t->type = x->type+1;
	t->fid = x->fid;
	t->tag = x->tag;
	if(x->buf == nil)
		x->buf = malloc(MAXFDATA+MAXMSG);
	n = convS2M(t, x->buf);
	if(n < 0)
		error("convert error in convS2M");
	if(write(fs->sfd, x->buf, n) != n)
		error("write error in respond");
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
filsysnop(Filsys *fs, Xfid *x, Fid*)
{
	Fcall t;

	return filsysrespond(fs, x, &t, nil);
}

static
Xfid*
filsyssession(Filsys *fs, Xfid *x, Fid*)
{
	Fcall t;

	memset(&t, 0, sizeof t);
	return filsysrespond(fs, x, &t, nil);
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
	f->qid = (Qid){CHDIR|Qdir, 0};
	f->dir = dirtab;
	f->nrpart = 0;
	sendp(x->c, xfidattach);
	return nil;
}

static
Xfid*
filsysclone(Filsys *fs, Xfid *x, Fid *f)
{
	Fid *nf;
	Fcall t;

	if(f->open)
		return filsysrespond(fs, x, &t, "is open");
	/* BUG: check exists */
	nf = newfid(fs, x->newfid);
	nf->busy = TRUE;
	nf->open = FALSE;
	nf->dir = f->dir;
	nf->qid = f->qid;
	nf->w = f->w;
	incref(f->w);
	nf->nrpart = 0;	/* not open, so must be zero */
	return filsysrespond(fs, x, &t, nil);
}

static
Xfid*
filsyswalk(Filsys *fs, Xfid *x, Fid *f)
{
	Fcall t;
	int c, i, id;
	uint qid;
	Dirtab *d;
	Window *w;

	if((f->qid.path & CHDIR) == 0)
		return filsysrespond(fs, x, &t, Enotdir);
	if(strcmp(x->name, "..") == 0){
		qid = Qdir|CHDIR;
		if(FILE(f->qid) == Qwsysdir)
			qid = Qwsys|CHDIR;
		id = 0;
		goto Found;
	}
	if(f->qid.path == (CHDIR|Qwsys)){
		/* is it a numeric name? */
		for(i=0; (c=x->name[i]); i++)
			if(c<'0' || '9'<c)
				goto Notfound;
		/* yes: it's a directory */
		id = atoi(x->name);
		qlock(&all);
		w = wlookid(id);
		if(w == nil){
			qunlock(&all);
			goto Notfound;
		}
		qid = CHDIR|Qwsysdir;
		qunlock(&all);
		incref(w);
		sendp(winclosechan, f->w);
		f->w = w;
		f->dir = dirtab;
		goto Found;
	}

	if(snarffd>=0 && strcmp(x->name, "snarf")==0)
		goto Notfound;
	id = WIN(f->qid);
	d = dirtab;
	d++;	/* skip '.' */
	for(; d->name; d++)
		if(strcmp(x->name, d->name) == 0){
			qid = d->qid;
			f->dir = d;
			goto Found;
		}

    Notfound:
	return filsysrespond(fs, x, &t, Eexist);

    Found:
	f->qid = (Qid){QID(id, qid), 0};
	t.qid = f->qid;
	return filsysrespond(fs, x, &t, nil);
}

static
Xfid*
filsysclwalk(Filsys *fs, Xfid *x, Fid*)
{
	Fcall t;

	return filsysrespond(fs, x, &t, "clwalk not implemented");
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
	if(((f->dir->perm&~(CHDIR|CHAPPEND))&m) != m)
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
	char *b;
	int i, n, o, e, j, k, *ids;
	Dirtab *d, dt;
	uint clock;
	char buf[16];

	if((f->qid.path & CHDIR) == 0){
		sendp(x->c, xfidread);
		return nil;
	}
	if(x->offset % DIRLEN)
		return filsysrespond(fs, x, &t, Eoffset);
	o = x->offset;
	e = x->offset+x->count;
	clock = getclock();
	b = malloc(MAXFDATA+MAXMSG);
	n = 0;
	switch(FILE(f->qid)){
	case Qdir:
	case Qwsysdir:
		d = dirtab;
		d++;	/* first entry is '.' */
		for(i=0; d->name!=nil && i+DIRLEN<e; i+=DIRLEN){
			if(i >= o){
				dostat(fs, WIN(x->f->qid), d, b+n, clock);
				n += DIRLEN;
			}
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
		for(i=0, j=0; j<nwindow && i+DIRLEN<e; i+=DIRLEN){
			if(i >= o){
				k = ids[j];
				sprint(dt.name, "%d", k);
				dt.qid = QID(k, CHDIR);
				dt.perm = CHDIR|0700;
				dostat(fs, k, &dt, b+n, clock);
				n += DIRLEN;
			}
			j++;
		}
		free(ids);
		break;
	}
	t.data = b;
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

	dostat(fs, WIN(x->f->qid), f->dir, t.stat, getclock());
	return filsysrespond(fs, x, &t, nil);
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
void
dostat(Filsys *fs, int id, Dirtab *dir, char *buf, uint clock)
{
	Dir d;

	d.qid.path = QID(id, dir->qid);
	if(dir->qid == Qsnarf)
		d.qid.vers = snarfversion;
	else
		d.qid.vers = 0;
	d.mode = dir->perm;
	d.length = 0;	/* would be nice to do better */
	strncpy(d.name, dir->name, NAMELEN);
	memmove(d.uid, fs->user, NAMELEN);
	memmove(d.gid, fs->user, NAMELEN);
	d.atime = clock;
	d.mtime = clock;
	convD2M(&d, buf);
}
