#include <sys/types.h>
#include <unistd.h>
#include <string.h>

void
bcopy(void *f, void *t, size_t n)
{
	memmove(t, f, n);
}

int
bcmp(void *a, void *b, size_t n)
{
	return memcmp(a, b, n);
}

void
bzero(void *a, size_t n)
{
	memset(a, 0, n);
}
