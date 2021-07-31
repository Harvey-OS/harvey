#include <u.h>
#include <libc.h>

ulong
getcallerpc(void *x)
{
	return (((ulong*)(x))[-1]);
}
