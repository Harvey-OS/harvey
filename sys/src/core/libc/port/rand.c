#include	<u.h>
#include	<libc.h>

int
rand(void)
{
	return lrand() & 0x7fff;
}
