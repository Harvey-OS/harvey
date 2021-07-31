#include <u.h>
#include <libc.h>
#include <auth.h>
#include "authsrv.h"

char *
findkey(char *user)
{
	char *key;
	char k[DESKEYLEN];
	int fd;
	char buf[sizeof KEYDB + NAMELEN + 6];

	sprint(buf, "%s/%s/key", KEYDB, user);
	fd = open(buf, OREAD);
	if(fd < 0)
		return 0;
	if(read(fd, k, DESKEYLEN) != DESKEYLEN){
		close(fd);
		return 0;
	}
	close(fd);
	key = malloc(DESKEYLEN);
	if(key == 0)
		return 0;
	memmove(key, k, DESKEYLEN);
	return key;
}
