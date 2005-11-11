#include <u.h>
#include <libc.h>

void*	listalloc(long, long);

void *
listalloc(long n, long size)
{
	char *p, *base;

	size = (size+sizeof(ulong)-1)/sizeof(ulong)*sizeof(ulong);
	p = base = malloc(n*size);
	while(--n > 0){
		*(char**)p = p+size;
		p += size;
	}
	*(char**)p = 0;
	return base;
}
