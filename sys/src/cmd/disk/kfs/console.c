#include	"all.h"
#include	"9p1.h"

void
fcall9p1(Chan *cp, Oldfcall *in, Oldfcall *ou)
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

	if(ou->err)
		if(CHAT(cp))
			print("	error: %s\n", errstring[ou->err]);
	cons.work.count++;
	runlock(&mainlock);
}

int
con_session(void)
{
	Oldfcall in, ou;

	in.type = Tsession9p1;
	fcall9p1(cons.chan, &in, &ou);
	return ou.err;
}

int
con_attach(int fid, char *uid, char *arg)
{
	Oldfcall in, ou;

	in.type = Tattach9p1;
	in.fid = fid;
	strncpy(in.uname, uid, NAMELEN);
	strncpy(in.aname, arg, NAMELEN);
	fcall9p1(cons.chan, &in, &ou);
	return ou.err;
}

int
con_clone(int fid1, int fid2)
{
	Oldfcall in, ou;

	in.type = Tclone9p1;
	in.fid = fid1;
	in.newfid = fid2;
	fcall9p1(cons.chan, &in, &ou);
	return ou.err;
}

int
con_path(int fid, char *path)
{
	Oldfcall in, ou;
	char *p;

	in.type = Twalk9p1;
	in.fid = fid;

loop:
	if(*path == 0)
		return 0;
	strncpy(in.name, path, NAMELEN);
	if(p = strchr(path, '/')) {
		path = p+1;
		if(p = strchr(in.name, '/'))
			*p = 0;
	} else
		path = strchr(path, 0);
	if(in.name[0]) {
		fcall9p1(cons.chan, &in, &ou);
		if(ou.err)
			return ou.err;
	}
	goto loop;
}

int
con_walk(int fid, char *name)
{
	Oldfcall in, ou;

	in.type = Twalk9p1;
	in.fid = fid;
	strncpy(in.name, name, NAMELEN);
	fcall9p1(cons.chan, &in, &ou);
	return ou.err;
}

int
con_stat(int fid, char *data)
{
	Oldfcall in, ou;

	in.type = Tstat9p1;
	in.fid = fid;
	fcall9p1(cons.chan, &in, &ou);
	if(ou.err == 0)
		memmove(data, ou.stat, sizeof ou.stat);
	return ou.err;
}

int
con_wstat(int fid, char *data)
{
	Oldfcall in, ou;

	in.type = Twstat9p1;
	in.fid = fid;
	memmove(in.stat, data, sizeof in.stat);
	fcall9p1(cons.chan, &in, &ou);
	return ou.err;
}

int
con_open(int fid, int mode)
{
	Oldfcall in, ou;

	in.type = Topen9p1;
	in.fid = fid;
	in.mode = mode;
	fcall9p1(cons.chan, &in, &ou);
	return ou.err;
}

int
con_read(int fid, char *data, long offset, int count)
{
	Oldfcall in, ou;

	in.type = Tread9p1;
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
con_write(int fid, char *data, long offset, int count)
{
	Oldfcall in, ou;

	in.type = Twrite9p1;
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
	Oldfcall in, ou;

	in.type = Tremove9p1;
	in.fid = fid;
	fcall9p1(cons.chan, &in, &ou);
	return ou.err;
}

int
con_create(int fid, char *name, int uid, int gid, long perm, int mode)
{
	Oldfcall in, ou;

	in.type = Tcreate9p1;
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
	if(isro(f->fs->dev)) {
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

	accessdir(p1, d1, FWRITE);
	putbuf(p1);
	p1 = 0;

	/*
	 * check on file to be deleted
	 */
	p = getbuf(f->fs->dev, f->addr, Bread);
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
f_clri(Chan *cp, Oldfcall *in, Oldfcall *ou)
{
	File *f;

	if(CHAT(cp)) {
		print("c_clri %d\n", cp->chan);
		print("	fid = %d\n", in->fid);
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
	Oldfcall in, ou;
	Chan *cp;

	in.type = Tremove9p1;
	in.fid = fid;
	cp = cons.chan;

	rlock(&mainlock);
	ou.type = Tremove9p1+1;
	ou.err = 0;

	rlock(&cp->reflock);

	f_clri(cp, &in, &ou);

	runlock(&cp->reflock);

	cons.work.count++;
	runlock(&mainlock);
	return ou.err;
}

int
con_swap(int fid1, int fid2)
{
	int err;
	Iobuf *p1, *p2;
	File *f1, *f2;
	Dentry *d1, *d2;
	Dentry dt1, dt2;
	Chan *cp;

	cp = cons.chan;
	err = 0;
	rlock(&mainlock);
	rlock(&cp->reflock);

	f2 = nil;
	p1 = p2 = nil;
	f1 = filep(cp, fid1, 0);
	if(!f1){
		err = Efid;
		goto out;
	}
	p1 = getbuf(f1->fs->dev, f1->addr, Bread|Bmod);
	d1 = getdir(p1, f1->slot);
	if(!d1 || !(d1->mode&DALLOC)){
		err = Ealloc;
		goto out;
	}

	f2 = filep(cp, fid2, 0);
	if(!f2){
		err = Efid;
		goto out;
	}
	if(memcmp(&f1->fs->dev, &f2->fs->dev, 4)==0
	&& f2->addr == f1->addr)
		p2 = p1;
	else
		p2 = getbuf(f2->fs->dev, f2->addr, Bread|Bmod);
	d2 = getdir(p2, f2->slot);
	if(!d2 || !(d2->mode&DALLOC)){
		err = Ealloc;
		goto out;
	}

	dt1 = *d1;
	dt2 = *d2;
	*d1 = dt2;
	*d2 = dt1;
	memmove(d1->name, dt1.name, NAMELEN);
	memmove(d2->name, dt2.name, NAMELEN);

	mkqid(&f1->qid, d1, 1);
	mkqid(&f2->qid, d2, 1);
out:
	if(f1)
		qunlock(f1);
	if(f2)
		qunlock(f2);
	if(p1)
		putbuf(p1);
	if(p2 && p2!=p1)
		putbuf(p2);

	runlock(&cp->reflock);
	cons.work.count++;
	runlock(&mainlock);

	return err;
}
