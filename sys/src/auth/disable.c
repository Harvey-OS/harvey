#include <u.h>
#include <libc.h>
#include <ctype.h>
#include <auth.h>
#include "authsrv.h"

enum{
	Plan9		= 1,
	Securenet	= 2,
};

int	setstatus(char*, char*, char*, char*);
void	usage(void);

void
main(int argc, char *argv[])
{
	char *u;
	int which, change;

	change = 0;
	which = 0;
	ARGBEGIN{
	case 'p':
		which |= Plan9;
		break;
	case 'n':
		which |= Securenet;
		break;
	default:
		usage();
	}ARGEND
	argv0 = "disable";

	if(argc != 1)
		usage();
	u = argv[0];
	if(memchr(u, '\0', NAMELEN) == 0)
		error("bad user name");
	if(!which)
		which = Plan9|Securenet;
	if(which & Plan9)
		change += setstatus(KEYDB, u, "disabled", "Plan 9");
	if(which & Securenet)
		change += setstatus(NETKEYDB, u, "disabled", "SecureNet");
	if(!change){
		print("couldn't disable %s\n", u);
		exits("user does not exist");
	}
	print("user %s disabled\n", u);
	syslog(0, AUTHLOG, "user %s disabled", u);
	exits(0);
}

int
setstatus(char *db, char *u, char *msg, char *where)
{
	char buf[sizeof KEYDB+sizeof NETKEYDB+2*NAMELEN+6];
	int fd;

	sprint(buf, "%s/%s/status", db, u);
	fd = open(buf, OWRITE);
	if(fd < 0 || write(fd, msg, strlen(msg)) != strlen(msg)){
		print("can't set %s status\n", where);
		return 0;
	}
	close(fd);
	return 1;
}

void
usage(void)
{
	fprint(2, "usage: disable [-pn] user\n");
	exits("usage");
}
