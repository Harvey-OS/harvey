#include <u.h>
#include <libc.h>
#include <auth.h>
#include "authsrv.h"

static int
getkey(char *authkey)
{
	int fd;
	int n;

	fd = open("/dev/key", OREAD);
	if(fd < 0)
		return -1;
	n = read(fd, authkey, DESKEYLEN);
	close(fd);
	return n != DESKEYLEN ? -1 : 0;
}

int
getauthkey(char *authkey)
{
	if(getkey(authkey) == 0)
		return 1;
	print("can't read /dev/key, please enter machine key\n");
	getpass(authkey, nil, 0, 1);
	return 1;
}
