#include	<lib9.h>

char *
myctime(long x)
{
	time_t t;

	t = x;
	return ctime(&t);
}
