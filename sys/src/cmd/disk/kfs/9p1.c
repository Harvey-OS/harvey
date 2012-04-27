#include	"all.h"
#include	"9p1.h"

/*
 * buggery to give false qid for
 * the top 2 levels of the dump fs
 */
void
mkqid(Qid* qid, Dentry *d, int buggery)
{
	int c;

	if(buggery && d->qid.path == QPROOT && (d->qid.path & QPDIR)){
		c = d->name[0];
		if(c >= '0' && c <= '9'){
			qid->path = 3;
			qid->vers = d->qid.version;
			qid->type = QTDIR;

			c = (c-'0')*10 + (d->name[1]-'0');
			if(c >= 1 && c <= 12)
				qid->path = 4;
			return;
		}
	}

	mkqid9p2(qid, &d->qid, d->mode);
}

int
mkqidcmp(Qid* qid, Dentry *d)
{
	Qid tmp;

	mkqid(&tmp, d, 1);
	if(qid->path==tmp.path && (qid->type&QTDIR)==(tmp.type&QTDIR))
		return 0;
	return Eqid;
}

void
f_nop(Chan *cp, Oldfcall *in, Oldfcall *ou)
{

	USED(in);
	USED(ou);
	if(CHAT(cp))
		print("c_nop %d\n", cp->chan);
}

void
f_flush(Chan *cp, Oldfcall *in, Oldfcall *ou)
{

	USED(in);
	USED(ou);
	if(CHAT(cp))
		print("c_flush %d\n", cp->chan);
	runlock(&cp->reflock);
	wlock(&cp->reflock);
	wunlock(&cp->reflock);
	rlock(&cp->reflock);
}

void
f_session(Chan *cp, Oldfcall *in, Oldfcall *ou)
{
	if(CHAT(cp))
		print("c_session %d\n", cp->chan);

	memmove(cp->rchal, in->chal, sizeof(cp->rchal));
	if(wstatallow || cp == cons.srvchan){
		memset(ou->chal, 0, sizeof(ou->chal));
		memset(ou->authid, 0, sizeof(ou->authid));
	}else{
		mkchallenge(cp);
		memmove(ou->chal, cp->chal, sizeof(ou->chal));
		memmove(ou->authid, nvr.authid, sizeof(ou->authid));
	}
	sprint(ou->authdom, "%s.%s", service, nvr.authdom);
	fileinit(cp);
}

void
f_attach(Chan *cp, Oldfcall *in, Oldfcall *ou)
{
	Iobuf *p;
	Dentry *d;
	File *f;
	int u;
	Filsys *fs;
	long raddr;

	if(CHAT(cp)) {
		print("c_attach %d\n", cp->chan);
		print("	fid = %d\n", in->fid);
		print("	uid = %s\n", in->uname);
		print("	arg = %s\n", in->aname);
	}

	ou->qid = QID9P1(0,0);
	ou->fid = in->fid;
	if(!in->aname[0])	/* default */
		strncpy(in->aname, filesys[0].name, sizeof(in->aname));
	p = 0;
	f = filep(cp, in->fid, 1);
	if(!f) {
		ou->err = Efid;
		goto out;
	}
	u = -1;
	if(cp != cons.chan){
		if(authorize(cp, in, ou) == 0 || strcmp(in->uname, "adm") == 0){
			ou->err = Eauth;
			goto out;
		}
		u = strtouid(in->uname);
		if(u < 0){
			ou->err = Ebadu;
			goto out;
		}
	}

	fs = fsstr(in->aname);
	if(fs == 0) {
		ou->err = Ebadspc;
		goto out;
	}
	raddr = getraddr(fs->dev);
	p = getbuf(fs->dev, raddr, Bread);
	d = getdir(p, 0);
	if(!d || checktag(p, Tdir, QPROOT) || !(d->mode & DALLOC)) {
		ou->err = Ealloc;
		goto out;
	}
	f->uid = u;
	if(iaccess(f, d, DREAD)) {
		ou->err = Eaccess;
		goto out;
	}
	accessdir(p, d, FREAD);
	mkqid(&f->qid, d, 1);
	f->fs = fs;
	f->addr = raddr;
	f->slot = 0;
	f->open = 0;
	freewp(f->wpath);
	f->wpath = 0;

	mkqid9p1(&ou->qid, &f->qid);

out:
	if(p)
		putbuf(p);
	if(f) {
		qunlock(f);
		if(ou->err)
			freefp(f);
	}
}

void
f_clone(Chan *cp, Oldfcall *in, Oldfcall *ou)
{
	File *f1, *f2;
	int fid, fid1;

	if(CHAT(cp)) {
		print("c_clone %d\n", cp->chan);
		print("	old fid = %d\n", in->fid);
		print("	new fid = %d\n", in->newfid);
	}

	fid = in->fid;
	fid1 = in->newfid;

	f1 = 0;
	f2 = 0;
	if(fid < fid1) {
		f1 = filep(cp, fid, 0);
		f2 = filep(cp, fid1, 1);
	} else
	if(fid1 < fid) {
		f2 = filep(cp, fid1, 1);
		f1 = filep(cp, fid, 0);
	}
	if(!f1 || !f2) {
		ou->err = Efid;
		goto out;
	}


	f2->fs = f1->fs;
	f2->addr = f1->addr;
	f2->open = f1->open & ~FREMOV;
	f2->uid = f1->uid;
	f2->slot = f1->slot;
	f2->qid = f1->qid;

	freewp(f2->wpath);
	f2->wpath = getwp(f1->wpath);

out:
	ou->fid = fid;
	if(f1)
		qunlock(f1);
	if(f2)
		qunlock(f2);
}

void
f_walk(Chan *cp, Oldfcall *in, Oldfcall *ou)
{
	Iobuf *p, *p1;
	Dentry *d, *d1;
	File *f;
	Wpath *w, *ow;
	int slot;
	long addr;

	if(CHAT(cp)) {
		print("c_walk %d\n", cp->chan);
		print("	fid = %d\n", in->fid);
		print("	name = %s\n", in->name);
	}

	ou->fid = in->fid;
	ou->qid = QID9P1(0,0);
	p = 0;
	f = filep(cp, in->fid, 0);
	if(!f) {
		ou->err = Efid;
		goto out;
	}
	p = getbuf(f->fs->dev, f->addr, Bread);
	d = getdir(p, f->slot);
	if(!d || checktag(p, Tdir, QPNONE) || !(d->mode & DALLOC)) {
		ou->err = Ealloc;
		goto out;
	}
	if(!(d->mode & DDIR)) {
		ou->err = Edir1;
		goto out;
	}
	if(ou->err = mkqidcmp(&f->qid, d))
		goto out;
	if(cp != cons.chan && iaccess(f, d, DEXEC)) {
		ou->err = Eaccess;
		goto out;
	}
	accessdir(p, d, FREAD);
	if(strcmp(in->name, ".") == 0)
		goto setdot;
	if(strcmp(in->name, "..") == 0) {
		if(f->wpath == 0)
			goto setdot;
		putbuf(p);
		p = 0;
		addr = f->wpath->addr;
		slot = f->wpath->slot;
		p1 = getbuf(f->fs->dev, addr, Bread);
		d1 = getdir(p1, slot);
		if(!d1 || checktag(p1, Tdir, QPNONE) || !(d1->mode & DALLOC)) {
			if(p1)
				putbuf(p1);
			ou->err = Ephase;
			goto out;
		}
		ow = f->wpath;
		f->wpath = ow->up;
		putwp(ow);
		goto found;
	}
	for(addr=0;; addr++) {
		p1 = dnodebuf(p, d, addr, 0);
		if(!p1 || checktag(p1, Tdir, d->qid.path) ) {
			if(p1)
				putbuf(p1);
			ou->err = Eentry;
			goto out;
		}
		for(slot=0; slot<DIRPERBUF; slot++) {
			d1 = getdir(p1, slot);
			if(!(d1->mode & DALLOC))
				continue;
			if(strncmp(in->name, d1->name, sizeof(in->name)))
				continue;
			/*
			 * update walk path
			 */
			w = newwp();
			if(!w) {
				ou->err = Ewalk;
				putbuf(p1);
				goto out;
			}
			w->addr = f->addr;
			w->slot = f->slot;
			w->up = f->wpath;
			f->wpath = w;
			slot += DIRPERBUF*addr;
			goto found;
		}
		putbuf(p1);
	}

found:
	f->addr = p1->addr;
	mkqid(&f->qid, d1, 1);
	putbuf(p1);
	f->slot = slot;

setdot:
	mkqid9p1(&ou->qid, &f->qid);
	f->open = 0;

out:
	if(p)
		putbuf(p);
	if(f)
		qunlock(f);
}

void
f_clunk(Chan *cp, Oldfcall *in, Oldfcall *ou)
{
	File *f;
	Tlock *t;
	long tim;

	if(CHAT(cp)) {
		print("c_clunk %d\n", cp->chan);
		print("	fid = %d\n", in->fid);
	}

	f = filep(cp, in->fid, 0);
	if(!f) {
		print("%p\n", f);
		ou->err = Efid;
		goto out;
	}
	if(t = f->tlock) {
		tim = time(0);
		if(t->time < tim || t->file != f)
			ou->err = Ebroken;
		t->time = 0;	/* free the lock */
		f->tlock = 0;
	}
	if(f->open & FREMOV)
		ou->err = doremove(f, 0);
	f->open = 0;
	freewp(f->wpath);
	freefp(f);

out:
	if(f)
		qunlock(f);
	ou->fid = in->fid;
}

void
f_clwalk(Chan *cp, Oldfcall *in, Oldfcall *ou)
{
	int er, fid;

	if(CHAT(cp))
		print("c_clwalk macro\n");

	f_clone(cp, in, ou);		/* sets tag, fid */
	if(ou->err)
		return;
	fid = in->fid;
	in->fid = in->newfid;
	f_walk(cp, in, ou);		/* sets tag, fid, qid */
	er = ou->err;
	if(er == Eentry) {
		/*
		 * if error is "no entry"
		 * return non error and fid
		 */
		ou->err = 0;
		f_clunk(cp, in, ou);	/* sets tag, fid */
		ou->err = 0;
		ou->fid = fid;
		if(CHAT(cp)) 
			print("	error: %s\n", errstring[er]);
		return;
	}
	if(er) {
		/*
		 * if any other error
		 * return an error
		 */
		ou->err = 0;
		f_clunk(cp, in, ou);	/* sets tag, fid */
		ou->err = er;
		return;
	}
	/*
	 * non error
	 * return newfid
	 */
}

void
f_open(Chan *cp, Oldfcall *in, Oldfcall *ou)
{
	Iobuf *p;
	Dentry *d;
	File *f;
	Tlock *t;
	Qid qid;
	int ro, fmod;

	if(CHAT(cp)) {
		print("c_open %d\n", cp->chan);
		print("	fid = %d\n", in->fid);
		print("	mode = %o\n", in->mode);
	}

	p = 0;
	f = filep(cp, in->fid, 0);
	if(!f) {
		ou->err = Efid;
		goto out;
	}

	/*
	 * if remove on close, check access here
	 */
	ro = isro(f->fs->dev) || (cp != cons.chan && writegroup && !ingroup(f->uid, writegroup));
	if(in->mode & MRCLOSE) {
		if(ro) {
			ou->err = Eronly;
			goto out;
		}
		/*
		 * check on parent directory of file to be deleted
		 */
		if(f->wpath == 0 || f->wpath->addr == f->addr) {
			ou->err = Ephase;
			goto out;
		}
		p = getbuf(f->fs->dev, f->wpath->addr, Bread);
		d = getdir(p, f->wpath->slot);
		if(!d || checktag(p, Tdir, QPNONE) || !(d->mode & DALLOC)) {
			ou->err = Ephase;
			goto out;
		}
		if(iaccess(f, d, DWRITE)) {
			ou->err = Eaccess;
			goto out;
		}
		putbuf(p);
	}
	p = getbuf(f->fs->dev, f->addr, Bread);
	d = getdir(p, f->slot);
	if(!d || checktag(p, Tdir, QPNONE) || !(d->mode & DALLOC)) {
		ou->err = Ealloc;
		goto out;
	}
	if(ou->err = mkqidcmp(&f->qid, d))
		goto out;
	mkqid(&qid, d, 1);
	switch(in->mode & 7) {

	case MREAD:
		if(iaccess(f, d, DREAD) && !writeallow)
			goto badaccess;
		fmod = FREAD;
		break;

	case MWRITE:
		if((d->mode & DDIR) ||
		   (iaccess(f, d, DWRITE) && !writeallow))
			goto badaccess;
		if(ro) {
			ou->err = Eronly;
			goto out;
		}
		fmod = FWRITE;
		break;

	case MBOTH:
		if((d->mode & DDIR) ||
		   (iaccess(f, d, DREAD) && !writeallow) ||
		   (iaccess(f, d, DWRITE) && !writeallow))
			goto badaccess;
		if(ro) {
			ou->err = Eronly;
			goto out;
		}
		fmod = FREAD+FWRITE;
		break;

	case MEXEC:
		if((d->mode & DDIR) ||
		   iaccess(f, d, DEXEC))
			goto badaccess;
		fmod = FREAD;
		break;

	default:
		ou->err = Emode;
		goto out;
	}
	if(in->mode & MTRUNC) {
		if((d->mode & DDIR) ||
		   (iaccess(f, d, DWRITE) && !writeallow))
			goto badaccess;
		if(ro) {
			ou->err = Eronly;
			goto out;
		}
	}
	t = 0;
	if(d->mode & DLOCK) {
		t = tlocked(p, d);
		if(t == 0) {
			ou->err = Elocked;
			goto out;
		}
		t->file = f;
	}
	if(in->mode & MRCLOSE)
		fmod |= FREMOV;
	f->open = fmod;
	if(in->mode & MTRUNC)
		if(!(d->mode & DAPND))
			dtrunc(p, d);
	f->tlock = t;
	f->lastra = 0;
	mkqid9p1(&ou->qid, &qid);
	goto out;

badaccess:
	ou->err = Eaccess;
	f->open = 0;

out:
	if(p)
		putbuf(p);
	if(f)
		qunlock(f);
	ou->fid = in->fid;
}

void
f_create(Chan *cp, Oldfcall *in, Oldfcall *ou)
{
	Iobuf *p, *p1;
	Dentry *d, *d1;
	File *f;
	int slot, slot1, fmod;
	long addr, addr1, path;
	Qid qid;
	Tlock *t;
	Wpath *w;

	if(CHAT(cp)) {
		print("c_create %d\n", cp->chan);
		print("	fid = %d\n", in->fid);
		print("	name = %s\n", in->name);
		print("	perm = %lx+%lo\n", (in->perm>>28)&0xf,
				in->perm&0777);
		print("	mode = %d\n", in->mode);
	}

	p = 0;
	f = filep(cp, in->fid, 0);
	if(!f) {
		ou->err = Efid;
		goto out;
	}
	if(isro(f->fs->dev) || (cp != cons.chan && writegroup && !ingroup(f->uid, writegroup))) {
		ou->err = Eronly;
		goto out;
	}

	p = getbuf(f->fs->dev, f->addr, Bread);
	d = getdir(p, f->slot);
	if(!d || checktag(p, Tdir, QPNONE) || !(d->mode & DALLOC)) {
		ou->err = Ealloc;
		goto out;
	}
	if(ou->err = mkqidcmp(&f->qid, d))
		goto out;
	if(!(d->mode & DDIR)) {
		ou->err = Edir2;
		goto out;
	}
	if(cp != cons.chan && iaccess(f, d, DWRITE) && !writeallow) {
		ou->err = Eaccess;
		goto out;
	}
	accessdir(p, d, FREAD);
	if(!strncmp(in->name, ".", sizeof(in->name)) ||
	   !strncmp(in->name, "..", sizeof(in->name))) {
		ou->err = Edot;
		goto out;
	}
	if(checkname(in->name)) {
		ou->err = Ename;
		goto out;
	}
	addr1 = 0;
	slot1 = 0;	/* set */
	for(addr=0;; addr++) {
		p1 = dnodebuf(p, d, addr, 0);
		if(!p1) {
			if(addr1)
				break;
			p1 = dnodebuf(p, d, addr, Tdir);
		}
		if(p1 == 0) {
			ou->err = Efull;
			goto out;
		}
		if(checktag(p1, Tdir, d->qid.path)) {
			putbuf(p1);
			goto phase;
		}
		for(slot=0; slot<DIRPERBUF; slot++) {
			d1 = getdir(p1, slot);
			if(!(d1->mode & DALLOC)) {
				if(!addr1) {
					addr1 = p1->addr;
					slot1 = slot + addr*DIRPERBUF;
				}
				continue;
			}
			if(!strncmp(in->name, d1->name, sizeof(in->name))) {
				putbuf(p1);
				ou->err = Eexist;
				goto out;
			}
		}
		putbuf(p1);
	}
	switch(in->mode & 7) {
	case MEXEC:
	case MREAD:		/* seems only useful to make directories */
		fmod = FREAD;
		break;

	case MWRITE:
		fmod = FWRITE;
		break;

	case MBOTH:
		fmod = FREAD+FWRITE;
		break;

	default:
		ou->err = Emode;
		goto out;
	}
	if(in->perm & PDIR)
		if((in->mode & MTRUNC) || (in->perm & PAPND) || (fmod & FWRITE))
			goto badaccess;
	/*
	 * do it
	 */
	path = qidpathgen(&f->fs->dev);
	p1 = getbuf(f->fs->dev, addr1, Bread|Bimm|Bmod);
	d1 = getdir(p1, slot1);
	if(!d1 || checktag(p1, Tdir, d->qid.path)) {
		if(p1)
			putbuf(p1);
		goto phase;
	}
	if(d1->mode & DALLOC) {
		putbuf(p1);
		goto phase;
	}

	strncpy(d1->name, in->name, sizeof(in->name));
	/*
	 * bogus argument passing -- see console.c
	 */
	if(cp == cons.chan) {
		d1->uid = cons.uid;
		d1->gid = cons.gid;
	} else {
		d1->uid = f->uid;
		d1->gid = d->gid;
		in->perm &= d->mode | ~0666;
		if(in->perm & PDIR)
			in->perm &= d->mode | ~0777;
	}
	d1->qid.path = path;
	d1->qid.version = 0;
	d1->mode = DALLOC | (in->perm & 0777);
	if(in->perm & PDIR) {
		d1->mode |= DDIR;
		d1->qid.path |= QPDIR;
	}
	if(in->perm & PAPND)
		d1->mode |= DAPND;
	t = 0;
	if(in->perm & PLOCK) {
		d1->mode |= DLOCK;
		t = tlocked(p1, d1);
	}
	accessdir(p1, d1, FWRITE);
	mkqid(&qid, d1, 0);
	putbuf(p1);
	accessdir(p, d, FWRITE);

	/*
	 * do a walk to new directory entry
	 */
	w = newwp();
	if(!w) {
		ou->err = Ewalk;
		goto out;
	}
	w->addr = f->addr;
	w->slot = f->slot;
	w->up = f->wpath;
	f->wpath = w;
	f->qid = qid;
	f->tlock = t;
	f->lastra = 0;
	if(in->mode & MRCLOSE)
		fmod |= FREMOV;
	f->open = fmod;
	f->addr = addr1;
	f->slot = slot1;
	if(t)
		t->file = f;
	mkqid9p1(&ou->qid, &qid);
	goto out;

badaccess:
	ou->err = Eaccess;
	goto out;

phase:
	ou->err = Ephase;

out:
	if(p)
		putbuf(p);
	if(f)
		qunlock(f);
	ou->fid = in->fid;
}

void
f_read(Chan *cp, Oldfcall *in, Oldfcall *ou)
{
	Iobuf *p, *p1;
	File *f;
	Dentry *d, *d1;
	Tlock *t;
	long addr, offset, tim;
	int nread, count, n, o, slot;

	if(CHAT(cp)) {
		print("c_read %d\n", cp->chan);
		print("	fid = %d\n", in->fid);
		print("	offset = %ld\n", in->offset);
		print("	count = %ld\n", in->count);
	}

	p = 0;
	count = in->count;
	offset = in->offset;
	nread = 0;
	f = filep(cp, in->fid, 0);
	if(!f) {
		ou->err = Efid;
		goto out;
	}
	if(!(f->open & FREAD)) {
		ou->err = Eopen;
		goto out;
	}
	if(count < 0 || count > MAXDAT) {
		ou->err = Ecount;
		goto out;
	}
	if(offset < 0) {
		ou->err = Eoffset;
		goto out;
	}
	p = getbuf(f->fs->dev, f->addr, Bread);
	d = getdir(p, f->slot);
	if(!d || !(d->mode & DALLOC)) {
		ou->err = Ealloc;
		goto out;
	}
	if(ou->err = mkqidcmp(&f->qid, d))
		goto out;
	if(t = f->tlock) {
		tim = time(0);
		if(t->time < tim || t->file != f) {
			ou->err = Ebroken;
			goto out;
		}
		/* renew the lock */
		t->time = tim + TLOCK;
	}
	accessdir(p, d, FREAD);
	if(d->mode & DDIR) {
		addr = 0;
		goto dread;
	}
	if(offset+count > d->size)
		count = d->size - offset;
	while(count > 0) {
		addr = offset / BUFSIZE;
		if(addr == f->lastra+1)
			dbufread(p, d, addr+1);
		f->lastra = addr;
		o = offset % BUFSIZE;
		n = BUFSIZE - o;
		if(n > count)
			n = count;
		p1 = dnodebuf(p, d, addr, 0);
		if(p1) {
			if(checktag(p1, Tfile, QPNONE)) {
				ou->err = Ephase;
				putbuf(p1);
				goto out;
			}
			memmove(ou->data+nread, p1->iobuf+o, n);
			putbuf(p1);
		} else
			memset(ou->data+nread, 0, n);
		count -= n;
		nread += n;
		offset += n;
	}
	goto out;

dread:
	p1 = dnodebuf(p, d, addr, 0);
	if(!p1)
		goto out;
	if(checktag(p1, Tdir, QPNONE)) {
		ou->err = Ephase;
		putbuf(p1);
		goto out;
	}
	n = DIRREC;
	for(slot=0; slot<DIRPERBUF; slot++) {
		d1 = getdir(p1, slot);
		if(!(d1->mode & DALLOC))
			continue;
		if(offset >= n) {
			offset -= n;
			continue;
		}
		if(count < n) {
			putbuf(p1);
			goto out;
		}
		if(convD2M9p1(d1, ou->data+nread) != n)
			print("dirread convD2M\n");
		nread += n;
		count -= n;
	}
	putbuf(p1);
	addr++;
	goto dread;

out:
	count = in->count - nread;
	if(count > 0)
		memset(ou->data+nread, 0, count);
	if(p)
		putbuf(p);
	if(f)
		qunlock(f);
	ou->fid = in->fid;
	ou->count = nread;
	if(CHAT(cp))
		print("	nread = %d\n", nread);
}

void
f_write(Chan *cp, Oldfcall *in, Oldfcall *ou)
{
	Iobuf *p, *p1;
	Dentry *d;
	File *f;
	Tlock *t;
	long offset, addr, tim;
	int count, nwrite, o, n;

	if(CHAT(cp)) {
		print("c_write %d\n", cp->chan);
		print("	fid = %d\n", in->fid);
		print("	offset = %ld\n", in->offset);
		print("	count = %ld\n", in->count);
	}

	offset = in->offset;
	count = in->count;
	nwrite = 0;
	p = 0;
	f = filep(cp, in->fid, 0);
	if(!f) {
		ou->err = Efid;
		goto out;
	}
	if(!(f->open & FWRITE)) {
		ou->err = Eopen;
		goto out;
	}
	if(isro(f->fs->dev) || (cp != cons.chan && writegroup && !ingroup(f->uid, writegroup))) {
		ou->err = Eronly;
		goto out;
	}
	if(count < 0 || count > MAXDAT) {
		ou->err = Ecount;
		goto out;
	}
	if(offset < 0) {
		ou->err = Eoffset;
		goto out;
	}
	p = getbuf(f->fs->dev, f->addr, Bread|Bmod);
	d = getdir(p, f->slot);
	if(!d || !(d->mode & DALLOC)) {
		ou->err = Ealloc;
		goto out;
	}
	if(ou->err = mkqidcmp(&f->qid, d))
		goto out;
	if(t = f->tlock) {
		tim = time(0);
		if(t->time < tim || t->file != f) {
			ou->err = Ebroken;
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
	while(count > 0) {
		addr = offset / BUFSIZE;
		o = offset % BUFSIZE;
		n = BUFSIZE - o;
		if(n > count)
			n = count;
		p1 = dnodebuf(p, d, addr, Tfile);
		if(p1 == 0) {
			ou->err = Efull;
			goto out;
		}
		if(checktag(p1, Tfile, d->qid.path)) {
			putbuf(p1);
			ou->err = Ephase;
			goto out;
		}
		memmove(p1->iobuf+o, in->data+nwrite, n);
		p1->flags |= Bmod;
		putbuf(p1);
		count -= n;
		nwrite += n;
		offset += n;
	}
	if(CHAT(cp))
		print("	nwrite = %d\n", nwrite);

out:
	if(p)
		putbuf(p);
	if(f)
		qunlock(f);
	ou->fid = in->fid;
	ou->count = nwrite;
}

int
doremove(File *f, int iscon)
{
	Iobuf *p, *p1;
	Dentry *d, *d1;
	long addr;
	int slot, err;

	p = 0;
	p1 = 0;
	if(isro(f->fs->dev) || (f->cp != cons.chan && writegroup && !ingroup(f->uid, writegroup))) {
		err = Eronly;
		goto out;
	}
	/*
	 * check on parent directory of file to be deleted
	 */
	if(f->wpath == 0 || f->wpath->addr == f->addr) {
		err = Ephase;
		goto out;
	}
	p1 = getbuf(f->fs->dev, f->wpath->addr, Bread);
	d1 = getdir(p1, f->wpath->slot);
	if(!d1 || checktag(p1, Tdir, QPNONE) || !(d1->mode & DALLOC)) {
		err = Ephase;
		goto out;
	}
	if(!iscon && iaccess(f, d1, DWRITE)) {
		err = Eaccess;
		goto out;
	}
	accessdir(p1, d1, FWRITE);
	putbuf(p1);
	p1 = 0;

	/*
	 * check on file to be deleted
	 */
	p = getbuf(f->fs->dev, f->addr, Bread);
	d = getdir(p, f->slot);
	if(!d || checktag(p, Tdir, QPNONE) || !(d->mode & DALLOC)) {
		err = Ealloc;
		goto out;
	}
	if(err = mkqidcmp(&f->qid, d))
		goto out;

	/*
	 * if deleting a directory, make sure it is empty
	 */
	if((d->mode & DDIR))
	for(addr=0;; addr++) {
		p1 = dnodebuf(p, d, addr, 0);
		if(!p1)
			break;
		if(checktag(p1, Tdir, d->qid.path)) {
			err = Ephase;
			goto out;
		}
		for(slot=0; slot<DIRPERBUF; slot++) {
			d1 = getdir(p1, slot);
			if(!(d1->mode & DALLOC))
				continue;
			err = Eempty;
			goto out;
		}
		putbuf(p1);
	}

	/*
	 * do it
	 */
	dtrunc(p, d);
	memset(d, 0, sizeof(Dentry));
	settag(p, Tdir, QPNONE);

out:
	if(p1)
		putbuf(p1);
	if(p)
		putbuf(p);
	return err;
}

void
f_remove(Chan *cp, Oldfcall *in, Oldfcall *ou)
{
	File *f;

	if(CHAT(cp)) {
		print("c_remove %d\n", cp->chan);
		print("	fid = %d\n", in->fid);
	}

	f = filep(cp, in->fid, 0);
	if(!f) {
		ou->err = Efid;
		goto out;
	}
	ou->err = doremove(f, cp==cons.chan);

out:
	ou->fid = in->fid;
	if(f)
		qunlock(f);
}

void
f_stat(Chan *cp, Oldfcall *in, Oldfcall *ou)
{
	Iobuf *p;
	Dentry *d;
	File *f;

	if(CHAT(cp)) {
		print("c_stat %d\n", cp->chan);
		print("	fid = %d\n", in->fid);
	}

	p = 0;
	memset(ou->stat, 0, sizeof(ou->stat));
	f = filep(cp, in->fid, 0);
	if(!f) {
		ou->err = Efid;
		goto out;
	}
	p = getbuf(f->fs->dev, f->addr, Bread);
	d = getdir(p, f->slot);
	if(!d || checktag(p, Tdir, QPNONE) || !(d->mode & DALLOC)) {
		ou->err = Ealloc;
		goto out;
	}
	if(ou->err = mkqidcmp(&f->qid, d))
		goto out;
	if(d->qid.path == QPROOT)	/* stat of root gives time */
		d->atime = time(0);
	if(convD2M9p1(d, ou->stat) != DIRREC)
		print("stat convD2M\n");

out:
	if(p)
		putbuf(p);
	if(f)
		qunlock(f);
	ou->fid = in->fid;
}

void
f_wstat(Chan *cp, Oldfcall *in, Oldfcall *ou)
{
	Iobuf *p, *p1;
	Dentry *d, *d1, xd;
	File *f;
	int slot;
	long addr;

	if(CHAT(cp)) {
		print("c_wstat %d\n", cp->chan);
		print("	fid = %d\n", in->fid);
	}

	p = 0;
	p1 = 0;
	d1 = 0;
	f = filep(cp, in->fid, 0);
	if(!f) {
		ou->err = Efid;
		goto out;
	}
	if(isro(f->fs->dev) || (cp != cons.chan && writegroup && !ingroup(f->uid, writegroup))) {
		ou->err = Eronly;
		goto out;
	}

	/*
	 * first get parent
	 */
	if(f->wpath) {
		p1 = getbuf(f->fs->dev, f->wpath->addr, Bread);
		d1 = getdir(p1, f->wpath->slot);
		if(!d1 || checktag(p1, Tdir, QPNONE) || !(d1->mode & DALLOC)) {
			ou->err = Ephase;
			goto out;
		}
	}

	p = getbuf(f->fs->dev, f->addr, Bread);
	d = getdir(p, f->slot);
	if(!d || checktag(p, Tdir, QPNONE) || !(d->mode & DALLOC)) {
		ou->err = Ealloc;
		goto out;
	}
	if(ou->err = mkqidcmp(&f->qid, d))
		goto out;

	convM2D9p1(in->stat, &xd);
	if(CHAT(cp)) {
		print("	d.name = %s\n", xd.name);
		print("	d.uid  = %d\n", xd.uid);
		print("	d.gid  = %d\n", xd.gid);
		print("	d.mode = %.4x\n", xd.mode);
	}

	/*
	 * if chown,
	 * must be god
	 */
	while(xd.uid != d->uid) {
		if(wstatallow)			/* set to allow chown during boot */
			break;
		ou->err = Enotu;
		goto out;
	}

	/*
	 * if chgroup,
	 * must be either
	 *	a) owner and in new group
	 *	b) leader of both groups
	 */
	while(xd.gid != d->gid) {
		if(wstatallow || writeallow)		/* set to allow chgrp during boot */
			break;
		if(d->uid == f->uid && ingroup(f->uid, xd.gid))
			break;
		if(leadgroup(f->uid, xd.gid))
			if(leadgroup(f->uid, d->gid))
				break;
		ou->err = Enotg;
		goto out;
	}

	/*
	 * if rename,
	 * must have write permission in parent
	 */
	if(xd.name[0] == 0)
		strncpy(xd.name, d->name, sizeof(xd.name));
	while(strncmp(d->name, xd.name, sizeof(d->name)) != 0) {
		if(checkname(xd.name)) {
			ou->err = Ename;
			goto out;
		}

		if(strcmp(xd.name, ".") == 0 || strcmp(xd.name, "..") == 0) {
			ou->err = Ename;
			goto out;
		}

		/*
		 * drop entry to prevent lock, then
		 * check that destination name is unique,
		 */
		putbuf(p);
		for(addr=0;; addr++) {
			p = dnodebuf(p1, d1, addr, 0);
			if(!p)
				break;
			if(checktag(p, Tdir, d1->qid.path)) {
				putbuf(p);
				continue;
			}
			for(slot=0; slot<DIRPERBUF; slot++) {
				d = getdir(p, slot);
				if(!(d->mode & DALLOC))
					continue;
				if(!strncmp(xd.name, d->name, sizeof(xd.name))) {
					ou->err = Eexist;
					goto out;
				}
			}
			putbuf(p);
		}

		/*
		 * reacquire entry
		 */
		p = getbuf(f->fs->dev, f->addr, Bread);
		d = getdir(p, f->slot);
		if(!d || checktag(p, Tdir, QPNONE) || !(d->mode & DALLOC)) {
			ou->err = Ephase;
			goto out;
		}

		if(wstatallow || writeallow) /* set to allow rename during boot */
			break;
		if(!d1 || iaccess(f, d1, DWRITE)) {
			ou->err = Eaccess;
			goto out;
		}
		break;
	}

	/*
	 * if mode/time, either
	 *	a) owner
	 *	b) leader of either group
	 */
	while(d->mtime != xd.mtime ||
	     ((d->mode^xd.mode) & (DAPND|DLOCK|0777))) {
		if(wstatallow)			/* set to allow chmod during boot */
			break;
		if(d->uid == f->uid)
			break;
		if(leadgroup(f->uid, xd.gid))
			break;
		if(leadgroup(f->uid, d->gid))
			break;
		ou->err = Enotu;
		goto out;
	}
	d->mtime = xd.mtime;
	d->uid = xd.uid;
	d->gid = xd.gid;
	d->mode = (xd.mode & (DAPND|DLOCK|0777)) | (d->mode & (DALLOC|DDIR));

	strncpy(d->name, xd.name, sizeof(d->name));
	if(wstatallow) {
		p->flags |= Bmod;
		if(xd.atime)
			d->atime = xd.atime;
		if(xd.mtime)
			d->mtime = xd.mtime;
	} else
		accessdir(p, d, FWSTAT);

out:
	if(p)
		putbuf(p);
	if(p1)
		putbuf(p1);
	if(f)
		qunlock(f);
	ou->fid = in->fid;
}

void
(*call9p1[MAXSYSCALL])(Chan*, Oldfcall*, Oldfcall*) =
{
	[Tnop9p1]		f_nop,
	[Tosession9p1]	f_session,
	[Tsession9p1]	f_session,
	[Tflush9p1]	f_flush,
	[Toattach9p1]	f_attach,
	[Tattach9p1]	f_attach,
	[Tclone9p1]	f_clone,
	[Twalk9p1]		f_walk,
	[Topen9p1]		f_open,
	[Tcreate9p1]	f_create,
	[Tread9p1]		f_read,
	[Twrite9p1]	f_write,
	[Tclunk9p1]	f_clunk,
	[Tremove9p1]	f_remove,
	[Tstat9p1]		f_stat,
	[Twstat9p1]	f_wstat,
	[Tclwalk9p1]	f_clwalk,
};

static void
send(Chan *c, uchar *buf, int n)
{
	int fd, m;

	fd = c->chan;
	m = write(fd, buf, n);
	if(m == n)
		return;
	panic("write failed");
}

void
error9p1(Chan *c, uchar *buf)
{
	buf[0] = Rnop9p1;
	buf[1] = ~0;
	buf[2] = ~0;

	send(c, buf, 3);
}

void
serve9p1(Chan *chan, uchar *ib, int nib)
{
	int n, t;
	uchar inbuf[MAXMSG+MAXDAT], outbuf[MAXMSG+MAXDAT];
	Oldfcall fi, fo;

	for(;;){
		if(nib){
			memmove(inbuf, ib, nib);
			n = nib;
			nib = 0;
		}else
			n = read(chan->chan, inbuf, sizeof inbuf);
		if(chat)
			print("read msg %d\n", n);
		if(n == 0 && (chan == cons.srvchan || chan == cons.chan))
			continue;
		if(n <= 0)
			return;
		if(convM2S9p1(inbuf, &fi, n) != n){
			error9p1(chan, outbuf);
			continue;
		}

		t = fi.type;
		if(t < 0 || t >= MAXSYSCALL || (t&1) || !call9p1[t]) {
			print("9p1: bad message type\n");
			error9p1(chan, outbuf);
			continue;
		}

		if(CHAT(chan))
			print("9p1: fi %O\n", &fi);

		/*
		 * set up reply message
		 */
		fo.err = 0;
		if(t == Tread9p1)
			fo.data = (char*)outbuf + 8;
	
		/*
		 * call the file system
		 */
		cons.work.count++;
		cons.rate.count += n;

		/*
		 * call the file system
		 */
		rlock(&mainlock);
		rlock(&chan->reflock);
	
		(*call9p1[t])(chan, &fi, &fo);
	
		runlock(&chan->reflock);
		runlock(&mainlock);
	
		fo.type = t+1;
		fo.tag = fi.tag;
	
		if(chat)
			print("9p1: fo %O\n", &fo);

		if(fo.err) {
			strcpy(fo.ename, errstring[fo.err]);
			if(CHAT(cp))
				print("	error: %s\n", fo.ename);
			fo.type = Terror9p1+1;
		}
	
		n = convS2M9p1(&fo, outbuf);
		if(n == 0) {
			print("9p1: bad S2M conversion\n");
			error9p1(chan, outbuf);
			continue;
		}

		cons.rate.count += n;
		send(chan, outbuf, n);
	}
}
