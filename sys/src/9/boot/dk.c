#include <u.h>
#include <libc.h>
#include <../boot/boot.h>

static int
dkdial(int *dfd, char *dest)
{
	int i;

	for(i = 0; ; i++){
		if(plumb("#k/dk", dest, dfd, 0) >= 0)
			break;
		if(i == 5)
			return -1;
		sleep(500);
	}
	sendmsg(dfd[0], "init");
	return dfd[1];
}

int
dkauth(void)
{
	char path[2*NAMELEN];
	char *p;
	int dfd[2];

	strcpy(path, sys);
	p = strrchr(path, '/');
	if(p)
		p++;
	else
		p = path;
	strcpy(p, "p9auth!ticket");
	if(dkdial(dfd, path) < 0)
		return -1;
	close(dfd[0]);
	return dfd[1];
}

int
dkconnect(void)
{
	int fd[2];

	if(*sys == 0)
		strcpy(sys, "Nfs");
	if(dkdial(fd, sys) < 0)
		return -1;
	if(cpuflag)
		sendmsg(fd[0], "push reboot");
	close(fd[0]);
	return fd[1];
}
