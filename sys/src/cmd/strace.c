/* Copyright (c) 2016 Google Inc., All Rights Reserved.
 * Ron Minnich <rminnich@google.com>
 * See LICENSE for details. */

#include <stdlib.h>
#include <stdio.h>
#include <parlib/parlib.h>
#include <unistd.h>
#include <signal.h>
#include <iplib/iplib.h>
#include <iplib/icmp.h>
#include <ctype.h>
#include <pthread.h>
#include <parlib/spinlock.h>
#include <parlib/timing.h>
#include <parlib/tsc-compat.h>
#include <parlib/printf-ext.h>
#include <benchutil/alarm.h>
#include <ndblib/ndb.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/wait.h>

void usage(void)
{
	fprintf(stderr, "usage: strace command [args...]\n");
	exit(1);
}

void main(int argc, char **argv, char **envp)
{
	int fd;
	int pid;
	int amt;
	static char p[2 * MAX_PATH_LEN];
	static char buf[16384];
	struct syscall sysc;
	char *prog_name = argv[1];


	if (argc < 2)
		usage();
	if ((*argv[1] != '/') && (*argv[1] != '.')) {
		snprintf(p, sizeof(p), "/bin/%s", argv[1]);
		prog_name = p;
	}

	pid = sys_proc_create(prog_name, strlen(prog_name), argv + 1, envp,
	                      PROC_DUP_FGRP);
	if (pid < 0) {
		perror("proc_create");
		exit(-1);
	}
	/* We need to wait on the child asynchronously.  If we hold a ref (as the
	 * parent), the child won't proc_free and that won't hangup/wake us from a
	 * read. */
	syscall_async(&sysc, SYS_waitpid, pid, NULL, 0, 0, 0, 0);

	snprintf(p, sizeof(p), "/proc/%d/ctl", pid);
	fd = open(p, O_WRITE);
	if (fd < 0) {
		fprintf(stderr, "open %s: %r\n", p);
		exit(1);
	}

	snprintf(p, sizeof(p), "straceall");
	if (write(fd, p, strlen(p)) < strlen(p)) {
		fprintf(stderr, "write to ctl %s %d: %r\n", p, fd);
		exit(1);
	}
	close(fd);

	snprintf(p, sizeof(p), "/proc/%d/strace", pid);
	fd = open(p, O_READ);
	if (fd < 0) {
		fprintf(stderr, "open %s: %r\n", p);
		exit(1);
	}

	/* now that we've set up the tracing, we can run the process.  isn't it
	 * great that the process doesn't immediately start when you make it? */
	sys_proc_run(pid);

	while ((amt = read(fd, buf, sizeof(buf))) > 0) {
		if (write(1, buf, amt) < amt) {
			fprintf(stderr, "Write to stdout: %r\n");
			exit(1);
		}
	}
	if ((amt < 0) && (errno != ESRCH))
		fprintf(stderr, "Read fd %d for %s: %r\n", fd, p);
}
