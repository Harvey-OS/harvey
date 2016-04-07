/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <thread.h>
#include <keyboard.h>
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
	DEBUG = 1
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
static int		dostat(Filsys*, int, Dirtab*, uint8_t*, int, uint);

int	clockfd;
int	firstmessage = 1;

static	Xfid*	filsysflush(Filsys*, Xfid*, Fid*);
static	Xfid*	filsysversion(Filsys*, Xfid*, Fid*);
static	Xfid*	filsysauth(Filsys*, Xfid*, Fid*);
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
	print_func_entry();
	int fd;
	char buf[32];

	fd = create(name, OWRITE|ORCLOSE|OCEXEC, 0600);
	if(fd < 0)
		error(name);
	sprint(buf, "%d",srvfd);
	if(write(fd, buf, strlen(buf)) != strlen(buf))
		error("srv write");
	putenv(envname, name);
	print_func_exit();
}

/*
 * Build pipe with OCEXEC set on second fd.
 * Can't put it on both because we want to post one in /srv.
 */
int
cexecpipe(int *p0, int *p1)
{
	print_func_entry();
	/* pipe the hard way to get close on exec */
	if(bind("#|", "/mnt/temp", MREPL) < 0) {
		print_func_exit();
		return -1;
	}
	*p0 = open("/mnt/temp/data", ORDWR);
	*p1 = open("/mnt/temp/data1", ORDWR|OCEXEC);
	unmount(nil, "/mnt/temp");
	if(*p0<0 || *p1<0) {
		print_func_exit();
		return -1;
	}
	print_func_exit();
	return 0;
}

Filsys*
filsysinit(Channel *cxfidalloc)
{
	print_func_entry();
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

	print_func_exit();
	return fs;

Rescue:
	free(fs);
	print_func_exit();
	return nil;
}

static
void
filsysproc(void *arg)
{
	print_func_entry();
	int n;
	Xfid *x;
	Fid *f;
	Fcall t;
	uint8_t *buf;
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
	print_func_exit();
}

/*
 * Called only from a different FD group
 */
int
filsysmount(Filsys *fs, int id)
{
	print_func_entry();
	char buf[32];

	close(fs->sfd);	/* close server end so mount won't hang if exiting */
	sprint(buf, "%d", id);
fprint(2, "mount %s\n", buf);
	if(mount(fs->cfd, -1, "/mnt/wsys", MREPL, buf, 'M') < 0){
		fprint(2, "mount failed: %r\n");
		print_func_exit();
		return -1;
	}
	if(bind("/mnt/wsys", "/dev", MBEFORE) < 0){
		fprint(2, "bind failed: %r\n");
		print_func_exit();
		return -1;
	}
	print_func_exit();
	return 0;
}

Xfid*
filsysrespond(Filsys *fs, Xfid *x, Fcall *t, char *err)
{
	print_func_entry();
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
	print_func_exit();
	return x;
}

void
filsyscancel(Xfid *x)
{
	print_func_entry();
	if(x->buf){
		free(x->buf);
		x->buf = nil;
	}
	print_func_exit();
}

static 
Xfid*
filsysversion(Filsys *fs, Xfid *x, Fid* f)
{
	print_func_entry();
	Fcall t;

	if(!firstmessage) {
		print_func_exit();
		return filsysrespond(x->fs, x, &t, "version request not first message");
	}
	if(x->msize < 256) {
		print_func_exit();
		return filsysrespond(x->fs, x, &t, "version: message size too small");
	}
	messagesize = x->msize;
	t.msize = messagesize;
	if(strncmp(x->version, "9P2000", 6) != 0) {
		print_func_exit();
		return filsysrespond(x->fs, x, &t, "unrecognized 9P version");
	}
	t.version = "9P2000";
	print_func_exit();
	return filsysrespond(fs, x, &t, nil);
}

static
Xfid*
filsysauth(Filsys *fs, Xfid *x, Fid* f)
{
	print_func_entry();
	Fcall t;

		print_func_exit();
		return filsysrespond(fs, x, &t, "rio: authentication not required");
}

static
Xfid*
filsysflush(Filsys* fs, Xfid *x, Fid* f)
{
	print_func_entry();
	sendp(x->c, xfidflush);
	print_func_exit();
	return nil;
}

static
Xfid*
filsysattach(Filsys * fs, Xfid *x, Fid *f)
{
	print_func_entry();
	Fcall t;

	if(strcmp(x->uname, x->fs->user) != 0) {
		print_func_exit();
		return filsysrespond(x->fs, x, &t, Eperm);
	}
	f->busy = TRUE;
	f->open = FALSE;
	f->qid.path = Qdir;
	f->qid.type = QTDIR;
	f->qid.vers = 0;
	f->dir = dirtab;
	f->nrpart = 0;
	sendp(x->c, xfidattach);
	print_func_exit();
	return nil;
}

static
int
numeric(char *s)
{
	print_func_entry();
	for(; *s!='\0'; s++)
		if(*s<'0' || '9'<*s) {
			print_func_exit();
			return 0;
		}
	print_func_exit();
	return 1;
}

static
Xfid*
filsyswalk(Filsys *fs, Xfid *x, Fid *f)
{
	print_func_entry();
	Fcall t;
	Fid *nf;
	int i, id;
	uint8_t type;
	uint32_t path;
	Dirtab *d, *dir;
	Window *w;
	char *err;
	Qid qid;

	if (DEBUG)
		fprint(2, "f %p\n", f);
	if (! f) {
		print_func_exit();
		return filsysrespond(fs, x, &t, "NO FID!");
	}

	if (DEBUG)
		fprint(2, "FID %f\n", f);

	if(f->open) {
		print_func_exit();
		return filsysrespond(fs, x, &t, "walk of open file");
	}
	nf = nil;
	if(x->fid  != x->newfid){
		/* BUG: check exists */
		nf = newfid(fs, x->newfid);
		if(nf->busy) {
			print_func_exit();
			return filsysrespond(fs, x, &t, "clone to busy fid");
		}
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

	print_func_exit();
	return filsysrespond(fs, x, &t, err);
}

static
Xfid*
filsysopen(Filsys *fs, Xfid *x, Fid *f)
{
	print_func_entry();
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
	print_func_exit();
	return nil;

    Deny:
	print_func_exit();
	return filsysrespond(fs, x, &t, Eperm);
}

static
Xfid*
filsyscreate(Filsys *fs, Xfid *x, Fid*f)
{
	print_func_entry();
	Fcall t;

	print_func_exit();
	return filsysrespond(fs, x, &t, Eperm);
}

static
int
idcmp(const void *a, const void *b)
{
	print_func_entry();
	print_func_exit();
	return *(int*)a - *(int*)b;
}

static
Xfid*
filsysread(Filsys *fs, Xfid *x, Fid *f)
{
	print_func_entry();
	Fcall t;
	uint8_t *b;
	int i, n, o, e, len, j, k, *ids;
	Dirtab *d, dt;
	uint clock;
	char buf[16];

	if((f->qid.type & QTDIR) == 0){
		sendp(x->c, xfidread);
		print_func_exit();
		return nil;
	}
	o = x->offset;
	e = x->offset+x->count;
	clock = getclock();
	b = malloc(messagesize-IOHDRSZ);	/* avoid memset of emalloc */
	if(b == nil) {
	print_func_exit();
	return filsysrespond(fs, x, &t, "out of memory");
	}
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
	print_func_exit();
	return x;
}

static
Xfid*
filsyswrite(Filsys* fs, Xfid *x, Fid*f)
{
	print_func_entry();
	sendp(x->c, xfidwrite);
	print_func_exit();
	return nil;
}

static
Xfid*
filsysclunk(Filsys *fs, Xfid *x, Fid *f)
{
	print_func_entry();
	Fcall t;

	if(f->open){
		f->busy = FALSE;
		f->open = FALSE;
		sendp(x->c, xfidclose);
		print_func_exit();
		return nil;
	}
	if(f->w)
		sendp(winclosechan, f->w);
	f->busy = FALSE;
	f->open = FALSE;
	print_func_exit();
	return filsysrespond(fs, x, &t, nil);
}

static
Xfid*
filsysremove(Filsys *fs, Xfid *x, Fid*f)
{
	print_func_entry();
	Fcall t;

	print_func_exit();
	return filsysrespond(fs, x, &t, Eperm);
}

static
Xfid*
filsysstat(Filsys *fs, Xfid *x, Fid *f)
{
	print_func_entry();
	Fcall t;

	t.stat = emalloc(messagesize-IOHDRSZ);
	t.nstat = dostat(fs, WIN(x->f->qid), f->dir, t.stat, messagesize-IOHDRSZ, getclock());
	x = filsysrespond(fs, x, &t, nil);
	free(t.stat);
	print_func_exit();
	return x;
}

static
Xfid*
filsyswstat(Filsys *fs, Xfid *x, Fid*f)
{
	print_func_entry();
	Fcall t;

	print_func_exit();
	return filsysrespond(fs, x, &t, Eperm);
}

static
Fid*
newfid(Filsys *fs, int fid)
{
	print_func_entry();
	Fid *f, *ff, **fh;

	ff = nil;
	fh = &fs->fids[fid&(Nhash-1)];
	for(f=*fh; f; f=f->next)
		if(f->fid == fid) {
			print_func_exit();
			return f;
		}
		else if(ff==nil && f->busy==FALSE)
			ff = f;
	if(ff){
		ff->fid = fid;
		print_func_exit();
		return ff;
	}
	f = emalloc(sizeof *f);
	f->fid = fid;
	f->next = *fh;
	*fh = f;
	print_func_exit();
	return f;
}

static
uint
getclock(void)
{
	print_func_entry();
	char buf[32];

	seek(clockfd, 0, 0);
	read(clockfd, buf, sizeof buf);
	print_func_exit();
	return atoi(buf);
}

static
int
dostat(Filsys *fs, int id, Dirtab *dir, uint8_t *buf, int nbuf, uint clock)
{
	print_func_entry();
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
	print_func_exit();
	return convD2M(&d, buf, nbuf);
}
int
fidfmt(Fmt *f)
{
	Fid *fid;

	fid = va_arg(f->args, Fid*);
	if (! fid)
		return fmtprint(f, "<NULL>");
	return fmtprint(f, "fid %d, %s%s, %W\n", 
			fid->fid, fid->busy ? "busy," : "",
			fid->open ? "open, " : "",
			fid->w);
}
