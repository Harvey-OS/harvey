#include <u.h>
#include <libc.h>
#include <auth.h>
#include "authsrv.h"

void	install(char*, char*, int);
void	usage(void);

void
main(int argc, char *argv[])
{
	char *key;
	char *u;

	argv0 = "printnetkey";
	fmtinstall('K', keyconv);

	ARGBEGIN{
	default:
		usage();
	}ARGEND
	if(argc != 1)
		usage();

	u = argv[0];
	fmtinstall('K', keyconv);
	
	if(argc != 1){
		fprint(2, "usage: printnetkey user\n");
		exits("usage");
	}
	if(memchr(u, '\0', NAMELEN) == 0)
		error("bad user name");
	key = findnetkey(u);
	if(!key)
		error("%s has no netkey\n", u);
	print("user %s: net key %K\n", u, key);
	exits(0);
}

void
usage(void)
{
	fprint(2, "usage: printnetkey user\n");
	exits("usage");
}
