#include <u.h>
#include <libc.h>

int
errstr(char *buf)
{
	strcpy(buf, "no error");
	return 0;
}
