#include <u.h>
#include <libc.h>

static char *nsgetwd(char*, int);

char*
getwd(char *buf, int nbuf)
{
	int n, fd;

	fd = open(".", OREAD);
	if(fd < 0)
		return nsgetwd(buf, nbuf);
	n = fd2path(fd, buf, nbuf);
	close(fd);
	if(n < 0)
		return nsgetwd(buf, nbuf);
	return buf;
}

/*
 * Attempt to read the current directory from the
 * proc file system.  Only used when fd2path can't be.
 */
static char*
nsgetwd(char *buf, int nbuf)
{
	char s[3+12+3+1];
	int lastn, n, fd;
	char *ibuf;

	if(nbuf < 1)
		return nil;

	ibuf = malloc(3+nbuf+1);
	if(ibuf == nil)
		return nil;

	sprint(s, "#p/%d/ns", getpid());
	if((fd = open(s, OREAD)) < 0)
		return nil;

	/*
	 * The ns file yields one line per read, so
	 * to find the ``cd '' line we just wait for the last one.
	 */
	lastn = 0;
	while((n = read(fd, ibuf, 3+nbuf+1)) > 0)
		lastn = n;
	close(fd);

	if(lastn < 3 || strncmp(ibuf, "cd ", 3) != 0)
		return nil;

	n = lastn - 3;
	if(n > nbuf-1)
		n = nbuf-1;
		
	memmove(buf, ibuf+3, n);
	free(ibuf);

	/* null terminate, remove trailing \n if present */
	buf[n] = 0;
	if(n > 0 && buf[n-1] == '\n')
		buf[n-1] = 0;
	return buf;
}
