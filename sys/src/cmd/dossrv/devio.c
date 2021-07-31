#include <u.h>
#include <libc.h>
#include "iotrack.h"
#include "dat.h"
#include "fns.h"

static int
deverror(char *name, Xfs *xf, long addr, long n, long nret)
{
	char errbuf[ERRLEN];

	if(nret < 0){
		errstr(errbuf);
		chat("%s errstr=\"%s\"...", name, errbuf);
		close(xf->dev);
		xf->dev = -1;
		/*if(strcmp(errbuf, "disk changed") == 0)*/
			return -1;
	}
	fprint(2, "dev %d sector %d, %s: %d, should be %d\n",
		xf->dev, addr, name, nret, n);
	panic(name);
	return -1;
}

int
devread(Xfs *xf, long addr, void *buf, long n)
{
	long nread;
/*
 *	chat("devread %d,%d...", dev, addr);
 */
	if(xf->dev < 0)
		return -1;
	seek(xf->dev, xf->offset+addr*Sectorsize, 0);
	nread = read(xf->dev, buf, n);
	if (nread == n)
		return 0;
	return deverror("read", xf, addr, n, nread);
}

int
devwrite(Xfs *xf, long addr, void *buf, long n)
{
	long nwrite;
/*
 *	chat("devwrite %d,%d...", p->dev, p->addr);
 */
	if(xf->dev < 0)
		return -1;
	seek(xf->dev, xf->offset+addr*Sectorsize, 0);
	nwrite = write(xf->dev, buf, n);
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
	seek(xf->dev, 0, 0);
	if(read(xf->dev, buf, Sectorsize) != Sectorsize){
		close(xf->dev);
		xf->dev = -1;
		return -1;
	}
	return 0;
}
