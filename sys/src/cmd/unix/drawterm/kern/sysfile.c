#include	"u.h"
#include	"lib.h"
#include	"dat.h"
#include	"fns.h"
#include	"error.h"

#include	"user.h"
#undef open
#undef mount
#undef read
#undef write
#undef seek
#undef stat
#undef wstat
#undef remove
#undef close
#undef fstat
#undef fwstat

/*
 * The sys*() routines needn't poperror() as they return directly to syscall().
 */

static void
unlockfgrp(Fgrp *f)
{
	int ex;

	ex = f->exceed;
	f->exceed = 0;
	unlock(&f->ref.lk);
	if(ex)
		pprint("warning: process exceeds %d file descriptors\n", ex);
}

int
growfd(Fgrp *f, int fd)	/* fd is always >= 0 */
{
	Chan **newfd, **oldfd;

	if(fd < f->nfd)
		return 0;
	if(fd >= f->nfd+DELTAFD)
		return -1;	/* out of range */
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
	if(fd > f->maxfd){
		if(fd/100 > f->maxfd/100)
			f->exceed = (fd/100)*100;
		f->maxfd = fd;
	}
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
	lock(&f->ref.lk);
	fd = findfreefd(f, 0);
	if(fd < 0){
		unlockfgrp(f);
		return -1;
	}
	if(fd > f->maxfd)
		f->maxfd = fd;
	f->fd[fd] = c;
	unlockfgrp(f);
	return fd;
}

int
newfd2(int fd[2], Chan *c[2])
{
	Fgrp *f;

	f = up->fgrp;
	lock(&f->ref.lk);
	fd[0] = findfreefd(f, 0);
	if(fd[0] < 0){
		unlockfgrp(f);
		return -1;
	}
	fd[1] = findfreefd(f, fd[0]+1);
	if(fd[1] < 0){
		unlockfgrp(f);
		return -1;
	}
	if(fd[1] > f->maxfd)
		f->maxfd = fd[1];
	f->fd[fd[0]] = c[0];
	f->fd[fd[1]] = c[1];
	unlockfgrp(f);

	return 0;
}

Chan*
fdtochan(int fd, int mode, int chkmnt, int iref)
{
	Chan *c;
	Fgrp *f;

	c = 0;
	f = up->fgrp;

	lock(&f->ref.lk);
	if(fd<0 || f->nfd<=fd || (c = f->fd[fd])==0) {
		unlock(&f->ref.lk);
		error(Ebadfd);
	}
	if(iref)
		incref(&c->ref);
	unlock(&f->ref.lk);

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
_sysfd2path(int fd, char *buf, uint nbuf)
{
	Chan *c;

	c = fdtochan(fd, -1, 0, 1);

	if(c->name == nil)
		snprint(buf, nbuf, "<null>");
	else
		snprint(buf, nbuf, "%s", c->name->s);
	cclose(c);
	return 0;
}

long
_syspipe(int fd[2])
{
	Chan *c[2];
	Dev *d;
	static char *datastr[] = {"data", "data1"};

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

	return 0;
}

long
_sysdup(int fd0, int fd1)
{
	int fd;
	Chan *c, *oc;
	Fgrp *f = up->fgrp;

	/*
	 * Close after dup'ing, so date > #d/1 works
	 */
	c = fdtochan(fd0, -1, 0, 1);
	fd = fd1;
	if(fd != -1){
		lock(&f->ref.lk);
		if(fd<0 || growfd(f, fd)<0) {
			unlockfgrp(f);
			cclose(c);
			error(Ebadfd);
		}
		if(fd > f->maxfd)
			f->maxfd = fd;

		oc = f->fd[fd];
		f->fd[fd] = c;
		unlockfgrp(f);
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
_sysopen(char *name, int mode)
{
	int fd;
	Chan *c = 0;

	openmode(mode);	/* error check only */
	if(waserror()){
		if(c)
			cclose(c);
		nexterror();
	}
	c = namec(name, Aopen, mode, 0);
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

	lock(&f->ref.lk);
	c = f->fd[fd];
	if(c == 0){
		/* can happen for users with shared fd tables */
		unlock(&f->ref.lk);
		return;
	}
	if(flag){
		if(c==0 || !(c->flag&flag)){
			unlock(&f->ref.lk);
			return;
		}
	}
	f->fd[fd] = 0;
	if(fd == f->maxfd)
		for(i=fd; --i>=0 && f->fd[i]==0; )
			f->maxfd = i;

	unlock(&f->ref.lk);
	cclose(c);
}

long
_sysclose(int fd)
{
	fdtochan(fd, -1, 0, 0);
	fdclose(fd, 0);

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
kread(int fd, void *buf, long n, vlong *offp)
{
	int dir;
	Chan *c;
	vlong off;

	c = fdtochan(fd, OREAD, 1, 1);

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
		n = unionread(c, buf, n);
	else
		n = devtab[c->type]->read(c, buf, n, off);

	if(offp == nil){
		lock(&c->ref.lk);
		c->offset += n;
		unlock(&c->ref.lk);
	}

	poperror();
	cclose(c);

	return n;
}

/* name conflicts with netbsd
long
_sys_read(int fd, void *buf, long n)
{
	return kread(fd, buf, n, nil);
}
*/

long
_syspread(int fd, void *buf, long n, vlong off)
{
	if(off == ((uvlong) ~0))
		return kread(fd, buf, n, nil);
	return kread(fd, buf, n, &off);
}

static long
kwrite(int fd, void *buf, long nn, vlong *offp)
{
	Chan *c;
	long m, n;
	vlong off;

	n = 0;
	c = fdtochan(fd, OWRITE, 1, 1);
	if(waserror()) {
		if(offp == nil){
			lock(&c->ref.lk);
			c->offset -= n;
			unlock(&c->ref.lk);
		}
		cclose(c);
		nexterror();
	}

	if(c->qid.type & QTDIR)
		error(Eisdir);

	n = nn;

	if(offp == nil){	/* use and maintain channel's offset */
		lock(&c->ref.lk);
		off = c->offset;
		c->offset += n;
		unlock(&c->ref.lk);
	}else
		off = *offp;

	if(off < 0)
		error(Enegoff);

	m = devtab[c->type]->write(c, buf, n, off);

	if(offp == nil && m < n){
		lock(&c->ref.lk);
		c->offset -= n - m;
		unlock(&c->ref.lk);
	}

	poperror();
	cclose(c);

	return m;
}

long
sys_write(int fd, void *buf, long n)
{
	return kwrite(fd, buf, n, nil);
}

long
_syspwrite(int fd, void *buf, long n, vlong off)
{
	if(off == ((uvlong) ~0))
		return kwrite(fd, buf, n, nil);
	return kwrite(fd, buf, n, &off);
}

static vlong
_sysseek(int fd, vlong off, int whence)
{
	Chan *c;
	uchar buf[sizeof(Dir)+100];
	Dir dir;
	int n;

	c = fdtochan(fd, -1, 1, 1);
	if(waserror()){
		cclose(c);
		nexterror();
	}
	if(devtab[c->type]->dc == '|')
		error(Eisstream);

	switch(whence){
	case 0:
		if((c->qid.type & QTDIR) && off != 0)
			error(Eisdir);
		if(off < 0)
			error(Enegoff);
		c->offset = off;
		break;

	case 1:
		if(c->qid.type & QTDIR)
			error(Eisdir);
		lock(&c->ref.lk);	/* lock for read/write update */
		off = off + c->offset;
		if(off < 0)
			error(Enegoff);
		c->offset = off;
		unlock(&c->ref.lk);
		break;

	case 2:
		if(c->qid.type & QTDIR)
			error(Eisdir);
		n = devtab[c->type]->stat(c, buf, sizeof buf);
		if(convM2D(buf, n, &dir, nil) == 0)
			error("internal error: stat error in seek");
		off = dir.length + off;
		if(off < 0)
			error(Enegoff);
		c->offset = off;
		break;

	default:
		error(Ebadarg);
	}
	c->uri = 0;
	c->dri = 0;
	cclose(c);
	poperror();
	return off;
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
_sysfstat(int fd, void *buf, long n)
{
	Chan *c;
	uint l;

	l = n;
	validaddr(buf, l, 1);
	c = fdtochan(fd, -1, 0, 1);
	if(waserror()) {
		cclose(c);
		nexterror();
	}
	l = devtab[c->type]->stat(c, buf, l);
	poperror();
	cclose(c);
	return l;
}

long
_sysstat(char *name, void *buf, long n)
{
	Chan *c;
	uint l;

	l = n;
	validaddr(buf, l, 1);
	validaddr(name, 1, 0);
	c = namec(name, Aaccess, 0, 0);
	if(waserror()){
		cclose(c);
		nexterror();
	}
	l = devtab[c->type]->stat(c, buf, l);
	poperror();
	cclose(c);
	return l;
}

long
_syschdir(char *name)
{
	Chan *c;

	validaddr(name, 1, 0);

	c = namec(name, Atodir, 0, 0);
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
_sysbind(char *old, char *new, int flag)
{
	return bindmount(0, -1, -1, old, new, flag, nil);
}

long
_sysmount(int fd, int afd, char *new, int flag, char *spec)
{
	return bindmount(1, fd, afd, nil, new, flag, spec);
}

long
_sysunmount(char *old, char *new)
{
	Chan *cmount, *cmounted;

	cmounted = 0;

	cmount = namec(new, Amount, 0, 0);

	if(old) {
		if(waserror()) {
			cclose(cmount);
			nexterror();
		}
		validaddr(old, 1, 0);
		/*
		 * This has to be namec(..., Aopen, ...) because
		 * if arg[0] is something like /srv/cs or /fd/0,
		 * opening it is the only way to get at the real
		 * Chan underneath.
		 */
		cmounted = namec(old, Aopen, OREAD, 0);
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
_syscreate(char *name, int mode, ulong perm)
{
	int fd;
	Chan *c = 0;

	openmode(mode&~OEXCL);	/* error check only; OEXCL okay here */
	if(waserror()) {
		if(c)
			cclose(c);
		nexterror();
	}
	validaddr(name, 1, 0);
	c = namec(name, Acreate, mode, perm);
	fd = newfd(c);
	if(fd < 0)
		error(Enofd);
	poperror();
	return fd;
}

long
_sysremove(char *name)
{
	Chan *c;

	c = namec(name, Aremove, 0, 0);
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
_syswstat(char *name, void *buf, long n)
{
	Chan *c;
	uint l;

	l = n;
	validstat(buf, l);
	validaddr(name, 1, 0);
	c = namec(name, Aaccess, 0, 0);
	if(waserror()){
		cclose(c);
		nexterror();
	}
	l = devtab[c->type]->wstat(c, buf, l);
	poperror();
	cclose(c);
	return l;
}

long
_sysfwstat(int fd, void *buf, long n)
{
	Chan *c;
	uint l;

	l = n;
	validaddr(buf, l, 0);
	validstat(buf, l);
	c = fdtochan(fd, -1, 1, 1);
	if(waserror()) {
		cclose(c);
		nexterror();
	}
	l = devtab[c->type]->wstat(c, buf, l);
	poperror();
	cclose(c);
	return l;
}


static void
starterror(void)
{
	assert(up->nerrlab == 0);
}

static void
_syserror(void)
{
	char *p;

	p = up->syserrstr;
	up->syserrstr = up->errstr;
	up->errstr = p;
}

static void
enderror(void)
{
	assert(up->nerrlab == 1);
	poperror();
}

int
sysbind(char *old, char *new, int flag)
{
	int n;

	starterror();
	if(waserror()){
		_syserror();
		return -1;
	}
	n = _sysbind(old, new, flag);
	enderror();
	return n;
}

int
syschdir(char *path)
{
	int n;

	starterror();
	if(waserror()){
		_syserror();
		return -1;
	}
	n = _syschdir(path);
	enderror();
	return n;
}

int
sysclose(int fd)
{
	int n;

	starterror();
	if(waserror()){
		_syserror();
		return -1;
	}
	n = _sysclose(fd);
	enderror();
	return n;
}

int
syscreate(char *name, int mode, ulong perm)
{
	int n;

	starterror();
	if(waserror()){
		_syserror();
		return -1;
	}
	n = _syscreate(name, mode, perm);
	enderror();
	return n;
}

int
sysdup(int fd0, int fd1)
{
	int n;

	starterror();
	if(waserror()){
		_syserror();
		return -1;
	}
	n = _sysdup(fd0, fd1);
	enderror();
	return n;
}

int
sysfstat(int fd, uchar *buf, int n)
{
	starterror();
	if(waserror()){
		_syserror();
		return -1;
	}
	n = _sysfstat(fd, buf, n);
	enderror();
	return n;
}

int
sysfwstat(int fd, uchar *buf, int n)
{
	starterror();
	if(waserror()){
		_syserror();
		return -1;
	}
	n = _sysfwstat(fd, buf, n);
	enderror();
	return n;
}

int
sysmount(int fd, int afd, char *new, int flag, char *spec)
{
	int n;

	starterror();
	if(waserror()){
		_syserror();
		return -1;
	}
	n = _sysmount(fd, afd, new, flag, spec);
	enderror();
	return n;
}

int
sysunmount(char *old, char *new)
{
	int n;

	starterror();
	if(waserror()){
		_syserror();
		return -1;
	}
	n = _sysunmount(old, new);
	enderror();
	return n;
}

int
sysopen(char *name, int mode)
{
	int n;

	starterror();
	if(waserror()){
		_syserror();
		return -1;
	}
	n = _sysopen(name, mode);
	enderror();
	return n;
}

int
syspipe(int *fd)
{
	int n;

	starterror();
	if(waserror()){
		_syserror();
		return -1;
	}
	n = _syspipe(fd);
	enderror();
	return n;
}

long
syspread(int fd, void *buf, long n, vlong off)
{
	starterror();
	if(waserror()){
		_syserror();
		return -1;
	}
	n = _syspread(fd, buf, n, off);
	enderror();
	return n;
}

long
syspwrite(int fd, void *buf, long n, vlong off)
{
	starterror();
	if(waserror()){
		_syserror();
		return -1;
	}
	n = _syspwrite(fd, buf, n, off);
	enderror();
	return n;
}

long
sysread(int fd, void *buf, long n)
{
	starterror();
	if(waserror()){
		_syserror();
		return -1;
	}
	n = _syspread(fd, buf, n, (uvlong) ~0);
	enderror();
	return n;
}

int
sysremove(char *path)
{
	int n;

	starterror();
	if(waserror()){
		_syserror();
		return -1;
	}
	n = _sysremove(path);
	enderror();
	return n;
}

vlong
sysseek(int fd, vlong off, int whence)
{
	starterror();
	if(waserror()){
		_syserror();
		return -1;
	}
	off = _sysseek(fd, off, whence);
	enderror();
	return off;
}

int
sysstat(char *name, uchar *buf, int n)
{
	starterror();
	if(waserror()){
		_syserror();
		return -1;
	}
	n = _sysstat(name, buf, n);
	enderror();
	return n;
}

long
syswrite(int fd, void *buf, long n)
{
	starterror();
	if(waserror()){
		_syserror();
		return -1;
	}
	n = _syspwrite(fd, buf, n, (uvlong) ~0);
	enderror();
	return n;
}

int
syswstat(char *name, uchar *buf, int n)
{
	starterror();
	if(waserror()){
		_syserror();
		return -1;
	}
	n = _syswstat(name, buf, n);
	enderror();
	return n;
}

void
werrstr(char *f, ...)
{
	char buf[ERRMAX];
	va_list arg;

	va_start(arg, f);
	vsnprint(buf, sizeof buf, f, arg);
	va_end(arg);

	if(up->nerrlab)
		strecpy(up->errstr, up->errstr+ERRMAX, buf);
	else
		strecpy(up->syserrstr, up->syserrstr+ERRMAX, buf);
}

int
__errfmt(Fmt *fmt)
{
	if(up->nerrlab)
		return fmtstrcpy(fmt, up->errstr);
	else
		return fmtstrcpy(fmt, up->syserrstr);
}

int
errstr(char *buf, uint n)
{
	char tmp[ERRMAX];
	char *p;

	p = up->nerrlab ? up->errstr : up->syserrstr;
	memmove(tmp, p, ERRMAX);
	utfecpy(p, p+ERRMAX, buf);
	utfecpy(buf, buf+n, tmp);
	return strlen(buf);
}

int
rerrstr(char *buf, uint n)
{
	char *p;

	p = up->nerrlab ? up->errstr : up->syserrstr;
	utfecpy(buf, buf+n, p);
	return strlen(buf);
}

void*
_sysrendezvous(void* arg0, void* arg1)
{
	void *tag, *val;
	Proc *p, **l;

	tag = arg0;
	l = &REND(up->rgrp, (uintptr)tag);
	up->rendval = (void*)~0;

	lock(&up->rgrp->ref.lk);
	for(p = *l; p; p = p->rendhash) {
		if(p->rendtag == tag) {
			*l = p->rendhash;
			val = p->rendval;
			p->rendval = arg1;

			while(p->mach != 0)
				;
			procwakeup(p);
			unlock(&up->rgrp->ref.lk);
			return val;
		}
		l = &p->rendhash;
	}

	/* Going to sleep here */
	up->rendtag = tag;
	up->rendval = arg1;
	up->rendhash = *l;
	*l = up;
	up->state = Rendezvous;
	unlock(&up->rgrp->ref.lk);

	procsleep();

	return up->rendval;
}

void*
sysrendezvous(void *tag, void *val)
{
	void *n;

	starterror();
	if(waserror()){
		_syserror();
		return (void*)~0;
	}
	n = _sysrendezvous(tag, val);
	enderror();
	return n;
}
