#include "all.h"
#include "9p1.h"

extern Nvrsafe	nvr;

typedef struct {
	uchar	chal[CHALLEN];		/* locally generated challenge */
	uchar	rchal[CHALLEN];		/* remotely generated challenge */
	Lock	idlock;
	ulong	idoffset;		/* offset of id vector */
	ulong	idvec;			/* vector of acceptable id's */
} Authinfo;

static void
f_nop(Chan *cp, Fcall*, Fcall*)
{
	if(CHAT(cp))
		print("c_nop %d\n", cp->chan);
}

static void
f_flush(Chan *cp, Fcall*, Fcall*)
{
	if(CHAT(cp))
		print("c_flush %d\n", cp->chan);
	runlock(&cp->reflock);
	wlock(&cp->reflock);
	wunlock(&cp->reflock);
	rlock(&cp->reflock);
}

/*
 *  create a challenge for a fid space
 */
static void
mkchallenge(Authinfo *aip)
{
	int i;

	srand((uintptr)aip + time(nil));
	for(i = 0; i < CHALLEN; i++)
		aip->chal[i] = nrand(256);

	aip->idoffset = 0;
	aip->idvec = 0;
}

static void
f_session(Chan *cp, Fcall *in, Fcall *ou)
{
	Authinfo *aip;

	aip = (Authinfo*)cp->authinfo;

	if(CHAT(cp))
		print("c_session %d\n", cp->chan);
	memmove(aip->rchal, in->chal, sizeof(aip->rchal));
	mkchallenge(aip);
	memmove(ou->chal, aip->chal, sizeof(ou->chal));
	if(noauth || wstatallow)
		memset(ou->authid, 0, sizeof(ou->authid));
	else
		memmove(ou->authid, nvr.authid, sizeof(ou->authid));

	sprint(ou->authdom, "%s.%s", service, nvr.authdom);
	fileinit(cp);
}

/*
 *  match a challenge from an attach
 */
static int
authorize(Chan *cp, Fcall *in, Fcall *ou)
{
	Ticket t;
	Authenticator a;
	int x;
	ulong bit;
	Authinfo *aip;

	if(noauth || wstatallow)	/* set to allow entry during boot */
		return 1;

	if(strcmp(in->uname, "none") == 0)
		return 1;

	if(in->type == Toattach)
		return 0;

	/* decrypt and unpack ticket */
	convM2T9p1(in->ticket, &t, nvr.machkey);
	if(t.num != AuthTs){
		print("9p1: bad AuthTs num\n");
		return 0;
	}

	/* decrypt and unpack authenticator */
	convM2A9p1(in->auth, &a, t.key);
	if(a.num != AuthAc){
		print("9p1: bad AuthAc num\n");
		return 0;
	}

	/* challenges must match */
	aip = (Authinfo*)cp->authinfo;
	if(memcmp(a.chal, aip->chal, sizeof(a.chal)) != 0){
		print("9p1: bad challenge\n");
		return 0;
	}

	/*
	 *  the id must be in a valid range.  the range is specified by a
	 *  lower bound (idoffset) and a bit vector (idvec) where a
	 *  bit set to 1 means unusable
	 */
	lock(&aip->idlock);
	x = a.id - aip->idoffset;
	bit = 1<<x;
	if(x < 0 || x > 31 || (bit&aip->idvec)){
		unlock(&aip->idlock);
		print("9p1: id out of range: idoff %ld idvec %lux id %ld\n",
		   aip->idoffset, aip->idvec, a.id);
		return 0;
	}
	aip->idvec |= bit;

	/* normalize the vector */
	while(aip->idvec&0xffff0001){
		aip->idvec >>= 1;
		aip->idoffset++;
	}
	unlock(&aip->idlock);

	/* ticket name and attach name must match */
	if(memcmp(in->uname, t.cuid, sizeof(in->uname)) != 0){
		print("9p1: names don't match\n");
		return 0;
	}

	/* copy translated name into input record */
	memmove(in->uname, t.suid, sizeof(in->uname));

	/* craft a reply */
	a.num = AuthAs;
	memmove(a.chal, aip->rchal, CHALLEN);
	convA2M9p1(&a, ou->rauth, t.key);

	return 1;
}

/*
 * buggery to give false qid for
 * the top 2 levels of the dump fs
 */
void
mkqid(Qid* qid, Dentry *d, int buggery)
{
	int c;

	if(buggery && d->qid.path == (QPDIR|QPROOT)){
		c = d->name[0];
		if(isascii(c) && isdigit(c)){
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
	if(qid->path == tmp.path && qid->type == tmp.type)
		return 0;
	return Eqid;
}

static void
f_attach(Chan *cp, Fcall *in, Fcall *ou)
{
	Iobuf *p;
	Dentry *d;
	File *f;
	int u;
	Filsys *fs;
	Off raddr;

	if(CHAT(cp)) {
		print("c_attach %d\n", cp->chan);
		print("\tfid = %d\n", in->fid);
		print("\tuid = %s\n", in->uname);
		print("\targ = %s\n", in->aname);
	}

	ou->qid = QID9P1(0,0);
	ou->fid = in->fid;
	if(!in->aname[0])	/* default */
		strncpy(in->aname, "main", sizeof(in->aname));
	p = 0;
	f = filep(cp, in->fid, 1);
	if(!f) {
		ou->err = Efid;
		goto out;
	}

	u = -1;
	if(cp != cons.chan) {
		if(noattach && strcmp(in->uname, "none")) {
			ou->err = Enoattach;
			goto out;
		}
		if(authorize(cp, in, ou) == 0 || strcmp(in->uname, "adm") == 0) {
			ou->err = Eauth;
			goto out;
		}
		u = strtouid(in->uname);
		if(u < 0) {
			ou->err = Ebadu;
			goto out;
		}
	}
	f->uid = u;

	fs = fsstr(in->aname);
	if(fs == 0) {
		ou->err = Ebadspc;
		goto out;
	}
	raddr = getraddr(fs->dev);
	p = getbuf(fs->dev, raddr, Brd);
	d = getdir(p, 0);
	if(!d || checktag(p, Tdir, QPROOT) || !(d->mode & DALLOC)) {
		ou->err = Ealloc;
		goto out;
	}
	if (iaccess(f, d, DEXEC) ||
	    f->uid == 0 && fs->dev->type == Devro) {
		/*
		 * 'none' not allowed on dump
		 */
		ou->err = Eaccess;
		goto out;
	}
	accessdir(p, d, FREAD, f->uid);
	mkqid(&f->qid, d, 1);
	f->fs = fs;
	f->addr = raddr;
	f->slot = 0;
	f->open = 0;
	freewp(f->wpath);
	f->wpath = 0;

	mkqid9p1(&ou->qid, &f->qid);

	strncpy(cp->whoname, in->uname, sizeof(cp->whoname));
	cp->whotime = time(nil);
	if(cons.flags & attachflag)
		print("9p1: attach %s %T to \"%s\" C%d\n",
			cp->whoname, cp->whotime, fs->name, cp->chan);

out:
	if((cons.flags & attachflag) && ou->err)
		print("9p1: attach %s %T SUCK EGGS --- %s\n",
			in->uname, time(nil), errstr9p[ou->err]);
	if(p)
		putbuf(p);
	if(f) {
		qunlock(f);
		if(ou->err)
			freefp(f);
	}
}

static void
f_clone(Chan *cp, Fcall *in, Fcall *ou)
{
	File *f1, *f2;
	Wpath *p;
	int fid, fid1;

	if(CHAT(cp)) {
		print("c_clone %d\n", cp->chan);
		print("\told fid = %d\n", in->fid);
		print("\tnew fid = %d\n", in->newfid);
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
	lock(&wpathlock);
	f2->wpath = f1->wpath;
	for(p = f2->wpath; p; p = p->up)
		p->refs++;
	unlock(&wpathlock);

out:
	ou->fid = fid;
	if(f1)
		qunlock(f1);
	if(f2) {
		qunlock(f2);
		if(ou->err)
			freefp(f2);
	}
}

static void
f_walk(Chan *cp, Fcall *in, Fcall *ou)
{
	Iobuf *p, *p1;
	Dentry *d, *d1;
	File *f;
	Wpath *w;
	int slot;
	Off addr, qpath;

	if(CHAT(cp)) {
		print("c_walk %d\n", cp->chan);
		print("\tfid = %d\n", in->fid);
		print("\tname = %s\n", in->name);
	}

	ou->fid = in->fid;
	ou->qid = QID9P1(0,0);
	p = 0;
	f = filep(cp, in->fid, 0);
	if(!f) {
		ou->err = Efid;
		goto out;
	}
	p = getbuf(f->fs->dev, f->addr, Brd);
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
	accessdir(p, d, FREAD, f->uid);
	if(strcmp(in->name, ".") == 0)
		goto setdot;
	if(strcmp(in->name, "..") == 0) {
		if(f->wpath == 0)
			goto setdot;
		putbuf(p);
		p = 0;
		addr = f->wpath->addr;
		slot = f->wpath->slot;
		p1 = getbuf(f->fs->dev, addr, Brd);
		d1 = getdir(p1, slot);
		if(!d1 || checktag(p1, Tdir, QPNONE) || !(d1->mode & DALLOC)) {
			if(p1)
				putbuf(p1);
			ou->err = Ephase;
			goto out;
		}
		lock(&wpathlock);
		f->wpath->refs--;
		f->wpath = f->wpath->up;
		unlock(&wpathlock);
		goto found;
	}
	for(addr=0;; addr++) {
		if(p == 0) {
			p = getbuf(f->fs->dev, f->addr, Brd);
			d = getdir(p, f->slot);
			if(!d || checktag(p, Tdir, QPNONE) || !(d->mode & DALLOC)) {
				ou->err = Ealloc;
				goto out;
			}
		}
		qpath = d->qid.path;
		p1 = dnodebuf1(p, d, addr, 0, f->uid);
		p = 0;
		if(!p1 || checktag(p1, Tdir, qpath) ) {
			if(p1)
				putbuf(p1);
			ou->err = Eentry;
			goto out;
		}
		for(slot=0; slot<DIRPERBUF; slot++) {
			d1 = getdir(p1, slot);
			if(!(d1->mode & DALLOC))
				continue;
			if(strncmp(in->name, d1->name, sizeof(in->name)) != 0)
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

static void
f_open(Chan *cp, Fcall *in, Fcall *ou)
{
	Iobuf *p;
	Dentry *d;
	File *f;
	Tlock *t;
	Qid qid;
	int ro, fmod, wok;

	if(CHAT(cp)) {
		print("c_open %d\n", cp->chan);
		print("\tfid = %d\n", in->fid);
		print("\tmode = %o\n", in->mode);
	}

	wok = 0;
	if(cp == cons.chan || writeallow)
		wok = 1;

	p = 0;
	f = filep(cp, in->fid, 0);
	if(!f) {
		ou->err = Efid;
		goto out;
	}

	/*
	 * if remove on close, check access here
	 */
	ro = f->fs->dev->type == Devro;
	if(in->mode & ORCLOSE) {
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
		p = getbuf(f->fs->dev, f->wpath->addr, Brd);
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
	p = getbuf(f->fs->dev, f->addr, Brd);
	d = getdir(p, f->slot);
	if(!d || checktag(p, Tdir, QPNONE) || !(d->mode & DALLOC)) {
		ou->err = Ealloc;
		goto out;
	}
	if(ou->err = mkqidcmp(&f->qid, d))
		goto out;
	mkqid(&qid, d, 1);
	switch(in->mode & 7) {

	case OREAD:
		if(iaccess(f, d, DREAD) && !wok)
			goto badaccess;
		fmod = FREAD;
		break;

	case OWRITE:
		if((d->mode & DDIR) ||
		   (iaccess(f, d, DWRITE) && !wok))
			goto badaccess;
		if(ro) {
			ou->err = Eronly;
			goto out;
		}
		fmod = FWRITE;
		break;

	case ORDWR:
		if((d->mode & DDIR) ||
		   (iaccess(f, d, DREAD) && !wok) ||
		   (iaccess(f, d, DWRITE) && !wok))
			goto badaccess;
		if(ro) {
			ou->err = Eronly;
			goto out;
		}
		fmod = FREAD+FWRITE;
		break;

	case OEXEC:
		if((d->mode & DDIR) ||
		   (iaccess(f, d, DEXEC) && !wok))
			goto badaccess;
		fmod = FREAD;
		break;

	default:
		ou->err = Emode;
		goto out;
	}
	if(in->mode & OTRUNC) {
		if((d->mode & DDIR) ||
		   (iaccess(f, d, DWRITE) && !wok))
			goto badaccess;
		if(ro) {
			ou->err = Eronly;
			goto out;
		}
	}
	t = 0;
	if(d->mode & DLOCK) {
		t = tlocked(p, d);
		if(t == nil) {
			ou->err = Elocked;
			goto out;
		}
	}
	if(in->mode & ORCLOSE)
		fmod |= FREMOV;
	f->open = fmod;
	if(in->mode & OTRUNC)
		if(!(d->mode & DAPND)) {
			dtrunc(p, d, f->uid);
			qid.vers = d->qid.version;
		}
	f->tlock = t;
	if(t)
		t->file = f;
	f->lastra = 1;
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

static void
f_create(Chan *cp, Fcall *in, Fcall *ou)
{
	Iobuf *p, *p1;
	Dentry *d, *d1;
	File *f;
	int slot, slot1, fmod, wok;
	Off addr, addr1, path;
	Qid qid;
	Tlock *t;
	Wpath *w;

	if(CHAT(cp)) {
		print("c_create %d\n", cp->chan);
		print("\tfid = %d\n", in->fid);
		print("\tname = %s\n", in->name);
		print("\tperm = %lx+%lo\n", (in->perm>>28)&0xf,
				in->perm&0777);
		print("\tmode = %o\n", in->mode);
	}

	wok = 0;
	if(cp == cons.chan || writeallow)
		wok = 1;

	p = 0;
	f = filep(cp, in->fid, 0);
	if(!f) {
		ou->err = Efid;
		goto out;
	}
	if(f->fs->dev->type == Devro) {
		ou->err = Eronly;
		goto out;
	}

	p = getbuf(f->fs->dev, f->addr, Brd);
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
	if(iaccess(f, d, DWRITE) && !wok) {
		ou->err = Eaccess;
		goto out;
	}
	accessdir(p, d, FREAD, f->uid);
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
		p1 = dnodebuf(p, d, addr, 0, f->uid);
		if(!p1) {
			if(addr1)
				break;
			p1 = dnodebuf(p, d, addr, Tdir, f->uid);
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
		ou->err = Emode;
		goto out;
	}
	if(in->perm & PDIR)
		if((in->mode & OTRUNC) || (in->perm & PAPND) || (fmod & FWRITE))
			goto badaccess;
	/*
	 * do it
	 */
	path = qidpathgen(f->fs->dev);
	p1 = getbuf(f->fs->dev, addr1, Brd|Bimm|Bmod);
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
		/* if nil, out of tlock structures */
	}
	accessdir(p1, d1, FWRITE, f->uid);
	mkqid(&qid, d1, 0);
	putbuf(p1);
	accessdir(p, d, FWRITE, f->uid);

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
	if(t)
		t->file = f;
	f->lastra = 1;
	if(in->mode & ORCLOSE)
		fmod |= FREMOV;
	f->open = fmod;
	f->addr = addr1;
	f->slot = slot1;
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

static void
f_read(Chan *cp, Fcall *in, Fcall *ou)
{
	Iobuf *p, *p1;
	File *f;
	Dentry *d, *d1;
	Tlock *t;
	Off addr, offset;
	Timet tim;
	int nread, count, n, o, slot;

	if(CHAT(cp)) {
		print("c_read %d\n", cp->chan);
		print("\tfid = %d\n", in->fid);
		print("\toffset = %lld\n", (Wideoff)in->offset);
		print("\tcount = %ld\n", in->count);
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
	p = getbuf(f->fs->dev, f->addr, Brd);
	d = getdir(p, f->slot);
	if(!d || !(d->mode & DALLOC)) {
		ou->err = Ealloc;
		goto out;
	}
	if(ou->err = mkqidcmp(&f->qid, d))
		goto out;
	if(t = f->tlock) {
		tim = toytime();
		if(t->time < tim || t->file != f) {
			ou->err = Ebroken;
			goto out;
		}
		/* renew the lock */
		t->time = tim + TLOCK;
	}
	accessdir(p, d, FREAD, f->uid);
	if(d->mode & DDIR) {
		addr = 0;
		goto dread;
	}

	/* XXXX terrible hack to get at raw data XXXX */
	if(rawreadok && strncmp(d->name, "--raw--", 7) == 0) {
		Device *dev;
		Devsize boff, bsize;

		dev = p->dev;
		putbuf(p);
		p = 0;

		boff = number(d->name + 7, 0, 10) * 100000;
		if(boff < 0)
			boff = 0;
		if(boff > devsize(dev))
			boff = devsize(dev);
		bsize = devsize(dev) - boff;

		if(offset+count >= 100000*RBUFSIZE)
			count = 100000*RBUFSIZE - offset;

		if((offset+count)/RBUFSIZE >= bsize)
			/* will not overflow */
			count = bsize*RBUFSIZE - offset;

		while(count > 0) {
			addr = offset / RBUFSIZE;
			addr += boff;
			o = offset % RBUFSIZE;
			n = RBUFSIZE - o;
			if(n > count)
				n = count;

			p1 = getbuf(dev, addr, Brd);
			if(p1) {
				memmove(ou->data+nread, p1->iobuf+o, n);
				putbuf(p1);
			} else
				memset(ou->data+nread, 0, n);
			count -= n;
			nread += n;
			offset += n;
		}
		goto out;
	}

	if(offset+count > d->size)
		count = d->size - offset;
	while(count > 0) {
		if(p == 0) {
			p = getbuf(f->fs->dev, f->addr, Brd);
			d = getdir(p, f->slot);
			if(!d || !(d->mode & DALLOC)) {
				ou->err = Ealloc;
				goto out;
			}
		}
		addr = offset / BUFSIZE;
		f->lastra = dbufread(p, d, addr, f->lastra, f->uid);
		o = offset % BUFSIZE;
		n = BUFSIZE - o;
		if(n > count)
			n = count;
		p1 = dnodebuf1(p, d, addr, 0, f->uid);
		p = 0;
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
	for (;;) {
		if(p == 0) {
			p = getbuf(f->fs->dev, f->addr, Brd);
			d = getdir(p, f->slot);
			if(!d || !(d->mode & DALLOC)) {
				ou->err = Ealloc;
				goto out;
			}
		}
		p1 = dnodebuf1(p, d, addr, 0, f->uid);
		p = 0;
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
				print("9p1: dirread convD2M1990\n");
			nread += n;
			count -= n;
		}
		putbuf(p1);
		addr++;
	}
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
		print("\tnread = %d\n", nread);
}

static void
f_write(Chan *cp, Fcall *in, Fcall *ou)
{
	Iobuf *p, *p1;
	Dentry *d;
	File *f;
	Tlock *t;
	Off offset, addr, qpath;
	Timet tim;
	int count, nwrite, o, n;

	if(CHAT(cp)) {
		print("c_write %d\n", cp->chan);
		print("\tfid = %d\n", in->fid);
		print("\toffset = %lld\n", (Wideoff)in->offset);
		print("\tcount = %ld\n", in->count);
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
	if(f->fs->dev->type == Devro) {
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
	p = getbuf(f->fs->dev, f->addr, Brd|Bmod);
	d = getdir(p, f->slot);
	if(!d || !(d->mode & DALLOC)) {
		ou->err = Ealloc;
		goto out;
	}
	if(ou->err = mkqidcmp(&f->qid, d))
		goto out;
	if(t = f->tlock) {
		tim = toytime();
		if(t->time < tim || t->file != f) {
			ou->err = Ebroken;
			goto out;
		}
		/* renew the lock */
		t->time = tim + TLOCK;
	}
	accessdir(p, d, FWRITE, f->uid);
	if(d->mode & DAPND)
		offset = d->size;
	if(offset+count > d->size)
		d->size = offset+count;
	while(count > 0) {
		if(p == 0) {
			p = getbuf(f->fs->dev, f->addr, Brd|Bmod);
			d = getdir(p, f->slot);
			if(!d || !(d->mode & DALLOC)) {
				ou->err = Ealloc;
				goto out;
			}
		}
		addr = offset / BUFSIZE;
		o = offset % BUFSIZE;
		n = BUFSIZE - o;
		if(n > count)
			n = count;
		qpath = d->qid.path;
		p1 = dnodebuf1(p, d, addr, Tfile, f->uid);
		p = 0;
		if(p1 == 0) {
			ou->err = Efull;
			goto out;
		}
		if(checktag(p1, Tfile, qpath)) {
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
		print("\tnwrite = %d\n", nwrite);

out:
	if(p)
		putbuf(p);
	if(f)
		qunlock(f);
	ou->fid = in->fid;
	ou->count = nwrite;
}

int
doremove(File *f, int wok)
{
	Iobuf *p, *p1;
	Dentry *d, *d1;
	Off addr;
	int slot, err;

	p = 0;
	p1 = 0;
	if(f->fs->dev->type == Devro) {
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
	p1 = getbuf(f->fs->dev, f->wpath->addr, Brd);
	d1 = getdir(p1, f->wpath->slot);
	if(!d1 || checktag(p1, Tdir, QPNONE) || !(d1->mode & DALLOC)) {
		err = Ephase;
		goto out;
	}
	if(iaccess(f, d1, DWRITE) && !wok) {
		err = Eaccess;
		goto out;
	}
	accessdir(p1, d1, FWRITE, f->uid);
	putbuf(p1);
	p1 = 0;

	/*
	 * check on file to be deleted
	 */
	p = getbuf(f->fs->dev, f->addr, Brd);
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
		p1 = dnodebuf(p, d, addr, 0, f->uid);
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
	dtrunc(p, d, f->uid);
	memset(d, 0, sizeof(Dentry));
	settag(p, Tdir, QPNONE);

out:
	if(p1)
		putbuf(p1);
	if(p)
		putbuf(p);
	return err;
}

static int
doclunk(File* f, int remove, int wok)
{
	Tlock *t;
	int err;

	err = 0;
	if(t = f->tlock) {
		if(t->file == f)
			t->time = 0;	/* free the lock */
		f->tlock = 0;
	}
	if(remove)
		err = doremove(f, wok);
	f->open = 0;
	freewp(f->wpath);
	freefp(f);

	return err;
}

static void
f_clunk(Chan *cp, Fcall *in, Fcall *ou)
{
	File *f;

	if(CHAT(cp)) {
		print("c_clunk %d\n", cp->chan);
		print("\tfid = %d\n", in->fid);
	}

	f = filep(cp, in->fid, 0);
	if(!f)
		ou->err = Efid;
	else {
		doclunk(f, f->open & FREMOV, 0);
		qunlock(f);
	}
	ou->fid = in->fid;
}

static void
f_remove(Chan *cp, Fcall *in, Fcall *ou)
{
	File *f;

	if(CHAT(cp)) {
		print("c_remove %d\n", cp->chan);
		print("\tfid = %d\n", in->fid);
	}

	f = filep(cp, in->fid, 0);
	if(!f)
		ou->err = Efid;
	else {
		ou->err = doclunk(f, 1, cp==cons.chan);
		qunlock(f);
	}
	ou->fid = in->fid;
}

static void
f_stat(Chan *cp, Fcall *in, Fcall *ou)
{
	Iobuf *p;
	Dentry *d;
	File *f;

	if(CHAT(cp)) {
		print("c_stat %d\n", cp->chan);
		print("\tfid = %d\n", in->fid);
	}

	p = 0;
	memset(ou->stat, 0, sizeof(ou->stat));
	f = filep(cp, in->fid, 0);
	if(!f) {
		ou->err = Efid;
		goto out;
	}
	p = getbuf(f->fs->dev, f->addr, Brd);
	d = getdir(p, f->slot);
	if(!d || checktag(p, Tdir, QPNONE) || !(d->mode & DALLOC)) {
		ou->err = Ealloc;
		goto out;
	}
	if(ou->err = mkqidcmp(&f->qid, d))
		goto out;
	if(d->qid.path == QPROOT)	/* stat of root gives time */
		d->atime = time(nil);
	if(convD2M9p1(d, ou->stat) != DIRREC)
		print("9p1: stat convD2M\n");

out:
	if(p)
		putbuf(p);
	if(f)
		qunlock(f);
	ou->fid = in->fid;
}

static void
f_wstat(Chan *cp, Fcall *in, Fcall *ou)
{
	Iobuf *p, *p1;
	Dentry *d, *d1, xd;
	File *f;
	int slot;
	Off addr;

	if(CHAT(cp)) {
		print("c_wstat %d\n", cp->chan);
		print("\tfid = %d\n", in->fid);
	}

	p = 0;
	p1 = 0;
	d1 = 0;
	f = filep(cp, in->fid, 0);
	if(!f) {
		ou->err = Efid;
		goto out;
	}
	if(f->fs->dev->type == Devro) {
		ou->err = Eronly;
		goto out;
	}

	/*
	 * first get parent
	 */
	if(f->wpath) {
		p1 = getbuf(f->fs->dev, f->wpath->addr, Brd);
		d1 = getdir(p1, f->wpath->slot);
		if(!d1 || checktag(p1, Tdir, QPNONE) || !(d1->mode & DALLOC)) {
			ou->err = Ephase;
			goto out;
		}
	}

	p = getbuf(f->fs->dev, f->addr, Brd);
	d = getdir(p, f->slot);
	if(!d || checktag(p, Tdir, QPNONE) || !(d->mode & DALLOC)) {
		ou->err = Ealloc;
		goto out;
	}
	if(ou->err = mkqidcmp(&f->qid, d))
		goto out;

	convM2D9p1(in->stat, &xd);
	if(CHAT(cp)) {
		print("\td.name = %s\n", xd.name);
		print("\td.uid  = %d\n", xd.uid);
		print("\td.gid  = %d\n", xd.gid);
		print("\td.mode = %o\n", xd.mode);
	}

	/*
	 * if user none,
	 * cant do anything
	 */
	if(f->uid == 0) {
		ou->err = Eaccess;
		goto out;
	}

	/*
	 * if chown,
	 * must be god
	 */
	if(xd.uid != d->uid && !wstatallow) { /* set to allow chown during boot */
		ou->err = Ewstatu;
		goto out;
	}

	/*
	 * if chgroup,
	 * must be either
	 *	a) owner and in new group
	 *	b) leader of both groups
	 */
	if (xd.gid != d->gid &&
	    (!wstatallow && !writeallow &&  /* set to allow chgrp during boot */
	     (d->uid != f->uid || !ingroup(f->uid, xd.gid)) &&
	     (!leadgroup(f->uid, xd.gid) || !leadgroup(f->uid, d->gid)))) {
		ou->err = Ewstatg;
		goto out;
	}

	/*
	 * if rename,
	 * must have write permission in parent
	 */
	if (strncmp(d->name, xd.name, sizeof(d->name)) != 0) {
		if (checkname(xd.name) || !d1 ||
		    strcmp(xd.name, ".") == 0 || strcmp(xd.name, "..") == 0) {
			ou->err = Ename;
			goto out;
		}

		/*
		 * drop entry to prevent lock, then
		 * check that destination name is unique,
		 */
		putbuf(p);
		for(addr=0;; addr++) {
			p = dnodebuf(p1, d1, addr, 0, f->uid);
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
		p = getbuf(f->fs->dev, f->addr, Brd);
		d = getdir(p, f->slot);
		if(!d || checktag(p, Tdir, QPNONE) || !(d->mode & DALLOC)) {
			ou->err = Ephase;
			goto out;
		}

		if (!wstatallow && !writeallow && /* set to allow rename during boot */
		    (!d1 || iaccess(f, d1, DWRITE))) {
			ou->err = Eaccess;
			goto out;
		}
	}

	/*
	 * if mode/time, either
	 *	a) owner
	 *	b) leader of either group
	 */
	if (d->mtime != xd.mtime ||
	    ((d->mode^xd.mode) & (DAPND|DLOCK|0777)))
		if (!wstatallow &&	/* set to allow chmod during boot */
		    d->uid != f->uid &&
		    !leadgroup(f->uid, xd.gid) &&
		    !leadgroup(f->uid, d->gid)) {
			ou->err = Ewstatu;
			goto out;
		}
	d->mtime = xd.mtime;
	d->uid = xd.uid;
	d->gid = xd.gid;
	d->mode = (xd.mode & (DAPND|DLOCK|0777)) | (d->mode & (DALLOC|DDIR));

	strncpy(d->name, xd.name, sizeof(d->name));
	accessdir(p, d, FREAD, f->uid);

out:
	if(p)
		putbuf(p);
	if(p1)
		putbuf(p1);
	if(f)
		qunlock(f);
	ou->fid = in->fid;
}

static void
f_clwalk(Chan *cp, Fcall *in, Fcall *ou)
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
			print("\terror: %s\n", errstr9p[er]);
	} else if(er) {
		/*
		 * if any other error
		 * return an error
		 */
		ou->err = 0;
		f_clunk(cp, in, ou);	/* sets tag, fid */
		ou->err = er;
	}
	/*
	 * non error
	 * return newfid
	 */
}

void (*call9p1[MAXSYSCALL])(Chan*, Fcall*, Fcall*) =
{
	[Tnop]		f_nop,
	[Tosession]	f_session,
	[Tsession]	f_session,
	[Tflush]	f_flush,
	[Toattach]	f_attach,
	[Tattach]	f_attach,
	[Tclone]	f_clone,
	[Twalk]		f_walk,
	[Topen]		f_open,
	[Tcreate]	f_create,
	[Tread]		f_read,
	[Twrite]	f_write,
	[Tclunk]	f_clunk,
	[Tremove]	f_remove,
	[Tstat]		f_stat,
	[Twstat]	f_wstat,
	[Tclwalk]	f_clwalk,
};

int
error9p1(Chan* cp, Msgbuf* mb)
{
	Msgbuf *mb1;

	print("type=%d count=%d\n", mb->data[0], mb->count);
	print(" %.2x %.2x %.2x %.2x\n",
		mb->data[1]&0xff, mb->data[2]&0xff,
		mb->data[3]&0xff, mb->data[4]&0xff);
	print(" %.2x %.2x %.2x %.2x\n",
		mb->data[5]&0xff, mb->data[6]&0xff,
		mb->data[7]&0xff, mb->data[8]&0xff);
	print(" %.2x %.2x %.2x %.2x\n",
		mb->data[9]&0xff, mb->data[10]&0xff,
		mb->data[11]&0xff, mb->data[12]&0xff);

	mb1 = mballoc(3, cp, Mbreply4);
	mb1->data[0] = Rnop;	/* your nop was ok */
	mb1->data[1] = ~0;
	mb1->data[2] = ~0;
	mb1->count = 3;
	mb1->param = mb->param;
	fs_send(cp->reply, mb1);

	return 1;
}

int
serve9p1(Msgbuf* mb)
{
	int t, n;
	Chan *cp;
	Msgbuf *mb1;
	Fcall fi, fo;

	assert(mb != nil);
	cp = mb->chan;
	assert(mb->data != nil);
	if(convM2S9p1(mb->data, &fi, mb->count) == 0){
		assert(cp != nil);
		if(cp->protocol == nil)
			return 0;
		print("9p1: bad M2S conversion\n");
		return error9p1(cp, mb);
	}

	t = fi.type;
	if(t < 0 || t >= MAXSYSCALL || (t&1) || !call9p1[t]) {
		print("9p1: bad message type\n");
		return error9p1(cp, mb);
	}

	/*
	 * allocate reply message
	 */
	if(t == Tread) {
		mb1 = mballoc(MAXMSG+MAXDAT, cp, Mbreply2);
		fo.data = (char*)(mb1->data + 8);
	} else
		mb1 = mballoc(MAXMSG, cp, Mbreply3);

	/*
	 * call the file system
	 */
	assert(cp != nil);
	fo.err = 0;

	(*call9p1[t])(cp, &fi, &fo);

	fo.type = t+1;
	fo.tag = fi.tag;

	if(fo.err) {
		if(cons.flags&errorflag)
			print("\ttype %d: error: %s\n", t, errstr9p[fo.err]);
		if(CHAT(cp))
			print("\terror: %s\n", errstr9p[fo.err]);
		fo.type = Rerror;
		strncpy(fo.ename, errstr9p[fo.err], sizeof(fo.ename));
	}

	n = convS2M9p1(&fo, mb1->data);
	if(n == 0) {
		print("9p1: bad S2M conversion\n");
		mbfree(mb1);
		return error9p1(cp, mb);
	}
	mb1->count = n;
	mb1->param = mb->param;
	fs_send(cp->reply, mb1);

	return 1;
}
