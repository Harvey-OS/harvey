#include <u.h>
#include <libc.h>
#include <auth.h>
#include "srv.h"

enum{
	SULEN	= AUTHLEN + NAMELEN + DESKEYLEN,
};

static int	readarg(int, char*, int);

int
getchall(char *user, char *chal)
{
	char uandh[2*NAMELEN+AUTHLEN], sysname[NAMELEN];
	int fd, n;

	fd = open("#c/sysname", OREAD);
	if(fd < 0)
		return -1;
	n = read(fd, sysname, sizeof sysname - 1);
	close(fd);
	if(n < 0)
		return -1;
	sysname[n] = '\0';
	fd = open("#c/chal", OREAD);
	if(fd < 0)
		return -1;
	n = read(fd, uandh+2*NAMELEN, AUTHLEN);
	close(fd);
	if(n != AUTHLEN)
		return -1;
	strncpy(uandh, user, NAMELEN);
	strncpy(uandh+NAMELEN, sysname, NAMELEN);
	fd = authdial("chal");
	if(fd < 0)
		return -1;
	if(write(fd, uandh, sizeof uandh) != sizeof uandh
	|| readarg(fd, chal, NETCHLEN) < 0){
		close(fd);
		return -1;
	}
	return fd;
}

int
challreply(int fd, char *user, char *reply){
	char su[SULEN];
	int n;

	if(write(fd, reply, strlen(reply)+1) != strlen(reply)+1
	|| read(fd, su, 2) != 2
	|| su[0] != 'O' || su[1] != 'K'){
		close(fd);
		return -1;
	}
	n = read(fd, su, sizeof su);
	close(fd);
	if(n != sizeof su)
		return -1;
	netchown(user);
	fd = open("#c/chal", OWRITE|OTRUNC);
	if(fd < 0)
		return -1;
	n = write(fd, su, sizeof su);
	close(fd);
	if(n != sizeof su)
		return -1;
	return 0;
}

static int
readarg(int fd, char *arg, int len)
{
	char buf[1];
	int i;

	i = 0;
	for(;;){
		if(read(fd, buf, 1) != 1)
			return -1;
		if(i < len - 1)
			arg[i++] = *buf;
		if(*buf == '\0'){
			arg[i] = '\0';
			return 0;
		}
	}
}
