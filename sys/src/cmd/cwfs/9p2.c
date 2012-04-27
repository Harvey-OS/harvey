#include "all.h"
#include <fcall.h>

enum { MSIZE = MAXDAT+MAXMSG };

static int
mkmode9p1(ulong mode9p2)
{
	int mode;

	/*
	 * Assume this is for an allocated entry.
	 */
	mode = DALLOC|(mode9p2 & 0777);
	if(mode9p2 & DMEXCL)
		mode |= DLOCK;
	if(mode9p2 & DMAPPEND)
		mode |= DAPND;
	if(mode9p2 & DMDIR)
		mode |= DDIR;

	return mode;
}

void
mkqid9p1(Qid9p1* qid9p1, Qid* qid)
{
	if(qid->path & 0xFFFFFFFF00000000LL)
		panic("mkqid9p1: path %lluX", (Wideoff)qid->path);
	qid9p1->path = qid->path & 0xFFFFFFFF;
	if(qid->type & QTDIR)
		qid9p1->path |= QPDIR;
	qid9p1->version = qid->vers;
}

static int
mktype9p2(int mode9p1)
{
	int type;

	type = 0;
	if(mode9p1 & DLOCK)
		type |= QTEXCL;
	if(mode9p1 & DAPND)
		type |= QTAPPEND;
	if(mode9p1 & DDIR)
		type |= QTDIR;

	return type;
}

static ulong
mkmode9p2(int mode9p1)
{
	ulong mode;

	mode = mode9p1 & 0777;
	if(mode9p1 & DLOCK)
		mode |= DMEXCL;
	if(mode9p1 & DAPND)
		mode |= DMAPPEND;
	if(mode9p1 & DDIR)
		mode |= DMDIR;

	return mode;
}

void
mkqid9p2(Qid* qid, Qid9p1* qid9p1, int mode9p1)
{
	qid->path = (ulong)(qid9p1->path & ~QPDIR);
	qid->vers = qid9p1->version;
	qid->type = mktype9p2(mode9p1);
}

static int
mkdir9p2(Dir* dir, Dentry* dentry, void* strs)
{
	char *op, *p;

	memset(dir, 0, sizeof(Dir));
	mkqid(&dir->qid, dentry, 1);
	dir->mode = mkmode9p2(dentry->mode);
	dir->atime = dentry->atime;
	dir->mtime = dentry->mtime;
	dir->length = dentry->size;

	op = p = strs;
	dir->name = p;
	p += sprint(p, "%s", dentry->name)+1;

	dir->uid = p;
	uidtostr(p, dentry->uid, 1);
	p += strlen(p)+1;

	dir->gid = p;
	uidtostr(p, dentry->gid, 1);
	p += strlen(p)+1;

	dir->muid = p;
	uidtostr(p, dentry->muid, 1);
	p += strlen(p)+1;

	return p-op;
}

static int
checkname9p2(char* name)
{
	char *p;

	/*
	 * Return error or 0 if OK.
	 */
	if(name == nil || *name == 0)
		return Ename;

	for(p = name; *p != 0; p++){
		if(p-name >= NAMELEN-1)
			return Etoolong;
		if((*p & 0xFF) <= 040)
			return Ename;
	}
	if(strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
		return Edot;

	return 0;
}

static int
version(Chan* chan, Fcall* f, Fcall* r)
{
	if(chan->protocol != nil)
		return Eversion;

	if(f->msize < MSIZE)
		r->msize = f->msize;
	else
		r->msize = MSIZE;

	/*
	 * Should check the '.' stuff here.
	 */
	if(strcmp(f->version, VERSION9P) == 0){
		r->version = VERSION9P;
		chan->protocol = serve9p2;
		chan->msize = r->msize;
	} else
		r->version = "unknown";

	fileinit(chan);
	return 0;
}

struct {
	Lock;
	ulong	hi;
} authpath;

static int
auth(Chan* chan, Fcall* f, Fcall* r)
{
	char *aname;
	File *file;
	Filsys *fs;
	int error;

	if(cons.flags & authdisableflag)
		return Eauthdisabled;

	error = 0;
	aname = f->aname;

	if(strcmp(f->uname, "none") == 0)
		return Eauthnone;

	if(!aname[0])	/* default */
		aname = "main";
	file = filep(chan, f->afid, 1);
	if(file == nil){
		error = Efidinuse;
		goto out;
	}
	fs = fsstr(aname);
	if(fs == nil){
		error = Ebadspc;
		goto out;
	}
	lock(&authpath);
	file->qid.path = authpath.hi++;
	unlock(&authpath);
	file->qid.type = QTAUTH;
	file->qid.vers = 0;
	file->fs = fs;
	file->open = FREAD+FWRITE;
	freewp(file->wpath);
	file->wpath = 0;
	file->auth = authnew(f->uname, f->aname);
	if(file->auth == nil){
		error = Eauthfile;
		goto out;
	}
	r->aqid = file->qid;

out:
	if((cons.flags & attachflag) && error)
		print("9p2: auth %s %T SUCK EGGS --- %s\n",
			f->uname, time(nil), errstr9p[error]);
	if(file != nil){
		qunlock(file);
		if(error)
			freefp(file);
	}
	return error;
}

int
authorize(Chan* chan, Fcall* f)
{
	File* af;
	int db, uid = -1;

	db = cons.flags & authdebugflag;

	if(strcmp(f->uname, "none") == 0){
		uid = strtouid(f->uname);
		if(db)
			print("permission granted to none: uid %s = %d\n",
				f->uname, uid);
		return uid;
	}

	if(cons.flags & authdisableflag){
		uid = strtouid(f->uname);
		if(db)
			print("permission granted by authdisable uid %s = %d\n",
				f->uname, uid);
		return uid;
	}

	af = filep(chan, f->afid, 0);
	if(af == nil){
		if(db)
			print("authorize: af == nil\n");
		return -1;
	}
	if(af->auth == nil){
		if(db)
			print("authorize: af->auth == nil\n");
		goto out;
	}
	if(strcmp(f->uname, authuname(af->auth)) != 0){
		if(db)
			print("authorize: strcmp(f->uname, authuname(af->auth)) != 0\n");
		goto out;
	}
	if(strcmp(f->aname, authaname(af->auth)) != 0){
		if(db)
			print("authorize: strcmp(f->aname, authaname(af->auth)) != 0\n");
		goto out;
	}
	uid = authuid(af->auth);
	if(db)
		print("authorize: uid is %d\n", uid);
out:
	qunlock(af);
	return uid;
}

static int
attach(Chan* chan, Fcall* f, Fcall* r)
{
	char *aname;
	Iobuf *p;
	Dentry *d;
	File *file;
	Filsys *fs;
	Off raddr;
	int error, u;

	aname = f->aname;
	if(!aname[0])	/* default */
		aname = "main";
	p = nil;
	error = 0;
	file = filep(chan, f->fid, 1);
	if(file == nil){
		error = Efidinuse;
		goto out;
	}

	u = -1;
	if(chan != cons.chan){
		if(noattach && strcmp(f->uname, "none")) {
			error = Enoattach;
			goto out;
		}
		u = authorize(chan, f);
		if(u < 0){
			error = Ebadu;
			goto out;
		}
	}
	file->uid = u;

	fs = fsstr(aname);
	if(fs == nil){
		error = Ebadspc;
		goto out;
	}
	raddr = getraddr(fs->dev);
	p = getbuf(fs->dev, raddr, Brd);
	if(p == nil || checktag(p, Tdir, QPROOT)){
		error = Ealloc;
		goto out;
	}
	d = getdir(p, 0);
	if(d == nil || !(d->mode & DALLOC)){
		error = Ealloc;
		goto out;
	}
	if (iaccess(file, d, DEXEC) ||
	    file->uid == 0 && fs->dev->type == Devro) {
		/*
		 * 'none' not allowed on dump
		 */
		error = Eaccess;
		goto out;
	}
	accessdir(p, d, FREAD, file->uid);
	mkqid(&file->qid, d, 1);
	file->fs = fs;
	file->addr = raddr;
	file->slot = 0;
	file->open = 0;
	freewp(file->wpath);
	file->wpath = 0;

	r->qid = file->qid;

	strncpy(chan->whoname, f->uname, sizeof(chan->whoname));
	chan->whotime = time(nil);
	if(cons.flags & attachflag)
		print("9p2: attach %s %T to \"%s\" C%d\n",
			chan->whoname, chan->whotime, fs->name, chan->chan);

out:
	if((cons.flags & attachflag) && error)
		print("9p2: attach %s %T SUCK EGGS --- %s\n",
			f->uname, time(nil), errstr9p[error]);
	if(p != nil)
		putbuf(p);
	if(file != nil){
		qunlock(file);
		if(error)
			freefp(file);
	}
	return error;
}

static int
flush(Chan* chan, Fcall*, Fcall*)
{
	runlock(&chan->reflock);
	wlock(&chan->reflock);
	wunlock(&chan->reflock);
	rlock(&chan->reflock);

	return 0;
}

static void
clone(File* nfile, File* file)
{
	Wpath *wpath;

	nfile->qid = file->qid;

	lock(&wpathlock);
	nfile->wpath = file->wpath;
	for(wpath = nfile->wpath; wpath != nil; wpath = wpath->up)
		wpath->refs++;
	unlock(&wpathlock);

	nfile->fs = file->fs;
	nfile->addr = file->addr;
	nfile->slot = file->slot;
	nfile->uid = file->uid;
	nfile->open = file->open & ~FREMOV;
}

static int
walkname(File* file, char* wname, Qid* wqid)
{
	Wpath *w;
	Iobuf *p, *p1;
	Dentry *d, *d1;
	int error, slot;
	Off addr, qpath;

	p = p1 = nil;

	/*
	 * File must not have been opened for I/O by an open
	 * or create message and must represent a directory.
	 */
	if(file->open != 0){
		error = Emode;
		goto out;
	}

	p = getbuf(file->fs->dev, file->addr, Brd);
	if(p == nil || checktag(p, Tdir, QPNONE)){
		error = Edir1;
		goto out;
	}
	d = getdir(p, file->slot);
	if(d == nil || !(d->mode & DALLOC)){
		error = Ealloc;
		goto out;
	}
	if(!(d->mode & DDIR)){
		error = Edir1;
		goto out;
	}
	if(error = mkqidcmp(&file->qid, d))
		goto out;

	/*
	 * For walked elements the implied user must
	 * have permission to search the directory.
	 */
	if(file->cp != cons.chan && iaccess(file, d, DEXEC)){
		error = Eaccess;
		goto out;
	}
	accessdir(p, d, FREAD, file->uid);

	if(strcmp(wname, ".") == 0){
setdot:
		if(wqid != nil)
			*wqid = file->qid;
		goto out;
	}
	if(strcmp(wname, "..") == 0){
		if(file->wpath == 0)
			goto setdot;
		putbuf(p);
		p = nil;
		addr = file->wpath->addr;
		slot = file->wpath->slot;
		p1 = getbuf(file->fs->dev, addr, Brd);
		if(p1 == nil || checktag(p1, Tdir, QPNONE)){
			error = Edir1;
			goto out;
		}
		d1 = getdir(p1, slot);
		if(d == nil || !(d1->mode & DALLOC)){
			error = Ephase;
			goto out;
		}
		lock(&wpathlock);
		file->wpath->refs--;
		file->wpath = file->wpath->up;
		unlock(&wpathlock);
		goto found;
	}

	for(addr = 0; ; addr++){
		if(p == nil){
			p = getbuf(file->fs->dev, file->addr, Brd);
			if(p == nil || checktag(p, Tdir, QPNONE)){
				error = Ealloc;
				goto out;
			}
			d = getdir(p, file->slot);
			if(d == nil || !(d->mode & DALLOC)){
				error = Ealloc;
				goto out;
			}
		}
		qpath = d->qid.path;
		p1 = dnodebuf1(p, d, addr, 0, file->uid);
		p = nil;
		if(p1 == nil || checktag(p1, Tdir, qpath)){
			error = Eentry;
			goto out;
		}
		for(slot = 0; slot < DIRPERBUF; slot++){
			d1 = getdir(p1, slot);
			if (!(d1->mode & DALLOC) ||
			    strncmp(wname, d1->name, NAMELEN) != 0)
				continue;
			/*
			 * update walk path
			 */
			if((w = newwp()) == nil){
				error = Ewalk;
				goto out;
			}
			w->addr = file->addr;
			w->slot = file->slot;
			w->up = file->wpath;
			file->wpath = w;
			slot += DIRPERBUF*addr;
			goto found;
		}
		putbuf(p1);
		p1 = nil;
	}

found:
	file->addr = p1->addr;
	mkqid(&file->qid, d1, 1);
	putbuf(p1);
	p1 = nil;
	file->slot = slot;
	if(wqid != nil)
		*wqid = file->qid;

out:
	if(p1 != nil)
		putbuf(p1);
	if(p != nil)
		putbuf(p);

	return error;
}

static int
walk(Chan* chan, Fcall* f, Fcall* r)
{
	int error, nwname;
	File *file, *nfile, tfile;

	/*
	 * The file identified by f->fid must be valid in the
	 * current session and must not have been opened for I/O
	 * by an open or create message.
	 */
	if((file = filep(chan, f->fid, 0)) == nil)
		return Efid;
	if(file->open != 0){
		qunlock(file);
		return Emode;
	}

	/*
	 * If newfid is not the same as fid, allocate a new file;
	 * a side effect is checking newfid is not already in use (error);
	 * if there are no names to walk this will be equivalent to a
	 * simple 'clone' operation.
	 * Otherwise, fid and newfid are the same and if there are names
	 * to walk make a copy of 'file' to be used during the walk as
	 * 'file' must only be updated on success.
	 * Finally, it's a no-op if newfid is the same as fid and f->nwname
	 * is 0.
	 */
	r->nwqid = 0;
	if(f->newfid != f->fid){
		if((nfile = filep(chan, f->newfid, 1)) == nil){
			qunlock(file);
			return Efidinuse;
		}
	} else if(f->nwname != 0){
		nfile = &tfile;
		memset(nfile, 0, sizeof(File));
		nfile->cp = chan;
		nfile->fid = ~0;
	} else {
		qunlock(file);
		return 0;
	}
	clone(nfile, file);

	/*
	 * Should check name is not too long.
	 */
	error = 0;
	for(nwname = 0; nwname < f->nwname; nwname++){
		error = walkname(nfile, f->wname[nwname], &r->wqid[r->nwqid]);
		if(error != 0 || ++r->nwqid >= MAXDAT/sizeof(Qid))
			break;
	}

	if(f->nwname == 0){
		/*
		 * Newfid must be different to fid (see above)
		 * so this is a simple 'clone' operation - there's
		 * nothing to do except unlock unless there's
		 * an error.
		 */
		if(error){
			freewp(nfile->wpath);
			qunlock(nfile);
			freefp(nfile);
		} else
			qunlock(nfile);
	} else if(r->nwqid < f->nwname){
		/*
		 * Didn't walk all elements, 'clunk' nfile
		 * and leave 'file' alone.
		 * Clear error if some of the elements were
		 * walked OK.
		 */
		freewp(nfile->wpath);
		if(nfile != &tfile){
			qunlock(nfile);
			freefp(nfile);
		}
		if(r->nwqid != 0)
			error = 0;
	} else {
		/*
		 * Walked all elements. If newfid is the same
		 * as fid must update 'file' from the temporary
		 * copy used during the walk.
		 * Otherwise just unlock (when using tfile there's
		 * no need to unlock as it's a local).
		 */
		if(nfile == &tfile){
			file->qid = nfile->qid;
			freewp(file->wpath);
			file->wpath = nfile->wpath;
			file->addr = nfile->addr;
			file->slot = nfile->slot;
		} else
			qunlock(nfile);
	}
	qunlock(file);

	return error;
}

static int
fs_open(Chan* chan, Fcall* f, Fcall* r)
{
	Iobuf *p;
	Dentry *d;
	File *file;
	Tlock *t;
	Qid qid;
	int error, ro, fmod, wok;

	wok = 0;
	p = nil;

	if(chan == cons.chan || writeallow)
		wok = 1;

	if((file = filep(chan, f->fid, 0)) == nil){
		error = Efid;
		goto out;
	}
	if(file->open != 0){
		error = Emode;
		goto out;
	}

	/*
	 * if remove on close, check access here
	 */
	ro = file->fs->dev->type == Devro;
	if(f->mode & ORCLOSE){
		if(ro){
			error = Eronly;
			goto out;
		}
		/*
		 * check on parent directory of file to be deleted
		 */
		if(file->wpath == 0 || file->wpath->addr == file->addr){
			error = Ephase;
			goto out;
		}
		p = getbuf(file->fs->dev, file->wpath->addr, Brd);
		if(p == nil || checktag(p, Tdir, QPNONE)){
			error = Ephase;
			goto out;
		}
		d = getdir(p, file->wpath->slot);
		if(d == nil || !(d->mode & DALLOC)){
			error = Ephase;
			goto out;
		}
		if(iaccess(file, d, DWRITE)){
			error = Eaccess;
			goto out;
		}
		putbuf(p);
	}
	p = getbuf(file->fs->dev, file->addr, Brd);
	if(p == nil || checktag(p, Tdir, QPNONE)){
		error = Ealloc;
		goto out;
	}
	d = getdir(p, file->slot);
	if(d == nil || !(d->mode & DALLOC)){
		error = Ealloc;
		goto out;
	}
	if(error = mkqidcmp(&file->qid, d))
		goto out;
	mkqid(&qid, d, 1);
	switch(f->mode & 7){

	case OREAD:
		if(iaccess(file, d, DREAD) && !wok)
			goto badaccess;
		fmod = FREAD;
		break;

	case OWRITE:
		if((d->mode & DDIR) || (iaccess(file, d, DWRITE) && !wok))
			goto badaccess;
		if(ro){
			error = Eronly;
			goto out;
		}
		fmod = FWRITE;
		break;

	case ORDWR:
		if((d->mode & DDIR)
		|| (iaccess(file, d, DREAD) && !wok)
		|| (iaccess(file, d, DWRITE) && !wok))
			goto badaccess;
		if(ro){
			error = Eronly;
			goto out;
		}
		fmod = FREAD+FWRITE;
		break;

	case OEXEC:
		if((d->mode & DDIR) || (iaccess(file, d, DEXEC) && !wok))
			goto badaccess;
		fmod = FREAD;
		break;

	default:
		error = Emode;
		goto out;
	}
	if(f->mode & OTRUNC){
		if((d->mode & DDIR) || (iaccess(file, d, DWRITE) && !wok))
			goto badaccess;
		if(ro){
			error = Eronly;
			goto out;
		}
	}
	t = 0;
	if(d->mode & DLOCK){
		if((t = tlocked(p, d)) == nil){
			error = Elocked;
			goto out;
		}
	}
	if(f->mode & ORCLOSE)
		fmod |= FREMOV;
	file->open = fmod;
	if((f->mode & OTRUNC) && !(d->mode & DAPND)){
		dtrunc(p, d, file->uid);
		qid.vers = d->qid.version;
	}
	r->qid = qid;
	file->tlock = t;
	if(t != nil)
		t->file = file;
	file->lastra = 1;
	goto out;

badaccess:
	error = Eaccess;
	file->open = 0;

out:
	if(p != nil)
		putbuf(p);
	if(file != nil)
		qunlock(file);

	r->iounit = chan->msize-IOHDRSZ;

	return error;
}

static int
fs_create(Chan* chan, Fcall* f, Fcall* r)
{
	Iobuf *p, *p1;
	Dentry *d, *d1;
	File *file;
	int error, slot, slot1, fmod, wok;
	Off addr, addr1, path;
	Tlock *t;
	Wpath *w;

	wok = 0;
	p = nil;

	if(chan == cons.chan || writeallow)
		wok = 1;

	if((file = filep(chan, f->fid, 0)) == nil){
		error = Efid;
		goto out;
	}
	if(file->fs->dev->type == Devro){
		error = Eronly;
		goto out;
	}
	if(file->qid.type & QTAUTH){
		error = Emode;
		goto out;
	}

	p = getbuf(file->fs->dev, file->addr, Brd);
	if(p == nil || checktag(p, Tdir, QPNONE)){
		error = Ealloc;
		goto out;
	}
	d = getdir(p, file->slot);
	if(d == nil || !(d->mode & DALLOC)){
		error = Ealloc;
		goto out;
	}
	if(error = mkqidcmp(&file->qid, d))
		goto out;
	if(!(d->mode & DDIR)){
		error = Edir2;
		goto out;
	}
	if(iaccess(file, d, DWRITE) && !wok) {
		error = Eaccess;
		goto out;
	}
	accessdir(p, d, FREAD, file->uid);

	/*
	 * Check the name is valid (and will fit in an old
	 * directory entry for the moment).
	 */
	if(error = checkname9p2(f->name))
		goto out;

	addr1 = 0;
	slot1 = 0;	/* set */
	for(addr = 0; ; addr++){
		if((p1 = dnodebuf(p, d, addr, 0, file->uid)) == nil){
			if(addr1 != 0)
				break;
			p1 = dnodebuf(p, d, addr, Tdir, file->uid);
		}
		if(p1 == nil){
			error = Efull;
			goto out;
		}
		if(checktag(p1, Tdir, d->qid.path)){
			putbuf(p1);
			goto phase;
		}
		for(slot = 0; slot < DIRPERBUF; slot++){
			d1 = getdir(p1, slot);
			if(!(d1->mode & DALLOC)){
				if(addr1 == 0){
					addr1 = p1->addr;
					slot1 = slot + addr*DIRPERBUF;
				}
				continue;
			}
			if(strncmp(f->name, d1->name, sizeof(d1->name)) == 0){
				putbuf(p1);
				error = Eexist;
				goto out;
			}
		}
		putbuf(p1);
	}

	switch(f->mode & 7){
	case OEXEC:
	case OREAD:		/* seems only useful to make directories */
		fmod = FREAD;
		break;

	case OWRITE:
		fmod = FWRITE;
		break;

	case ORDWR:
		fmod = FREAD+FWRITE;
		break;

	default:
		error = Emode;
		goto out;
	}
	if(f->perm & PDIR)
		if((f->mode & OTRUNC) || (f->perm & PAPND) || (fmod & FWRITE))
			goto badaccess;
	/*
	 * do it
	 */
	path = qidpathgen(file->fs->dev);
	if((p1 = getbuf(file->fs->dev, addr1, Brd|Bimm|Bmod)) == nil)
		goto phase;
	d1 = getdir(p1, slot1);
	if(d1 == nil || checktag(p1, Tdir, d->qid.path)) {
		putbuf(p1);
		goto phase;
	}
	if(d1->mode & DALLOC){
		putbuf(p1);
		goto phase;
	}

	strncpy(d1->name, f->name, sizeof(d1->name));
	if(chan == cons.chan){
		d1->uid = cons.uid;
		d1->gid = cons.gid;
	} else {
		d1->uid = file->uid;
		d1->gid = d->gid;
		f->perm &= d->mode | ~0666;
		if(f->perm & PDIR)
			f->perm &= d->mode | ~0777;
	}
	d1->qid.path = path;
	d1->qid.version = 0;
	d1->mode = DALLOC | (f->perm & 0777);
	if(f->perm & PDIR) {
		d1->mode |= DDIR;
		d1->qid.path |= QPDIR;
	}
	if(f->perm & PAPND)
		d1->mode |= DAPND;
	t = nil;
	if(f->perm & PLOCK){
		d1->mode |= DLOCK;
		t = tlocked(p1, d1);
		/* if nil, out of tlock structures */
	}
	accessdir(p1, d1, FWRITE, file->uid);
	mkqid(&r->qid, d1, 0);
	putbuf(p1);
	accessdir(p, d, FWRITE, file->uid);

	/*
	 * do a walk to new directory entry
	 */
	if((w = newwp()) == nil){
		error = Ewalk;
		goto out;
	}
	w->addr = file->addr;
	w->slot = file->slot;
	w->up = file->wpath;
	file->wpath = w;
	file->qid = r->qid;
	file->tlock = t;
	if(t != nil)
		t->file = file;
	file->lastra = 1;
	if(f->mode & ORCLOSE)
		fmod |= FREMOV;
	file->open = fmod;
	file->addr = addr1;
	file->slot = slot1;
	goto out;

badaccess:
	error = Eaccess;
	goto out;

phase:
	error = Ephase;

out:
	if(p != nil)
		putbuf(p);
	if(file != nil)
		qunlock(file);

	r->iounit = chan->msize-IOHDRSZ;

	return error;
}

static int
fs_read(Chan* chan, Fcall* f, Fcall* r, uchar* data)
{
	Iobuf *p, *p1;
	File *file;
	Dentry *d, *d1;
	Tlock *t;
	Off addr, offset, start;
	Timet tim;
	int error, iounit, nread, count, n, o, slot;
	Msgbuf *dmb;
	Dir dir;

	p = nil;

	error = 0;
	count = f->count;
	offset = f->offset;
	nread = 0;
	if((file = filep(chan, f->fid, 0)) == nil){
		error = Efid;
		goto out;
	}
	if(!(file->open & FREAD)){
		error = Eopen;
		goto out;
	}
	iounit = chan->msize-IOHDRSZ;
	if(count < 0 || count > iounit){
		error = Ecount;
		goto out;
	}
	if(offset < 0){
		error = Eoffset;
		goto out;
	}
	if(file->qid.type & QTAUTH){
		nread = authread(file, (uchar*)data, count);
		if(nread < 0)
			error = Eauth2;
		goto out;
	}
	p = getbuf(file->fs->dev, file->addr, Brd);
	if(p == nil || checktag(p, Tdir, QPNONE)){
		error = Ealloc;
		goto out;
	}
	d = getdir(p, file->slot);
	if(d == nil || !(d->mode & DALLOC)){
		error = Ealloc;
		goto out;
	}
	if(error = mkqidcmp(&file->qid, d))
		goto out;
	if(t = file->tlock){
		tim = toytime();
		if(t->time < tim || t->file != file){
			error = Ebroken;
			goto out;
		}
		/* renew the lock */
		t->time = tim + TLOCK;
	}
	accessdir(p, d, FREAD, file->uid);
	if(d->mode & DDIR)
		goto dread;
	if(offset+count > d->size)
		count = d->size - offset;
	while(count > 0){
		if(p == nil){
			p = getbuf(file->fs->dev, file->addr, Brd);
			if(p == nil || checktag(p, Tdir, QPNONE)){
				error = Ealloc;
				goto out;
			}
			d = getdir(p, file->slot);
			if(d == nil || !(d->mode & DALLOC)){
				error = Ealloc;
				goto out;
			}
		}
		addr = offset / BUFSIZE;
		file->lastra = dbufread(p, d, addr, file->lastra, file->uid);
		o = offset % BUFSIZE;
		n = BUFSIZE - o;
		if(n > count)
			n = count;
		p1 = dnodebuf1(p, d, addr, 0, file->uid);
		p = nil;
		if(p1 != nil){
			if(checktag(p1, Tfile, QPNONE)){
				error = Ephase;
				putbuf(p1);
				goto out;
			}
			memmove(data+nread, p1->iobuf+o, n);
			putbuf(p1);
		} else
			memset(data+nread, 0, n);
		count -= n;
		nread += n;
		offset += n;
	}
	goto out;

dread:
	/*
	 * Pick up where we left off last time if nothing has changed,
	 * otherwise must scan from the beginning.
	 */
	if(offset == file->doffset /*&& file->qid.vers == file->dvers*/){
		addr = file->dslot/DIRPERBUF;
		slot = file->dslot%DIRPERBUF;
		start = offset;
	} else {
		addr = 0;
		slot = 0;
		start = 0;
	}

	dmb = mballoc(iounit, chan, Mbreply1);
	for (;;) {
		if(p == nil){
			/*
			 * This is just a check to ensure the entry hasn't
			 * gone away during the read of each directory block.
			 */
			p = getbuf(file->fs->dev, file->addr, Brd);
			if(p == nil || checktag(p, Tdir, QPNONE)){
				error = Ealloc;
				goto out1;
			}
			d = getdir(p, file->slot);
			if(d == nil || !(d->mode & DALLOC)){
				error = Ealloc;
				goto out1;
			}
		}
		p1 = dnodebuf1(p, d, addr, 0, file->uid);
		p = nil;
		if(p1 == nil)
			goto out1;
		if(checktag(p1, Tdir, QPNONE)){
			error = Ephase;
			putbuf(p1);
			goto out1;
		}

		for(; slot < DIRPERBUF; slot++){
			d1 = getdir(p1, slot);
			if(!(d1->mode & DALLOC))
				continue;
			mkdir9p2(&dir, d1, dmb->data);
			n = convD2M(&dir, data+nread, iounit - nread);
			if(n <= BIT16SZ){
				putbuf(p1);
				goto out1;
			}
			start += n;
			if(start < offset)
				continue;
			if(count < n){
				putbuf(p1);
				goto out1;
			}
			count -= n;
			nread += n;
			offset += n;
		}
		putbuf(p1);
		slot = 0;
		addr++;
	}
out1:
	mbfree(dmb);
	if(error == 0){
		file->doffset = offset;
		file->dvers = file->qid.vers;
		file->dslot = slot+DIRPERBUF*addr;
	}

out:
	/*
	 * Do we need this any more?
	count = f->count - nread;
	if(count > 0)
		memset(data+nread, 0, count);
	 */
	if(p != nil)
		putbuf(p);
	if(file != nil)
		qunlock(file);
	r->count = nread;
	r->data = (char*)data;

	return error;
}

static int
fs_write(Chan* chan, Fcall* f, Fcall* r)
{
	Iobuf *p, *p1;
	Dentry *d;
	File *file;
	Tlock *t;
	Off offset, addr, qpath;
	Timet tim;
	int count, error, nwrite, o, n;

	error = 0;
	offset = f->offset;
	count = f->count;

	nwrite = 0;
	p = nil;

	if((file = filep(chan, f->fid, 0)) == nil){
		error = Efid;
		goto out;
	}
	if(!(file->open & FWRITE)){
		error = Eopen;
		goto out;
	}
	if(count < 0 || count > chan->msize-IOHDRSZ){
		error = Ecount;
		goto out;
	}
	if(offset < 0) {
		error = Eoffset;
		goto out;
	}

	if(file->qid.type & QTAUTH){
		nwrite = authwrite(file, (uchar*)f->data, count);
		if(nwrite < 0)
			error = Eauth2;
		goto out;
	} else if(file->fs->dev->type == Devro){
		error = Eronly;
		goto out;
	}

	if ((p = getbuf(file->fs->dev, file->addr, Brd|Bmod)) == nil ||
	    (d = getdir(p, file->slot)) == nil || !(d->mode & DALLOC)) {
		error = Ealloc;
		goto out;
	}
	if(error = mkqidcmp(&file->qid, d))
		goto out;
	if(t = file->tlock) {
		tim = toytime();
		if(t->time < tim || t->file != file){
			error = Ebroken;
			goto out;
		}
		/* renew the lock */
		t->time = tim + TLOCK;
	}
	accessdir(p, d, FWRITE, file->uid);
	if(d->mode & DAPND)
		offset = d->size;
	if(offset+count > d->size)
		d->size = offset+count;
	while(count > 0){
		if(p == nil){
			p = getbuf(file->fs->dev, file->addr, Brd|Bmod);
			if(p == nil){
				error = Ealloc;
				goto out;
			}
			d = getdir(p, file->slot);
			if(d == nil || !(d->mode & DALLOC)){
				error = Ealloc;
				goto out;
			}
		}
		addr = offset / BUFSIZE;
		o = offset % BUFSIZE;
		n = BUFSIZE - o;
		if(n > count)
			n = count;
		qpath = d->qid.path;
		p1 = dnodebuf1(p, d, addr, Tfile, file->uid);
		p = nil;
		if(p1 == nil) {
			error = Efull;
			goto out;
		}
		if(checktag(p1, Tfile, qpath)){
			putbuf(p1);
			error = Ephase;
			goto out;
		}
		memmove(p1->iobuf+o, f->data+nwrite, n);
		p1->flags |= Bmod;
		putbuf(p1);
		count -= n;
		nwrite += n;
		offset += n;
	}

out:
	if(p != nil)
		putbuf(p);
	if(file != nil)
		qunlock(file);
	r->count = nwrite;

	return error;
}

static int
_clunk(File* file, int remove, int wok)
{
	Tlock *t;
	int error;

	error = 0;
	if(t = file->tlock){
		if(t->file == file)
			t->time = 0;		/* free the lock */
		file->tlock = 0;
	}
	if(remove && (file->qid.type & QTAUTH) == 0)
		error = doremove(file, wok);
	file->open = 0;
	freewp(file->wpath);
	authfree(file->auth);
	freefp(file);
	qunlock(file);

	return error;
}

static int
clunk(Chan* chan, Fcall* f, Fcall*)
{
	File *file;

	if((file = filep(chan, f->fid, 0)) == nil)
		return Efid;

	_clunk(file, file->open & FREMOV, 0);
	return 0;
}

static int
fs_remove(Chan* chan, Fcall* f, Fcall*)
{
	File *file;

	if((file = filep(chan, f->fid, 0)) == nil)
		return Efid;

	return _clunk(file, 1, chan == cons.chan);
}

static int
fs_stat(Chan* chan, Fcall* f, Fcall* r, uchar* data)
{
	Dir dir;
	Iobuf *p;
	Dentry *d, dentry;
	File *file;
	int error, len;

	error = 0;
	p = nil;
	if((file = filep(chan, f->fid, 0)) == nil)
		return Efid;
	if(file->qid.type & QTAUTH){
		memset(&dentry, 0, sizeof dentry);
		d = &dentry;
		mkqid9p1(&d->qid, &file->qid);
		strcpy(d->name, "#¿");
		d->uid = authuid(file->auth);
		d->gid = d->uid;
		d->muid = d->uid;
		d->atime = time(nil);
		d->mtime = d->atime;
		d->size = 0;
	} else {
		p = getbuf(file->fs->dev, file->addr, Brd);
		if(p == nil || checktag(p, Tdir, QPNONE)){
			error = Edir1;
			goto out;
		}
		d = getdir(p, file->slot);
		if(d == nil || !(d->mode & DALLOC)){
			error = Ealloc;
			goto out;
		}
		if(error = mkqidcmp(&file->qid, d))
			goto out;

		if(d->qid.path == QPROOT)	/* stat of root gives time */
			d->atime = time(nil);
	}
	len = mkdir9p2(&dir, d, data);
	data += len;

	if((r->nstat = convD2M(&dir, data, chan->msize - len)) == 0)
		error = Eedge;
	r->stat = data;

out:
	if(p != nil)
		putbuf(p);
	if(file != nil)
		qunlock(file);

	return error;
}

static int
fs_wstat(Chan* chan, Fcall* f, Fcall*, char* strs)
{
	Iobuf *p, *p1;
	Dentry *d, *d1;
	File *file;
	int error, err, gid, gl, muid, op, slot, tsync, uid;
	long addr;
	Dir dir;

	if(convM2D(f->stat, f->nstat, &dir, strs) == 0)
		return Econvert;

	/*
	 * Get the file.
	 * If user 'none' (uid == 0), can't do anything;
	 * if filesystem is read-only, can't change anything.
	 */
	if((file = filep(chan, f->fid, 0)) == nil)
		return Efid;
	p = p1 = nil;
	if(file->uid == 0){
		error = Eaccess;
		goto out;
	}
	if(file->fs->dev->type == Devro){
		error = Eronly;
		goto out;
	}
	if(file->qid.type & QTAUTH){
		error = Emode;
		goto out;
	}

	/*
	 * Get the current entry and check it is still valid.
	 */
	p = getbuf(file->fs->dev, file->addr, Brd);
	if(p == nil || checktag(p, Tdir, QPNONE)){
		error = Ealloc;
		goto out;
	}
	d = getdir(p, file->slot);
	if(d == nil || !(d->mode & DALLOC)){
		error = Ealloc;
		goto out;
	}
	if(error = mkqidcmp(&file->qid, d))
		goto out;

	/*
	 * Run through each of the (sub-)fields in the provided Dir
	 * checking for validity and whether it's a default:
	 * .type, .dev and .atime are completely ignored and not checked;
	 * .qid.path, .qid.vers and .muid are checked for validity but
	 * any attempt to change them is an error.
	 * .qid.type/.mode, .mtime, .name, .length, .uid and .gid can
	 * possibly be changed (and .muid iff wstatallow).
	 *
	 * 'Op' flags there are changed fields, i.e. it's not a no-op.
	 * 'Tsync' flags all fields are defaulted.
	 *
	 * Wstatallow and writeallow are set to allow changes during the
	 * fileserver bootstrap phase.
	 */
	tsync = 1;
	if(dir.qid.path != ~0){
		if(dir.qid.path != file->qid.path){
			error = Ewstatp;
			goto out;
		}
		tsync = 0;
	}
	if(dir.qid.vers != ~0){
		if(dir.qid.vers != file->qid.vers){
			error = Ewstatv;
			goto out;
		}
		tsync = 0;
	}

	/*
	 * .qid.type and .mode have some bits in common. Only .mode
	 * is currently needed for comparisons with the old mode but
	 * if there are changes to the bits also encoded in .qid.type
	 * then file->qid must be updated appropriately later.
	 */
	if(dir.qid.type == (uchar)~0){
		if(dir.mode == ~0)
			dir.qid.type = mktype9p2(d->mode);
		else
			dir.qid.type = dir.mode>>24;
	} else
		tsync = 0;
	if(dir.mode == ~0)
		dir.mode = mkmode9p2(d->mode);
	else
		tsync = 0;

	/*
	 * Check dir.qid.type and dir.mode agree, check for any unknown
	 * type/mode bits, check for an attempt to change the directory bit.
	 */
	if(dir.qid.type != ((dir.mode>>24) & 0xFF)){
		error = Ewstatq;
		goto out;
	}
	if(dir.mode & ~(DMDIR|DMAPPEND|DMEXCL|0777)){
		error = Ewstatb;
		goto out;
	}

	op = dir.mode^mkmode9p2(d->mode);
	if(op & DMDIR){
		error = Ewstatd;
		goto out;
	}

	if(dir.mtime != ~0){
		if(dir.mtime != d->mtime)
			op = 1;
		tsync = 0;
	} else
		dir.mtime = d->mtime;

	if(dir.length == ~(Off)0)
		dir.length = d->size;
	else {
		if (dir.length < 0) {
			error = Ewstatl;
			goto out;
		} else if(dir.length != d->size)
			op = 1;
		tsync = 0;
	}

	/*
	 * Check for permission to change .mode, .mtime or .length,
	 * must be owner or leader of either group, for which test gid
	 * is needed; permission checks on gid will be done later.
	 * 'Gl' counts whether neither, one or both groups are led.
	 */
	if(dir.gid != nil && *dir.gid != '\0'){
		gid = strtouid(dir.gid);
		tsync = 0;
	} else
		gid = d->gid;
	gl = leadgroup(file->uid, gid) != 0;
	gl += leadgroup(file->uid, d->gid) != 0;

	if(op && !wstatallow && d->uid != file->uid && !gl){
		error = Ewstato;
		goto out;
	}

	/*
	 * Rename.
	 * Check .name is valid and different to the current.
	 */
	if(dir.name != nil && *dir.name != '\0'){
		if(error = checkname9p2(dir.name))
			goto out;
		if(strncmp(dir.name, d->name, NAMELEN))
			op = 1;
		else
			dir.name = d->name;
		tsync = 0;
	} else
		dir.name = d->name;

	/*
	 * If the name is really to be changed check it's unique
	 * and there is write permission in the parent.
	 */
	if(dir.name != d->name){
		/*
		 * First get parent.
		 * Must drop current entry to prevent
		 * deadlock when searching that new name
		 * already exists below.
		 */
		putbuf(p);
		p = nil;

		if(file->wpath == nil){
			error = Ephase;
			goto out;
		}
		p1 = getbuf(file->fs->dev, file->wpath->addr, Brd);
		if(p1 == nil || checktag(p1, Tdir, QPNONE)){
			error = Ephase;
			goto out;
		}
		d1 = getdir(p1, file->wpath->slot);
		if(d1 == nil || !(d1->mode & DALLOC)){
			error = Ephase;
			goto out;
		}

		/*
		 * Check entries in parent for new name.
		 */
		for(addr = 0; ; addr++){
			if((p = dnodebuf(p1, d1, addr, 0, file->uid)) == nil)
				break;
			if(checktag(p, Tdir, d1->qid.path)){
				putbuf(p);
				continue;
			}
			for(slot = 0; slot < DIRPERBUF; slot++){
				d = getdir(p, slot);
				if(!(d->mode & DALLOC) ||
				   strncmp(dir.name, d->name, sizeof d->name))
					continue;
				error = Eexist;
				goto out;
			}
			putbuf(p);
		}

		/*
		 * Reacquire entry and check it's still OK.
		 */
		p = getbuf(file->fs->dev, file->addr, Brd);
		if(p == nil || checktag(p, Tdir, QPNONE)){
			error = Ephase;
			goto out;
		}
		d = getdir(p, file->slot);
		if(d == nil || !(d->mode & DALLOC)){
			error = Ephase;
			goto out;
		}

		/*
		 * Check write permission in the parent.
		 */
		if(!wstatallow && !writeallow && iaccess(file, d1, DWRITE)){
			error = Eaccess;
			goto out;
		}
	}

	/*
	 * Check for permission to change owner - must be god.
	 */
	if(dir.uid != nil && *dir.uid != '\0'){
		uid = strtouid(dir.uid);
		if(uid != d->uid){
			if(!wstatallow){
				error = Ewstatu;
				goto out;
			}
			op = 1;
		}
		tsync = 0;
	} else
		uid = d->uid;
	if(dir.muid != nil && *dir.muid != '\0'){
		muid = strtouid(dir.muid);
		if(muid != d->muid){
			if(!wstatallow){
				error = Ewstatm;
				goto out;
			}
			op = 1;
		}
		tsync = 0;
	} else
		muid = d->muid;

	/*
	 * Check for permission to change group, must be
	 * either owner and in new group or leader of both groups.
	 */
	if(gid != d->gid){
		if(!(wstatallow || writeallow)
		&& !(d->uid == file->uid && ingroup(file->uid, gid))
		&& !(gl == 2)){
			error = Ewstatg;
			goto out;
		}
		op = 1;
	}

	/*
	 * Checks all done, update if necessary.
	 */
	if(op){
		d->mode = mkmode9p1(dir.mode);
		file->qid.type = mktype9p2(d->mode);
		d->mtime = dir.mtime;
		if (dir.length < d->size) {
			err = dtrunclen(p, d, dir.length, uid);
			if (error == 0)
				error = err;
		}
		d->size = dir.length;
		if(dir.name != d->name)
			strncpy(d->name, dir.name, sizeof(d->name));
		d->uid = uid;
		d->gid = gid;
		d->muid = muid;
	}
	if(!tsync)
		accessdir(p, d, FREAD, file->uid);

out:
	if(p != nil)
		putbuf(p);
	if(p1 != nil)
		putbuf(p1);
	qunlock(file);

	return error;
}

int
serve9p2(Msgbuf* mb)
{
	Chan *chan;
	Fcall f, r;
	Msgbuf *data, *rmb;
	char ename[64];
	int error, n, type;
	static int once;

	if(once == 0){
		fmtinstall('F', fcallfmt);
		once = 1;
	}

	/*
	 * 0 return means i don't understand this message,
	 * 1 return means i dealt with it, including error
	 * replies.
	 */
	if(convM2S(mb->data, mb->count, &f) != mb->count)
{
print("didn't like %d byte message\n", mb->count);
		return 0;
}
	type = f.type;
	if(type < Tversion || type >= Tmax || (type & 1) || type == Terror)
		return 0;

	chan = mb->chan;
	if(CHAT(chan))
		print("9p2: f %F\n", &f);
	r.type = type+1;
	r.tag = f.tag;
	error = 0;
	data = nil;

	switch(type){
	default:
		r.type = Rerror;
		snprint(ename, sizeof(ename), "unknown message: %F", &f);
		r.ename = ename;
		break;
	case Tversion:
		error = version(chan, &f, &r);
		break;
	case Tauth:
		error = auth(chan, &f, &r);
		break;
	case Tattach:
		error = attach(chan, &f, &r);
		break;
	case Tflush:
		error = flush(chan, &f, &r);
		break;
	case Twalk:
		error = walk(chan, &f, &r);
		break;
	case Topen:
		error = fs_open(chan, &f, &r);
		break;
	case Tcreate:
		error = fs_create(chan, &f, &r);
		break;
	case Tread:
		data = mballoc(chan->msize, chan, Mbreply1);
		error = fs_read(chan, &f, &r, data->data);
		break;
	case Twrite:
		error = fs_write(chan, &f, &r);
		break;
	case Tclunk:
		error = clunk(chan, &f, &r);
		break;
	case Tremove:
		error = fs_remove(chan, &f, &r);
		break;
	case Tstat:
		data = mballoc(chan->msize, chan, Mbreply1);
		error = fs_stat(chan, &f, &r, data->data);
		break;
	case Twstat:
		data = mballoc(chan->msize, chan, Mbreply1);
		error = fs_wstat(chan, &f, &r, (char*)data->data);
		break;
	}

	if(error != 0){
		r.type = Rerror;
		if(error >= MAXERR){
			snprint(ename, sizeof(ename), "error %d", error);
			r.ename = ename;
		} else
			r.ename = errstr9p[error];
	}
	if(CHAT(chan))
		print("9p2: r %F\n", &r);

	rmb = mballoc(chan->msize, chan, Mbreply2);
	n = convS2M(&r, rmb->data, chan->msize);
	if(data != nil)
		mbfree(data);
	if(n == 0){
		type = r.type;
		r.type = Rerror;

		/*
		 * If a Tversion has not been seen on the chan then
		 * chan->msize will be 0. In that case craft a special
		 * Rerror message. It's fortunate that the mballoc above
		 * for rmb will have returned a Msgbuf of MAXMSG size
		 * when given a request with count of 0...
		 */
		if(chan->msize == 0){
			r.ename = "Tversion not seen";
			n = convS2M(&r, rmb->data, MAXMSG);
		} else {
			snprint(ename, sizeof(ename), "9p2: convS2M: type %d",
				type);
			r.ename = ename;
			n = convS2M(&r, rmb->data, chan->msize);
		}
		print("%s\n", r.ename);
		if(n == 0){
			/*
			 * What to do here, the failure notification failed?
			 */
			mbfree(rmb);
			return 1;
		}
	}
	rmb->count = n;
	rmb->param = mb->param;

	/* done 9P processing, write reply to network */
	fs_send(chan->reply, rmb);

	return 1;
}
