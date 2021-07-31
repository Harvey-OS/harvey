#include <u.h>
#include <libc.h>
#include <auth.h>
#include "authsrv.h"

enum{
	Plan9		= 1,
	Securenet	= 2,
};

int	rem(char*, char*);
void	usage(void);

void
main(int argc, char *argv[])
{
	char *u;
	int which;

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
	argv0 = "removeuser";

	if(argc != 1)
		usage();
	u = *argv;
	if(memchr(u, '\0', NAMELEN) == 0)
		error("bad user name");

	if(!which)
		which = Plan9|Securenet;
	if(which & Plan9){
		if(rem(KEYDB, u))
			print("user %s removed from Plan 9\n", u);
		else
			print("couldn't remove %s from Plan 9: %r\n", u);
		syslog(0, AUTHLOG, "user %s removed from Plan 9", u);
	}
	if(which & Securenet){
		if(rem(NETKEYDB, u))
			print("user %s removed from SecureNet\n", u);
		else
			print("couldn't remove %s from SecureNet: %r\n", u);
		syslog(0, AUTHLOG, "user %s removed from Securenet", u);
	}
	exits(0);
}

int
rem(char *db, char *u)
{
	char buf[KEYDBBUF+NAMELEN+6];

	sprint(buf, "%s/%s", db, u);
	return remove(buf) >= 0;
}

void
usage(void)
{
	fprint(2, "usage: removeuser [-pn] user\n");
	exits("usage");
}
