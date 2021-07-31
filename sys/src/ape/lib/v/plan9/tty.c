#define _POSIX_SOURCE
#define _RESEARCH_SOURCE
#include <termios.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <libv.h>

static int rconsfd = -1;

/* fd is ignored */

tty_echooff(int fd)
{
	struct termios tio;

	if(tcgetattr(fd, &tio) < 0){
		/* pcons not running; use /dev/rcons */
		if(rconsfd >= 0)
			return 0;
		rconsfd = open("/dev/rcons", O_RDONLY);
		if(rconsfd < 0)
			return -1;
		return 0;
	}
	tio.c_lflag &= ~ECHO;
	if(tcsetattr(fd, 0, &tio) < 0)
		return -1;
	return 0;
}

tty_echoon(int fd)
{
	struct termios tio;

	if(tcgetattr(fd, &tio) < 0){
		if(rconsfd >= 0){
			close(rconsfd);
			rconsfd = -1;
			return 0;
		}
		return -1;
	}
	tio.c_lflag |= ECHO;
	if(tcsetattr(fd, 0, &tio) < 0)
		return -1;
	return 0;
}
