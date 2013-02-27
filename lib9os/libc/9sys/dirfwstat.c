#include <u.h>
#include <libc.h>
#include <fcall.h>

int
dirfwstat(int fd, Dir *d)
{
	uchar *buf;
	int r;

	r = sizeD2M(d);
	buf = malloc(r);
	if(buf == nil)
		return -1;
	convD2M(d, buf, r);
	r = fwstat(fd, buf, r);
	free(buf);
	return r;
}
