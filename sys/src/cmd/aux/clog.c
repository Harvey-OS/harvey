/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/* clog - log console */
#include <u.h>
#include <libc.h>
#include <bio.h>

char *argv0;

int
openlog(char *name)
{
	int fd;

	fd = open(name, OWRITE);
	if(fd < 0)
		fd = create(name, OWRITE, DMAPPEND|0666);
	if(fd < 0){
		fprint(2, "%s: can't open %s: %r\n", argv0, name);
		return -1;
	}
	seek(fd, 0, 2);
	return fd;
}

void
main(int argc, char **argv)
{
	Biobuf in;
	int fd;
	char *p, *t;
	char buf[Bsize];

	argv0 = argv[0];
	if(argc < 3){
		fprint(2, "usage: %s console logfile \n", argv0);
		exits("usage");
	}

	fd = open(argv[1], OREAD);
	if(fd < 0){
		fprint(2, "%s: can't open %s: %r\n", argv0, argv[1]);
		exits("open");
	}
	Binit(&in, fd, OREAD);

	fd = openlog(argv[2]);

	for(;;){
		if(p = Brdline(&in, '\n')){
			p[Blinelen(&in)-1] = 0;
			t = ctime(time(0));
			t[19] = 0;
			while(fprint(fd, "%s: %s\n", t, p) < 0) {
				close(fd);
				sleep(500);
				fd = openlog(argv[2]);
			}
		} else if(Blinelen(&in) == 0)	/* true eof or error */
			break;
		/* discard partial buffer? perhaps due to very long line */
		else if (Bread(&in, buf, sizeof buf) < 0)
			break;
	}
	exits(0);
}
