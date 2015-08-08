#include <u.h>
#include <libc.h>

void
main(void)
{
	char* p = segbrk((void*)(1024ULL * 1024 * 1024 * 1024), nil);
	print("%p\n", p);
	memset(p, 0, 4096);
}
