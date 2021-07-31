#include "common.h"

/* make a stream to a child process */
extern stream *
instream(void)
{
	stream *rv;
	int pfd[2];

	if ((rv = (stream *)malloc(sizeof(stream))) == 0)
		return 0;
	if (pipe(pfd) < 0)
		return 0;
	if(Binit(&rv->bb, pfd[1], OWRITE) < 0){
		close(pfd[0]);
		close(pfd[1]);
		return 0;
	}
	rv->fp = &rv->bb;
	rv->fd = pfd[0];	
	return rv;
}

/* make a stream from a child process */
extern stream *
outstream(void)
{
	stream *rv;
	int pfd[2];

	if ((rv = (stream *)malloc(sizeof(stream))) == 0)
		return 0;
	if (pipe(pfd) < 0)
		return 0;
	if (Binit(&rv->bb, pfd[0], OREAD) < 0){
		close(pfd[0]);
		close(pfd[1]);
		return 0;
	}
	rv->fp = &rv->bb;
	rv->fd = pfd[1];	
	return rv;
}

extern void
stream_free(stream *sp)
{
	int fd;

	close(sp->fd);
	fd = Bfildes(sp->fp);
	Bterm(sp->fp);
	close(fd);
	free((char *)sp);
}

/* start a new process */
extern process *
proc_start(char *cmd, stream *inp, stream *outp, stream *errp, int newpg, int none)
{
	process *pp;
	int i;

	if ((pp = (process *)malloc(sizeof(process))) == 0) {
		if (inp != 0)
			stream_free(inp);
		if (outp != 0)
			stream_free(outp);
		if (errp != 0)
			stream_free(errp);
		return 0;
	}
	pp->std[0] = inp;
	pp->std[1] = outp;
	pp->std[2] = errp;
	switch (pp->pid = fork()) {
	case -1:
		proc_free(pp);
		return 0;
	case 0:
		if(newpg)
			newprocgroup();
		if(none)
			becomenone();
		for (i=0; i<3; i++)
			if (pp->std[i] != 0)
				close(Bfildes(pp->std[i]->fp));
		for (i=0; i<3; i++)
			if (pp->std[i] != 0)
				dup(pp->std[i]->fd, i);
				
		for (i = 3; i < nofile; i++)
			close(i);
		execl("/bin/rc", "rc", "-c", cmd, 0);
		perror("proc_start");
		exits("proc_start");
	default:
		for (i=0; i<nsysfile; i++)
			if (pp->std[i] != 0) {
				close(pp->std[i]->fd);
				pp->std[i]->fd = -1;
			}
		return pp;
	}
}

/* wait for a process to stop */
extern int
proc_wait(process *pp)
{
	Waitmsg status;
	int pid;
	char err[ERRLEN];

	for(;;){
		pid = wait(&status);
		if(pid < 0){
			errstr(err);
			if(strstr(err, "interrupt") == 0)
				break;
		}
		if (pid==pp->pid)
			break;
	}
	pp->pid = -1;
	pp->status = status.msg[0];
	return pp->status;
}

static int junk;

/* free a process */
extern int
proc_free(process *pp)
{
	int i;

	if(pp->std[1] == pp->std[2])
		pp->std[2] = 0;		/* avoid freeing it twice */
	for (i = 0; i < 3; i++)
		if (pp->std[i])
			stream_free(pp->std[i]);
	if (pp->pid >= 0){
		USE(proc_wait(pp));
	}
	free((char *)pp);
	return 0;
}

/* kill a process */
extern int
proc_kill(process *pp)
{
	return syskill(pp->pid);
}
