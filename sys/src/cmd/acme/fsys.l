#include <alef.h>
#include <libg.h>
#include <frame.h>
#include <fcall.h>
#include "dat.h"
#include "fns.h"

intern	int	cfd;
intern	int	sfd;

enum
{
	Nhash	= 16,
	DEBUG	= 0
};

intern	Fid	*fids[Nhash];

Fid	*newfid(int);

Xfid	*fsysnop(Xfid*, Fid*);
Xfid	*fsyssession(Xfid*, Fid*);
Xfid	*fsysflush(Xfid*, Fid*);
Xfid	*fsysattach(Xfid*, Fid*);
Xfid	*fsysclone(Xfid*, Fid*);
Xfid	*fsyswalk(Xfid*, Fid*);
Xfid	*fsysclwalk(Xfid*, Fid*);
Xfid	*fsysopen(Xfid*, Fid*);
Xfid	*fsyscreate(Xfid*, Fid*);
Xfid	*fsysread(Xfid*, Fid*);
Xfid	*fsyswrite(Xfid*, Fid*);
Xfid	*fsysclunk(Xfid*, Fid*);
Xfid	*fsysremove(Xfid*, Fid*);
Xfid	*fsysstat(Xfid*, Fid*);
Xfid	*fsyswstat(Xfid*, Fid*);

Xfid* 	(*fcall[])(Xfid*, Fid*) = {
	[Tflush]	fsysflush,
	[Tsession]	fsyssession,
	[Tnop]	fsysnop,
	[Tattach]	fsysattach,
	[Tclone]	fsysclone,
	[Twalk]	fsyswalk,
	[Tclwalk]	fsysclwalk,
	[Topen]	fsysopen,
	[Tcreate]	fsyscreate,
	[Tread]	fsysread,
	[Twrite]	fsyswrite,
	[Tclunk]	fsysclunk,
	[Tremove]fsysremove,
	[Tstat]	fsysstat,
	[Twstat]	fsyswstat,
};

byte *Eperm = "permission denied";
byte *Eexist = "file does not exist";
byte *Enotdir = "not a directory";

Dirtab dirtab[]=
{
	{ ".",			Qdir|CHDIR,	0500|CHDIR },
	{ "acme",		Qacme|CHDIR,	0500|CHDIR },
	{ "cons",		Qcons,		0600 },
	{ "consctl",	Qconsctl,		0000 },
	{ "index",		Qindex,		0400 },
	{ "label",		Qlabel,		0600 },
	{ "new",		Qnew,		0500|CHDIR },
	{ nil, }
};

Dirtab dirtabw[]=
{
	{ ".",			Qdir|CHDIR,	0500|CHDIR },
	{ "addr",		QWaddr,		0600 },
	{ "body",		QWbody,		0600|CHAPPEND },
	{ "ctl",		QWctl,		0600 },
	{ "data",		QWdata,		0600 },
	{ "event",		QWevent,		0600 },
	{ "tag",		QWtag,		0600|CHAPPEND },
	{ nil, }
};

aggr Mnt
{
	QLock;
	int		id;
	Mntdir	*md;
};

Mnt	mnt;

Xfid*	respond(Xfid *, Fcall*, byte*);
void	dostat(int, Dirtab*, byte*, uint);
uint	getclock();

byte	user[NAMELEN];
int	clockfd;

void	fsysproc();

void
fsysinit()
{
	int p[2];
	int n, fd;

	if(pipe(p) < 0)
		error("can't create pipe");
	cfd = p[0];
	sfd = p[1];
	fmtinstall('F', fcallconv);
	clockfd = open("/dev/time", OREAD|OCEXEC);
	fd = open("/dev/user", OREAD);
	strcpy(user, "Wile. E. Coyote");
	if(fd >= 0){
		n = read(fd, user, NAMELEN);
		if(n > 0)
			user[n] = 0;
		close(fd);
	}
	proc fsysproc();
}

void
fsysproc()
{
	int n;
	Xfid *x;
	Fid *f;
	Fcall t;
	byte *buf;

	fsyspid = getpid();
	x = nil;
	for(;;){
		buf = fbufalloc();
		n = read(sfd, buf, MAXRPC);
		if(n <= 0)
			error("i/o error on server channel");
		if(x == nil){
			cxfidalloc <-= nil;
			x = <-cxfidalloc;
		}
		x->buf = buf;
		if(convM2S(buf, x, n) != n)
			error("convert error in convM2S");
		if(DEBUG)
			fprint(2, "%F\n", &x->Fcall);
		if(fcall[x->type] == nil)
			x = respond(x, &t, "bad fcall type");
		else{
			if(x->type==Tnop || x->type==Tsession)
				f = nil;
			else
				f = newfid(x->fid);
			x->f = f;
			x  = (*fcall[x->type])(x, f);
		}
	}
}

Mntdir*
fsysaddid(Rune *dir, int ndir, Rune **incl, int nincl)
{
	Mntdir *m;
	int id;

	mnt.lock();
	id = ++mnt.id;
	m = malloc(sizeof *m);
	m->id = id;
	m->dir =  dir;
	m->ref = 1;	/* one for Command, one will be incremented in attach */
	m->ndir = ndir;
	m->next = mnt.md;
	m->incl = incl;
	m->nincl = nincl;
	mnt.md = m;
	mnt.unlock();
	return m;
}

void
fsysdelid(Mntdir *idm)
{
	Mntdir *m, *prev;
	int i;
	byte buf[64];

	if(idm == nil)
		return;
	mnt.lock();
	if(--idm->ref > 0){
		mnt.unlock();
		return;
	}
	prev = nil;
	for(m=mnt.md; m; m=m->next){
		if(m == idm){
			if(prev)
				prev->next = m->next;
			else
				mnt.md = m->next;
			for(i=0; i<m->nincl; i++)
				free(m->incl[i]);
			free(m->incl);
			free(m->dir);
			free(m);
			mnt.unlock();
			return;
		}
		prev = m;
	}
	mnt.unlock();
	sprint(buf, "fsysdelid: can't find id %d\n", idm->id);
	cerr <-= strdup(buf);
}

/*
 * Called only in exec.l:run(), from a different FD group
 */
Mntdir*
fsysmount(Rune *dir, int ndir, Rune **incl, int nincl)
{
	byte buf[16];
	Mntdir *m;

	/* close server side so don't hang if acme is half-exited */
	close(sfd);
	m = fsysaddid(dir, ndir, incl, nincl);
	sprint(buf, "%d", m->id);
	if(mount(cfd, "/mnt/acme", MREPL, buf) < 0){
		fsysdelid(m);
		return nil;
	}
	close(cfd);
	bind("/mnt/acme", "/mnt/8½", MREPL);
	if(bind("/mnt/acme", "/dev", MBEFORE) < 0){
		fsysdelid(m);
		return nil;
	}
	return m;
}

void
fsysclose()
{
	close(cfd);
	close(sfd);
}

Xfid*
respond(Xfid *x, Fcall *t, byte *err)
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
		x->buf = fbufalloc();
	n = convS2M(t, x->buf);
	if(n < 0)
		error("convert error in convS2M");
	if(write(sfd, x->buf, n) != n)
		error("write error in respond");
	fbuffree(x->buf);
	x->buf = nil;
	if(DEBUG)
		fprint(2, "r: %F\n", t);
	return x;
}

Xfid*
fsysnop(Xfid *x, Fid*)
{
	Fcall t;

	return respond(x, &t, nil);
}

Xfid*
fsyssession(Xfid *x, Fid*)
{
	Fcall t;

	/* BUG: should shut everybody down ?? */
	memset(&t, 0, sizeof t);
	return respond(x, &t, nil);
}

Xfid*
fsysflush(Xfid *x, Fid*)
{
	x->c <-= x->flush;
	return nil;
}

Xfid*
fsysattach(Xfid *x, Fid *f)
{
	Fcall t;
	int id;
	Mntdir *m;

	f->busy = TRUE;
	f->open = FALSE;
	f->qid = (Qid)(CHDIR|Qdir, 0);
	f->dir = dirtab;
	f->nrpart = 0;
	f->w = nil;
	t.qid = f->qid;
	f->mntdir = nil;
	id = atoi(x->aname);
	mnt.lock();
	for(m=mnt.md; m; m=m->next)
		if(m->id == id){
			f->mntdir = m;
			m->ref++;
			break;
		}
	if(m == nil)
		cerr <-= strdup("unknown id in attach");
	mnt.unlock();
	return respond(x, &t, nil);
}

Xfid*
fsysclone(Xfid *x, Fid *f)
{
	Fid *nf;
	Fcall t;

	if(f->open)
		return respond(x, &t, "is open");
	/* BUG: check exists */
	nf = newfid(x->newfid);
	nf->busy = TRUE;
	nf->open = FALSE;
	nf->mntdir = f->mntdir;
	if(f->mntdir)
		f->mntdir->ref++;
	nf->dir = f->dir;
	nf->qid = f->qid;
	nf->w = f->w;
	nf->nrpart = 0;	/* not open, so must be zero */
	if(nf->w)
		nf->w->inc();
	return respond(x, &t, nil);
}

Xfid*
fsyswalk(Xfid *x, Fid *f)
{
	Fcall t;
	int c, i, id;
	uint qid;
	Dirtab *d;
	Window *w;

	if((f->qid.path & CHDIR) == 0)
		return respond(x, &t, Enotdir);
	if(strcmp(x->name, "..") == 0){
		qid = Qdir|CHDIR;
		id = 0;
		goto Found;
	}
	/* is it a numeric name? */
	for(i=0; (c=x->name[i]); i++)
		if(c<'0' || '9'<c)
			goto Regular;
	/* yes: it's a directory */
	id = atoi(x->name);
	row.lock();
	w = lookid(id, FALSE);
	if(w == nil){
		row.unlock();
		goto Notfound;
	}
	w->inc();
	qid = CHDIR|Qdir;
	row.unlock();
	f->dir = dirtabw;
	f->w = w;
	goto Found;

    Regular:
	if(FILE(f->qid) == Qacme)	/* empty directory */
		goto Notfound;
	id = WIN(f->qid);
	if(id == 0)
		d = dirtab;
	else
		d = dirtabw;
	d++;	/* skip '.' */
	for(; d->name; d++)
		if(strcmp(x->name, d->name) == 0){
			qid = d->qid;
			f->dir = d;
			goto Found;
		}

    Notfound:
	return respond(x, &t, Eexist);

    Found:
	f->qid = (Qid)(QID(id, qid), 0);
	if(strcmp(x->name, "new") == 0){
		f->dir = dirtabw;
		x->c <-= x->walk;
		return nil;
	}
	t.qid = f->qid;
	return respond(x, &t, nil);
}

Xfid*
fsysclwalk(Xfid *x, Fid*)
{
	Fcall t;

	return respond(x, &t, "clwalk not implemented");
}

Xfid*
fsysopen(Xfid *x, Fid *f)
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
		
	x->c <-= x->open;
	return nil;

    Deny:
	return respond(x, &t, Eperm);
}

Xfid*
fsyscreate(Xfid *x, Fid*)
{
	Fcall t;

	return respond(x, &t, Eperm);
}

int
idcmp(void *a, void *b)
{
	return *(int*)a - *(int*)b;
}

Xfid*
fsysread(Xfid *x, Fid *f)
{
	Fcall t;
	byte *b;
	int i, id, n, o, e, j, k, *ids, nids;
	Dirtab *d, dt;
	Column *c;
	uint clock;
	byte buf[16];

	if(f->qid.path & CHDIR){
		if(x->offset % DIRLEN)
			return respond(x, &t, "illegal offset in directory");
		if(FILE(f->qid) == Qacme){	/* empty dir */
			t.data = nil;
			t.count = 0;
			respond(x, &t, nil);
			return x;
		}
		o = x->offset;
		e = x->offset+x->count;
		clock = getclock();
		b = fbufalloc();
		id = WIN(f->qid);
		n = 0;
		if(id > 0)
			d = dirtabw;
		else
			d = dirtab;
		d++;	/* first entry is '.' */
		for(i=0; d->name!=nil && i+DIRLEN<e; i+=DIRLEN){
			if(i >= o){
				dostat(WIN(x->f->qid), d, b+n, clock);
				n += DIRLEN;
			}
			d++;
		}
		if(id == 0){
			row.lock();
			nids = 0;
			ids = nil;
			for(j=0; j<row.ncol; j++){
				c = row.col[j];
				for(k=0; k<c->nw; k++){
					ids = realloc(ids, (nids+1)*sizeof(int));
					ids[nids++] = c->w[k]->id;
				}
			}
			row.unlock();
			qsort(ids, nids, sizeof ids[0], idcmp);
			j = 0;
			dt.name = buf;
			for(; j<nids && i+DIRLEN<e; i+=DIRLEN){
				if(i >= o){
					k = ids[j];
					sprint(dt.name, "%d", k);
					dt.qid = QID(k, CHDIR);
					dt.perm = CHDIR|0700;
					dostat(k, &dt, b+n, clock);
					n += DIRLEN;
				}
				j++;
			}
			free(ids);
		}
		t.data = b;
		t.count = n;
		respond(x, &t, nil);
		fbuffree(b);
		return x;
	}
	x->c <-= x->read;
	return nil;
}

Xfid*
fsyswrite(Xfid *x, Fid*)
{
	x->c <-= x->write;
	return nil;
}

Xfid*
fsysclunk(Xfid *x, Fid *f)
{
	Fcall t;

	fsysdelid(f->mntdir);
	if(f->open){
		f->busy = FALSE;
		f->open = FALSE;
		x->c <-= x->close;
		return nil;
	}
	if(f->w)
		f->w->close();
	f->busy = FALSE;
	f->open = FALSE;
	return respond(x, &t, nil);
}

Xfid*
fsysremove(Xfid *x, Fid*)
{
	Fcall t;

	return respond(x, &t, Eperm);
}

Xfid*
fsysstat(Xfid *x, Fid *f)
{
	Fcall t;

	dostat(WIN(x->f->qid), f->dir, t.stat, getclock());
	return respond(x, &t, nil);
}

Xfid*
fsyswstat(Xfid *x, Fid*)
{
	Fcall t;

	return respond(x, &t, Eperm);
}

Fid*
newfid(int fid)
{
	Fid *f, *ff, **fh;

	ff = nil;
	fh = &fids[fid&(Nhash-1)];
	for(f=*fh; f; f=f->next)
		if(f->fid == fid)
			return f;
		else if(ff==nil && f->busy==FALSE)
			ff = f;
	if(ff){
		ff->fid = fid;
		return ff;
	}
	f = malloc(sizeof *f);
	f->fid = fid;
	f->next = *fh;
	*fh = f;
	return f;
}

uint
getclock()
{
	byte buf[32];

	seek(clockfd, 0, 0);
	read(clockfd, buf, sizeof buf);
	return atoi(buf);
}

void
dostat(int id, Dirtab *dir, byte *buf, uint clock)
{
	Dir d;

	d.qid.path = QID(id, dir->qid);
	d.qid.vers = 0;
	d.mode = dir->perm;
	d.length = 0;	/* would be nice to do better */
	d.hlength = 0;
	strcpy(d.name, dir->name);
	memmove(d.uid, user, NAMELEN);
	memmove(d.gid, user, NAMELEN);
	d.atime = clock;
	d.mtime = clock;
	convD2M(&d, buf);
}
