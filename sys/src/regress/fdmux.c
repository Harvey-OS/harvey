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
main(void)
{
	int pid, ctl, m, s;
	char c[1];
	if (bind("#<", "/tmp", MAFTER) < 0)
		sysfatal(smprint("%r"));

	if ((ctl = open("/tmp/ctl", ORDWR)) < 0)
		sysfatal(smprint("%r"));
	if ((m = open("/tmp/data", ORDWR)) < 0)
		sysfatal(smprint("%r"));
	if ((s = open("/tmp/data1", ORDWR)) < 0)
		sysfatal(smprint("%r"));

	pid = fork();
	if (pid < 0)
		sysfatal(smprint("%r"));

	if (pid == 0) {
		dup(s, 0);
		dup(s, 1);
		dup(s, 2);
		close(s);
		execl("/bin/rc", "rc", "-m", "/rc/lib/rcmain", "-i", nil);
		sysfatal(smprint("%r"));
	}

	pid = fork();
	if (pid < 0)
		sysfatal(smprint("%r"));
	if (pid == 0) {
		int amt;
		while ((amt = read(m, c, 1)) > -1) {
			/* fdmux indicates child eof with a zero byte read.
			 * so ignore it.
			 */
			if (amt == 0)
				continue;
			if (write(1, c, 1) < 1)
				sysfatal(smprint("%r"));
		}
	}

	for(;;) {
		int amt;
		amt = read(0, c, 1);
		if (amt < 0)
			break;
		/* we indicated ^D to the child with a zero byte write. */
		if (write(m, c, amt) < amt)
			sysfatal(smprint("%r"));
		/* for the record: it's totally legal to try to keep reading after a 0 byte
		 * read, even on unix. This is one of those things many people don't seem
		 * to know.
		 */
	}
		
	
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
	exits(nil);
}
