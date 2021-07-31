#include <u.h>
#include <libc.h>
#include <libg.h>
#include "cons.h"

#define	BUFSIZ	4000

extern int	outfd;

int	hostpid;

int dbg;

void
edie(void)
{
	char buf[32];
	int fd;
	static int dead = 0;

	if(dead++ > 0) return;
	close(outfd);
	sprint(buf, "/proc/%d/notepg", getpid());
	fd = open(buf, OWRITE);
	if(fd > 0){
		write(fd, "exit", 4);
		close(fd);
	}
}

static int
start_host(void)
{
	int	fd;

	cs = consctl();
	if(cs == 0){
		perror("consctl");
		exits("consctl");
	}

	switch((hostpid = rfork(RFPROC|RFNAMEG|RFFDG|RFNOTEG))) {
	case 0:
		fd = open("/dev/cons", OREAD);
		dup(fd,0);
		if(fd != 0)
			close(fd);
		fd = open("/dev/cons", OWRITE);
		dup(fd,1);
		dup(fd,2);
		if(fd != 1 && fd !=2)
			close(fd);
		execl("/bin/rc","rcX",0);
		fprint(2,"failed to start up rc\n");
		_exits("rc");
	case -1:
		fprint(2,"rc startup: fork error\n");
		_exits("rc_fork");
	}

	return open("/mnt/cons/cons/data", ORDWR);
}

void
ebegin(int Ehost)
{
dbg = open("dbg",OWRITE);

	atexit(edie);

	einit(Emouse|Ekeyboard);

	outfd = start_host();
	if( estart(Ehost, outfd, BUFSIZ) != Ehost) {
		exits("event init error");
	}
}

void
send_interrupt(void)
{
if(dbg>0) fprint(dbg,"post interrupt to pid %d\n",hostpid);
	postnote(-hostpid,"interrupt");
}
