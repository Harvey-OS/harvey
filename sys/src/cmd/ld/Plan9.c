#include	"l.h"

void*
mysbrk(ulong size)
{
	return sbrk(size);
}

int
fileexists(char *s)
{
	uchar dirbuf[400];

	/* it's fine if stat result doesn't fit in dirbuf, since even then the file exists */
	return stat(s, dirbuf, sizeof(dirbuf)) >= 0;
}
