#include <u.h>
#include <libc.h>
#include "iotrack.h"
#include "dat.h"
#include "fns.h"

int readonly;

static int
deverror(char *name, Xfs *xf, long addr, long n, long nret)
{
	errno = Eio;
	if(nret < 0){
		chat("%s errstr=\"%r\"...", name);
		close(xf->dev);
		xf->dev = -1;
		return -1;
	}
	fprint(2, "dev %d sector %ld, %s: %ld, should be %ld\n", xf->dev, addr, name, nret, n);
	return -1;
}

int
devread(Xfs *xf, long addr, void *buf, long n)
{
	long nread;

	if(xf->dev < 0)
		return -1;
	nread = pread(xf->dev, buf, n, xf->offset+(vlong)addr*Sectorsize);
	if (nread == n)
		return 0;
	return deverror("read", xf, addr, n, nread);
}

int
devwrite(Xfs *xf, long addr, void *buf, long n)
{
	long nwrite;

	if(xf->omode==OREAD)
		return -1;

	if(xf->dev < 0)
		return -1;
	nwrite = pwrite(xf->dev, buf, n, xf->offset+(vlong)addr*Sectorsize);
	if (nwrite == n)
		return 0;
	return deverror("write", xf, addr, n, nwrite);
}

int
devcheck(Xfs *xf)
{
	char buf[Sectorsize];

	if(xf->dev < 0)
		return -1;
	if(pread(xf->dev, buf, Sectorsize, 0) != Sectorsize){
		close(xf->dev);
		xf->dev = -1;
		return -1;
	}
	return 0;
}
