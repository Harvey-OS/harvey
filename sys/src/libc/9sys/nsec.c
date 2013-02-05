#include <u.h>
#include <libc.h>

vlong
nsec(void)
{
	return nanotime();
}
