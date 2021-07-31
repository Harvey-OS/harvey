#include "all.h"

char *	dsrvname = "/srv/fone";
char *	fname = "/dev/eia0";
char *	shname = "/bin/rc";
char *	telcmd = "/bin/tel";

void
openfone(void)
{
	char buf[2*NAMELEN];

	sprint(buf, "%sctl", fname);
	fonectlfd = open(buf, OWRITE);
	if(fonectlfd < 0){
		perror(buf);
		exits("open");
	}
	fprint(fonectlfd, "b19200");
	fprint(fonectlfd, "d1");
	fonefd = open(fname, ORDWR);
	if(fonefd < 0){
		perror(fname);
		exits("open");
	}
}

void
killproc(int pid)
{
	int fd;
	char name[3*NAMELEN];

	sprint(name, "/proc/%d/note", pid);
	fd = open(name, OWRITE);
	write(fd, "exit\n", 5);
	close(fd);
}

int
fork2(void)
{
	int i, j;
	char buf[20], *p;
	Waitmsg w;

	i = fork();
	if(i != 0){
		if(i < 0)
			return -1;
		do; while((j=wait(&w))!=-1 && j!=i);
		if(j == i){
			p = strchr(w.msg, ':');
			return atoi(p?p+1:w.msg); /* pid of final child */
		}
		return j;
	}
	i = fork();
	if(i != 0){
		sprint(buf, "%d", i);
		_exits(buf);
	}
	return 0;
}

char *
ascnow(void)
{
	return ctime(time(0));
}

void
msleep(int msec)
{
	sleep(msec);
}

Tm *
Localtime(long t)
{
	return localtime(t);
}

static void
rmsrv(void)
{
	remove(srvname);
}

void
plumb(void)
{
	int fd;

	if(pipe(pipefd) < 0){
		fprint(2, "%s: can't pipe: %r\n", argv0);
		exits("pipe");
	}
	if(srvname == 0)
		return;
	rmsrv();
	fd = create(srvname, OWRITE, 0666);
	if(fd < 0){
		fprint(2, "%s: can't create %s: %r\n",
			argv0, srvname);
	}else{
		atexit(rmsrv);
		fprint(fd, "%d", pipefd[1]);
		close(fd);
	}
}
