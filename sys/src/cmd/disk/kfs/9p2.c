#include	"all.h"

#define MSIZE	(MAXDAT+128)

static void
seterror(Fcall *ou, int err)
{

	if(0 <= err && err < MAXERR)
		ou->ename = errstring[err];
	else
		ou->ename = "unknown error";
}

static int
fsversion(Chan* chan, Fcall* f, Fcall* r)
{
	if(f->msize < MSIZE)
		r->msize = f->msize;
	else
		r->msize = MSIZE;
	/*
	 * Should check the '.' stuff here.
	 * What happens if Tversion has already been seen?
	 */
	if(strcmp(f->version, VERSION9P) == 0){
		r->version = VERSION9P;
		chan->msize = r->msize;
	}else
		r->version = "unknown";

	fileinit(chan);
	return 0;
}

char *keyspec = "proto=p9any role=server";

static int
fsauth(Chan *chan, Fcall *f, Fcall *r)
{
	int err, fd;
	char *aname;
	File *file;
	int afd;
	AuthRpc *rpc;

	err = 0;
	if(chan == cons.srvchan)
		return Eauthmsg;
	file = filep(chan, f->afid, 1);
	if(file == nil)
		return Efidinuse;

	/* forget any previous authentication */
	file->cuid = 0;

	if(access("/mnt/factotum", 0) < 0)
		if((fd = open("/srv/factotum", ORDWR)) >= 0)
			mount(fd, -1, "/mnt", MBEFORE, "");

	afd = open("/mnt/factotum/rpc", ORDWR);
	if(afd < 0){
		err = Esystem;
		goto out;
	}
	rpc = auth_allocrpc(afd);
	if(rpc == nil){
		close(afd);
		err = Esystem;
		goto out;
	}
	file->rpc = rpc;
	if(auth_rpc(rpc, "start", keyspec, strlen(keyspec)) != ARok){
		err = Esystem;
		goto out;
	}

	aname = f->aname;
	if(!aname[0])
		aname = "main";
	file->fs = fsstr(aname);
	if(file->fs == nil){
		err = Ebadspc;
		goto out;
	}
	file->uid = strtouid(f->uname);
	if(file->uid < 0){
		err = Ebadu;
		goto out;
	}
	file->qid.path = 0;
	file->qid.vers = 0;
	file->qid.type = QTAUTH;
	r->qid = file->qid;

out:
	if(file != nil){
		qunlock(file);
		if(err != 0)
			freefp(file);
	}
	return err;
}

int
authread(File *file, uchar *data, int count)
{
	AuthInfo *ai;
	AuthRpc *rpc;
	int rv;

	rpc = file->rpc;
	if(rpc == nil)
		return -1;

	rv = auth_rpc(rpc, "read", nil, 0);
	switch(rv){
	case ARdone:
		ai = auth_getinfo(rpc);
		if(ai == nil)
			return -1;
		if(chat)
			print("authread identifies user as %s\n", ai->cuid);
		file->cuid = strtouid(ai->cuid);
		auth_freeAI(ai);
		if(file->cuid == 0)
			return -1;
		if(chat)
			print("%s is a known user\n", ai->cuid);
		return 0;
	case ARok:
		if(count < rpc->narg)
			return -1;
		memmove(data, rpc->arg, rpc->narg);
		return rpc->narg;
	case ARphase:
		return -1;
	default:
		return -1;
	}
}

int
authwrite(File *file, uchar *data, int count)
{
	int ret;

	ret = auth_rpc(file->rpc, "write", data, count);
	if(ret != ARok)
		return -1;
	return count;
}

void
mkqid9p1(Qid9p1* qid9p1, Qid* qid)
{
	if(qid->path & 0xFFFFFFFF00000000LL)
		panic("mkqid9p1: path %lluX\n", qid->path);
	qid9p1->path = qid->path & 0xFFFFFFFF;
	if(qid->type & QTDIR)
		qid9p1->path |= QPDIR;
	qid9p1->version = qid->vers;
}

void
authfree(File *fp)
{
	if(fp->rpc != nil){
		close(fp->rpc->afd);
		free(fp->rpc);
		fp->rpc = nil;
	}
}

void
mkqid9p2(Qid* qid, Qid9p1* qid9p1, int mode)
{
	qid->path = (ulong)(qid9p1->path & ~QPDIR);
	qid->vers = qid9p1->version;
	qid->type = 0;
	if(mode & DDIR)
		qid->type |= QTDIR;
	if(mode & DAPND)
		qid->type |= QTAPPEND;
	if(mode & DLOCK)
		qid->type |= QTEXCL;
}

static int
checkattach(Chan *chan, File *afile, File *file, Filsys *fs)
{
	uchar buf[1];

	if(chan == cons.srvchan || chan == cons.chan)
		return 0;

	/* if no afile, this had better be none */
	if(afile == nil){
		if(file->uid == 0){
			if(!allownone && !chan->authed)
				return Eauth;
			return 0;
		}
		return Eauth;
	}

	/* otherwise, we'ld better have a usable cuid */
	if(!(afile->qid.type&QTAUTH))
		return Eauth;
	if(afile->uid != file->uid || afile->fs != fs)
		return Eauth;
	if(afile->cuid <= 0){
		if(authread(afile, buf, 0) != 0)
			return Eauth;
		if(afile->cuid <= 0)
			return Eauth;
	}
	file->uid = afile->cuid;

	/* once someone has authenticated on the channel, others can become none */
	chan->authed = 1;

	return 0;
}		

static int
fsattach(Chan* chan, Fcall* f, Fcall* r)
{
	char *aname;
	Iobuf *p;
	Dentry *d;
	File *file;
	File *afile;
	Filsys *fs;
	long raddr;
	int error, u;

	aname = f->aname;
	if(!aname[0])	/* default */
		aname = "main";
	p = nil;
	afile = filep(chan, f->afid, 0);
	file = filep(chan, f->fid, 1);
	if(file == nil){
		error = Efidinuse;
		goto out;
	}

	u = -1;
	if(chan != cons.chan){
		if(strcmp(f->uname, "adm") == 0){
			error = Eauth;
			goto out;
		}
		u = strtouid(f->uname);
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

	if(error = checkattach(chan, afile, file, fs))
		goto out;

	raddr = getraddr(fs->dev);
	p = getbuf(fs->dev, raddr, Bread);
	d = getdir(p, 0);
	if(d == nil || checktag(p, Tdir, QPROOT) || !(d->mode & DALLOC)){
		error = Ealloc;
		goto out;
	}
	if(iaccess(file, d, DEXEC)){
		error = Eaccess;
		goto out;
	}
	if(file->uid == 0 && isro(fs->dev)) {
		/*
		 * 'none' not allowed on dump
		 */
		error = Eaccess;
		goto out;
	}
	accessdir(p, d, FREAD);
	mkqid(&file->qid, d, 1);
	file->fs = fs;
	file->addr = raddr;
	file->slot = 0;
	file->open = 0;
	freewp(file->wpath);
	file->wpath = 0;

	r->qid = file->qid;

//	if(cons.flags & attachflag)
//		print("9p2: attach %s %T to \"%s\" C%d\n",
//			chan->whoname, chan->whotime, fs->name, chan->chan);

out:
//	if((cons.flags & attachflag) && error)
//		print("9p2: attach %s %T SUCK EGGS --- %s\n",
//			f->uname, time(), errstr[error]);
	if(p != nil)
		putbuf(p);
	if(afile != nil)
		qunlock(afile);
	if(file != nil){
		qunlock(file);
		if(error)
			freefp(file);
	}

	return error;
}

static int
fsflush(Chan* chan, Fcall*, Fcall*)
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
	nfile->cuid = 0;
	nfile->open = file->open & ~FREMOV;
}

static int
walkname(File* file, char* wname, Qid* wqid)
{
	Wpath *w;
	Iobuf *p, *p1;
	Dentry *d, *d1;
	int error, slot;
	long addr, qpath;

	p = p1 = nil;

	/*
	 * File must not have been opened for I/O by an open
	 * or create message and must represent a directory.
	 */
	if(file->open != 0){
		error = Emode;
		goto out;
	}

	p = getbuf(file->fs->dev, file->addr, Bread);
	if(p == nil || checktag(p, Tdir, QPNONE)){
		error = Edir1;
		goto out;
	}
	if((d = getdir(p, file->slot)) == nil || !(d->mode & DALLOC)){
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
	accessdir(p, d, FREAD);

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
		p1 = getbuf(file->fs->dev, addr, Bread);
		if(p1 == nil || checktag(p1, Tdir, QPNONE)){
			error = Edir1;
			goto out;
		}
		if((d1 = getdir(p1, slot)) == nil || !(d1->mode & DALLOC)){
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
			p = getbuf(file->fs->dev, file->addr, Bread);
			if(p == nil || checktag(p, Tdir, QPNONE)){
				error = Ealloc;
				goto out;
			}
			d = getdir(p, file->slot);
			if(d == nil ||  !(d->mode & DALLOC)){
				error = Ealloc;
				goto out;
			}
		}
		qpath = d->qid.path;
		p1 = dnodebuf1(p, d, addr, 0);
		p = nil;
		if(p1 == nil || checktag(p1, Tdir, qpath)){
			error = Eentry;
			goto out;
		}
		for(slot = 0; slot < DIRPERBUF; slot++){
			d1 = getdir(p1, slot);
			if(!(d1->mode & DALLOC))
				continue;
			if(strncmp(wname, d1->name, NAMELEN) != 0)
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
fswalk(Chan* chan, Fcall* f, Fcall* r)
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
	}
	else if(f->nwname != 0){
		nfile = &tfile;
		memset(nfile, 0, sizeof(File));
		nfile->cp = chan;
		nfile->fid = ~0;
	}
	else{
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
		}
		else
			qunlock(nfile);
	}
	else if(r->nwqid < f->nwname){
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
	}
	else{
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
		}
		else
			qunlock(nfile);
	}
	qunlock(file);

	return error;
}

static int
fsopen(Chan* chan, Fcall* f, Fcall* r)
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

	/*
	 * if remove on close, check access here
	 */
	ro = isro(file->fs->dev) || (writegroup && !ingroup(file->uid, writegroup));
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
		p = getbuf(file->fs->dev, file->wpath->addr, Bread);
		if(p == nil || checktag(p, Tdir, QPNONE)){
			error = Ephase;
			goto out;
		}
		if((d = getdir(p, file->wpath->slot)) == nil || !(d->mode & DALLOC)){
			error = Ephase;
			goto out;
		}
		if(iaccess(file, d, DWRITE)){
			error = Eaccess;
			goto out;
		}
		putbuf(p);
	}
	p = getbuf(file->fs->dev, file->addr, Bread);
	if(p == nil || checktag(p, Tdir, QPNONE)){
		error = Ealloc;
		goto out;
	}
	if((d = getdir(p, file->slot)) == nil || !(d->mode & DALLOC)){
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
		dtrunc(p, d);
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
dir9p2(Dir* dir, Dentry* dentry, void* strs)
{
	char *op, *p;

	memset(dir, 0, sizeof(Dir));
	mkqid(&dir->qid, dentry, 1);
	dir->mode = (dir->qid.type<<24)|(dentry->mode & 0777);
	dir->atime = dentry->atime;
	dir->mtime = dentry->mtime;
	dir->length = dentry->size;

	op = p = strs;
	dir->name = p;
	p += sprint(p, "%s", dentry->name)+1;

	dir->uid = p;
	uidtostr(p, dentry->uid);
	p += strlen(p)+1;

	dir->gid = p;
	uidtostr(p, dentry->gid);
	p += strlen(p)+1;

	dir->muid = p;
	strcpy(p, "");
	p += strlen(p)+1;

	return p-op;
}

static int
checkname9p2(char* name)
{
	char *p;

	/*
	 * Return length of string if valid, 0 if not.
	 */
	if(name == nil)
		return 0;

	for(p = name; *p != 0; p++){
		if((*p & 0xFF) <= 040)
			return 0;
	}

	return p-name;
}

static int
fscreate(Chan* chan, Fcall* f, Fcall* r)
{
	Iobuf *p, *p1;
	Dentry *d, *d1;
	File *file;
	int error, slot, slot1, fmod, wok, l;
	long addr, addr1, path;
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
	if(isro(file->fs->dev) || (writegroup && !ingroup(file->uid, writegroup))){
		error = Eronly;
		goto out;
	}

	p = getbuf(file->fs->dev, file->addr, Bread);
	if(p == nil || checktag(p, Tdir, QPNONE)){
		error = Ealloc;
		goto out;
	}
	if((d = getdir(p, file->slot)) == nil || !(d->mode & DALLOC)){
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
	accessdir(p, d, FREAD);

	/*
	 * Check the name is valid and will fit in an old
	 * directory entry.
	 */
	if((l = checkname9p2(f->name)) == 0){
		error = Ename;
		goto out;
	}
	if(l+1 > NAMELEN){
		error = Etoolong;
		goto out;
	}
	if(strcmp(f->name, ".") == 0 || strcmp(f->name, "..") == 0){
		error = Edot;
		goto out;
	}

	addr1 = 0;
	slot1 = 0;	/* set */
	for(addr = 0; ; addr++){
		if((p1 = dnodebuf(p, d, addr, 0)) == nil){
			if(addr1 != 0)
				break;
			p1 = dnodebuf(p, d, addr, Tdir);
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
	path = qidpathgen(&file->fs->dev);
	if((p1 = getbuf(file->fs->dev, addr1, Bread|Bimm|Bmod)) == nil)
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
	}
	else{
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
	accessdir(p1, d1, FWRITE);
	mkqid(&r->qid, d1, 0);
	putbuf(p1);
	accessdir(p, d, FWRITE);

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
fsread(Chan* chan, Fcall* f, Fcall* r)
{
	uchar *data;
	Iobuf *p, *p1;
	File *file;
	Dentry *d, *d1;
	Tlock *t;
	long addr, offset, start, tim;
	int error, iounit, nread, count, n, o, slot;
	Dir dir;
	char strdata[28*10];

	p = nil;
	data = (uchar*)r->data;
	count = f->count;
	offset = f->offset;
	nread = 0;
	if((file = filep(chan, f->fid, 0)) == nil){
		error = Efid;
		goto out;
	}
	if(file->qid.type & QTAUTH){
		nread = authread(file, data, count);
		if(nread < 0)
			error = Esystem;
		else
			error = 0;
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
	p = getbuf(file->fs->dev, file->addr, Bread);
	if(p == nil || checktag(p, Tdir, QPNONE)){
		error = Ealloc;
		goto out;
	}
	if((d = getdir(p, file->slot)) == nil || !(d->mode & DALLOC)){
		error = Ealloc;
		goto out;
	}
	if(error = mkqidcmp(&file->qid, d))
		goto out;
	if(t = file->tlock){
		tim = time(0);
		if(t->time < tim || t->file != file){
			error = Ebroken;
			goto out;
		}
		/* renew the lock */
		t->time = tim + TLOCK;
	}
	accessdir(p, d, FREAD);
	if(d->mode & DDIR)
		goto dread;
	if(offset+count > d->size)
		count = d->size - offset;
	while(count > 0){
		if(p == nil){
			p = getbuf(file->fs->dev, file->addr, Bread);
			if(p == nil || checktag(p, Tdir, QPNONE)){
				error = Ealloc;
				goto out;
			}
			if((d = getdir(p, file->slot)) == nil || !(d->mode & DALLOC)){
				error = Ealloc;
				goto out;
			}
		}
		addr = offset / BUFSIZE;
		o = offset % BUFSIZE;
		n = BUFSIZE - o;
		if(n > count)
			n = count;
		p1 = dnodebuf1(p, d, addr, 0);
		p = nil;
		if(p1 != nil){
			if(checktag(p1, Tfile, QPNONE)){
				error = Ephase;
				putbuf(p1);
				goto out;
			}
			memmove(data+nread, p1->iobuf+o, n);
			putbuf(p1);
		}
		else
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
	}
	else{
		addr = 0;
		slot = 0;
		start = 0;
	}

dread1:
	if(p == nil){
		/*
		 * This is just a check to ensure the entry hasn't
		 * gone away during the read of each directory block.
		 */
		p = getbuf(file->fs->dev, file->addr, Bread);
		if(p == nil || checktag(p, Tdir, QPNONE)){
			error = Ealloc;
			goto out1;
		}
		if((d = getdir(p, file->slot)) == nil || !(d->mode & DALLOC)){
			error = Ealloc;
			goto out1;
		}
	}
	p1 = dnodebuf1(p, d, addr, 0);
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
		dir9p2(&dir, d1, strdata);
		if((n = convD2M(&dir, data+nread, iounit - nread)) <= BIT16SZ){
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
	goto dread1;

out1:
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
fswrite(Chan* chan, Fcall* f, Fcall* r)
{
	Iobuf *p, *p1;
	Dentry *d;
	File *file;
	Tlock *t;
	long offset, addr, tim, qpath;
	int count, error, nwrite, o, n;

	offset = f->offset;
	count = f->count;

	nwrite = 0;
	p = nil;

	if((file = filep(chan, f->fid, 0)) == nil){
		error = Efid;
		goto out;
	}
	if(file->qid.type & QTAUTH){
		nwrite = authwrite(file, (uchar*)f->data, count);
		if(nwrite < 0)
			error = Esystem;
		else
			error = 0;
		goto out;
	}
	if(!(file->open & FWRITE)){
		error = Eopen;
		goto out;
	}
	if(isro(file->fs->dev) || (writegroup && !ingroup(file->uid, writegroup))){
		error = Eronly;
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
	if((p = getbuf(file->fs->dev, file->addr, Bread|Bmod)) == nil){
		error = Ealloc;
		goto out;
	}
	if((d = getdir(p, file->slot)) == nil || !(d->mode & DALLOC)){
		error = Ealloc;
		goto out;
	}
	if(error = mkqidcmp(&file->qid, d))
		goto out;
	if(t = file->tlock) {
		tim = time(0);
		if(t->time < tim || t->file != file){
			error = Ebroken;
			goto out;
		}
		/* renew the lock */
		t->time = tim + TLOCK;
	}
	accessdir(p, d, FWRITE);
	if(d->mode & DAPND)
		offset = d->size;
	if(offset+count > d->size)
		d->size = offset+count;
	while(count > 0){
		if(p == nil){
			p = getbuf(file->fs->dev, file->addr, Bread|Bmod);
			if((d = getdir(p, file->slot)) == nil || !(d->mode & DALLOC)){
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
		p1 = dnodebuf1(p, d, addr, Tfile);
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
	if(remove)
		error = doremove(file, wok);
	file->open = 0;
	freewp(file->wpath);
	freefp(file);
	qunlock(file);

	return error;
}

static int
fsclunk(Chan* chan, Fcall* f, Fcall*)
{
	File *file;

	if((file = filep(chan, f->fid, 0)) == nil)
		return Efid;

	_clunk(file, file->open & FREMOV, 0);
	return 0;
}

static int
fsremove(Chan* chan, Fcall* f, Fcall*)
{
	File *file;

	if((file = filep(chan, f->fid, 0)) == nil)
		return Efid;

	return _clunk(file, 1, chan == cons.chan);
}

static int
fsstat(Chan* chan, Fcall* f, Fcall* r, uchar* data)
{
	Dir dir;
	Iobuf *p;
	Dentry *d;
	File *file;
	int error, len;

	if((file = filep(chan, f->fid, 0)) == nil)
		return Efid;

	p = getbuf(file->fs->dev, file->addr, Bread);
	if(p == nil || checktag(p, Tdir, QPNONE)){
		error = Edir1;
		goto out;
	}
	if((d = getdir(p, file->slot)) == nil || !(d->mode & DALLOC)){
		error = Ealloc;
		goto out;
	}
	if(error = mkqidcmp(&file->qid, d))
		goto out;

	if(d->qid.path == QPROOT)	/* stat of root gives time */
		d->atime = time(0);

	len = dir9p2(&dir, d, data);
	data += len;
	if((r->nstat = convD2M(&dir, data, chan->msize - len)) == 0)
		error = Ersc;
	else
		r->stat = data;

out:
	if(p != nil)
		putbuf(p);
	if(file != nil)
		qunlock(file);

	return error;
}

static int
fswstat(Chan* chan, Fcall* f, Fcall*, char *strs)
{
	Iobuf *p, *p1;
	Dentry *d, *d1, xd;
	File *file;
	int error, slot, uid, gid, l;
	long addr;
	Dir dir;
	ulong mode;

	p = p1 = nil;
	d1 = nil;

	if((file = filep(chan, f->fid, 0)) == nil){
		error = Efid;
		goto out;
	}

	/*
	 * if user none,
	 * can't do anything
	 * unless allow.
	 */
	if(file->uid == 0 && !wstatallow){
		error = Eaccess;
		goto out;
	}

	if(isro(file->fs->dev) || (writegroup && !ingroup(file->uid, writegroup))){
		error = Eronly;
		goto out;
	}

	/*
	 * first get parent
	 */
	if(file->wpath){
		p1 = getbuf(file->fs->dev, file->wpath->addr, Bread);
		if(p1 == nil){
			error = Ephase;
			goto out;
		}
		d1 = getdir(p1, file->wpath->slot);
		if(d1 == nil || checktag(p1, Tdir, QPNONE) || !(d1->mode & DALLOC)){
			error = Ephase;
			goto out;
		}
	}

	if((p = getbuf(file->fs->dev, file->addr, Bread)) == nil){
		error = Ealloc;
		goto out;
	}
	d = getdir(p, file->slot);
	if(d == nil || checktag(p, Tdir, QPNONE) || !(d->mode & DALLOC)){
		error = Ealloc;
		goto out;
	}
	if(error = mkqidcmp(&file->qid, d))
		goto out;

	/*
	 * Convert the message and fix up
	 * fields not to be changed.
	 */
	if(convM2D(f->stat, f->nstat, &dir, strs) == 0){
		print("9p2: convM2D returns 0\n");
		error = Econvert;
		goto out;
	}
	if(dir.uid == nil || strlen(dir.uid) == 0)
		uid = d->uid;
	else
		uid = strtouid(dir.uid);
	if(dir.gid == nil || strlen(dir.gid) == 0)
		gid = d->gid;
	else
		gid = strtouid(dir.gid);
	if(dir.name == nil || strlen(dir.name) == 0)
		dir.name = d->name;
	else{
		if((l = checkname9p2(dir.name)) == 0){
			error = Ename;
			goto out;
		}
		if(l > NAMELEN){
			error = Etoolong;
			goto out;
		}
	}

	/*
	 * Before doing sanity checks, find out what the
	 * new 'mode' should be:
	 * if 'type' and 'mode' are both defaults, take the
	 * new mode from the old directory entry;
	 * else if 'type' is the default, use the new mode entry;
	 * else if 'mode' is the default, create the new mode from
	 * 'type' or'ed with the old directory mode;
	 * else neither are defaults, use the new mode but check
	 * it agrees with 'type'.
	 */
	if(dir.qid.type == 0xFF && dir.mode == ~0){
		dir.mode = d->mode & 0777;
		if(d->mode & DLOCK)
			dir.mode |= DMEXCL;
		if(d->mode & DAPND)
			dir.mode |= DMAPPEND;
		if(d->mode & DDIR)
			dir.mode |= DMDIR;
	}
	else if(dir.qid.type == 0xFF){
		/* nothing to do */
	}
	else if(dir.mode == ~0)
		dir.mode = (dir.qid.type<<24)|(d->mode & 0777);
	else if(dir.qid.type != ((dir.mode>>24) & 0xFF)){
		error = Eqidmode;
		goto out;
	}

	/*
	 * Check for unknown type/mode bits
	 * and an attempt to change the directory bit.
	 */
	if(dir.mode & ~(DMDIR|DMAPPEND|DMEXCL|0777)){
		error = Enotm;
		goto out;
	}
	if(d->mode & DDIR)
		mode = DMDIR;
	else
		mode = 0;
	if((dir.mode^mode) & DMDIR){
		error = Enotd;
		goto out;
	}

	if(dir.mtime == ~0)
		dir.mtime = d->mtime;
	if(dir.length == ~0)
		dir.length = d->size;

	/*
	 * Currently, can't change length.
	 */
	if(dir.length != d->size){
		error = Enotl;
		goto out;
	}

	/*
	 * if chown,
	 * must be god
	 * wstatallow set to allow chown during boot
	 */
	if(uid != d->uid && !wstatallow) {
		error = Enotu;
		goto out;
	}

	/*
	 * if chgroup,
	 * must be either
	 *	a) owner and in new group
	 *	b) leader of both groups
	 * wstatallow and writeallow are set to allow chgrp during boot
	 */
	while(gid != d->gid) {
		if(wstatallow || writeallow)
			break;
		if(d->uid == file->uid && ingroup(file->uid, gid))
			break;
		if(leadgroup(file->uid, gid))
			if(leadgroup(file->uid, d->gid))
				break;
		error = Enotg;
		goto out;
	}

	/*
	 * if rename,
	 * must have write permission in parent
	 */
	while(strncmp(d->name, dir.name, sizeof(d->name)) != 0) {
		if(checkname(dir.name) || d1 == nil) {
			error = Ename;
			goto out;
		}
		if(strcmp(dir.name, ".") == 0 || strcmp(xd.name, "..") == 0) {
			error = Ename;
			goto out;
		}

		/*
		 * drop entry to prevent lock, then
		 * check that destination name is unique,
		 */
		putbuf(p);
		for(addr = 0; ; addr++) {
			if((p = dnodebuf(p1, d1, addr, 0)) == nil)
				break;
			if(checktag(p, Tdir, d1->qid.path)) {
				putbuf(p);
				continue;
			}
			for(slot = 0; slot < DIRPERBUF; slot++) {
				d = getdir(p, slot);
				if(!(d->mode & DALLOC))
					continue;
				if(strncmp(dir.name, d->name, sizeof(d->name)) == 0) {
					error = Eexist;
					goto out;
				}
			}
			putbuf(p);
		}

		/*
		 * reacquire entry
		 */
		if((p = getbuf(file->fs->dev, file->addr, Bread)) == nil){
			error = Ephase;
			goto out;
		}
		d = getdir(p, file->slot);
		if(d == nil || checktag(p, Tdir, QPNONE) || !(d->mode & DALLOC)) {
			error = Ephase;
			goto out;
		}

		if(wstatallow || writeallow) /* set to allow rename during boot */
			break;
		if(d1 == nil || iaccess(file, d1, DWRITE)) {
			error = Eaccess;
			goto out;
		}
		break;
	}

	/*
	 * if mode/time, either
	 *	a) owner
	 *	b) leader of either group
	 */
	mode = dir.mode & 0777;
	if(dir.mode & DMAPPEND)
		mode |= DAPND;
	if(dir.mode & DMEXCL)
		mode |= DLOCK;
	while(d->mtime != dir.mtime || ((d->mode^mode) & (DAPND|DLOCK|0777))) {
		if(wstatallow)			/* set to allow chmod during boot */
			break;
		if(d->uid == file->uid)
			break;
		if(leadgroup(file->uid, gid))
			break;
		if(leadgroup(file->uid, d->gid))
			break;
		error = Enotu;
		goto out;
	}
	d->mtime = dir.mtime;
	d->uid = uid;
	d->gid = gid;
	d->mode = (mode & (DAPND|DLOCK|0777)) | (d->mode & (DALLOC|DDIR));

	strncpy(d->name, dir.name, sizeof(d->name));
	accessdir(p, d, FWSTAT);

out:
	if(p != nil)
		putbuf(p);
	if(p1 != nil)
		putbuf(p1);
	if(file != nil)
		qunlock(file);

	return error;
}

static int
recv(Chan *c, uchar *buf, int n)
{
	int fd, m, len;

	fd = c->chan;
	/* read count */
	qlock(&c->rlock);
	m = readn(fd, buf, BIT32SZ);
	if(m != BIT32SZ){
		qunlock(&c->rlock);
		if(m < 0){
			print("readn(BIT32SZ) fails: %r\n");
			return -1;
		}
		print("readn(BIT32SZ) returns %d: %r\n", m);
		return 0;
	}

	len = GBIT32(buf);
	if(len <= BIT32SZ || len > n){
		print("recv bad length %d\n", len);
		werrstr("bad length in 9P2000 message header");
		qunlock(&c->rlock);
		return -1;
	}
	len -= BIT32SZ;
	m = readn(fd, buf+BIT32SZ, len);
	qunlock(&c->rlock);
	if(m < len){
		print("recv wanted %d got %d\n", len, m);
		return 0;
	}
	return BIT32SZ+m;
}

static void
send(Chan *c, uchar *buf, int n)
{
	int fd, m;

	fd = c->chan;
	qlock(&c->wlock);
	m = write(fd, buf, n);
	qunlock(&c->wlock);
	if(m == n)
		return;
	panic("write failed");
}

void
serve9p2(Chan *chan, uchar *ib, int nib)
{
	uchar inbuf[MSIZE+IOHDRSZ], outbuf[MSIZE+IOHDRSZ];
	Fcall f, r;
	char ename[64];
	int error, n, type;

	chan->msize = MSIZE;
	fmtinstall('F', fcallfmt);

	for(;;){
		if(nib){
			memmove(inbuf, ib, nib);
			n = nib;
			nib = 0;
		}else
			n = recv(chan, inbuf, sizeof inbuf);
		if(chat){
			print("read msg %d (fd %d)\n", n, chan->chan);
			if(n <= 0)
				print("\terr: %r\n");
		}
		if(n == 0 && (chan == cons.srvchan || chan == cons.chan))
			continue;
		if(n <= 0)
			break;
		if(convM2S(inbuf, n, &f) != n){
			print("9p2: cannot decode\n");
			continue;
		}

		type = f.type;
		if(type < Tversion || type >= Tmax || (type&1) || type == Terror){
			print("9p2: bad message type %d\n", type);
			continue;
		}

		if(CHAT(chan))
			print("9p2: f %F\n", &f);

		r.type = type+1;
		r.tag = f.tag;
		error = 0;

		rlock(&mainlock);
		rlock(&chan->reflock);
		switch(type){
		default:
			r.type = Rerror;
			snprint(ename, sizeof ename, "unknown message: %F", &f);
			r.ename = ename;
			break;
		case Tversion:
			error = fsversion(chan, &f, &r);
			break;
		case Tauth:
			error = fsauth(chan, &f, &r);
			break;
		case Tattach:
			error = fsattach(chan, &f, &r);
			break;
		case Tflush:
			error = fsflush(chan, &f, &r);
			break;
		case Twalk:
			error = fswalk(chan, &f, &r);
			break;
		case Topen:
			error = fsopen(chan, &f, &r);
			break;
		case Tcreate:
			error = fscreate(chan, &f, &r);
			break;
		case Tread:
			r.data = (char*)inbuf;
			error = fsread(chan, &f, &r);
			break;
		case Twrite:
			error = fswrite(chan, &f, &r);
			break;
		case Tclunk:
			error = fsclunk(chan, &f, &r);
			break;
		case Tremove:
			error = fsremove(chan, &f, &r);
			break;
		case Tstat:
			error = fsstat(chan, &f, &r, inbuf);
			break;
		case Twstat:
			error = fswstat(chan, &f, &r, (char*)outbuf);
			break;
		}
		runlock(&chan->reflock);
		runlock(&mainlock);

		if(error != 0){
			r.type = Rerror;
			if(error >= MAXERR){
				snprint(ename, sizeof(ename), "error %d", error);
				r.ename = ename;
			}
			else
				r.ename = errstring[error];
		}
		if(CHAT(chan))
			print("9p2: r %F\n", &r);
	
		n = convS2M(&r, outbuf, sizeof outbuf);
		if(n == 0){
			type = r.type;
			r.type = Rerror;
			snprint(ename, sizeof(ename), "9p2: convS2M: type %d", type);
			r.ename = ename;
			print(ename);
			n = convS2M(&r, outbuf, sizeof outbuf);
			if(n == 0){
				/*
				 * What to do here, the failure notification failed?
				 */
				panic("can't write anything at all");
			}
		}
		send(chan, outbuf, n);
	}
	fileinit(chan);
	close(chan->chan);
	if(chan == cons.srvchan || chan == cons.chan)
		print("console chan read error");
}

