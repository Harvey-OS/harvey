#include <u.h>
#include <libc.h>

extern int _seek(int64_t*, int, int64_t, int);

int64_t
seek(int fd, int64_t o, int p)
{
	int64_t l;

	if(_seek(&l, fd, o, p) < 0)
		l = -1LL;
	return l;
}
