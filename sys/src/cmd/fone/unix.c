#include "all.h"
#define	ushort	_ushort
#define	ulong	_ulong
#define	fd_set	_fd_set
#include <sys/fcntl.h>
#undef	ushort
#undef	ulong
#undef	fd_set
#define	_MODERN_C
#include <sys/termio.h>

extern int	mkfifo(char*, int);
extern int	atexit(void(*)(void));
extern char *	getenv(char*);

char *	dsrvname = "/dev/fone";
char *	fname = "/dev/ttyd2";
char *	shname = "/bin/sh";
char *	telcmd = "/v/bin/tel";

void
openfone(void)
{
	struct termio ctl;

	if(debug)
		print("openfone...");
	fonefd = open(fname, O_NOCTTY|ORDWR);
	if(fonefd < 0){
		perror(fname);
		exits("open");
	}
	if(debug)
		print("memset...");
	memset(&ctl, 0, sizeof ctl);
	ctl.c_cflag = CS8|CREAD|CLOCAL|B19200;
	ctl.c_line = LDISC0;
	ctl.c_cc[VMIN] = 1;
	ctl.c_cc[VTIME] = 0;
	if(debug)
		print("flush...");
	ioctl(fonefd, TCFLSH, (void *)2);
	if(debug)
		print("ioctl...");
	ioctl(fonefd, TCSETA, &ctl);
	if(debug)
		print("OK\n");
}

void
killproc(int pid)
{
	kill(pid, 9);
}

int
fork2(void)
{
	int i, j;
	char buf[20], *p;
	int w;

	i = fork();
	if(i != 0){
		if(i < 0)
			return -1;
		do; while((j=wait((void*)&w))!=-1 && j!=i);
		if(j == i)
			return w; /* pid of final child */
		return j;
	}
	i = fork();
	if(i != 0)
		_exit(i);
	return 0;
}

char *
ascnow(void)
{
	long lt;

	time(&lt);
	return ctime(&lt);
}

void
msleep(int msec)
{
	sleep((msec+999)/1000);
}

Tm *
Localtime(long t)
{
	return (Tm *)localtime(&t);
}

static void
rmsrv(void)
{
	unlink(srvname);
}

void
plumb(void)
{
	char *p;
	int r;

	if(p = getenv("SHELL"))	/* assign = */
		shname = strdup(p);

	if(srvname == 0){
		if(pipe(pipefd) < 0){
			fprint(2, "%s: can't pipe: %r\n", argv0);
			exits("pipe");
		}
		return;
	}
	rmsrv();
	r = mkfifo(srvname, 0660);
	if(r < 0){
		fprint(2, "%s: can't mkfifo %s: %r\n",
			argv0, srvname);
			exits("mkfifo");
	}
	atexit(rmsrv);
	switch(fork2()){
	case -1:
		fprint(2, "%s: can't fork: %r\n", argv0);
		exits("fork");
	case 0:
		pipefd[0] = open(srvname, OREAD);
		_exits(0);
	}
	pipefd[1] = open(srvname, OWRITE);
	if(pipefd[1] >= 0)
		pipefd[0] = open(srvname, OREAD);
	if(pipefd[0] < 0 || pipefd[1] < 0){
		fprint(2, "%s: can't open %s: %r\n",
			argv0, srvname);
			exits("open");
	}
}
