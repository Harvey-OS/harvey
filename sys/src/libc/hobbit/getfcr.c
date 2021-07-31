#include <u.h>
#include <libc.h>

ulong
getfsr(void)
{
	return 0;
}

void
setfsr(ulong x)
{
	USED(x);
}

ulong
getfcr(void)
{
	return 0;
}

void
setfcr(ulong x)
{
	USED(x);
}

