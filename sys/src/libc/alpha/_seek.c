#include <u.h>
#include <libc.h>

extern	long	_seek(vlong*, int, vlong, int);

vlong
seek(int fd, vlong o, int p)
{
	vlong l;

	if(_seek(&l, fd, o, p) < 0)
		l = -1;
	return l;
}
