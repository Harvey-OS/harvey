#include <alef.h>

void*	ALEFalloc(uint, int);

void*
malloc(uint size)
{
	return ALEFalloc(size, 1);
}
