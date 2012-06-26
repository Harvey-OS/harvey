#include <string.h>

void*
memcpy(void *a1, const void *a2, size_t n)
{
	return memmove(a1, a2, n);
}
