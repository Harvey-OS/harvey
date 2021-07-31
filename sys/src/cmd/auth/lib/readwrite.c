#include <u.h>
#include <libc.h>
#include <auth.h>
#include "authsrv.h"

int
readfile(char *file, char *buf, int n)
{
	int fd;

	fd = open(file, OREAD);
	if(fd < 0){
		werrstr("%s: %r", file);
		return -1;
	}
	n = read(fd, buf, n);
	close(fd);
	return n;
}

int
writefile(char *file, char *buf, int n)
{
	int fd;

	fd = open(file, OWRITE);
	if(fd < 0)
		return -1;
	n = write(fd, buf, n);
	close(fd);
	return n;
}

char*
findkey(char *db, char *user, char *key)
{
	int n;
	char filename[3*NAMELEN];

	sprint(filename, "%s/%s/key", db, user);
	n = readfile(filename, key, DESKEYLEN);
	if(n != DESKEYLEN)
		return 0;
	else
		return key;
}

char*
setkey(char *db, char *user, char *key)
{
	int n;
	char filename[3*NAMELEN];

	sprint(filename, "%s/%s/key", db, user);
	n = writefile(filename, key, DESKEYLEN);
	if(n != DESKEYLEN)
		return 0;
	else
		return key;
}
