#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"
#include	"kernel.h"

#define validaddr(a, b, c)

long
kclose(int fd)
{
	if(waserror())
		return -1;
	/*
	 * Take no reference on the chan because we don't really need the
	 * data structure, and are calling fdtochan only for error checks.
	 * fdclose takes care of processes racing through here.
	 */
	fdtochan(fd, -1, 0, 0);
	fdclose(fd, 0);

	poperror();
	return 0;
}

long
kopen(char *path, int mode)
{
	int fd;
	Chan *c = 0;

	if(waserror()) {
		if(c)
			cclose(c);
		return -1;
	}

	openmode(mode);
	validaddr((ulong)path, 1, 0);
	c = namec(path, Aopen, mode, 0);
	fd = newfd(c);

	poperror();
	return fd;
}

long
kread(int fd, void *va, long n)
{
	int dir;
	Chan *c;

	if(waserror())
		return -1;

	validaddr((ulong)va, n, 1);
	c = fdtochan(fd, OREAD, 1, 1);
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
		n = unionread(c, va, n);
	else if(devtab[c->type]->dc != L'M')
		n = devtab[c->type]->read(c, va, n, c->offset);
	else
		n = mntread9p(c, va, n, c->offset);

	lock(c);
	c->offset += n;
	unlock(c);

	poperror();
	cclose(c);

	poperror();
	return n;
}

long
kseek(int fd, vlong offset, int whence)
{
	Chan *c;
	char buf[DIRLEN];
	Dir dir;
	long off;

	if(waserror())
		return -1;

	c = fdtochan(fd, -1, 1, 1);
	if(waserror()) {
		cclose(c);
		nexterror();
	}
	if(c->qid.path & CHDIR)
		error(Eisdir);

	if(devtab[c->type]->dc == '|')
		error(Eisstream);

	off = 0;
	switch(whence) {
	case 0:
		off = offset;
		c->offset = offset;
		break;
	case 1:
		lock(c);	/* lock for read/write update */
		c->offset += offset;
		off = c->offset;
		unlock(c);
		break;
	case 2:
		devtab[c->type]->stat(c, buf);
		convM2D(buf, &dir);
		c->offset = dir.length + offset;
		off = c->offset;
		break;
	}
	poperror();
	cclose(c);
	poperror();
	return off;
}

long
kwrite(int fd, void *va, long n)
{
	Chan *c;

	if(waserror())
		return -1;

	validaddr((ulong)va, n, 0);
	c = fdtochan(fd, OWRITE, 1, 1);
	if(waserror()) {
		cclose(c);
		nexterror();
	}

	if(c->qid.path & CHDIR)
		error(Eisdir);

	n = devtab[c->type]->write(c, va, n, c->offset);

	lock(c);
	c->offset += n;
	unlock(c);

	poperror();
	cclose(c);

	poperror();
	return n;
}

/* Set kernel error string */
void
kerrstr(char *err)
{
	if(up != nil)
		strncpy(up->error, err, ERRLEN);
}

/* Get kernel error string */
void
kgerrstr(char *err)
{
	char *s;

	s = "<no-up>";
	if(up != nil)
		s = up->error;
	strncpy(err, s, ERRLEN);
}

/* Set kernel error string, using formatted print */
void
kwerrstr(char *fmt, ...)
{
	va_list arg;
	char buf[256];

	va_start(arg, fmt);
	doprint(buf, buf+sizeof(buf), fmt, arg);
	va_end(arg);
	strncpy(up->error, buf, ERRLEN);
}
