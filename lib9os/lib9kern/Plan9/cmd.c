#include	"dat.h"
#include	"fns.h"
#include	"error.h"

extern	void	vstack(void*);

/*
 * all this is for the benefit of devcmd.
 * i hope it's grateful.
 */

typedef struct Targ Targ;
struct Targ
{
	int	fd[3];	/* standard input, output and error */
	int	wfd;
	int*	spin;
	char**	args;
	char*	dir;
	int	pid;
	int	nice;
};

/*
 * called by vstack once it has moved to
 * the unshared stack in the new process.
 */
void
exectramp(Targ *t)
{
	int *fd, i, nfd;
	char filename[128], err[ERRMAX], status[2*ERRMAX];

	t->pid = getpid();
	*t->spin = 0;	/* allow parent to proceed: can't just rendezvous: see below */
	fd = t->fd;

	snprint(filename, sizeof(filename), "#d/%d", t->wfd);
	t->wfd = open(filename, OWRITE|OCEXEC);
	/* if it failed, we'll manage */

	nfd = MAXNFD;	/* TO DO: should read from /fd */
	for(i = 0; i < nfd; i++)
		if(i != fd[0] && i != fd[1] && i != fd[2] && i != t->wfd)
			close(i);

	if(fd[0] != 0){
		dup(fd[0], 0);
		close(fd[0]);
	}
	if(fd[1] != 1){
		dup(fd[1], 1);
		close(fd[1]);
	}
	if(fd[2] != 2){
		dup(fd[2], 2);
		close(fd[2]);
	}

	if(t->dir != nil && chdir(t->dir) < 0){
		if(t->wfd > 0)
			fprint(t->wfd, "chdir: %s: %r", t->dir);
		_exits("bad dir");
	}
	if(t->nice)
		oslopri();

	exec(t->args[0], t->args);
	err[0] = 0;
	errstr(err, sizeof(err));
	if(t->args[0][0] != '/' && t->args[0][0] != '#' &&
	   strncmp(t->args[0], "../", 3) != 0 && strncmp(t->args[0], "./", 2) != 0 &&
	   strlen(t->args[0])+5 < sizeof(filename)){
		snprint(filename, sizeof(filename), "/bin/%s", t->args[0]);
		exec(filename, t->args);
		errstr(err, sizeof(err));
	}
	snprint(status, sizeof(status), "%s: can't exec: %s", t->args[0], err);
	if(t->wfd > 0)
		write(t->wfd, status, strlen(status));
	_exits(status);
}

void*
oscmd(char **args, int nice, char *dir, int *fd)
{
	Targ *t;
	int spin, *spinptr, fd0[2], fd1[2], fd2[2], wfd[2], n;
	Dir *d;

	up->genbuf[0] = 0;
	t = mallocz(sizeof(*t), 1);
	if(t == nil)
		return nil;
	t->args = args;
	t->dir = dir;
	t->nice = nice;
	fd0[0] = fd0[1] = -1;
	fd1[0] = fd1[1] = -1;
	fd2[0] = fd2[1] = -1;
	wfd[0] = wfd[1] = -1;
	if(dir != nil){
		d = dirstat(dir);
		if(d == nil)
			goto Error;
		free(d);
	}
	if(pipe(fd0) < 0 || pipe(fd1) < 0 || pipe(fd2) < 0 || pipe(wfd) < 0)
		goto Error;

	spinptr = &spin;
	spin = 1;

	t->fd[0] = fd0[0];
	t->fd[1] = fd1[1];
	t->fd[2] = fd2[1];
	t->wfd = wfd[1];
	t->spin = spinptr;
	switch(rfork(RFPROC|RFMEM|RFREND|RFNOTEG|RFFDG|RFNAMEG|RFENVG)) {
	case -1:
		goto Error;
	case 0:
		/* if child returns first from rfork, its call to vstack replaces ... */
		vstack(t);
		/* ... parent's return address from rfork and parent returns here */
	default:
		/* if parent returns first from rfork, it comes here */
		/* can't call anything: on shared stack until child releases spin in exectramp */
		while(*spinptr)
			;
		break;
	}

	close(fd0[0]);
	close(fd1[1]);
	close(fd2[1]);
	close(wfd[1]);

	n = read(wfd[0], up->genbuf, sizeof(up->genbuf)-1);
	close(wfd[0]);
	if(n > 0){
		close(fd0[1]);
		close(fd1[0]);
		close(fd2[0]);
		up->genbuf[n] = 0;
		errstr(up->genbuf, sizeof(up->genbuf));
		free(t);
		return nil;
	}

	fd[0] = fd0[1];
	fd[1] = fd1[0];
	fd[2] = fd2[0];
	return t;

Error:
	errstr(up->genbuf, sizeof(up->genbuf));	/* save the message before close */
	close(fd0[0]);
	close(fd0[1]);
	close(fd1[0]);
	close(fd1[1]);
	close(fd2[0]);
	close(fd2[1]);
	close(wfd[0]);
	close(wfd[1]);
	free(t);
	errstr(up->genbuf, sizeof(up->genbuf));
	return nil;
}

int
oscmdkill(void *a)
{
	Targ *t = a;

	return postnote(PNGROUP, t->pid, "kill");
}

int
oscmdwait(void*, char *buf, int n)
{
	return await(buf, n);
}

void
oscmdfree(void *a)
{
	free(a);
}
