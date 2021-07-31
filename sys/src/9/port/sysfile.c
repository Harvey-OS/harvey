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
	c[1] = cclone(c[0], 0);
	if(walk(&c[0], "data", 1) < 0)
		error(Egreg);
	if(walk(&c[1], "data1", 1) < 0)
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
	Chan *nc;
	Mhead *m;
	Mount *mount;

	m = c->mh;
	rlock(&m->lock);
	mount = m->mount;
	for(i = 0; mount != nil && i < c->uri; i++)
		mount = mount->next;

	while(mount != nil) {
		if(waserror()) {
			runlock(&m->lock);
			nexterror();
		}
		if(mount->to == nil)
			goto next;
		nc = cclone(mount->to, 0);
		poperror();

		/* Error causes component of union to be skipped */
		if(waserror()) {
			cclose(nc);
			goto next;
		}

		nc = devtab[nc->type]->open(nc, OREAD);
		nc->offset = c->offset;
		nr = devtab[nc->type]->read(nc, va, n, nc->offset);
		/* devdirread e.g. changes it */
		c->offset = nc->offset;
		poperror();

		cclose(nc);
		if(nr > 0) {
			runlock(&m->lock);
			return nr;
		}
		/* Advance to next element */
	next:
		c->uri++;
		mount = mount->next;
		if(mount == nil)
			break;
		c->offset = 0;
	}
	runlock(&m->lock);
	return 0;
}

long
sysread9p(ulong *arg)
{
	int dir;
	long n;
	Chan *c;

	validaddr(arg[1], arg[2], 1);
	c = fdtochan(arg[0], OREAD, 1, 1);
	if(waserror()) {
		cclose(c);
		nexterror();
	}

	n = arg[2];
	dir = c->qid.path&CHDIR;

	if(dir) {
		n -= n%DIRLEN;
		if(c->offset%DIRLEN || n==0)
			error(Etoosmall);
	}

	if(dir && c->mh)
		n = unionread(c, (void*)arg[1], n);
	else if(devtab[c->type]->dc != L'M')
		n = devtab[c->type]->read(c, (void*)arg[1], n, c->offset);
	else
		n = mntread9p(c, (void*)arg[1], n, c->offset);

	lock(c);
	c->offset += n;
	unlock(c);

	poperror();
	cclose(c);

	return n;
}

long
sysread(ulong *arg)
{
	int dir;
	long n;
	Chan *c;

	n = arg[2];
	validaddr(arg[1], n, 1);
	c = fdtochan(arg[0], OREAD, 1, 1);

	if(waserror()) {
		cclose(c);
		nexterror();
	}

	dir = c->qid.path&CHDIR;

	if(dir) {
		n -= n%DIRLEN;
		if(c->offset%DIRLEN || n==0)
			error(Etoosmall);
	}

	if(dir && c->mh)
		n = unionread(c, (void*)arg[1], n);
	else
		n = devtab[c->type]->read(c, (void*)arg[1], n, c->offset);

	lock(c);
	c->offset += n;
	unlock(c);

	poperror();
	cclose(c);

	return n;
}

long
syspread(ulong *arg)
{
	long n;
	Chan *c;
	union {
		uvlong v;
		ulong u[2];
	} o;

	n = arg[2];
	validaddr(arg[1], n, 1);
	c = fdtochan(arg[0], OREAD, 1, 1);
	o.u[0] = arg[3];
	o.u[1] = arg[4];

	if(waserror()) {
		cclose(c);
		nexterror();
	}

	if(c->qid.path&CHDIR)
		error(Eisdir);

	n = devtab[c->type]->read(c, (void*)arg[1], n, o.v);

	poperror();
	cclose(c);

	return n;
}

long
syswrite9p(ulong *arg)
{
	Chan *c;
	long n;

	validaddr(arg[1], arg[2], 0);
	c = fdtochan(arg[0], OWRITE, 1, 1);
	if(waserror()) {
		cclose(c);
		nexterror();
	}

	if(c->qid.path & CHDIR)
		error(Eisdir);

	if(devtab[c->type]->dc != L'M')
		n = devtab[c->type]->write(c, (void*)arg[1], arg[2], c->offset);
	else
		n = mntwrite9p(c, (void*)arg[1], arg[2], c->offset);
	lock(c);
	c->offset += n;
	unlock(c);

	poperror();
	cclose(c);

	return n;
}

long
syswrite(ulong *arg)
{
	Chan *c;
	long m, n;
	uvlong oo;

	validaddr(arg[1], arg[2], 0);
	n = arg[2];
	c = fdtochan(arg[0], OWRITE, 1, 1);
	if(waserror()) {
		cclose(c);
		lock(c);
		c->offset -= n;
		unlock(c);
		nexterror();
	}

	if(c->qid.path & CHDIR)
		error(Eisdir);

	lock(c);
	oo = c->offset;
	c->offset += n;
	unlock(c);

	m = devtab[c->type]->write(c, (void*)arg[1], n, oo);

	if(m < n){
		lock(c);
		c->offset -= n - m;
		unlock(c);
	}

	poperror();
	cclose(c);

	return m;
}

long
syspwrite(ulong *arg)
{
	Chan *c;
	long m, n;
	union {
		uvlong v;
		ulong u[2];
	} o;

	validaddr(arg[1], arg[2], 0);
	n = arg[2];
	c = fdtochan(arg[0], OWRITE, 1, 1);
	o.u[0] = arg[3];
	o.u[1] = arg[4];
	if(waserror()) {
		cclose(c);
		nexterror();
	}

	if(c->qid.path & CHDIR)
		error(Eisdir);

	m = devtab[c->type]->write(c, (void*)arg[1], n, o.v);

	poperror();
	cclose(c);

	return m;
}

static void
sseek(ulong *arg)
{
	Chan *c;
	char buf[DIRLEN];
	Dir dir;
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
	if(c->qid.path & CHDIR)
		error(Eisdir);

	if(devtab[c->type]->dc == '|')
		error(Eisstream);

	off = 0;
	o.u[0] = arg[2];
	o.u[1] = arg[3];
	switch(arg[4]){
	case 0:
		off = o.v;
		c->offset = off;
		break;

	case 1:
		lock(c);	/* lock for read/write update */
		off = o.v + c->offset;
		c->offset = off;
		unlock(c);
		break;

	case 2:
		devtab[c->type]->stat(c, buf);
		convM2D(buf, &dir);
		off = dir.length + o.v;
		c->offset = off;
		break;
	}
	*(vlong*)arg[0] = off;
	c->uri = 0;
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

long
sysfstat(ulong *arg)
{
	Chan *c;

	validaddr(arg[1], DIRLEN, 1);
	evenaddr(arg[1]);
	c = fdtochan(arg[0], -1, 0, 1);
	if(waserror()) {
		cclose(c);
		nexterror();
	}
	devtab[c->type]->stat(c, (char*)arg[1]);
	poperror();
	cclose(c);
	return 0;
}

long
sysstat(ulong *arg)
{
	Chan *c;

	validaddr(arg[1], DIRLEN, 1);
	evenaddr(arg[1]);
	validaddr(arg[0], 1, 0);
	c = namec((char*)arg[0], Aaccess, 0, 0);
	if(waserror()){
		cclose(c);
		nexterror();
	}
	devtab[c->type]->stat(c, (char*)arg[1]);
	poperror();
	cclose(c);
	return 0;
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
bindmount(ulong *arg, int ismount)
{
	ulong flag;
	int fd, ret;
	Chan *c0, *c1, *bc;
	struct{
		Chan	*chan;
		char	*spec;
		int	flags;
	}bogus;

	flag = arg[2];
	fd = arg[0];
	if(flag>MMASK || (flag&MORDER)==(MBEFORE|MAFTER))
		error(Ebadarg);

	bogus.flags = flag & MCACHE;

	if(ismount){
		if(up->pgrp->noattach)
			error(Enoattach);

		bc = fdtochan(fd, ORDWR, 0, 1);
		if(waserror()) {
			cclose(bc);
			nexterror();
		}
		bogus.chan = bc;

		validaddr(arg[3], 1, 0);
		if(vmemchr((char*)arg[3], '\0', NAMELEN) == 0)
			error(Ebadarg);

		bogus.spec = (char*)arg[3];
		if(waserror())
			error(Ebadspec);
		nameok(bogus.spec, 1);
		poperror();

		ret = devno('M', 0);
		c0 = devtab[ret]->attach((char*)&bogus);

		poperror();
		cclose(bc);
	}
	else {
		bogus.spec = 0;
		validaddr(arg[0], 1, 0);
		c0 = namec((char*)arg[0], Aaccess, 0, 0);
	}

	if(waserror()){
		cclose(c0);
		nexterror();
	}

	validaddr(arg[1], 1, 0);
	c1 = namec((char*)arg[1], Amount, 0, 0);
	if(waserror()){
		cclose(c1);
		nexterror();
	}

	ret = cmount(c0, c1, flag, bogus.spec);

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
	return bindmount(arg, 0);
}

long
sysmount(ulong *arg)
{
	return bindmount(arg, 1);
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

	openmode(arg[1]);	/* error check only */
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
	c = namec((char*)arg[0], Aaccess, 0, 0);
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

	validaddr(arg[1], DIRLEN, 0);
	nameok((char*)arg[1], 0);
	validaddr(arg[0], 1, 0);
	c = namec((char*)arg[0], Aaccess, 0, 0);
	if(waserror()){
		cclose(c);
		nexterror();
	}
	devtab[c->type]->wstat(c, (char*)arg[1]);
	poperror();
	cclose(c);
	return 0;
}

long
sysfwstat(ulong *arg)
{
	Chan *c;

	validaddr(arg[1], DIRLEN, 0);
	nameok((char*)arg[1], 0);
	c = fdtochan(arg[0], -1, 1, 1);
	if(waserror()) {
		cclose(c);
		nexterror();
	}
	devtab[c->type]->wstat(c, (char*)arg[1]);
	poperror();
	cclose(c);
	return 0;
}
