#include <u.h>
#include <libc.h>
#include <auth.h>
#include <ctype.h>
#include "authsrv.h"

void
main(int argc, char **argv)
{
	char user[NAMELEN];
	char p9pass[32];
	char key[DESKEYLEN];
	int fd;

	ARGBEGIN{
	}ARGEND;

	switch(argc){
	case 2:
		strncpy(user, argv[0], NAMELEN);
		user[NAMELEN-1] = 0;
		passtokey(key, argv[1]);
		break;
	case 1:
		strncpy(user, argv[0], NAMELEN);
		user[NAMELEN-1] = 0;
		getpass(key, p9pass, 0, 0);
		break;
	case 0:
		strcpy(user, getuser());
		getpass(key, p9pass, 0, 0);
		break;
	default:
		fprint(2, "usage: auth/iam [user [password]]\n");
		break; 
	}

	fd = open("/dev/key", OWRITE);
	if(fd < 0)
		sysfatal("open key");
	write(fd, key, DESKEYLEN);
	close(fd);

	fd = open("/dev/hostowner", OWRITE);
	if(fd < 0)
		sysfatal("open hostowner");
	write(fd, user, strlen(user));
	close(fd);
}
