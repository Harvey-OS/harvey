#include	<u.h>
#include	<libc.h>

int
fprint(int fd, char *fmt, ...)
{
	int n;
	va_list args;

	va_start(args, fmt);
	n = vfprint(fd, fmt, args);
	va_end(args);
	return n;
}
