/*
 * popen, pclose - open and close pipes (Plan 9 version)
 */

#include <u.h>
#include <libc.h>
#include <stdio.h>

enum { Stdin, Stdout, };
enum {
	Rd,
	Wr,
	Maxfd = 200,
};

typedef struct {
	long	pid;
	char	*sts;		/* malloced; valid if non-nil */
} Pipe;

static Pipe pipes[Maxfd];

static int
_pipefd(int rfd, int wfd)
{
	close(wfd);
	return rfd;
}

FILE *
popen(char *file, char *mode)
{
	int pipedes[2];
	long pid;

	if (pipe(pipedes) < 0)		/* cat's got the last pipe */
		return 0;
	if ((pid = fork()) < 0) {	/* can't fork */
		close(pipedes[Rd]);
		close(pipedes[Wr]);
		return 0;
	}
	/*
	 * The pipe was created and the fork succeeded.
	 * Now fiddle the file descriptors in both processes.
	 */
	if (pid == 0) {			/* child process */
		int sts;

		/*
		 * If the mode is 'r', the child writes on stdout so the
		 * parent can read on its stdin from the child.
		 * If the mode is not 'r', the child reads on stdin so the
		 * parent can write on its stdout to the child.
		 */
		if (mode[0] == 'r')		/* read from child */
			sts = dup(pipedes[Wr], Stdout);
		else				/* write to child */
			sts = dup(pipedes[Rd], Stdin);
		if (sts < 0)			/* couldn't fiddle fd's */
			_exits("no pipe");
		close(pipedes[Rd]);
		close(pipedes[Wr]);
		execl("/bin/rc", "rc", "-Ic", file, (char *)nil);
		_exits("no /bin/rc");		/* no shell */
		/* NOTREACHED */
		return 0;
	} else {			/* parent process */
		int fd;

		/*
		 * If the mode is 'r', the parent reads on its stdin the child;
		 * otherwise the parent writes on its stdout to the child.
		 */
		if (mode[0] == 'r')	/* read from child */
			fd = _pipefd(pipedes[Rd], pipedes[Wr]);
		else
			fd = _pipefd(pipedes[Wr], pipedes[Rd]);
		if (fd >= 0 && fd < Maxfd) {
			Pipe *pp = pipes + fd;

			pp->pid = pid;		/* save fd's child's pid */
			free(pp->sts);
			pp->sts = nil;
		}
		return fdopen(fd, mode[0] == 'r'? "w": "r");
	}
}

static volatile int waiting;

static int
gotnote(void *, char *note)
{
	if (strcmp(note, "interrupt") == 0)
		if (waiting)
			return 1;	/* NCONT */
	return 0;			/* not a known note: NDFLT */
}

int
pclose(FILE *fp)
{
	int fd;
	int pid;		/* pid, wait status for some child */
	Pipe *fpp, *app = nil, *spp;
	Waitmsg *wm;
	static int registered;

	fd = fileno(fp);
	if (fd < 0 || fd >= Maxfd)
		return -1;	// "fd out of range";
	fpp = pipes + fd;
	if (fpp->pid <= 0)
		return -1;	// "no child process for fd";
	close(fd);
	/*
	 * Ignore notes in case this process was catching them.
	 * Otherwise both this process and its child(ren) would
	 * catch these notes.
	 * Ideally I suppose popen should ignore the notes.
	 */
	if (!registered) {
		atnotify(gotnote, 1);
		registered = 1;
	}
	waiting = 1;
	/*
	 * Wait for fd's child to die.
	 * ``Bring out your dead!''
	 */
	while (fpp->sts == nil && (wm = wait()) != nil) {
		pid = wm->pid;
		/*
		 * See if any fd is attached to this corpse;
		 * if so, give that fd its wait status.
		 */
		if (pid == fpp->pid)	/* quick check */
			app = fpp;
		else
			for (spp = pipes; spp < pipes + Maxfd; spp++)
				if (pid == spp->pid) {
					app = spp;
					break;
				}
		if (app != nil) {
			/* record pid's status, possibly for later use */
			free(app->sts);
			app->sts = strdup(wm->msg);
		}
		free(wm);
	}
	waiting = 0;
	return fpp->sts != nil && fpp->sts[0] != '\0'? 1: 0;
}
