/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>

void
usage(void)
{
	fprint(2, "usage: %s [-q] [-t seconds] command\n", argv0);
	exits("usage");
}

struct {
	char *resp;
	int ok;
} tab[] =
    {
     {"ok\n", 1},
     {"connect\n", 1},
     {"no carrier\n", 0},
     {"no dialtone\n", 0},
     {"error\n", 0},
     {"busy\n", 0},
     {"no answer\n", 0},
     {"delayed\n", 0},
     {"blacklisted\n", 0},
};

int
writewithoutcr(int fd, char *p, int i)
{
	char *q, *e;

	/* dump cr's */
	for(e = p + i; p < e;) {
		q = memchr(p, '\r', e - p);
		if(q == nil)
			break;
		if(q > p)
			if(write(fd, p, q - p) < 0)
				return -1;
		p = q + 1;
	}
	if(p < e)
		if(write(fd, p, e - p) < 0)
			return -1;
	return i;
}

int
readln(int fd, char *buf, int n)
{
	int c, i, sofar;

	sofar = 0;
	buf[sofar] = 0;
	while(sofar < n - 1) {
		i = read(fd, buf + sofar, 1);
		if(i <= 0)
			return i;
		c = buf[sofar];
		if(c == '\r')
			continue;
		sofar++;
		if(c == '\n')
			break;
	}
	buf[sofar] = 0;
	return sofar;
}

void
docmd(char *cmd, int timeout, int quiet, int consfd)
{
	char buf[4096];
	int i;
	char *p, *cp;

	if(timeout == 0) {
		if(*cmd == 'd' || *cmd == 'D')
			timeout = 2 * 60;
		else
			timeout = 5;
	}

	p = smprint("at%s\r", cmd);
	for(cp = p; *cp; cp++) {
		write(1, cp, 1);
		sleep(100);
	}
	free(p);

	alarm(timeout * 1000);
	for(;;) {
		i = readln(0, buf, sizeof(buf));
		if(i <= 0) {
			rerrstr(buf, sizeof buf);
			exits(buf);
		}
		if(!quiet)
			writewithoutcr(consfd, buf, i);
		for(i = 0; i < nelem(tab); i++)
			if(cistrcmp(buf, tab[i].resp) == 0) {
				if(tab[i].ok)
					goto out;
				else
					exits(buf);
			}
	}
out:
	alarm(0);
}

void
main(int argc, char **argv)
{
	int timeout;
	int quiet;
	int i;
	int consfd;

	timeout = 0;
	quiet = 0;
	ARGBEGIN
	{
	case 't':
		timeout = atoi(EARGF(usage()));
		break;
	case 'q':
		quiet = 1;
		break;
	default:
		usage();
	}
	ARGEND;

	if(argc < 1)
		usage();

	consfd = open("/dev/cons", ORDWR);
	if(consfd < 0)
		sysfatal("opening /dev/cons: %r");

	for(i = 0; i < argc; i++)
		docmd(argv[i], timeout, quiet, consfd);

	exits(0);
}
