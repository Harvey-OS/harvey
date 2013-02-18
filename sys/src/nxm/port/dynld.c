#include "u.h"
#include "../port/lib.h"
#include "../port/error.h"
#include "mem.h"
#include	"dat.h"
#include	"fns.h"
#include	<a.out.h>
#include	<dynld.h>
#include	<kernel.h>

/*
 * kernel interface to dynld, for use by devdynld.c,
 * libinterp/dlm.c, and possibly others
 */

typedef struct Fd Fd;
struct Fd {
	int	fd;
};

static long
readfd(void *a, void *buf, long nbytes)
{
	return kread(((Fd*)a)->fd, buf, nbytes);
}

static vlong
seekfd(void *a, vlong off, int t)
{
	return kseek(((Fd*)a)->fd, off, t);
}

static void
errfd(char *s)
{
	kstrcpy(up->env->errstr, s, ERRMAX);
}

Dynobj*
kdynloadfd(int fd, Dynsym *tab, int ntab)
{
	Fd f;

	f.fd = fd;
	return dynloadgen(&f, readfd, seekfd, errfd, tab, ntab, 0);
}

int
kdynloadable(int fd)
{
	Fd f;

	f.fd = fd;
	return dynloadable(&f, readfd, seekfd);
}

/* auxiliary routines for dynamic loading of C modules */

Dynobj*
dynld(int fd)
{
	Fd f;
	
	f.fd = fd;
	return dynloadgen(&f, readfd, seekfd, errfd, _exporttab, dyntabsize(_exporttab), 0);
}

int
dynldable(int fd)
{
	Fd f;

	f.fd = fd;
	return dynloadable(&f, readfd, seekfd);
}
