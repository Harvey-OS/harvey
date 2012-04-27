#include	"u.h"
#include	"lib.h"
#include	"dat.h"
#include	"fns.h"
#include	"error.h"

Chan*
fdtochan(int fd, int mode, int chkmnt, int iref)
{
	Fgrp *f;
	Chan *c;

	c = 0;
	f = up->fgrp;

	lock(&f->ref.lk);
	if(fd<0 || NFD<=fd || (c = f->fd[fd])==0) {
		unlock(&f->ref.lk);
		error(Ebadfd);
	}
	if(iref)
		refinc(&c->ref);
	unlock(&f->ref.lk);

	if(chkmnt && (c->flag&CMSG))
		goto bad;
	if(mode<0 || c->mode==ORDWR)
		return c;
	if((mode&OTRUNC) && c->mode==OREAD)
		goto bad;
	if((mode&~OTRUNC) != c->mode)
		goto bad;
	return c;
bad:
	if(iref)
		cclose(c);
	error(Ebadusefd);
	return nil; /* shut up compiler */
}

static void
fdclose(int fd, int flag)
{
	int i;
	Chan *c;
	Fgrp *f;

	f = up->fgrp;

	lock(&f->ref.lk);
	c = f->fd[fd];
	if(c == 0) {
		unlock(&f->ref.lk);
		return;
	}
	if(flag) {
		if(c==0 || !(c->flag&flag)) {
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

static int
newfd(Chan *c)
{
	int i;
	Fgrp *f;

	f = up->fgrp;
	lock(&f->ref.lk);
	for(i=0; i<NFD; i++)
		if(f->fd[i] == 0){
			if(i > f->maxfd)
				f->maxfd = i;
			f->fd[i] = c;
			unlock(&f->ref.lk);
			return i;
		}
	unlock(&f->ref.lk);
	error("no file descriptors");
	return 0;
}

int
sysclose(int fd)
{
	if(waserror())
		return -1;

	fdtochan(fd, -1, 0, 0);
	fdclose(fd, 0);
	poperror();
	return 0;
}

int
syscreate(char *path, int mode, ulong perm)
{
	int fd;
	Chan *c = 0;

	if(waserror()) {
		cclose(c);
		return -1;
	}

	openmode(mode);			/* error check only */
	c = namec(path, Acreate, mode, perm);
	fd = newfd((Chan*)c);
	poperror();
	return fd;
}

int
sysdup(int old, int new)
{
	Chan *oc;
	Fgrp *f = up->fgrp;
	Chan *c = 0;

	if(waserror())
		return -1;

	c = fdtochan(old, -1, 0, 1);
	if(new != -1) {
		if(new < 0 || NFD <= new) {
			cclose(c);
			error(Ebadfd);
		}
		lock(&f->ref.lk);
		if(new > f->maxfd)
			f->maxfd = new;
		oc = f->fd[new];
		f->fd[new] = (Chan*)c;
		unlock(&f->ref.lk);
		if(oc != 0)
			cclose(oc);
	}
	else {
		if(waserror()) {
			cclose(c);
			nexterror();
		}
		new = newfd((Chan*)c);
		poperror();
	}
	poperror();
	return new;
}

int
sysfstat(int fd, char *buf)
{
	Chan *c = 0;

	if(waserror()) {
		cclose(c);
		return -1;
	}
	c = fdtochan(fd, -1, 0, 1);
	devtab[c->type]->stat((Chan*)c, buf);
	poperror();
	cclose(c);
	return 0;
}

int
sysfwstat(int fd, char *buf)
{
	Chan *c = 0;

	if(waserror()) {
		cclose(c);
		return -1;
	}
	nameok(buf);
	c = fdtochan(fd, -1, 1, 1);
	devtab[c->type]->wstat((Chan*)c, buf);
	poperror();
	cclose(c);
	return 0;
}

int
syschdir(char *dir)
{
	return 0;
}

long
bindmount(Chan *c0, char *old, int flag, char *spec)
{
	int ret;
	Chan *c1 = 0;

	if(flag>MMASK || (flag&MORDER) == (MBEFORE|MAFTER))
		error(Ebadarg);

	c1 = namec(old, Amount, 0, 0);
	if(waserror()){
		cclose(c1);
		nexterror();
	}

	ret = cmount(c0, c1, flag, spec);

	poperror();
	cclose(c1);
	return ret;
}

int
sysbind(char *new, char *old, int flags)
{
	long r;
	Chan *c0 = 0;

	if(waserror()) {
		cclose(c0);
		return -1;
	}
	c0 = namec(new, Aaccess, 0, 0);
	r = bindmount(c0, old, flags, "");
	poperror();
	cclose(c0);
	return 0;
}

int
sysmount(int fd, char *old, int flags, char *spec)
{
	long r;
	Chan *c0 = 0, *bc = 0;
	struct {
		Chan*	chan;
		char*	spec;
		int	flags;
	} mntparam;

	if(waserror()) {
		cclose(bc);
		cclose(c0);
		return -1;
	}
	bc = fdtochan(fd, ORDWR, 0, 1);
	mntparam.chan = (Chan*)bc;
	mntparam.spec = spec;
	mntparam.flags = flags;
	c0 = (*devtab[devno('M', 0)].attach)(&mntparam);
	cclose(bc);
	r = bindmount(c0, old, flags, spec);
	poperror();
	cclose(c0);

	return r;
}

int
sysunmount(char *old, char *new)
{
	Chan *cmount = 0, *cmounted = 0;

	if(waserror()) {
		cclose(cmount);
		cclose(cmounted);
		return -1;
	}

	cmount = namec(new, Amount, OREAD, 0);
	if(old != 0)
		cmounted = namec(old, Aopen, OREAD, 0);

	cunmount(cmount, cmounted);
	poperror();	
	cclose(cmount);
	cclose(cmounted);
	return 0;
}

int
sysopen(char *path, int mode)
{
	int fd;
	Chan *c = 0;

	if(waserror()){
		cclose(c);
		return -1;
	}
	openmode(mode);				/* error check only */
	c = namec(path, Aopen, mode, 0);
	fd = newfd((Chan*)c);
	poperror();
	return fd;
}

long
unionread(Chan *c, void *va, long n)
{
	long nr;
	Chan *nc = 0;
	Pgrp *pg = 0;

	pg = up->pgrp;
	rlock(&pg->ns);

	for(;;) {
		if(waserror()) {
			runlock(&pg->ns);
			nexterror();
		}
		nc = clone(c->mnt->to, 0);
		poperror();

		if(c->mountid != c->mnt->mountid) {
			runlock(&pg->ns);
			cclose(nc);
			return 0;
		}

		/* Error causes component of union to be skipped */
		if(waserror()) {	
			cclose(nc);
			goto next;
		}

		nc = (*devtab[nc->type].open)((Chan*)nc, OREAD);
		nc->offset = c->offset;
		nr = (*devtab[nc->type].read)((Chan*)nc, va, n, nc->offset);
		/* devdirread e.g. changes it */
		c->offset = nc->offset;	
		poperror();

		cclose(nc);
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
sysread(int fd, void *va, long n)
{
	int dir;
	Lock *cl;
	Chan *c = 0;

	if(waserror()) {
		cclose(c);
		return -1;
	}
	c = fdtochan(fd, OREAD, 1, 1);

	dir = c->qid.path&CHDIR;
	if(dir) {
		n -= n%DIRLEN;
		if(c->offset%DIRLEN || n==0)
			error(Etoosmall);
	}

	if(dir && c->mnt)
		n = unionread((Chan*)c, va, n);
	else
		n = (*devtab[c->type].read)((Chan*)c, va, n, c->offset);

	cl = (Lock*)&c->r.l;
	lock(cl);
	c->offset += n;
	unlock(cl);

	poperror();
	cclose(c);

	return n;
}

int
sysremove(char *path)
{
	Chan *c = 0;

	if(waserror()) {
		if(c != 0)
			c->type = 0;	/* see below */
		cclose(c);
		return -1;
	}
	c = namec(path, Aaccess, 0, 0);
	(*devtab[c->type].remove)((Chan*)c);
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
sysseek(int fd, long off, int whence)
{
	Dir dir;
	Chan *c;
	char buf[DIRLEN];

	if(waserror())
		return -1;

	c = fdtochan(fd, -1, 1, 0);
	if(c->qid.path & CHDIR)
		error(Eisdir);

	switch(whence) {
	case 0:
		c->offset = off;
		break;

	case 1:
		lock(&c->r.l);	/* lock for read/write update */
		c->offset += off;
		off = c->offset;
		unlock(&c->r.l);
		break;

	case 2:
		(*devtab[c->type].stat)(c, buf);
		convM2D(buf, &dir);
		c->offset = dir.length + off;
		off = c->offset;
		break;
	}
	poperror();
	return off;
}

int
sysstat(char *path, char *buf)
{
	Chan *c = 0;

	if(waserror()){
		cclose(c);
		return -1;
	}
	c = namec(path, Aaccess, 0, 0);
	(*devtab[c->type].stat)((Chan*)c, buf);
	poperror();
	cclose(c);
	return 0;
}

long
syswrite(int fd, void *va, long n)
{
	Lock *cl;
	Chan *c = 0;

	if(waserror()) {
		cclose(c);
		return -1;
	}
	c = fdtochan(fd, OWRITE, 1, 1);
	if(c->qid.path & CHDIR)
		error(Eisdir);

	n = (*devtab[c->type].write)((Chan*)c, va, n, c->offset);

	cl = (Lock*)&c->r.l;
	lock(cl);
	c->offset += n;
	unlock(cl);

	poperror();
	cclose(c);

	return n;
}

int
syswstat(char *path, char *buf)
{
	Chan *c = 0;

	if(waserror()) {
		cclose(c);
		return -1;
	}

	nameok(buf);
	c = namec(path, Aaccess, 0, 0);
	(*devtab[c->type].wstat)((Chan*)c, buf);
	poperror();
	cclose(c);
	return 0;
}

int
sysdirstat(char *name, Dir *dir)
{
	char buf[DIRLEN];

	if(sysstat(name, buf) == -1)
		return -1;
	convM2D(buf, dir);
	return 0;
}

int
sysdirfstat(int fd, Dir *dir)
{
	char buf[DIRLEN];

	if(sysfstat(fd, buf) == -1)
		return -1;

	convM2D(buf, dir);
	return 0;
}

int
sysdirwstat(char *name, Dir *dir)
{
	char buf[DIRLEN];

	convD2M(dir, buf);
	return syswstat(name, buf);
}

int
sysdirfwstat(int fd, Dir *dir)
{
	char buf[DIRLEN];

	convD2M(dir, buf);
	return sysfwstat(fd, buf);
}

long
sysdirread(int fd, Dir *dbuf, long count)
{
	int c, n, i, r;
	char buf[DIRLEN*50];

	n = 0;
	count = (count/sizeof(Dir)) * DIRLEN;
	while(n < count) {
		c = count - n;
		if(c > sizeof(buf))
			c = sizeof(buf);
		r = sysread(fd, buf, c);
		if(r == 0)
			break;
		if(r < 0 || r % DIRLEN)
			return -1;
		for(i=0; i<r; i+=DIRLEN) {
			convM2D(buf+i, dbuf);
			dbuf++;
		}
		n += r;
		if(r != c)
			break;
	}

	return (n/DIRLEN) * sizeof(Dir);
}

static int
call(char *clone, char *dest, int *cfdp, char *dir, char *local)
{
	int fd, cfd, n;
	char *p, name[3*NAMELEN+5], data[3*NAMELEN+10];

	cfd = sysopen(clone, ORDWR);
	if(cfd < 0){
		werrstr("%s: %r", clone);
		return -1;
	}

	/* get directory name */
	n = sysread(cfd, name, sizeof(name)-1);
	if(n < 0) {
		sysclose(cfd);
		return -1;
	}
	name[n] = 0;
	sprint(name, "%d", strtoul(name, 0, 0));
	p = strrchr(clone, '/');
	*p = 0;
	if(dir)
		snprint(dir, 2*NAMELEN, "%s/%s", clone, name);
	snprint(data, sizeof(data), "%s/%s/data", clone, name);

	/* connect */
	/* set local side (port number, for example) if we need to */
	if(local)
		snprint(name, sizeof(name), "connect %s %s", dest, local);
	else
		snprint(name, sizeof(name), "connect %s", dest);
	if(syswrite(cfd, name, strlen(name)) < 0){
		werrstr("%s failed: %r", name);
		sysclose(cfd);
		return -1;
	}

	/* open data connection */
	fd = sysopen(data, ORDWR);
	if(fd < 0){
		werrstr("can't open %s: %r", data);
		sysclose(cfd);
		return -1;
	}
	if(cfdp)
		*cfdp = cfd;
	else
		sysclose(cfd);
	return fd;
}

int
sysdial(char *dest, char *local, char *dir, int *cfdp)
{
	int n, fd, rv;
	char *p, net[128], clone[NAMELEN+12];

	/* go for a standard form net!... */
	p = strchr(dest, '!');
	if(p == 0){
		snprint(net, sizeof(net), "net!%s", dest);
	} else {
		strncpy(net, dest, sizeof(net)-1);
		net[sizeof(net)-1] = 0;
	}

	/* call the connection server */
	fd = sysopen("/net/cs", ORDWR);
	if(fd < 0){
		/* no connection server, don't translate */
		p = strchr(net, '!');
		*p++ = 0;
		snprint(clone, sizeof(clone), "/net/%s/clone", net);
		return call(clone, p, cfdp, dir, local);
	}

	/*
	 *  send dest to connection to translate
	 */
	if(syswrite(fd, net, strlen(net)) < 0){
		werrstr("%s: %r", net);
		sysclose(fd);
		return -1;
	}

	/*
	 *  loop through each address from the connection server till
	 *  we get one that works.
	 */
	rv = -1;
	sysseek(fd, 0, 0);
	while((n = sysread(fd, net, sizeof(net) - 1)) > 0){
		net[n] = 0;
		p = strchr(net, ' ');
		if(p == 0)
			continue;
		*p++ = 0;
		rv = call(net, p, cfdp, dir, local);
		if(rv >= 0)
			break;
	}
	sysclose(fd);
	return rv;
}

static int
identtrans(char *addr, char *naddr, int na, char *file, int nf)
{
	char *p;
	char reply[4*NAMELEN];

	/* parse the network */
	strncpy(reply, addr, sizeof(reply));
	reply[sizeof(reply)-1] = 0;
	p = strchr(reply, '!');
	if(p)
		*p++ = 0;

	sprint(file, "/net/%.*s/clone", na - sizeof("/net//clone"), reply);
	strncpy(naddr, p, na);
	naddr[na-1] = 0;
	return 1;
}

static int
nettrans(char *addr, char *naddr, int na, char *file, int nf)
{
	long n;
	int fd;
	char *cp;
	char reply[4*NAMELEN];

	/*
	 *  ask the connection server
	 */
	fd = sysopen("/net/cs", ORDWR);
	if(fd < 0)
		return identtrans(addr, naddr, na, file, nf);
	if(syswrite(fd, addr, strlen(addr)) < 0){
		sysclose(fd);
		return -1;
	}
	sysseek(fd, 0, 0);
	n = sysread(fd, reply, sizeof(reply)-1);
	sysclose(fd);
	if(n <= 0)
		return -1;
	reply[n] = '\0';

	/*
	 *  parse the reply
	 */
	cp = strchr(reply, ' ');
	if(cp == 0)
		return -1;
	*cp++ = 0;
	strncpy(naddr, cp, na);
	naddr[na-1] = 0;
	strncpy(file, reply, nf);
	file[nf-1] = 0;
	return 0;
}

int
sysannounce(char *addr, char *dir)
{
	char *cp;
	int ctl, n, m;
	char buf[3*NAMELEN];
	char buf2[3*NAMELEN];
	char netdir[2*NAMELEN];
	char naddr[3*NAMELEN];

	/*
	 *  translate the address
	 */
	if(nettrans(addr, naddr, sizeof(naddr), netdir, sizeof(netdir)) < 0){
		werrstr("can't translate address");
		return -1;
	}

	/*
	 * get a control channel
	 */
	ctl = sysopen(netdir, ORDWR);
	if(ctl<0){
		werrstr("can't open control channel");
		return -1;
	}
	cp = strrchr(netdir, '/');
	*cp = 0;

	/*
	 *  find out which line we have
	 */
	n = sprint(buf, "%.*s/", 2*NAMELEN+1, netdir);
	m = sysread(ctl, &buf[n], sizeof(buf)-n-1);
	if(m <= 0) {
		sysclose(ctl);
		werrstr("can't read control file");
		return -1;
	}
	buf[n+m] = 0;

	/*
	 *  make the call
	 */
	n = sprint(buf2, "announce %.*s", 2*NAMELEN, naddr);
	if(syswrite(ctl, buf2, n) != n) {
		sysclose(ctl);
		werrstr("announcement fails");
		return -1;
	}

	strcpy(dir, buf);

	return ctl;
}

int
syslisten(char *dir, char *newdir)
{
	char *cp;
	int ctl, n, m;
	char buf[3*NAMELEN];

	/*
	 *  open listen, wait for a call
	 */
	sprint(buf, "%.*s/listen", 2*NAMELEN+1, dir);
	ctl = sysopen(buf, ORDWR);
	if(ctl < 0)
		return -1;

	/*
	 *  find out which line we have
	 */
	strcpy(buf, dir);
	cp = strrchr(buf, '/');
	*++cp = 0;
	n = cp-buf;
	m = sysread(ctl, cp, sizeof(buf) - n - 1);
	if(n<=0){
		sysclose(ctl);
		return -1;
	}
	buf[n+m] = 0;

	strcpy(newdir, buf);
	return ctl;
}
