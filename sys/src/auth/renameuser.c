#include <u.h>
#include <libc.h>
#include <auth.h>
#include "authsrv.h"

enum{
	Plan9		= 1,
	Securenet	= 2,
};

int	changename(char*, char*, char*, char*);
void	check(char*, char*, char*);
void	usage(void);

void
main(int argc, char *argv[])
{
	char *old, *new;
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
	argv0 = "renameuser";

	if(argc != 2)
		usage();
	old = argv[0];
	if(memchr(old, '\0', NAMELEN) == 0)
		error("bad old user name");
	new = argv[1];
	if(memchr(new, '\0', NAMELEN) == 0)
		error("bad new user name");

	if(!which)
		which = Plan9|Securenet;
	if(which & Plan9)
		check(KEYDB, new, "Plan 9");
	if(which & Securenet)
		check(NETKEYDB, new, "SecureNet");
	if((which & Plan9) && changename(KEYDB, old, new, "Plan 9")){
		print("user %s changed to %s for Plan 9\n", old, new);
		syslog(0, AUTHLOG, "user %s changed to %s for Plan 9", old, new);
	}
	if((which & Securenet) && changename(NETKEYDB, old, new, "SecureNet")){
		print("user %s changed to %s for SecureNet\n", old, new);
		syslog(0, AUTHLOG, "user %s changed to %s for SecureNet", old, new);
	}
	exits(0);
}

void
check(char *db, char *u, char *where)
{
	char buf[KEYDBBUF+NAMELEN+6], dir[DIRLEN];

	sprint(buf, "%s/%s", db, u);
	if(stat(buf, dir) >= 0)
		error("user %s already exists for %s", u, where);
}

int
changename(char *db, char *u, char *new, char *where)
{
	Dir d;
	char buf[KEYDBBUF+NAMELEN+6];

	sprint(buf, "%s/%s", db, u);
	if(dirstat(buf, &d) < 0){
		print("user does not exist for %s\n", u, where);
		return 0;
	}
	strncpy(d.name, new, NAMELEN);
	if(dirwstat(buf, &d) < 0)
		error("can't change user name for %s", where);
	return 1;
}

void
usage(void)
{
	fprint(2, "usage: renameuser [-pn] old new\n");
	exits("usage");
}
