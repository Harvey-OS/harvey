#include	"l.h"

/*
 * fake malloc
 */
void*
malloc(ulong n)
{
	void *p;

	while(n & 7)
		n++;
	while(nhunk < n)
		gethunk();
	p = hunk;
	nhunk -= n;
	hunk += n;
	return p;
}

void
free(void *p)
{
	USED(p);
}

void*
calloc(ulong m, ulong n)
{
	void *p;

	n *= m;
	p = malloc(n);
	memset(p, 0, n);
	return p;
}

void*
realloc(void*, ulong)
{
	fprint(2, "realloc called\n");
	abort();
	return 0;
}

void*
mysbrk(ulong size)
{
	return sbrk(size);
}

void
setmalloctag(void *v, ulong pc)
{
	USED(v, pc);
}

int
fileexists(char *s)
{
	uchar dirbuf[400];

	/* it's fine if stat result doesn't fit in dirbuf, since even then the file exists */
	return stat(s, dirbuf, sizeof(dirbuf)) >= 0;
}
