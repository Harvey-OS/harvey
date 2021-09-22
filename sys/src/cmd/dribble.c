/*
 * dribble - copy with delays, to share bandwidth (disk, network, etc.)
 */
#include <u.h>
#include <libc.h>

void
dribble(int f, char *s)
{
	char buf[16*1024];
	long n;

	while ((n = read(f, buf, sizeof buf)) > 0) {
		if (write(1, buf, n) != n)
			sysfatal("write error copying %s: %r", s);
		sleep(150);
	}
	if (n < 0)
		sysfatal("error reading %s: %r", s);
}


void
main(int argc, char *argv[])
{
	int f, i;

	argv0 = "dribble";
	if (argc == 1)
		dribble(0, "<stdin>");
	else
		for (i = 1; i < argc; i++) {
			f = open(argv[i], OREAD);
			if (f < 0)
				sysfatal("can't open %s: %r", argv[i]);
			else {
				dribble(f, argv[i]);
				close(f);
			}
		}
	exits(0);
}


