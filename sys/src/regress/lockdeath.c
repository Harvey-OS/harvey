#include <u.h>
#include <libc.h>

int np = 32;
int niter = 65536;
int debug = 0;
char data[8192];
int i;
int id;
int intr = 0;

void
handler(void *v, char *s)
{
	fprint(2, "%d: %d iterations\n", id, i);
	if (intr++ < 16)
		noted(NCONT);
	fprint(2, "FAIL: too many interrupts\n");
	exits("FAIL");
}

void
main(int argc, char *argv[])
{
	int pid;
	int fd;

	if (notify(handler)){
		sysfatal("FAIL: notify: %r");
	}

	if (argc > 1)
		np = strtoull(argv[1], 0, 0);

	if (argc > 2)
		niter = strtoull(argv[2], 0, 0);

	debug = argc > 3;

	fd = open("#Z/qlock", ORDWR);
	if (fd < 0)
		sysfatal("FAIL: qlock: %r");

	print("Running with %d kids %d times\n", np, niter);

	for(i = 0, id = 0; i < np-1; i++, id++) {
		pid = fork();
		if (pid == 0)
			break;
		if (pid < 0)
			sysfatal("FAIL: fork");
	}

	if (debug) fprint(2, "Forked. pid %d\n", getpid());

	for(i = 0; i < niter; i++) {
		if (read(fd, data, 1) < 1)
			fprint(2, "%d: iter %d: %r", id, i);
	}
	print("PASS\n");
	exits(nil);
}
