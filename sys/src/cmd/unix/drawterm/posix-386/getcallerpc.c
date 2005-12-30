#include "u.h"
#include "libc.h"

ulong
getcallerpc(void *a)
{
	return ((ulong*)a)[-1];
}
