#include <u.h>
#include <libc.h>
	
void
main(int argc, char *argv[])
{
	char buf[ERRLEN];
	int r;

	if(argc < 2  || argc > 3) {
		fprint(2, "usage: unmount mountpoint\n");
		fprint(2, "       unmount mounted mountpoint\n");
		exits("usage");
	}

	/* unmount take arguments in the same order as mount */
	if(argc < 3)
		r = unmount(0, argv[1]);
	else
		r = unmount(argv[1], argv[2]);

	if(r < 0) {
		errstr(buf);
		fprint(2, "unmount: %s\n", buf);
		exits(buf);
	}
	exits(0);
}
