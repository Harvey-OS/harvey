#include <u.h>
#include <libc.h>
#include "plumb.h"

int
plumbsendtext(int fd, char *src, char *dst, char *wdir, char *data)
{
	Plumbmsg m;

	m.src = src;
	m.dst = dst;
	m.wdir = wdir;
	m.type = "text";
	m.attr = nil;
	m.ndata = strlen(data);
	m.data = data;
	return plumbsend(fd, &m);
}
