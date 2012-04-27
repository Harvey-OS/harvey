#include	"all.h"

#include	"9p1.h"

void
fcall9p1(Chan *cp, Fcall *in, Fcall *ou)
{
	int t;

	rlock(&mainlock);
	t = in->type;
	if(t < 0 || t >= MAXSYSCALL || (t&1) || !call9p1[t]) {
		print("bad message type %d\n", t);
		panic("");
	}
	ou->type = t+1;
	ou->err = 0;

	rlock(&cp->reflock);
	(*call9p1[t])(cp, in, ou);
	runlock(&cp->reflock);

	if(ou->err && CHAT(cp))
		print("\terror: %s\n", errstr9p[ou->err]);
	runlock(&mainlock);
}

int
con_session(void)
{
	Fcall in, ou;

	in.type = Tsession;
	fcall9p1(cons.chan, &in, &ou);
	return ou.err;
}

int
con_attach(int fid, char *uid, char *arg)
{
	Fcall in, ou;

	in.type = Tattach;
	in.fid = fid;
	strncpy(in.uname, uid, NAMELEN);
	strncpy(in.aname, arg, NAMELEN);
	fcall9p1(cons.chan, &in, &ou);
	return ou.err;
}

int
con_clone(int fid1, int fid2)
{
	Fcall in, ou;

	in.type = Tclone;
	in.fid = fid1;
	in.newfid = fid2;
	fcall9p1(cons.chan, &in, &ou);
	return ou.err;
}

int
con_walk(int fid, char *name)
{
	Fcall in, ou;

	in.type = Twalk;
	in.fid = fid;
	strncpy(in.name, name, NAMELEN);
	fcall9p1(cons.chan, &in, &ou);
	return ou.err;
}

int
con_open(int fid, int mode)
{
	Fcall in, ou;

	in.type = Topen;
	in.fid = fid;
	in.mode = mode;
	fcall9p1(cons.chan, &in, &ou);
	return ou.err;
}

int
con_read(int fid, char *data, Off offset, int count)
{
	Fcall in, ou;

	in.type = Tread;
	in.fid = fid;
	in.offset = offset;
	in.count = count;
	ou.data = data;
	fcall9p1(cons.chan, &in, &ou);
	if(ou.err)
		return 0;
	return ou.count;
}

int
con_write(int fid, char *data, Off offset, int count)
{
	Fcall in, ou;

	in.type = Twrite;
	in.fid = fid;
	in.data = data;
	in.offset = offset;
	in.count = count;
	fcall9p1(cons.chan, &in, &ou);
	if(ou.err)
		return 0;
	return ou.count;
}

int
con_remove(int fid)
{
	Fcall in, ou;

	in.type = Tremove;
	in.fid = fid;
	fcall9p1(cons.chan, &in, &ou);
	return ou.err;
}

int
con_create(int fid, char *name, int uid, int gid, long perm, int mode)
{
	Fcall in, ou;

	in.type = Tcreate;
	in.fid = fid;
	strncpy(in.name, name, NAMELEN);
	in.perm = perm;
	in.mode = mode;
	cons.uid = uid;			/* beyond ugly */
	cons.gid = gid;
	fcall9p1(cons.chan, &in, &ou);
	return ou.err;
}

int
doclri(File *f)
{
	Iobuf *p, *p1;
	Dentry *d, *d1;
	int err;

	err = 0;
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

	accessdir(p1, d1, FWRITE, 0);
	putbuf(p1);
	p1 = 0;

	/*
	 * check on file to be deleted
	 */
	p = getbuf(f->fs->dev, f->addr, Brd);
	d = getdir(p, f->slot);

	/*
	 * do it
	 */
	memset(d, 0, sizeof(Dentry));
	settag(p, Tdir, QPNONE);
	freewp(f->wpath);
	freefp(f);

out:
	if(p1)
		putbuf(p1);
	if(p)
		putbuf(p);
	return err;
}

void
f_fstat(Chan *cp, Fcall *in, Fcall *ou)
{
	File *f;
	Iobuf *p;
	Dentry *d;
	int i;

	if(CHAT(cp)) {
		print("c_fstat %d\n", cp->chan);
		print("\tfid = %d\n", in->fid);
	}

	p = 0;
	f = filep(cp, in->fid, 0);
	if(!f) {
		ou->err = Efid;
		goto out;
	}
	p = getbuf(f->fs->dev, f->addr, Brd);
	d = getdir(p, f->slot);
	if(d == 0)
		goto out;

	print("name = %.*s\n", NAMELEN, d->name);
	print("uid = %d; gid = %d; muid = %d\n", d->uid, d->gid, d->muid);
	print("size = %lld; qid = %llux/%lux\n", (Wideoff)d->size,
		(Wideoff)d->qid.path, d->qid.version);
	print("atime = %ld; mtime = %ld\n", d->atime, d->mtime);
	print("dblock =");
	for(i=0; i<NDBLOCK; i++)
		print(" %lld", (Wideoff)d->dblock[i]);
	for (i = 0; i < NIBLOCK; i++)
		print("; iblocks[%d] = %lld", i, (Wideoff)d->iblocks[i]);
	print("\n\n");

out:
	if(p)
		putbuf(p);
	ou->fid = in->fid;
	if(f)
		qunlock(f);
}

void
f_clri(Chan *cp, Fcall *in, Fcall *ou)
{
	File *f;

	if(CHAT(cp)) {
		print("c_clri %d\n", cp->chan);
		print("\tfid = %d\n", in->fid);
	}

	f = filep(cp, in->fid, 0);
	if(!f) {
		ou->err = Efid;
		goto out;
	}
	ou->err = doclri(f);

out:
	ou->fid = in->fid;
	if(f)
		qunlock(f);
}

int
con_clri(int fid)
{
	Fcall in, ou;
	Chan *cp;

	in.type = Tremove;
	in.fid = fid;
	cp = cons.chan;

	rlock(&mainlock);
	ou.type = Tremove+1;
	ou.err = 0;

	rlock(&cp->reflock);
	f_clri(cp, &in, &ou);
	runlock(&cp->reflock);

	runlock(&mainlock);
	return ou.err;
}

int
con_fstat(int fid)
{
	Fcall in, ou;
	Chan *cp;

	in.type = Tstat;
	in.fid = fid;
	cp = cons.chan;

	rlock(&mainlock);
	ou.type = Tstat+1;
	ou.err = 0;

	rlock(&cp->reflock);
	f_fstat(cp, &in, &ou);
	runlock(&cp->reflock);

	runlock(&mainlock);
	return ou.err;
}
