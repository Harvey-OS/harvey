#include <u.h>
#include <libc.h>
#include <../boot/boot.h>

void
configcyc(Method *mp)
{
	USED(mp);
}

int
authcyc(void)
{
	return -1;
}

int
connectcyc(void)
{
	return open("#C/cyc", ORDWR);
}
