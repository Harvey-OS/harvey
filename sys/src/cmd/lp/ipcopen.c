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

int ppid;

/*
 * predefined
 */
void pass(int from, int to);


/*
 *  Connect to given datakit port
 */
main(int argc, char *argv[])
{
	int fd0, fd1;
	int cpid;
	char c;
	char *cp, *devdir, *buf;

	if (argc != 4) {
		fprint(2, "usage: %s destination network service\n", argv[0]);
		exits("incorrect number of arguments");
	}
	if(!(cp = malloc((long)(strlen(argv[1])+strlen(argv[2])+strlen(argv[3])+8)))) {
		perror("malloc");
		exits("malloc failed");
	}
	sprint(cp, "%s!%s!%s", argv[2], argv[1], argv[3]);
	if (dial(cp, &devdir, 0) < 0) {
		fprint(2, "dialing %s\n", cp);
		perror("dial");
		exits("can't dial");
	}

	/*
	 * Initialize the input fd, and copy bytes.
	 */

	if(!(buf = malloc((long)(strlen(devdir)+6)))) {
		perror("malloc");
		exits("malloc failed");
	}
	sprint(buf, "%s/data", devdir);
	fd0=open(buf, OREAD);
	fd1=open(buf, OWRITE);
	if(fd0<0 || fd1<0) {
		print("can't open", buf);
		exits("can't open port");
	}
	ppid = getpid();
	switch(cpid = fork()){
	case -1:
		perror("fork failed");
		exits("fork failed");
	case 0:
		close(0);
		close(fd1);
		pass(fd0, 1);	/* from remote */
		hangup(fd0);
		close(1);
		close(fd0);
		exits("");
	default:
		close(1);
		close(fd0);
		pass(0, fd1);	/* to remote */
		hangup(fd1);
		close(0);
		close(fd1);
		exits("");
	}
}

void
pass(int from, int to)
{
	char buf[1024];
	int ppid, cpid;
	int n, tot = 0; 

	while ((n=read(from, buf, sizeof(buf))) > 0) {
		if (n==1 && tot==0 && *buf=='\0')
			break;
		tot += n;
		if (write(to, buf, n)!=n) {
			perror("pass write error");
			exits("pass write error");
		}
	}
}
