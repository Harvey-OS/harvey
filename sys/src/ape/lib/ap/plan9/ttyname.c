#include <unistd.h>
#include <stdio.h>

char *
ttyname(int fd)
{
	static char buf[100];

	sprintf(buf, "/fd/%d", fd);
	return buf;
}
