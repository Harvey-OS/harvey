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

int cons = -1;
void unraw(void)
{
	if (write(cons, "rawoff", 6) < 6)
		fprint(2, "rawoff: %r");
}
void
main(void)
{
	int pid, ctl, m, s;
	char c[1];

	cons = open("/dev/consctl", OWRITE);
	if (cons < 0)
		sysfatal(smprint("%r"));
	if (write(cons, "rawon", 5) < 5)
		sysfatal(smprint("%r"));
	atexit(unraw);

	if (bind("#<", "/dev", MBEFORE) < 0)
		sysfatal(smprint("%r"));

	if ((ctl = open("/dev/consctl", ORDWR)) < 0)
		sysfatal(smprint("%r"));
	if ((m = open("/dev/m", ORDWR)) < 0)
		sysfatal(smprint("%r"));
	if ((s = open("/dev/cons", ORDWR)) < 0)
		sysfatal(smprint("%r"));

	pid = fork();
	if (pid < 0)
		sysfatal(smprint("%r"));

	if (pid == 0) {
		dup(s, 0);
		dup(s, 1);
		dup(s, 2);
		close(s);
		close(m);
		execl("/bin/rc", "rc", "-m", "/rc/lib/rcmain", "-i", nil);
		sysfatal(smprint("%r"));
	}

	close(s);

	pid = fork();
	if (pid < 0)
		sysfatal(smprint("%r"));
	if (pid == 0) {
		int amt;
		while ((amt = read(m, c, 1)) > 0) {
			if (write(1, c, 1) < 1)
				sysfatal(smprint("%r"));
		}
	}

	for(;;) {
		int amt;
		amt = read(0, c, 1);
		if (amt < 0) {
			fprint(2, "EOF from fd 0\n");
			break;
		}
		/* we indicated ^D to the child with a zero byte write. */
		if (c[0] == 4) { //^D
			write(m, c, 0);
			write(1, "^D", 2);
			continue;
		}
		if (c[0] == 3) { //^C
			write(1, "^C", 2);
			write(ctl, "n", 1);
			continue;
		}
		if (c[0] == 26) { //^Z
			write(1, "^Z", 2);
			write(ctl, "s", 1);
			continue;
		}
		if (c[0] == '\r') {
			write(1, c, 1);
			c[0] = '\n';
		}
		if (write(1, c, 1) < 1)
			sysfatal(smprint("%r"));
		if (write(m, c, amt) < amt) {
			fprint(2, "%r");
			break;
		}
		/* for the record: it's totally legal to try to keep reading after a 0 byte
		 * read, even on unix. This is one of those things many people don't seem
		 * to know.
		 */
	}
		
	if (write(cons, "rawoff", 6) < 6)
		fprint(2, "rawoff: %r");
#if 0
	pid = rfork(RFMEM|RFPROC);
	if (pid < 0) {
		print("FAIL: rfork\n");
		exits("FAIL");
	}
	if (pid > 0) {
		c++;
	}
	if (pid == 0) {
		print("PASS\n");
		exits(nil);
	}

	if (c > 1) {
		print("FAIL\n");
		exits("FAIL");
	}
#endif
}
