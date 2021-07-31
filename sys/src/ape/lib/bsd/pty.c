/* posix */
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <sys/pty.h>
#include "lib.h"
#include "sys9.h"
#include "dir.h"
#include "fcall.h"

/*
 * return the name of the slave
 */
char*
ptsname(int fd)
{
	Dir d;
	char cd[DIRLEN];
	static char buf[32];

	if(_FSTAT(fd, cd) < 0) {
		_syserrno();
		return 0;
	}
	convM2D(cd, &d);

	sprintf(buf, "/dev/ptty%d", atoi(d.name+4));
	return buf;
}

/*
 * return the name of the master
 */
char*
ptmname(int fd)
{
	Dir d;
	char cd[DIRLEN];
	static char buf[32];

	if(_FSTAT(fd, cd) < 0) {
		_syserrno();
		return 0;
	}
	convM2D(cd, &d);

	sprintf(buf, "/dev/ttym%d", atoi(d.name+4));
	return buf;
}

static char ptycl[] = "/dev/ptyclone";
static char fssrv[] = "/srv/ptyfs";

static void
mkserver(void)
{
	int fd, i;
	char *argv[3], tbuf[2*TICKETLEN];

	fd = _OPEN(fssrv, 3);
	if(_MOUNT(fd, "/dev", MAFTER, "") < 0) {
		switch(_RFORK(RFPROC|RFFDG)) {
		case -1:
			return;
		case 0:
			argv[0] = "ptyfs";
			argv[1] = 0;
			_EXEC("/bin/ape/ptyfs", argv);
			_EXITS(0);
		default:
			for(i = 0; i < 3; i++) {
				fd = _OPEN(fssrv, 3);
				if(fd >= 0)
					break;
				sleep(1);
			}
		}
		if(fd < 0)
			return;
		memset(tbuf, 0, sizeof(tbuf));
		_FSESSION(fd, tbuf);
		_MOUNT(fd, "/dev", MAFTER, "");
	}
}

/*
 * allocate a new pty
 */
int
_getpty(void)
{
	struct stat sb;

	if(stat(ptycl, &sb) < 0)
		mkserver();

	return open(ptycl, O_RDWR);
}
