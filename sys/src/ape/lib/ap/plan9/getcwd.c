#include "lib.h"
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include "sys9.h"
#include "dir.h"

char*
getcwd(char *buf, size_t len)
{
	int fd;

	fd = _OPEN(".", OREAD);
	if(fd < 0) {
		errno = EACCES;
		return 0;
	}
	if(_FD2PATH(fd, buf, len) < 0) {
		errno = EIO;
		_CLOSE(fd);
		return 0;
	}
	_CLOSE(fd);

/* RSC: is this necessary? */
	if(buf[0] == '\0')
		strcpy(buf, "/");
	return buf;
}
