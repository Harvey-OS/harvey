/*
 * turn raw (no echo, etc.) on and off.
 * ptyfs is gone, so don't even try tcsetattr, etc.
 */
#define _POSIX_SOURCE
#define _RESEARCH_SOURCE

#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <libv.h>

static int ctlfd = -1;

/* fd is ignored */

tty_echooff(int fd)
{
	if(ctlfd >= 0)
		return 0;
	ctlfd = open("/dev/consctl", O_WRONLY);
	if(ctlfd < 0)
		return -1;
	write(ctlfd, "rawon", 5);
	return 0;
}

tty_echoon(int fd)
{
	if(ctlfd >= 0){
		write(ctlfd, "rawoff", 6);
		close(ctlfd);
		ctlfd = -1;
		return 0;
	}
	return -1;
}
