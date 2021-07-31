/*
 * stubs to make bootstrap kernels link, copies of a few functions
 * to avoid including system calls yet have access to i/o functions,
 * and some convenience routines.
 */
#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

void (*proctrace)(Proc*, int, vlong);

/* devmnt.c */

void
muxclose(Mnt *)
{
}

Chan*
mntauth(Chan *, char *)
{
	return nil;
}

long
mntversion(Chan *, char *, int, int)
{
	return 0;
}

/* swap.c */

Image swapimage;

void
putswap(Page *)
{
}

void
dupswap(Page *)
{
}

void
kickpager(void)
{
}

int
swapcount(ulong)
{
	return 0;
}

void
pagersummary(void)
{
}

void
setswapchan(Chan *)
{
}

/* devenv.c */

void
closeegrp(Egrp *)
{
}

void
ksetenv(char *, char *, int)
{
}

/* devproc.c */

Segment*
data2txt(Segment *s)
{
	Segment *ps;

	ps = newseg(SG_TEXT, s->base, s->size);
	ps->image = s->image;
	incref(ps->image);
	ps->fstart = s->fstart;
	ps->flen = s->flen;
	ps->flushme = 1;
	return ps;
}

/* sysproc.c */

int
return0(void*)
{
	return 0;
}

/* syscallfmt.c */
void
syscallfmt(int, ulong, va_list)
{
}

void
sysretfmt(int, va_list, long, uvlong, uvlong)
{
}

/* sysfile.c */

int
newfd(Chan *)
{
	return -1;
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

int
bind(char *old, char *new, int flag)
{
	int ret;
	Chan *c0, *c1;

	if((flag&~MMASK) || (flag&MORDER)==(MBEFORE|MAFTER))
		error(Ebadarg);

	c0 = namec(old, Abind, 0, 0);
	if(waserror()){
		cclose(c0);
		return -1;
	}

	c1 = namec(new, Amount, 0, 0);
	if(waserror()){
		cclose(c1);
		nexterror();
	}

	ret = cmount(&c0, c1, flag, nil);

	poperror();
	cclose(c1);
	poperror();
	cclose(c0);
	return ret;
}

long
unmount(char *name, char *old)
{
	Chan *cmount, *cmounted;

	cmounted = 0;
	cmount = namec(old, Amount, 0, 0);
	if(waserror()) {
		cclose(cmount);
		if(cmounted)
			cclose(cmounted);
		return -1;
	}

	if(name)
		/*
		 * This has to be namec(..., Aopen, ...) because
		 * if name is something like /srv/cs or /fd/0,
		 * opening it is the only way to get at the real
		 * Chan underneath.
		 */
		cmounted = namec(name, Aopen, OREAD, 0);
	cunmount(cmount, cmounted);
	poperror();
	cclose(cmount);
	if(cmounted)
		cclose(cmounted);
	return 0;
}

long
chdir(char *dir)
{
	Chan *c;

	if (waserror())
		return -1;
	c = namec(dir, Atodir, 0, 0);
	if (up->dot)
		cclose(up->dot);
	up->dot = c;
	poperror();
	return 0;
}

Chan *
namecopen(char *name, int mode)
{
	Chan *c;

	if (waserror())
		return nil;
	c = namec(name, Aopen, mode, 0);
	poperror();
	return c;
}

Chan *
enamecopen(char *name, int mode)
{
	Chan *c;

	c = namecopen(name, mode);
	if (c == nil)
		panic("can't open %s", name);
	return c;
}

Chan *
nameccreate(char *name, int mode)
{
	Chan *c;

	if (waserror())
		return nil;
	c = namec(name, Acreate, mode, 0);
	poperror();
	return c;
}

Chan *
enameccreate(char *name, int mode)
{
	Chan *c;

	c = nameccreate(name, mode);
	if (c == nil)
		panic("can't create %s", name);
	return c;
}

int
myreadn(Chan *c, void *vp, long n)
{
	char *p = vp;
	long nn;

	while(n > 0) {
		nn = devtab[c->type]->read(c, p, n, c->offset);
		if(nn == 0)
			break;
		c->offset += nn;
		p += nn;
		n -= nn;
	}
	return p - (char *)vp;
}

int
readfile(char *file, void *buf, int len)
{
	int n;
	Chan *cc;

	cc = nil;
	if (waserror()) {
		if (cc)
			cclose(cc);
		return -1;
	}
	cc = namecopen(file, OREAD);
	if (cc == nil)
		error("no such file");
	n = myreadn(cc, buf, len);
	poperror();
	cclose(cc);
	return n;
}

static int
dumpfile(char *file)
{
	int n;
	char *buf;

	buf = smalloc(Maxfile + 1);
	n = readfile(file, buf, Maxfile);
	if (n < 0)
		return -1;
	buf[n] = 0;
	print("%s (%d bytes):\n", file, n);
	print("%s\n", buf);
	free(buf);
	return 0;
}

/* main.c */

void
fpx87restore(FPsave*)
{
}

void
fpx87save(FPsave*)
{
}

void
fpssesave(FPsave *)
{
}

void
fpsserestore(FPsave *)
{
}
