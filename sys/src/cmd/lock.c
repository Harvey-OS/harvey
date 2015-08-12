/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * lock - keep a lock alive while a command runs
 */

#include <u.h>
#include <libc.h>
#include <ctype.h>

static int debug;
static int lockwait;

void error(char *);
void notifyf(void *c, char *);

static void
usage(void)
{
	fprint(2, "usage: %s [-dw] lock [command [file]...]\n", argv0);
	exits("usage");
}

static Waitmsg *
waitfor(int pid)
{
	char err[ERRMAX];
	Waitmsg *w;

	for(;;) {
		w = wait();
		if(w == nil) {
			errstr(err, sizeof err);
			if(strcmp(err, "interrupted") == 0)
				continue;
			return nil;
		}
		if(w->pid == pid)
			return w;
	}
}

static int
openlock(char *lock)
{
	int lckfd;
	Dir *dir;

	/* first ensure that the lock file has the lock bit set */
	dir = dirstat(lock);
	if(dir == nil)
		sysfatal("can't stat %s: %r", lock);
	if(!(dir->mode & DMEXCL)) {
		dir->mode |= DMEXCL;
		dir->qid.type |= QTEXCL;
		if(dirwstat(lock, dir) < 0)
			sysfatal("can't make %s exclusive access: %r", lock);
	}
	free(dir);

	if(lockwait)
		while((lckfd = open(lock, ORDWR)) < 0)
			sleep(1000);
	else
		lckfd = open(lock, ORDWR);
	if(lckfd < 0)
		sysfatal("can't open %s read/write: %r", lock);
	return lckfd;
}

void
main(int argc, char *argv[])
{
	int fd, lckfd, lckpid, cmdpid;
	char *cmd, *p, *lock;
	char **args;
	char *argarr[2];
	Waitmsg *w;

	ARGBEGIN
	{
	case 'd':
		++debug;
		break;
	case 'w':
		++lockwait;
		break;
	default:
		usage();
		break;
	}
	ARGEND

	if(argc < 1)
		usage();
	if(argc == 1) {
		args = argarr;
		args[0] = cmd = "rc";
		args[1] = nil;
	} else {
		cmd = argv[1];
		args = &argv[1];
	}

	/* set up lock and process to keep it alive */
	lock = argv[0];
	lckfd = openlock(lock);
	lckpid = fork();
	switch(lckpid) {
	case -1:
		error("fork");
	case 0:
		/* keep lock alive until killed */
		for(;;) {
			sleep(60 * 1000);
			seek(lckfd, 0, 0);
			fprint(lckfd, "\n");
		}
	}

	/* spawn argument command */
	cmdpid = rfork(RFFDG | RFREND | RFPROC | RFENVG);
	switch(cmdpid) {
	case -1:
		error("fork");
	case 0:
		fd = create("/env/prompt", OWRITE, 0666);
		if(fd >= 0) {
			fprint(fd, "%s%% ", lock);
			close(fd);
		}
		exec(cmd, args);
		if(cmd[0] != '/' && strncmp(cmd, "./", 2) != 0 &&
		   strncmp(cmd, "../", 3) != 0)
			exec(smprint("/bin/%s", cmd), args);
		error(cmd);
	}

	notify(notifyf);

	w = waitfor(cmdpid);
	if(w == nil)
		error("wait");

	postnote(PNPROC, lckpid, "die");
	waitfor(lckpid);
	if(w->msg[0]) {
		p = utfrune(w->msg, ':');
		if(p && p[1])
			p++;
		else
			p = w->msg;
		while(isspace(*p))
			p++;
		fprint(2, "%s: %s  # status=%s\n", argv0, cmd, p);
	}
	exits(w->msg);
}

void
error(char *s)
{
	fprint(2, "%s: %s: %r\n", argv0, s);
	exits(s);
}

void
notifyf(void *a, char *s)
{
	USED(a);
	if(strcmp(s, "interrupt") == 0)
		noted(NCONT);
	noted(NDFLT);
}
