#include <u.h>
#include <libc.h>

void*	listalloc(long, long);

void *
listalloc(long n, long size)
{
	ulong *p, *base;

	size += sizeof(ulong) - 1;
	size /= sizeof(ulong);
	p = base = malloc(n*size*sizeof(ulong));
	while(--n > 0){
		*p = (ulong)(p+size);
		p += size;
	}
	*p = 0;
	return base;
}
