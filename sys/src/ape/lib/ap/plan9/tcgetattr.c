#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <termios.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

static int ctlfd = -1;

/* BUG: only works if pcons has been executed; ignores fd */

int
tcgetattr(int fd, struct termios *t)
{
	char buf[4+4*9+NCCS], *p;
	int n;

	if(ctlfd < 0){
		ctlfd = open("/dev/pttyctl", O_RDWR);
		if(ctlfd < 0){
			errno = ENOTTY;
			return -1;
		}
	}
	if((n=read(ctlfd, buf, sizeof(buf))) != sizeof(buf)){
		errno = ENOTTY;
		return -1;
	}
	p = buf;
	t->c_iflag = strtol(p+5, 0, 16);
	t->c_oflag = strtol(p+5+9, 0, 16);
	t->c_cflag = strtol(p+5+18, 0, 16);
	t->c_lflag = strtol(p+5+27, 0, 16);
	memcpy(t->c_cc, p+5+36, NCCS);
	return 0;
}

/* BUG: ignores optional actions */

int
tcsetattr(int fd, int optactions, const struct termios *t)
{
	char buf[4+4*9+NCCS];

	if(ctlfd < 0){
		ctlfd = open("/dev/pttyctl", O_RDWR);
		if(ctlfd < 0){
			errno = ENOTTY;
			return -1;
		}
	}
	sprintf(buf, "stty %8x %8x %8x %8x ",
		t->c_iflag, t->c_oflag, t->c_cflag, t->c_lflag);
	memcpy(buf+5+36, t->c_cc, NCCS);
	if(write(ctlfd, buf, sizeof(buf)) != sizeof(buf)){
		errno = ENOTTY;
		return -1;
	}
	return 0;
}
