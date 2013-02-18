#include "lib9.h"
#include <a.out.h>
#include <dynld.h>

typedef struct Fd Fd;
struct Fd {
	int	fd;
};

static long
readfd(void *a, void *buf, long nbytes)
{
	return read(((Fd*)a)->fd, buf, nbytes);
}

static vlong
seekfd(void *a, vlong off, int t)
{
	return seek(((Fd*)a)->fd, off, t);
}

static void
errfd(char *s)
{
	werrstr("%s", s);
}

Dynobj*
dynloadfd(int fd, Dynsym *sym, int nsym, ulong maxsize)
{
	Fd f;

	f.fd = fd;
	return dynloadgen(&f, readfd, seekfd, errfd, sym, nsym, maxsize);
}
