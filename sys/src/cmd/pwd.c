#include <u.h>
#include <libc.h>
/*
 * Print working (current) directory
 */

void
main(int argc, char *argv[])
{
	char pathname[512];

	USED(argc, argv);
	if(getwd(pathname, sizeof(pathname)) == 0) {
		fprint(2, "pwd: %r\n");
		exits("getwd");
	}
	print("%s\n", pathname);
	exits(0);
}
