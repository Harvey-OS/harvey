#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

/*
 * The sys*() routines needn't poperror() as they return directly to syscall().
 */

int
growfd(Fgrp *f, int fd)	/* fd is always >= 0 */
{
	Chan **newfd, **oldfd;

	if(fd < f->nfd)
		return 0;
	if(fd >= f->nfd+DELTAFD)
		return -1;	/* out of range */
	if(fd % 100 == 0)
		pprint("warning: process exceeds %d file descriptors\n", fd);
	/*
	 * Unbounded allocation is unwise; besides, there are only 16 bits
	 * of fid in 9P
	 */
	if(f->nfd >= 5000){
    Exhausted:
		print("no free file descriptors\n");
		return -1;
	}
	newfd = malloc((f->nfd+DELTAFD)*sizeof(Chan*));
	if(newfd == 0)
		goto Exhausted;
	oldfd = f->fd;
	memmove(newfd, oldfd, f->nfd*sizeof(Chan*));
	f->fd = newfd;
	free(oldfd);
	f->nfd += DELTAFD;
	if(fd > f->maxfd)
		f->maxfd = fd;
	return 1;
}

/*
 *  this assumes that the fgrp is locked
 */
int
findfreefd(Fgrp *f, int start)
{
	int fd;

	for(fd=start; fd<f->nfd; fd++)
		if(f->fd[fd] == 0)
			break;
	if(fd >= f->nfd && growfd(f, fd) < 0)
		return -1;
	return fd;
}

int
newfd(Chan *c)
{
	int fd;
	Fgrp *f;

	f = up->fgrp;
	lock(f);
	fd = findfreefd(f, 0);
	if(fd < 0){
		unlock(f);
		return -1;
	}
	if(fd > f->maxfd)
		f->maxfd = fd;
	f->fd[fd] = c;
	unlock(f);
	return fd;
}

int
newfd2(int fd[2], Chan *c[2])
{
	Fgrp *f;

	f = up->fgrp;
	lock(f);
	fd[0] = findfreefd(f, 0);
	if(fd[0] < 0){
		unlock(f);
		return -1;
	}
	fd[1] = findfreefd(f, fd[0]+1);
	if(fd[1] < 0){
		unlock(f);
		return -1;
	}
	if(fd[1] > f->maxfd)
		f->maxfd = fd[1];
	f->fd[fd[0]] = c[0];
	f->fd[fd[1]] = c[1];
	unlock(f);

	return 0;
}

Chan*
fdtochan(int fd, int mode, int chkmnt, int iref)
{
	Chan *c;
	Fgrp *f;

	c = 0;
	f = up->fgrp;

	lock(f);
	if(fd<0 || f->nfd<=fd || (c = f->fd[fd])==0) {
		unlock(f);
		error(Ebadfd);
	}
	if(iref)
		incref(c);
	unlock(f);

	if(chkmnt && (c->flag&CMSG)) {
		if(iref)
			cclose(c);
		error(Ebadusefd);
	}

	if(mode<0 || c->mode==ORDWR)
		return c;

	if((mode&OTRUNC) && c->mode==OREAD) {
		if(iref)
			cclose(c);
		error(Ebadusefd);
	}

	if((mode&~OTRUNC) != c->mode) {
		if(iref)
			cclose(c);
		error(Ebadusefd);
	}

	return c;
}

int
openmode(ulong o)
{
	o &= ~(OTRUNC|OCEXEC|ORCLOSE);
	if(o > OEXEC)
		error(Ebadarg);
	if(o == OEXEC)
		return OREAD;
	return o;
}

long
sysfd2path(ulong *arg)
{
	Chan *c;

	validaddr(arg[1], arg[2], 1);

	c = fdtochan(arg[0], -1, 0, 1);

	if(c->name == nil)
		snprint((char*)arg[1], arg[2], "<null>");
	else
		snprint((char*)arg[1], arg[2], "%s", c->name->s);
	cclose(c);
	return 0;
}

long
syspipe(ulong *arg)
{
	int fd[2];
	Chan *c[2];
	Dev *d;
	static char *datastr[] = {"data", "data1"};

	validaddr(arg[0], 2*BY2WD, 1);
	evenaddr(arg[0]);
	d = devtab[devno('|', 0)];
	c[0] = namec("#|", Atodir, 0, 0);
	c[1] = 0;
	fd[0] = -1;
	fd[1] = -1;

	if(waserror()){
		cclose(c[0]);
		if(c[1])
			cclose(c[1]);
		nexterror();
	}
	c[1] = cclone(c[0]);
	if(walk(&c[0], datastr+0, 1, 1, nil) < 0)
		error(Egreg);
	if(walk(&c[1], datastr+1, 1, 1, nil) < 0)
		error(Egreg);
	c[0] = d->open(c[0], ORDWR);
	c[1] = d->open(c[1], ORDWR);
	if(newfd2(fd, c) < 0)
		error(Enofd);
	poperror();

	((long*)arg[0])[0] = fd[0];
	((long*)arg[0])[1] = fd[1];
	return 0;
}

long
sysdup(ulong *arg)
{
	int fd;
	Chan *c, *oc;
	Fgrp *f = up->fgrp;

	/*
	 * Close after dup'ing, so date > #d/1 works
	 */
	c = fdtochan(arg[0], -1, 0, 1);
	fd = arg[1];
	if(fd != -1){
		lock(f);
		if(fd<0 || growfd(f, fd)<0) {
			unlock(f);
			cclose(c);
			error(Ebadfd);
		}
		if(fd > f->maxfd)
			f->maxfd = fd;

		oc = f->fd[fd];
		f->fd[fd] = c;
		unlock(f);
		if(oc)
			cclose(oc);
	}else{
		if(waserror()) {
			cclose(c);
			nexterror();
		}
		fd = newfd(c);
		if(fd < 0)
			error(Enofd);
		poperror();
	}

	return fd;
}

long
sysopen(ulong *arg)
{
	int fd;
	Chan *c = 0;

	openmode(arg[1]);	/* error check only */
	if(waserror()){
		if(c)
			cclose(c);
		nexterror();
	}
	validaddr(arg[0], 1, 0);
	c = namec((char*)arg[0], Aopen, arg[1], 0);
	fd = newfd(c);
	if(fd < 0)
		error(Enofd);
	poperror();
	return fd;
}

void
fdclose(int fd, int flag)
{
	int i;
	Chan *c;
	Fgrp *f = up->fgrp;

	lock(f);
	c = f->fd[fd];
	if(c == 0){
		/* can happen for users with shared fd tables */
		unlock(f);
		return;
	}
	if(flag){
		if(c==0 || !(c->flag&flag)){
			unlock(f);
			return;
		}
	}
	f->fd[fd] = 0;
	if(fd == f->maxfd)
		for(i=fd; --i>=0 && f->fd[i]==0; )
			f->maxfd = i;

	unlock(f);
	cclose(c);
}

long
sysclose(ulong *arg)
{
	fdtochan(arg[0], -1, 0, 0);
	fdclose(arg[0], 0);

	return 0;
}

long
unionread(Chan *c, void *va, long n)
{
	int i;
	long nr;
	Mhead *m;
	Mount *mount;

	qlock(&c->umqlock);
	m = c->umh;
	rlock(&m->lock);
	mount = m->mount;
	/* bring mount in sync with c->uri and c->umc */
	for(i = 0; mount != nil && i < c->uri; i++)
		mount = mount->next;

	nr = 0;
	while(mount != nil) {
		/* Error causes component of union to be skipped */
		if(mount->to && !waserror()) {
			if(c->umc == nil){
				c->umc = cclone(mount->to);
				c->umc = devtab[c->umc->type]->open(c->umc, OREAD);
			}
	
			nr = devtab[c->umc->type]->read(c->umc, va, n, c->umc->offset);
			c->umc->offset += nr;
			poperror();
		}
		if(nr > 0)
			break;

		/* Advance to next element */
		c->uri++;
		if(c->umc) {
			cclose(c->umc);
			c->umc = nil;
		}
		mount = mount->next;
	}
	runlock(&m->lock);
	qunlock(&c->umqlock);
	return nr;
}

static long
read(ulong *arg, vlong *offp)
{
	int dir;
	long n;
	Chan *c;
	vlong off;

	n = arg[2];
	validaddr(arg[1], n, 1);
	c = fdtochan(arg[0], OREAD, 1, 1);

	if(waserror()) {
		cclose(c);
		nexterror();
	}

	dir = c->qid.type&QTDIR;
	/*
	 * The offset is passed through on directories, normally. sysseek complains but
	 * pread is used by servers and e.g. exportfs that shouldn't need to worry about this issue.
	 */

	if(offp == nil)	/* use and maintain channel's offset */
		off = c->offset;
	else
		off = *offp;

	if(off < 0)
		error(Enegoff);

	if(dir && c->umh)
		n = unionread(c, (void*)arg[1], n);
	else
		n = devtab[c->type]->read(c, (void*)arg[1], n, off);

	if(offp == nil){
		lock(c);
		c->offset += n;
		unlock(c);
	}

	poperror();
	cclose(c);

	return n;
}

long
sys_read(ulong *arg)
{
	return read(arg, nil);
}

long
syspread(ulong *arg)
{
	vlong v;
	va_list list;

	/* use varargs to guarantee alignment of vlong */
	va_start(list, arg[2]);
	v = va_arg(list, vlong);
	va_end(list);

	if(v == ~0ULL)
		return read(arg, nil);

	return read(arg, &v);
}

static long
write(ulong *arg, vlong *offp)
{
	Chan *c;
	long m, n;
	vlong off;

	validaddr(arg[1], arg[2], 0);
	n = 0;
	c = fdtochan(arg[0], OWRITE, 1, 1);
	if(waserror()) {
		if(offp == nil){
			lock(c);
			c->offset -= n;
			unlock(c);
		}
		cclose(c);
		nexterror();
	}

	if(c->qid.type & QTDIR)
		error(Eisdir);

	n = arg[2];

	if(offp == nil){	/* use and maintain channel's offset */
		lock(c);
		off = c->offset;
		c->offset += n;
		unlock(c);
	}else
		off = *offp;

	if(off < 0)
		error(Enegoff);

	m = devtab[c->type]->write(c, (void*)arg[1], n, off);

	if(offp == nil && m < n){
		lock(c);
		c->offset -= n - m;
		unlock(c);
	}

	poperror();
	cclose(c);

	return m;
}

long
sys_write(ulong *arg)
{
	return write(arg, nil);
}

long
syspwrite(ulong *arg)
{
	vlong v;
	va_list list;

	/* use varargs to guarantee alignment of vlong */
	va_start(list, arg[2]);
	v = va_arg(list, vlong);
	va_end(list);

	if(v == ~0ULL)
		return write(arg, nil);

	return write(arg, &v);
}

static void
sseek(ulong *arg)
{
	Chan *c;
	uchar buf[sizeof(Dir)+100];
	Dir dir;
	int n;
	vlong off;
	union {
		vlong v;
		ulong u[2];
	} o;

	c = fdtochan(arg[1], -1, 1, 1);
	if(waserror()){
		cclose(c);
		nexterror();
	}
	if(devtab[c->type]->dc == '|')
		error(Eisstream);

	off = 0;
	o.u[0] = arg[2];
	o.u[1] = arg[3];
	switch(arg[4]){
	case 0:
		off = o.v;
		if((c->qid.type & QTDIR) && off != 0)
			error(Eisdir);
		if(off < 0)
			error(Enegoff);
		c->offset = off;
		break;

	case 1:
		if(c->qid.type & QTDIR)
			error(Eisdir);
		lock(c);	/* lock for read/write update */
		off = o.v + c->offset;
		if(off < 0)
			error(Enegoff);
		c->offset = off;
		unlock(c);
		break;

	case 2:
		if(c->qid.type & QTDIR)
			error(Eisdir);
		n = devtab[c->type]->stat(c, buf, sizeof buf);
		if(convM2D(buf, n, &dir, nil) == 0)
			error("internal error: stat error in seek");
		off = dir.length + o.v;
		if(off < 0)
			error(Enegoff);
		c->offset = off;
		break;

	default:
		error(Ebadarg);
	}
	*(vlong*)arg[0] = off;
	c->uri = 0;
	c->dri = 0;
	cclose(c);
	poperror();
}

long
sysseek(ulong *arg)
{
	validaddr(arg[0], BY2V, 1);
	sseek(arg);
	return 0;
}

long
sysoseek(ulong *arg)
{
	union {
		vlong v;
		ulong u[2];
	} o;
	ulong a[5];

	o.v = (long)arg[1];
	a[0] = (ulong)&o.v;
	a[1] = arg[0];
	a[2] = o.u[0];
	a[3] = o.u[1];
	a[4] = arg[2];
	sseek(a);
	return o.v;
}

void
validstat(uchar *s, int n)
{
	int m;
	char buf[64];

	if(statcheck(s, n) < 0)
		error(Ebadstat);
	/* verify that name entry is acceptable */
	s += STATFIXLEN - 4*BIT16SZ;	/* location of first string */
	/*
	 * s now points at count for first string.
	 * if it's too long, let the server decide; this is
	 * only for his protection anyway. otherwise
	 * we'd have to allocate and waserror.
	 */
	m = GBIT16(s);
	s += BIT16SZ;
	if(m+1 > sizeof buf)
		return;
	memmove(buf, s, m);
	buf[m] = '\0';
	/* name could be '/' */
	if(strcmp(buf, "/") != 0)
		validname(buf, 0);
}

long
sysfstat(ulong *arg)
{
	Chan *c;
	uint l;

	l = arg[2];
	validaddr(arg[1], l, 1);
	c = fdtochan(arg[0], -1, 0, 1);
	if(waserror()) {
		cclose(c);
		nexterror();
	}
	l = devtab[c->type]->stat(c, (uchar*)arg[1], l);
	poperror();
	cclose(c);
	return l;
}

long
sysstat(ulong *arg)
{
	Chan *c;
	uint l;

	l = arg[2];
	validaddr(arg[1], l, 1);
	validaddr(arg[0], 1, 0);
	c = namec((char*)arg[0], Aaccess, 0, 0);
	if(waserror()){
		cclose(c);
		nexterror();
	}
	l = devtab[c->type]->stat(c, (uchar*)arg[1], l);
	poperror();
	cclose(c);
	return l;
}

long
syschdir(ulong *arg)
{
	Chan *c;

	validaddr(arg[0], 1, 0);

	c = namec((char*)arg[0], Atodir, 0, 0);
	cclose(up->dot);
	up->dot = c;
	return 0;
}

long
bindmount(int ismount, int fd, int afd, char* arg0, char* arg1, ulong flag, char* spec)
{
	int ret;
	Chan *c0, *c1, *ac, *bc;
	struct{
		Chan	*chan;
		Chan	*authchan;
		char	*spec;
		int	flags;
	}bogus;

	if((flag&~MMASK) || (flag&MORDER)==(MBEFORE|MAFTER))
		error(Ebadarg);

	bogus.flags = flag & MCACHE;

	if(ismount){
		if(up->pgrp->noattach)
			error(Enoattach);

		ac = nil;
		bc = fdtochan(fd, ORDWR, 0, 1);
		if(waserror()) {
			if(ac)
				cclose(ac);
			cclose(bc);
			nexterror();
		}

		if(afd >= 0)
			ac = fdtochan(afd, ORDWR, 0, 1);

		bogus.chan = bc;
		bogus.authchan = ac;

		validaddr((ulong)spec, 1, 0);
		bogus.spec = spec;
		if(waserror())
			error(Ebadspec);
		validname(spec, 1);
		poperror();

		ret = devno('M', 0);
		c0 = devtab[ret]->attach((char*)&bogus);

		poperror();
		if(ac)
			cclose(ac);
		cclose(bc);
	}else{
		bogus.spec = 0;
		validaddr((ulong)arg0, 1, 0);
		c0 = namec(arg0, Abind, 0, 0);
	}

	if(waserror()){
		cclose(c0);
		nexterror();
	}

	validaddr((ulong)arg1, 1, 0);
	c1 = namec(arg1, Amount, 0, 0);
	if(waserror()){
		cclose(c1);
		nexterror();
	}

	ret = cmount(&c0, c1, flag, bogus.spec);

	poperror();
	cclose(c1);
	poperror();
	cclose(c0);
	if(ismount)
		fdclose(fd, 0);

	return ret;
}

long
sysbind(ulong *arg)
{
	return bindmount(0, -1, -1, (char*)arg[0], (char*)arg[1], arg[2], nil);
}

long
sysmount(ulong *arg)
{
	return bindmount(1, arg[0], arg[1], nil, (char*)arg[2], arg[3], (char*)arg[4]);
}

long
sys_mount(ulong *arg)
{
	return bindmount(1, arg[0], -1, nil, (char*)arg[1], arg[2], (char*)arg[3]);
}

long
sysunmount(ulong *arg)
{
	Chan *cmount, *cmounted;

	cmounted = 0;

	validaddr(arg[1], 1, 0);
	cmount = namec((char *)arg[1], Amount, 0, 0);

	if(arg[0]) {
		if(waserror()) {
			cclose(cmount);
			nexterror();
		}
		validaddr(arg[0], 1, 0);
		/*
		 * This has to be namec(..., Aopen, ...) because
		 * if arg[0] is something like /srv/cs or /fd/0,
		 * opening it is the only way to get at the real
		 * Chan underneath.
		 */
		cmounted = namec((char*)arg[0], Aopen, OREAD, 0);
		poperror();
	}

	if(waserror()) {
		cclose(cmount);
		if(cmounted)
			cclose(cmounted);
		nexterror();
	}

	cunmount(cmount, cmounted);
	cclose(cmount);
	if(cmounted)
		cclose(cmounted);
	poperror();
	return 0;
}

long
syscreate(ulong *arg)
{
	int fd;
	Chan *c = 0;

	openmode(arg[1]&~OEXCL);	/* error check only; OEXCL okay here */
	if(waserror()) {
		if(c)
			cclose(c);
		nexterror();
	}
	validaddr(arg[0], 1, 0);
	c = namec((char*)arg[0], Acreate, arg[1], arg[2]);
	fd = newfd(c);
	if(fd < 0)
		error(Enofd);
	poperror();
	return fd;
}

long
sysremove(ulong *arg)
{
	Chan *c;

	validaddr(arg[0], 1, 0);
	c = namec((char*)arg[0], Aremove, 0, 0);
	if(waserror()){
		c->type = 0;	/* see below */
		cclose(c);
		nexterror();
	}
	devtab[c->type]->remove(c);
	/*
	 * Remove clunks the fid, but we need to recover the Chan
	 * so fake it up.  rootclose() is known to be a nop.
	 */
	c->type = 0;
	poperror();
	cclose(c);
	return 0;
}

long
syswstat(ulong *arg)
{
	Chan *c;
	uint l;

	l = arg[2];
	validaddr(arg[1], l, 0);
	validstat((uchar*)arg[1], l);
	validaddr(arg[0], 1, 0);
	c = namec((char*)arg[0], Aaccess, 0, 0);
	if(waserror()){
		cclose(c);
		nexterror();
	}
	l = devtab[c->type]->wstat(c, (uchar*)arg[1], l);
	poperror();
	cclose(c);
	return l;
}

long
sysfwstat(ulong *arg)
{
	Chan *c;
	uint l;

	l = arg[2];
	validaddr(arg[1], l, 0);
	validstat((uchar*)arg[1], l);
	c = fdtochan(arg[0], -1, 1, 1);
	if(waserror()) {
		cclose(c);
		nexterror();
	}
	l = devtab[c->type]->wstat(c, (uchar*)arg[1], l);
	poperror();
	cclose(c);
	return l;
}

static void
packoldstat(uchar *buf, Dir *d)
{
	uchar *p;
	ulong q;

	/* lay down old stat buffer - grotty code but it's temporary */
	p = buf;
	strncpy((char*)p, d->name, 28);
	p += 28;
	strncpy((char*)p, d->uid, 28);
	p += 28;
	strncpy((char*)p, d->gid, 28);
	p += 28;
	q = d->qid.path & ~DMDIR;	/* make sure doesn't accidentally look like directory */
	if(d->qid.type & QTDIR)	/* this is the real test of a new directory */
		q |= DMDIR;
	PBIT32(p, q);
	p += BIT32SZ;
	PBIT32(p, d->qid.vers);
	p += BIT32SZ;
	PBIT32(p, d->mode);
	p += BIT32SZ;
	PBIT32(p, d->atime);
	p += BIT32SZ;
	PBIT32(p, d->mtime);
	p += BIT32SZ;
	PBIT64(p, d->length);
	p += BIT64SZ;
	PBIT16(p, d->type);
	p += BIT16SZ;
	PBIT16(p, d->dev);
}

long
sys_stat(ulong *arg)
{
	Chan *c;
	uint l;
	uchar buf[128];	/* old DIRLEN plus a little should be plenty */
	char strs[128];
	Dir d;
	char old[] = "old stat system call - recompile";

	validaddr(arg[1], 116, 1);
	validaddr(arg[0], 1, 0);
	c = namec((char*)arg[0], Aaccess, 0, 0);
	if(waserror()){
		cclose(c);
		nexterror();
	}
	l = devtab[c->type]->stat(c, buf, sizeof buf);
	/* buf contains a new stat buf; convert to old. yuck. */
	if(l <= BIT16SZ)	/* buffer too small; time to face reality */
		error(old);
	l = convM2D(buf, l, &d, strs);
	if(l == 0)
		error(old);
	packoldstat((uchar*)arg[1], &d);
	
	poperror();
	cclose(c);
	return 0;
}

long
sys_fstat(ulong *arg)
{
	Chan *c;
	uint l;
	uchar buf[128];	/* old DIRLEN plus a little should be plenty */
	char strs[128];
	Dir d;
	char old[] = "old fstat system call - recompile";

	validaddr(arg[1], 116, 1);
	c = fdtochan(arg[0], -1, 0, 1);
	if(waserror()){
		cclose(c);
		nexterror();
	}
	l = devtab[c->type]->stat(c, buf, sizeof buf);
	/* buf contains a new stat buf; convert to old. yuck. */
	if(l <= BIT16SZ)	/* buffer too small; time to face reality */
		error(old);
	l = convM2D(buf, l, &d, strs);
	if(l == 0)
		error(old);
	packoldstat((uchar*)arg[1], &d);
	
	poperror();
	cclose(c);
	return 0;
}

long
sys_wstat(ulong *)
{
	error("old wstat system call - recompile");
	return -1;
}

long
sys_fwstat(ulong *)
{
	error("old fwstat system call - recompile");
	return -1;
}
