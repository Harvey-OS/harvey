#include <u.h>
#include <libc.h>
#include "fmtdef.h"

int
vfprint(int fd, char *fmt, va_list args)
{
	Fmt f;
	char buf[256];
	int n;

	fmtfdinit(&f, fd, buf, sizeof(buf));
	VA_COPY(f.args,args);
	n = dofmt(&f, fmt);
	VA_END(f.args);
	if(n > 0 && __fmtFdFlush(&f) == 0)
		return -1;
	return n;
}
