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
#include <bio.h>

int debug = 0;
int outf = 2;

void
usage(void)
{
	fprint(2, "Usage: ratrace [-o file] [-d] [-c cmd [arg...]] | [pid]\n");
	exits("usage");
}

void
hang(void)
{
	int me, amt;
	char *myctl;
	static char hang[] = "hang";

	myctl = smprint("/proc/%d/ctl", getpid());
	me = open(myctl, OWRITE);
	if (me < 0)
		sysfatal("can't open %s: %r", myctl);
	amt = write(me, hang, sizeof hang - 1);
	if (amt < (sizeof(hang) - 1))
		sysfatal("Write 'hang' to %s: %r", myctl);
	close(me);
	free(myctl);
}

void
main(int argc, char **argv)
{
	int pid, fd, amt;
	char *cmd = nil;
	char **args = nil;
	static char p[512], buf[16384];
	/*
	 * don't bother with fancy arg processing, because it picks up options
	 * for the command you are starting.  Just check for -c as argv[1]
	 * and then take it from there.
	 */
	++argv;
	--argc;
	while (argv[0][0] == '-') {
		switch(argv[0][1]) {
		case 'c':
			if (argc < 2)
				usage();
			cmd = strdup(argv[1]);
			args = &argv[1];
			break;
		case 'd':
			debug = 1;
			break;
		case 'o':
			if (argc < 2)
				usage();
			outf = create(argv[1], OWRITE, 0666);
			if (outf < 0)
				sysfatal(smprint("%s: %r", argv[1]));
			++argv;
			--argc;
			break;
		default:
			usage();
		}
		++argv;
		--argc;
	}

	/* run a command? */
	if(cmd) {
		pid = fork();
		if (pid < 0)
			sysfatal("fork failed: %r");
		if(pid == 0) {
			hang();
			exec(cmd, args);
			if(cmd[0] != '/')
				exec(smprint("/bin/%s", cmd), args);
			sysfatal("exec %s failed: %r", cmd);
		}
	} else {
		if(argc != 1)
			usage();
		pid = atoi(argv[0]);
	}

	/* the child is hanging, now we can start it up and collect the output. */
	snprint(p, sizeof(p), "/proc/%d/ctl", pid);
	fd = open(p, OWRITE);
	if (fd < 0) {
		sysfatal("open %s: %r\n", p);
	}

	snprint(p, sizeof(p), "straceall");
	if (write(fd, p, strlen(p)) < strlen(p)) {
		sysfatal("write to ctl %s %d: %r\n", p, fd);
	}

	// This kind of sucks. We have to wait just a bit or we get
	// an error when we send the start command. FIXME.
	sleep(1000);
	// unset hang. We only need it initially.
	snprint(p, sizeof(p), "nohang");
	if (write(fd, p, strlen(p)) < strlen(p)) {
		sysfatal("write to ctl %s %d: %r\n", p, fd);
	}

	snprint(p, sizeof(p), "start");
	if (write(fd, p, strlen(p)) < strlen(p)) {
		sysfatal("write to ctl %s %d: %r\n", p, fd);
	}
	close(fd);

	snprint(p, sizeof(p), "/proc/%d/strace", pid);
	fd = open(p, OREAD);
	if (fd < 0) {
		sysfatal("open %s: %r\n", p);
	}

	while ((amt = read(fd, buf, sizeof(buf))) > 0) {
		if (write(outf, buf, amt) < amt) {
			sysfatal("Write to stdout: %r\n");
		}
	}
	if (amt < 0)
		fprint(2, "Read fd %d for %s: %r\n", fd, p);


}
