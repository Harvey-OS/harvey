#include "u.h"
#include "libc.h"

uintptr
getcallerpc(void *a)
{
	return ((uintptr*)a)[-1];
}
