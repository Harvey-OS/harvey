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
newfd(Chan *c)
{
	Fgrp *f = u->p->fgrp;
	int i;

	lock(f);
	for(i=0; i<NFD; i++)
		if(f->fd[i] == 0){
			if(i > f->maxfd)
				f->maxfd = i;
			f->fd[i] = c;
			unlock(f);
			return i;
		}
	unlock(f);
	exhausted("file descriptors");
	return 0;
}

Chan*
fdtochan(int fd, int mode, int chkmnt, int iref)
{
	Chan *c;
	Fgrp *f;

	c = 0;
	f = u->p->fgrp;

	lock(f);
	if(fd<0 || NFD<=fd || (c = f->fd[fd])==0) {
		unlock(f);
		error(Ebadfd);
	}
	if(iref)
		incref(c);
	unlock(f);

	if(chkmnt && (c->flag&CMSG)) {
		if(iref)
			close(c);
		error(Ebadusefd);
	}

	if(mode<0 || c->mode==ORDWR)
		return c;

	if((mode&OTRUNC) && c->mode==OREAD) {
		if(iref)
			close(c);
		error(Ebadusefd);
	}

	if((mode&~OTRUNC) != c->mode) {
		if(iref)
			close(c);
		error(Ebadusefd);
	}

	return c;
}

int
openmode(ulong o)
{
	if(o >= (OTRUNC|OCEXEC|ORCLOSE|OEXEC))
		error(Ebadarg);
	o &= ~(OTRUNC|OCEXEC|ORCLOSE);
	if(o > OEXEC)
		error(Ebadarg);
	if(o == OEXEC)
		return OREAD;
	return o;
}

long
syspipe(ulong *arg)
{
	int fd[2];
	Chan *c[2];
	Dev *d;
	Fgrp *f = u->p->fgrp;

	validaddr(arg[0], 2*BY2WD, 1);
	evenaddr(arg[0]);
	d = &devtab[devno('|', 0)];
	c[0] = (*d->attach)(0);
	c[1] = 0;
	fd[0] = -1;
	fd[1] = -1;
	if(waserror()){
		close(c[0]);
		if(c[1])
			close(c[1]);
		if(fd[0] >= 0)
			f->fd[fd[0]]=0;
		if(fd[1] >= 0)
			f->fd[fd[1]]=0;
		nexterror();
	}
	c[1] = (*d->clone)(c[0], 0);
	(*d->walk)(c[0], "data");
	(*d->walk)(c[1], "data1");
	c[0] = (*d->open)(c[0], ORDWR);
	c[1] = (*d->open)(c[1], ORDWR);
	fd[0] = newfd(c[0]);
	fd[1] = newfd(c[1]);
	((long*)arg[0])[0] = fd[0];
	((long*)arg[0])[1] = fd[1];
	poperror();
	return 0;
}

long
sysdup(ulong *arg)
{
	int fd;
	Chan *c, *oc;
	Fgrp *f = u->p->fgrp;

	/*
	 * Close after dup'ing, so date > #d/1 works
	 */
	c = fdtochan(arg[0], -1, 0, 1);
	fd = arg[1];
	if(fd != -1){
		if(fd<0 || NFD<=fd) {
			close(c);
			error(Ebadfd);
		}
		lock(f);
		if(fd > f->maxfd)
			f->maxfd = fd;

		oc = f->fd[fd];
		f->fd[fd] = c;
		unlock(f);
		if(oc)
			close(oc);
	}else{
		if(waserror()) {
			close(c);
			nexterror();
		}
		fd = newfd(c);
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
			close(c);
		nexterror();
	}
	validaddr(arg[0], 1, 0);
	c = namec((char*)arg[0], Aopen, arg[1], 0);
	fd = newfd(c);
	poperror();
	return fd;
}

void
fdclose(int fd, int flag)
{
	int i;
	Chan *c;
	Fgrp *f = u->p->fgrp;

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
	close(c);
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
	long nr;
	Chan *nc;
	Pgrp *pg;

	pg = u->p->pgrp;
	rlock(&pg->ns);

	for(;;) {
		/* Error causes component of union to be skipped */
		if(waserror())
			goto next;

		nc = clone(c->mnt->to, 0);
		poperror();

		if(c->mountid != c->mnt->mountid) {
			pprint("unionread: changed underfoot?\n");
			runlock(&pg->ns);
			close(nc);
			return 0;
		}

		if(waserror()) {	
			close(nc);
			goto next;
		}

		nc = (*devtab[nc->type].open)(nc, OREAD);
		nc->offset = c->offset;
		nr = (*devtab[nc->type].read)(nc, va, n, nc->offset);
		/* devdirread e.g. changes it */
		c->offset = nc->offset;	
		poperror();

		close(nc);
		if(nr > 0) {
			runlock(&pg->ns);
			return nr;
		}
		/* Advance to next element */
	next:
		c->mnt = c->mnt->next;
		if(c->mnt == 0)
			break;
		c->mountid = c->mnt->mountid;
		c->offset = 0;
	}
	runlock(&pg->ns);
	return 0;
}

long
sysread(ulong *arg)
{
	int dir;
	long n;
	Chan *c;

	validaddr(arg[1], arg[2], 1);
	c = fdtochan(arg[0], OREAD, 1, 1);
	if(waserror()) {
		close(c);
		nexterror();
	}

	n = arg[2];
	dir = c->qid.path&CHDIR;

	if(dir) {
		n -= n%DIRLEN;
		if(c->offset%DIRLEN || n==0)
			error(Etoosmall);
	}

	if(dir && c->mnt)
		n = unionread(c, (void*)arg[1], n);
	else
		n = (*devtab[c->type].read)(c, (void*)arg[1], n, c->offset);

	lock(c);
	c->offset += n;
	unlock(c);

	poperror();
	close(c);

	return n;
}

long
syswrite(ulong *arg)
{
	Chan *c;
	long n;

	validaddr(arg[1], arg[2], 0);
	c = fdtochan(arg[0], OWRITE, 1, 1);
	if(waserror()) {
		close(c);
		nexterror();
	}

	if(c->qid.path & CHDIR)
		error(Eisdir);

	n = (*devtab[c->type].write)(c, (void*)arg[1], arg[2], c->offset);

	lock(c);
	c->offset += n;
	unlock(c);

	poperror();
	close(c);

	return n;
}

long
sysseek(ulong *arg)
{
	Chan *c;
	char buf[DIRLEN];
	Dir dir;
	long off;

	c = fdtochan(arg[0], -1, 1, 0);
	if(c->qid.path & CHDIR)
		error(Eisdir);

	if(devchar[c->type] == '|')
		error(Eisstream);

	off = 0;
	switch(arg[2]){
	case 0:
		off = c->offset = arg[1];
		break;

	case 1:
		lock(c);	/* lock for read/write update */
		c->offset += (long)arg[1];
		off = c->offset;
		unlock(c);
		break;

	case 2:
		(*devtab[c->type].stat)(c, buf);
		convM2D(buf, &dir);
		c->offset = dir.length + (long)arg[1];
		off = c->offset;
		break;
	}
	return off;
}

long
sysfstat(ulong *arg)
{
	Chan *c;

	validaddr(arg[1], DIRLEN, 1);
	evenaddr(arg[1]);
	c = fdtochan(arg[0], -1, 0, 1);
	if(waserror()) {
		close(c);
		nexterror();
	}
	(*devtab[c->type].stat)(c, (char*)arg[1]);
	poperror();
	close(c);
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
		close(c);
		nexterror();
	}
	(*devtab[c->type].stat)(c, (char*)arg[1]);
	poperror();
	close(c);
	return 0;
}

long
syschdir(ulong *arg)
{
	Chan *c;

	validaddr(arg[0], 1, 0);
	c = namec((char*)arg[0], Atodir, 0, 0);
	close(u->dot);
	u->dot = c;
	return 0;
}

long
bindmount(ulong *arg, int ismount)
{
	Chan *c0, *c1, *bc;
	ulong flag;
	long ret;
	int fd;
	struct{
		Chan	*chan;
		char	*spec;
	}bogus;

	flag = arg[2];
	fd = arg[0];
	if(flag>MMASK || (flag&MORDER)==(MBEFORE|MAFTER))
		error(Ebadarg);
	if(ismount){
		bc = fdtochan(fd, 2, 0, 1);
		if(waserror()) {
			close(bc);
			nexterror();
		}
		bogus.chan = bc;

		validaddr(arg[3], 1, 0);
		if(vmemchr((char*)arg[3], '\0', NAMELEN) == 0)
			error(Ebadarg);

		bogus.spec = (char*)arg[3];

		ret = devno('M', 0);
		c0 = (*devtab[ret].attach)((char*)&bogus);

		poperror();
		close(bc);
	}
	else {
		validaddr(arg[0], 1, 0);
		c0 = namec((char*)arg[0], Aaccess, 0, 0);
	}
	if(waserror()){
		close(c0);
		nexterror();
	}
	validaddr(arg[1], 1, 0);
	c1 = namec((char*)arg[1], Amount, 0, 0);
	if(waserror()){
		close(c1);
		nexterror();
	}
	if((c0->qid.path^c1->qid.path) & CHDIR)
		error(Emount);
	if(flag && !(c0->qid.path&CHDIR))
		error(Emount);
	ret = mount(c0, c1, flag);
	poperror();
	close(c1);
	poperror();
	close(c0);
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
			close(cmount);
			nexterror();
		}
		validaddr(arg[0], 1, 0);
		cmounted = namec((char*)arg[0], Aopen, OREAD, 0);
		poperror();
	}

	if(waserror()) {
		close(cmount);
		if(cmounted)
			close(cmounted);
		nexterror();
	}
	unmount(cmount, cmounted);
	close(cmount);
	if(cmounted)
		close(cmounted);
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
			close(c);
		nexterror();
	}
	validaddr(arg[0], 1, 0);
	c = namec((char*)arg[0], Acreate, arg[1], arg[2]);
	fd = newfd(c);
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
		close(c);
		nexterror();
	}
	(*devtab[c->type].remove)(c);
	/*
	 * Remove clunks the fid, but we need to recover the Chan
	 * so fake it up.  rootclose() is known to be a nop.
	 */
	c->type = 0;
	poperror();
	close(c);
	return 0;
}

long
syswstat(ulong *arg)
{
	Chan *c;

	validaddr(arg[1], DIRLEN, 0);
	nameok((char*)arg[1]);
	validaddr(arg[0], 1, 0);
	c = namec((char*)arg[0], Aaccess, 0, 0);
	if(waserror()){
		close(c);
		nexterror();
	}
	(*devtab[c->type].wstat)(c, (char*)arg[1]);
	poperror();
	close(c);
	return 0;
}

long
sysfwstat(ulong *arg)
{
	Chan *c;

	validaddr(arg[1], DIRLEN, 0);
	nameok((char*)arg[1]);
	c = fdtochan(arg[0], -1, 1, 1);
	if(waserror()) {
		close(c);
		nexterror();
	}
	(*devtab[c->type].wstat)(c, (char*)arg[1]);
	poperror();
	close(c);
	return 0;
}
