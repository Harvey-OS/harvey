/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

/*
 * The sys*() routines needn't poperror() as they return directly to syscall().
 */

static void
unlockfgrp(Fgrp *f)
{
	int ex;

	ex = f->exceed;
	f->exceed = 0;
	unlock(&f->r.l);
	if(ex)
		pprint("warning: process exceeds %d file descriptors\n", ex);
}

static int
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
static int
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
	Proc *up = externup();
	int fd;
	Fgrp *f;

	f = up->fgrp;
	lock(&f->r.l);
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

static int
newfd2(int fd[2], Chan *c[2])
{
	Proc *up = externup();
	Fgrp *f;

	f = up->fgrp;
	lock(&f->r.l);
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
	Proc *up = externup();
	Chan *c;
	Fgrp *f;

	c = nil;
	f = up->fgrp;

	lock(&f->r.l);
	if(fd<0 || f->nfd<=fd || (c = f->fd[fd])==0) {
		unlock(&f->r.l);
		error(Ebadfd);
	}
	if(iref)
		incref(&c->r);
	unlock(&f->r.l);

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
openmode(int omode)
{
	omode &= ~(OTRUNC|OCEXEC|ORCLOSE);
	if(omode > OEXEC)
		error(Ebadarg);
	if(omode == OEXEC)
		return OREAD;
	return omode;
}

void
sysfd2path(Ar0* ar0, ...)
{
	Chan *c;
	char *buf;
	int fd;
	usize nbuf;
	va_list list;
	va_start(list, ar0);

	/*
	 * int fd2path(int fd, char* buf, int nbuf);
	 * should be
	 * int fd2path(int fd, char* buf, usize nbuf);
	 */
	fd = va_arg(list, int);
	buf = va_arg(list, char*);
	nbuf = va_arg(list, usize);
	va_end(list);
	buf = validaddr(buf, nbuf, 1);

	c = fdtochan(fd, -1, 0, 1);
	snprint(buf, nbuf, "%s", chanpath(c));
	cclose(c);

	ar0->i = 0;
}

void
syspipe(Ar0* ar0, ...)
{
	Proc *up = externup();
	int *a, fd[2];
	Chan *c[2];
	static char *datastr[] = {"data", "data1"};
	va_list list;
	va_start(list, ar0);

	/*
	 * int pipe(int fd[2]);
	 */
	a = va_arg(list, int*);
	va_end(list);
	a = validaddr(a, sizeof(fd), 1);
	evenaddr(PTR2UINT(a));

	c[0] = namec("#|", Atodir, 0, 0);
	c[1] = nil;
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
	c[0] = c[0]->dev->open(c[0], ORDWR);
	c[1] = c[1]->dev->open(c[1], ORDWR);
	if(newfd2(fd, c) < 0)
		error(Enofd);
	poperror();

	a[0] = fd[0];
	a[1] = fd[1];

	ar0->i = 0;
}

void
sysdup(Ar0* ar0, ...)
{
	Proc *up = externup();
	int nfd, ofd;
	Chan *nc, *oc;
	Fgrp *f;
	va_list list;
	va_start(list, ar0);

	/*
	 * int dup(int oldfd, int newfd);
	 *
	 * Close after dup'ing, so date > #d/1 works
	 */
	ofd = va_arg(list, int);
	oc = fdtochan(ofd, -1, 0, 1);
	nfd = va_arg(list, int);
	va_end(list);

	if(nfd != -1){
		f = up->fgrp;
		lock(&f->r.l);
		if(nfd < 0 || growfd(f, nfd) < 0) {
			unlockfgrp(f);
			cclose(oc);
			error(Ebadfd);
		}
		if(nfd > f->maxfd)
			f->maxfd = nfd;

		nc = f->fd[nfd];
		f->fd[nfd] = oc;
		unlockfgrp(f);
		if(nc != nil)
			cclose(nc);
	}else{
		if(waserror()) {
			cclose(oc);
			nexterror();
		}
		nfd = newfd(oc);
		if(nfd < 0)
			error(Enofd);
		poperror();
	}

	ar0->i = nfd;
}

void
sysopen(Ar0* ar0, ...)
{
	Proc *up = externup();
	va_list list;
	char *aname;
	int fd, omode;
	Chan *c;

	/*
	 * int open(char* file, int omode);
	 */
	va_start(list, ar0);
	aname = va_arg(list, char*);
	omode = va_arg(list, int);
	va_end(list);
	openmode(omode);	/* error check only */

	c = nil;
	if(waserror()){
		if(c != nil)
			cclose(c);
		nexterror();
	}
	aname = validaddr(aname, 1, 0);
	c = namec(aname, Aopen, omode, 0);
	fd = newfd(c);
	if(fd < 0)
		error(Enofd);
	poperror();

	ar0->i = fd;
}

void
sysnsec(Ar0* ar0, ...)
{
	va_list list;
	va_start(list, ar0);
	va_end(list);
	/*
	 * int64_t nsec(void);
	 */
	ar0->vl = todget(nil);
}

void
fdclose(int fd, int flag)
{
	Proc *up = externup();
	int i;
	Chan *c;
	Fgrp *f;

	f = up->fgrp;
	lock(&f->r.l);
	c = f->fd[fd];
	if(c == nil){
		/* can happen for users with shared fd tables */
		unlock(&f->r.l);
		return;
	}
	if(flag){
		if(c == nil || !(c->flag&flag)){
			unlock(&f->r.l);
			return;
		}
	}
	f->fd[fd] = nil;
	if(fd == f->maxfd)
		for(i = fd; --i >= 0 && f->fd[i] == 0; )
			f->maxfd = i;

	unlock(&f->r.l);
	cclose(c);
}

void
sysclose(Ar0* ar0, ...)
{
	int fd;
	va_list list;
	va_start(list, ar0);

	/*
	 * int close(int fd);
	 */
	fd = va_arg(list, int);
	va_end(list);

	fdtochan(fd, -1, 0, 0);
	fdclose(fd, 0);

	ar0->i = 0;
}

static int32_t
unionread(Chan *c, void *va, int32_t n)
{
	Proc *up = externup();
	int i;
	int32_t nr;
	Mhead *mh;
	Mount *mount;

	qlock(&c->umqlock);
	mh = c->umh;
	rlock(&mh->lock);
	mount = mh->mount;
	/* bring mount in sync with c->uri and c->umc */
	for(i = 0; mount != nil && i < c->uri; i++)
		mount = mount->next;

	nr = 0;
	while(mount != nil){
		/* Error causes component of union to be skipped */
		if(mount->to && !waserror()){
			if(c->umc == nil){
				c->umc = cclone(mount->to);
				c->umc = c->umc->dev->open(c->umc, OREAD);
			}

			nr = c->umc->dev->read(c->umc, va, n, c->umc->offset);
			c->umc->offset += nr;
			poperror();
		}
		if(nr > 0)
			break;

		/* Advance to next element */
		c->uri++;
		if(c->umc){
			cclose(c->umc);
			c->umc = nil;
		}
		mount = mount->next;
	}
	runlock(&mh->lock);
	qunlock(&c->umqlock);
	return nr;
}

static void
unionrewind(Chan *c)
{
	qlock(&c->umqlock);
	c->uri = 0;
	if(c->umc){
		cclose(c->umc);
		c->umc = nil;
	}
	qunlock(&c->umqlock);
}

static usize
dirfixed(uint8_t *p, uint8_t *e, Dir *d)
{
	int len;
	Dev *dev;

	len = GBIT16(p)+BIT16SZ;
	if(p + len > e)
		return 0;

	p += BIT16SZ;	/* ignore size */
	dev = devtabget(GBIT16(p), 1);			//XDYNX
	if(dev != nil){
		d->type = dev->dc;
		//devtabdecr(dev);
	}
	else
		d->type = -1;
	p += BIT16SZ;
	d->dev = GBIT32(p);
	p += BIT32SZ;
	d->qid.type = GBIT8(p);
	p += BIT8SZ;
	d->qid.vers = GBIT32(p);
	p += BIT32SZ;
	d->qid.path = GBIT64(p);
	p += BIT64SZ;
	d->mode = GBIT32(p);
	p += BIT32SZ;
	d->atime = GBIT32(p);
	p += BIT32SZ;
	d->mtime = GBIT32(p);
	p += BIT32SZ;
	d->length = GBIT64(p);

	return len;
}

static char*
dirname(uint8_t *p, usize *n)
{
	p += BIT16SZ+BIT16SZ+BIT32SZ+BIT8SZ+BIT32SZ+BIT64SZ
		+ BIT32SZ+BIT32SZ+BIT32SZ+BIT64SZ;
	*n = GBIT16(p);

	return (char*)p+BIT16SZ;
}

static usize
dirsetname(char *name, usize len, uint8_t *p, usize n, usize maxn)
{
	char *oname;
	usize nn, olen;

	if(n == BIT16SZ)
		return BIT16SZ;

	oname = dirname(p, &olen);

	nn = n+len-olen;
	PBIT16(p, nn-BIT16SZ);
	if(nn > maxn)
		return BIT16SZ;

	if(len != olen)
		memmove(oname+len, oname+olen, p+n-(uint8_t*)(oname+olen));
	PBIT16((uint8_t*)(oname-2), len);
	memmove(oname, name, len);

	return nn;
}

/*
 * Mountfix might have caused the fixed results of the directory read
 * to overflow the buffer.  Catch the overflow in c->dirrock.
 */
static void
mountrock(Chan *c, uint8_t *p, uint8_t **pe)
{
	uint8_t *e, *r;
	int len, n;

	e = *pe;

	/* find last directory entry */
	for(;;){
		len = BIT16SZ+GBIT16(p);
		if(p+len >= e)
			break;
		p += len;
	}

	/* save it away */
	qlock(&c->rockqlock);
	if(c->nrock+len > c->mrock){
		n = ROUNDUP(c->nrock+len, 1024);
		r = smalloc(n);
		memmove(r, c->dirrock, c->nrock);
		free(c->dirrock);
		c->dirrock = r;
		c->mrock = n;
	}
	memmove(c->dirrock+c->nrock, p, len);
	c->nrock += len;
	qunlock(&c->rockqlock);

	/* drop it */
	*pe = p;
}

/*
 * Satisfy a directory read with the results saved in c->dirrock.
 */
static int
mountrockread(Chan *c, uint8_t *op, int32_t n, int32_t *nn)
{
	int32_t dirlen;
	uint8_t *rp, *erp, *ep, *p;

	/* common case */
	if(c->nrock == 0)
		return 0;

	/* copy out what we can */
	qlock(&c->rockqlock);
	rp = c->dirrock;
	erp = rp+c->nrock;
	p = op;
	ep = p+n;
	while(rp+BIT16SZ <= erp){
		dirlen = BIT16SZ+GBIT16(rp);
		if(p+dirlen > ep)
			break;
		memmove(p, rp, dirlen);
		p += dirlen;
		rp += dirlen;
	}

	if(p == op){
		qunlock(&c->rockqlock);
		return 0;
	}

	/* shift the rest */
	if(rp != erp)
		memmove(c->dirrock, rp, erp-rp);
	c->nrock = erp - rp;

	*nn = p - op;
	qunlock(&c->rockqlock);
	return 1;
}

static void
mountrewind(Chan *c)
{
	c->nrock = 0;
}

/*
 * Rewrite the results of a directory read to reflect current
 * name space bindings and mounts.  Specifically, replace
 * directory entries for bind and mount points with the results
 * of statting what is mounted there.  Except leave the old names.
 */
static int32_t
mountfix(Chan *c, uint8_t *op, int32_t n, int32_t maxn)
{
	Proc *up = externup();
	char *name;
	int nbuf;
	Chan *nc;
	Mhead *mh;
	Mount *mount;
	usize dirlen, nname, r, rest;
	int32_t l;
	uint8_t *buf, *e, *p;
	Dir d;

	p = op;
	buf = nil;
	nbuf = 0;
	for(e=&p[n]; p+BIT16SZ<e; p+=dirlen){
		dirlen = dirfixed(p, e, &d);
		if(dirlen == 0)
			break;
		nc = nil;
		mh = nil;
		if(findmount(&nc, &mh, d.type, d.dev, d.qid)){
			/*
			 * If it's a union directory and the original is
			 * in the union, don't rewrite anything.
			 */
			for(mount=mh->mount; mount; mount=mount->next)
				if(eqchanddq(mount->to, d.type, d.dev, d.qid, 1))
					goto Norewrite;

			name = dirname(p, &nname);
			/*
			 * Do the stat but fix the name.  If it fails,
			 * leave old entry.
			 * BUG: If it fails because there isn't room for
			 * the entry, what can we do?  Nothing, really.
			 * Might as well skip it.
			 */
			if(buf == nil){
				buf = smalloc(4096);
				nbuf = 4096;
			}
			if(waserror())
				goto Norewrite;
			l = nc->dev->stat(nc, buf, nbuf);
			r = dirsetname(name, nname, buf, l, nbuf);
			if(r == BIT16SZ)
				error("dirsetname");
			poperror();

			/*
			 * Shift data in buffer to accomodate new entry,
			 * possibly overflowing into rock.
			 */
			rest = e - (p+dirlen);
			if(r > dirlen){
				while(p+r+rest > op+maxn){
					mountrock(c, p, &e);
					if(e == p){
						dirlen = 0;
						goto Norewrite;
					}
					rest = e - (p+dirlen);
				}
			}
			if(r != dirlen){
				memmove(p+r, p+dirlen, rest);
				dirlen = r;
				e = p+dirlen+rest;
			}

			/*
			 * Rewrite directory entry.
			 */
			memmove(p, buf, r);

		    Norewrite:
			cclose(nc);
			putmhead(mh);
		}
	}
	if(buf)
		free(buf);
	if(p != e)
		error("oops in mountfix");

	return e-op;
}

static int32_t
read(int fd, void *p, int32_t n, int64_t off)
{
	Proc *up = externup();
	int32_t nn, nnn;
	int sequential;
	Chan *c;

	p = validaddr(p, n, 1);

	c = fdtochan(fd, OREAD, 1, 1);

	if(waserror()){
		cclose(c);
		nexterror();
	}

	/*
	 * The offset is passed through on directories, normally.
	 * Sysseek complains, but pread is used by servers like exportfs,
	 * that shouldn't need to worry about this issue.
	 *
	 * Notice that c->devoffset is the offset that c's dev is seeing.
	 * The number of bytes read on this fd (c->offset) may be different
	 * due to rewritings in mountfix.
	 */
	if(off == ~0LL){	/* use and maintain channel's offset */
		off = c->offset;
		sequential = 1;
	} else {
		sequential = 0;
	}
	if(c->qid.type & QTDIR){
		/*
		 * Directory read:
		 * rewind to the beginning of the file if necessary;
		 * try to fill the buffer via mountrockread;
		 * set sequential to always maintain the Chan offset.
		 */
		if(off == 0LL){
			if(sequential){
				c->offset = 0;
				c->devoffset = 0;
			}
			mountrewind(c);
			unionrewind(c);
		}

		if(!mountrockread(c, p, n, &nn)){
			if(c->umh)
				nn = unionread(c, p, n);
			else{
				if(off != c->offset)
					error(Edirseek);
				nn = c->dev->read(c, p, n, c->devoffset);
			}
		}
		nnn = mountfix(c, p, nn, n);

		sequential = 1;
	}
	else
		nnn = nn = c->dev->read(c, p, n, off);

	if(sequential){
		lock(&c->r.l);
		c->devoffset += nn;
		c->offset += nnn;
		unlock(&c->r.l);
	}

	poperror();
	cclose(c);

	return nnn;
}

void
syspread(Ar0* ar0, ...)
{
	int fd;
	void *p;
	int32_t n;
	int64_t off;
	va_list list;
	va_start(list, ar0);
	fd = va_arg(list, int);
	p = va_arg(list, void*);
	n = va_arg(list, int32_t);
	off = va_arg(list, int64_t);
	va_end(list);
	/*
	 * long pread(int fd, void* buf, long nbytes, int64_t offset);
	 */
	ar0->l = read(fd, p, n, off);
}

static int32_t
write(int fd, void *p, int32_t n, int64_t off)
{
	Proc *up = externup();
	int32_t r;
	int sequential;
	Chan *c;

	r = n;

	p = validaddr(p, n, 0);
	n = 0;
	c = fdtochan(fd, OWRITE, 1, 1);

	if(off == ~0LL)
		sequential = 1;
	else
		sequential = 0;

	if(waserror()) {
		if(sequential){
			lock(&c->r.l);
			c->offset -= n;
			unlock(&c->r.l);
		}
		cclose(c);
		nexterror();
	}

	if(c->qid.type & QTDIR)
		error(Eisdir);

	n = r;

	if(sequential){	/* use and maintain channel's offset */
		lock(&c->r.l);
		off = c->offset;
		c->offset += n;
		unlock(&c->r.l);
	}

	r = c->dev->write(c, p, n, off);

	if(sequential && r < n){
		lock(&c->r.l);
		c->offset -= n - r;
		unlock(&c->r.l);
	}

	poperror();
	cclose(c);

	return r;
}

void
syspwrite(Ar0* ar0, ...)
{
	va_list list;
	va_start(list, ar0);
	int fd = va_arg(list, int);
	void *buf = va_arg(list, void *);
	long nbytes = va_arg(list, long);
	int64_t offset = va_arg(list, int64_t);
	va_end(list);
	/*
	 * long pwrite(int fd, void *buf, long nbytes, int64_t offset);
	 */
	ar0->l = write(fd, buf, nbytes, offset);
}

static int64_t
sseek(int fd, int64_t offset, int whence)
{
	Proc *up = externup();
	Chan *c;
	uint8_t buf[sizeof(Dir)+100];
	Dir dir;
	int n;

	c = fdtochan(fd, -1, 1, 1);
	if(waserror()){
		cclose(c);
		nexterror();
	}
	if(c->dev->dc == '|')
		error(Eisstream);

	switch(whence){
	case 0:
		if((c->qid.type & QTDIR) && offset != 0LL)
			error(Eisdir);
		c->offset = offset;
		break;

	case 1:
		if(c->qid.type & QTDIR)
			error(Eisdir);
		lock(&c->r.l);	/* lock for read/write update */
		offset += c->offset;
		c->offset = offset;
		unlock(&c->r.l);
		break;

	case 2:
		if(c->qid.type & QTDIR)
			error(Eisdir);
		n = c->dev->stat(c, buf, sizeof buf);
		if(convM2D(buf, n, &dir, nil) == 0)
			error("internal error: stat error in seek");
		offset += dir.length;
		c->offset = offset;
		break;

	default:
		error(Ebadarg);
	}
	c->uri = 0;
	c->dri = 0;
	cclose(c);
	poperror();

	return offset;
}

void
sysseek(Ar0* ar0, ...)
{
	int fd, whence;
	int64_t offset;
	va_list list;
	va_start(list, ar0);

	/*
	 * int64_t seek(int fd, int64_t n, int type);
	 */
	fd = va_arg(list, int);
	offset = va_arg(list, int64_t);
	whence = va_arg(list, int);
	va_end(list);

	ar0->vl = sseek(fd, offset, whence);
}

void
validstat(uint8_t *s, usize n)
{
	usize m;
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

static char*
pathlast(Path *p)
{
	char *s;

	if(p == nil)
		return nil;
	if(p->len == 0)
		return nil;
	s = strrchr(p->s, '/');
	if(s)
		return s+1;
	return p->s;
}

void
sysfstat(Ar0* ar0, ...)
{
	Proc *up = externup();
	int fd;
	Chan *c;
	usize n;
	int r;
	uint8_t *p;
	va_list list;
	va_start(list, ar0);

	/*
	 * int fstat(int fd, uchar* edir, int nedir);
	 * should really be
	 * usize fstat(int fd, uchar* edir, usize nedir);
	 * but returning an unsigned is probably too
	 * radical.
	 */
	fd = va_arg(list, int);
	p = va_arg(list, uint8_t*);
	n = va_arg(list, usize);
	va_end(list);

	p = validaddr(p, n, 1);
	c = fdtochan(fd, -1, 0, 1);
	if(waserror()) {
		cclose(c);
		nexterror();
	}
	r = c->dev->stat(c, p, n);
	poperror();
	cclose(c);

	ar0->i = r;
}

void
sysstat(Ar0* ar0, ...)
{
	Proc *up = externup();
	char *aname;
	Chan *c;
	usize n;
	int r;
	uint8_t *p;
	va_list list;
	va_start(list, ar0);

	/*
	 * int stat(char* name, uchar* edir, int nedir);
	 * should really be
	 * usize stat(char* name, uchar* edir, usize nedir);
	 * but returning an unsigned is probably too
	 * radical.
	 */
	aname = va_arg(list, char*);
	aname = validaddr(aname, 1, 0);
	p = va_arg(list, uint8_t*);
	n = va_arg(list, usize);
	va_end(list);

	p = validaddr(p, n, 1);
	c = namec(aname, Aaccess, 0, 0);
	if(waserror()){
		cclose(c);
		nexterror();
	}
	r = c->dev->stat(c, p, n);
	aname = pathlast(c->path);
	if(aname)
		r = dirsetname(aname, strlen(aname), p, r, n);

	poperror();
	cclose(c);

	ar0->i = r;
}

void
syschdir(Ar0* ar0, ...)
{
	Proc *up = externup();
	Chan *c;
	char *aname;
	va_list list;
	va_start(list, ar0);

	/*
	 * int chdir(char* dirname);
	 */
	aname = va_arg(list, char*);
	aname = validaddr(aname, 1, 0);
	va_end(list);

	c = namec(aname, Atodir, 0, 0);
	cclose(up->dot);
	up->dot = c;

	ar0->i = 0;
}

/* white list of devices we allow mounting on.
 * At some point we can have build generate this if we ever
 * really start using it.
 */

static int dcok[] =  {
	'M',
	'N',
	'9'
};

/* to be used in chan.c to disallow attach to these devices
 */

int 
checkdc(int dc)
{
	int i;
	/* we check for non-zero in case somebody ever puts a ,
	 * after the last element and we end up with 0 as the last thing ...
	 */
	for(i = 0; (i < nelem(dcok)) && dcok[i]; i++)
		if (dcok[i] == dc)
			return 1;
	return 0;
}
/* if dc is non-zero, it means we're doing a mount and dc is the mount device to use. */
static int
bindmount(int dc, int fd, int afd, char* arg0, char* arg1, int flag, char* spec)
{
	Proc *up = externup();
	int i;
	Dev *dev;
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

	if(dc){
		if (! checkdc(dc))
			error(Ebadarg);

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

		bogus.spec = validaddr(spec, 1, 0);
		if(waserror())
			error(Ebadspec);
		spec = validnamedup(spec, 1);
		poperror();

		if(waserror()){
			free(spec);
			nexterror();
		}

		dev = devtabget(dc, 0);		//XDYNX
		if(waserror()){
			//devtabdecr(dev);
			nexterror();
		}
		c0 = dev->attach((char*)&bogus);
		poperror();
		//devtabdecr(dev);

		poperror();	/* spec */
		free(spec);
		poperror();	/* ac bc */
		if(ac)
			cclose(ac);
		cclose(bc);
	}else{
		bogus.spec = nil;
		c0 = namec(validaddr(arg0, 1, 0), Abind, 0, 0);
	}

	if(waserror()){
		cclose(c0);
		nexterror();
	}

	c1 = namec(validaddr(arg1, 1, 0), Amount, 0, 0);
	if(waserror()){
		cclose(c1);
		nexterror();
	}

	i = cmount(&c0, c1, flag, bogus.spec);

	poperror();
	cclose(c1);
	poperror();
	cclose(c0);
	if(dc)
		fdclose(fd, 0);

	return i;
}

void
sysbind(Ar0* ar0, ...)
{
	int flag;
	char *name, *old;

	va_list list;
	va_start(list, ar0);
	/*
	 * int bind(char* name, char* old, int flag);
	 * should be
	 * long bind(char* name, char* old, int flag);
	 */
	name = va_arg(list, char*);
	old = va_arg(list, char*);
	flag = va_arg(list, int);
	va_end(list);

	ar0->i = bindmount(0, -1, -1, name, old, flag, nil);
}

void
sysmount(Ar0* ar0, ...)
{
	int afd, fd, flag;
	char *aname, *old;
	int dc;
	va_list list;
	va_start(list, ar0);

	/*
	 * int mount(int fd, int afd, char* old, int flag, char* aname);
	 * should be
	 * long mount(int fd, int afd, char* old, int flag, char* aname);
	 */
	fd = va_arg(list, int);
	afd = va_arg(list, int);
	old = va_arg(list, char*);
	flag = va_arg(list, int);
	aname = va_arg(list, char*);
	dc = va_arg(list, int);
	va_end(list);

	ar0->i = bindmount(dc, fd, afd, nil, old, flag, aname);
}

void
sysunmount(Ar0* ar0, ...)
{
	Proc *up = externup();
	char *name, *old;
	Chan *cmount, *cmounted;
	va_list list;
	va_start(list, ar0);

	/*
	 * int unmount(char* name, char* old);
	 */
	name = va_arg(list, char*);
	old = va_arg(list, char*);
	cmount = namec(validaddr(old, 1, 0), Amount, 0, 0);
	va_end(list);

	cmounted = nil;
	if(name != nil) {
		if(waserror()) {
			cclose(cmount);
			nexterror();
		}

		/*
		 * This has to be namec(..., Aopen, ...) because
		 * if arg[0] is something like /srv/cs or /fd/0,
		 * opening it is the only way to get at the real
		 * Chan underneath.
		 */
		cmounted = namec(validaddr(name, 1, 0), Aopen, OREAD, 0);
		poperror();
	}

	if(waserror()) {
		cclose(cmount);
		if(cmounted != nil)
			cclose(cmounted);
		nexterror();
	}

	cunmount(cmount, cmounted);
	cclose(cmount);
	if(cmounted != nil)
		cclose(cmounted);
	poperror();

	ar0->i = 0;
}

void
syscreate(Ar0* ar0, ...)
{
	Proc *up = externup();
	char *aname;
	int fd, omode, perm;
	Chan *c;
	va_list list;
	va_start(list, ar0);

	/*
	 * int create(char* file, int omode, uint32_t perm);
	 * should be
	 * int create(char* file, int omode, int perm);
	 */
	aname = va_arg(list, char*);
	omode = va_arg(list, int);
	perm = va_arg(list, int);
	va_end(list);

	openmode(omode & ~OEXCL);	/* error check only; OEXCL okay here */
	c = nil;
	if(waserror()) {
		if(c != nil)
			cclose(c);
		nexterror();
	}
	c = namec(validaddr(aname, 1, 0), Acreate, omode, perm);
	fd = newfd(c);
	if(fd < 0)
		error(Enofd);
	poperror();

	ar0->i = fd;
}

void
sysremove(Ar0* ar0, ...)
{
	Proc *up = externup();
	Chan *c;
	char *aname;
	va_list list;
	va_start(list, ar0);

	/*
	 * int remove(char* file);
	 */
	aname = va_arg(list, char*);
	c = namec(validaddr(aname, 1, 0), Aremove, 0, 0);
	va_end(list);

	/*
	 * Removing mount points is disallowed to avoid surprises
	 * (which should be removed: the mount point or the mounted Chan?).
	 */
	if(c->ismtpt){
		cclose(c);
		error(Eismtpt);
	}
	if(waserror()){
		c->dev = nil;	/* see below */
		cclose(c);
		nexterror();
	}
	c->dev->remove(c);

	/*
	 * Remove clunks the fid, but we need to recover the Chan
	 * so fake it up.  rootclose() is known to be a nop.
Not sure this dicking around is right for Dev ref counts.
	 */
	c->dev = nil;
	poperror();
	cclose(c);

	ar0->i = 0;
}

static int32_t
wstat(Chan* c, uint8_t* p, usize n)
{
	Proc *up = externup();
	int32_t l;
	usize namelen;

	if(waserror()){
		cclose(c);
		nexterror();
	}

	/*
	 * Renaming mount points is disallowed to avoid surprises
	 * (which should be renamed? the mount point or the mounted Chan?).
	 */
	if(c->ismtpt){
		dirname(p, &namelen);
		if(namelen)
			nameerror(chanpath(c), Eismtpt);
	}
	l = c->dev->wstat(c, p, n);
	poperror();
	cclose(c);

	return l;
}

void
syswstat(Ar0* ar0, ...)
{
	Chan *c;
	char *aname;
	uint8_t *p;
	usize n;
	va_list list;
	va_start(list, ar0);

	/*
	 * int wstat(char* name, uchar* edir, int nedir);
	 * should really be
	 * usize wstat(char* name, uchar* edir, usize nedir);
	 * but returning an unsigned is probably too
	 * radical.
	 */
	aname = va_arg(list, char*);
	p = va_arg(list, uint8_t*);
	n = va_arg(list, usize);

	p = validaddr(p, n, 0);
	validstat(p, n);
	c = namec(validaddr(aname, 1, 0), Aaccess, 0, 0);
	va_end(list);

	ar0->l = wstat(c, p, n);
}

void
sysfwstat(Ar0* ar0, ...)
{
	Chan *c;
	int fd;
	uint8_t *p;
	usize n;
	va_list list;
	va_start(list, ar0);

	/*
	 * int fwstat(int fd, uchar* edir, int nedir);
	 * should really be
	 * usize wstat(int fd, uchar* edir, usize nedir);
	 * but returning an unsigned is probably too
	 * radical.
	 */
	fd = va_arg(list, int);
	p = va_arg(list, uint8_t*);
	n = va_arg(list, usize);

	p = validaddr(p, n, 0);
	validstat(p, n);
	c = fdtochan(fd, -1, 1, 1);
	va_end(list);

	ar0->l = wstat(c, p, n);
}
