#include <u.h>
#include <libc.h>
#include <fcall.h>
#include "iotrack.h"
#include "dat.h"
#include "fns.h"

#include "errstr.h"

Fcall	thdr;
Fcall	rhdr;
char	data[sizeof(Fcall)+MAXFDATA];
char	fdata[MAXFDATA];
int	errno;
void	rmservice(void);
char	srvfile[64];
char	*deffile;

void
usage(void)
{
	fprint(2, "usage: %s [-v] [-s] [-f devicefile] [srvname]\n", argv0);
	exits("usage");
}

void
main(int argc, char **argv)
{
	int srvfd, pipefd[2], n;
	int stdio;

	stdio = 0;
	ARGBEGIN{
	case 'v':
		++chatty;
		break;
	case 'f':
		deffile = ARGF();
		break;
	case 's':
		stdio = 1;
		break;
	default:
		usage();
	}ARGEND

	if(argc == 0)
		strcpy(srvfile, "#s/dos");
	else if(argc == 1)
		sprint(srvfile, "#s/%s", argv[0]);
	else
		usage();

	if(stdio){
		pipefd[0] = 0;
		pipefd[1] = 1;
	}else{
		close(0);
		close(1);
		open("/dev/null", OREAD);
		open("/dev/null", OWRITE);
		if(pipe(pipefd) < 0)
			panic("pipe");
		srvfd = create(srvfile, OWRITE, 0666);
		if(srvfd < 0)
			panic(srvfile);
		fprint(srvfd, "%d", pipefd[0]);
		close(pipefd[0]);
		close(srvfd);
		atexit(rmservice);
		fprint(2, "%s %d: serving %s\n", argv0, getpid(), srvfile);
	}
	srvfd = pipefd[1];

	switch(rfork(RFNOWAIT|RFNOTEG|RFFDG|RFPROC)){
	case -1:
		panic("fork");
	default:
		_exits(0);
	case 0:
		break;
	}

	iotrack_init();

	if(!chatty){
		close(2);
		open("#c/cons", OWRITE);
	}
	while((n = read(srvfd, data, sizeof data)) > 0){
		if(convM2S(data, &thdr, n) <= 0)
			panic("convM2S");
		errno = 0;
		switch(thdr.type){
		default:	panic("type");	break;
		case Tnop:	rnop();		break;
		case Tauth:	rauth();	break;
		case Tsession:	rsession();	break;
		case Tflush:	rflush();	break;
		case Tattach:	rattach();	break;
		case Tclone:	rclone();	break;
		case Twalk:	rwalk();	break;
		case Topen:	ropen();	break;
		case Tcreate:	rcreate();	break;
		case Tread:	rread();	break;
		case Twrite:	rwrite();	break;
		case Tclunk:	rclunk();	break;
		case Tremove:	rremove();	break;
		case Tstat:	rstat();	break;
		case Twstat:	rwstat();	break;
		}
		if(errno == 0)
			rhdr.type = thdr.type+1;
		else{
			rhdr.type = Rerror;
			strncpy(rhdr.ename, xerrstr(errno), ERRLEN);
		}
		rhdr.fid = thdr.fid;
		rhdr.tag = thdr.tag;
		chat((errno==0 ? "OK\n" : "%s (%d)\n"),
			xerrstr(errno), errno);
		if((n = convS2M(&rhdr, data)) <= 0)
			panic("convS2M");
		if(write(srvfd, data, n) != n)
			panic("write");
	}
	exits(0);
}

void
rmservice(void)
{
	remove(srvfile);
}

char *
xerrstr(int e)
{
	if (e < 0 || e >= sizeof errmsg/sizeof errmsg[0])
		return "no such error";
	else
		return errmsg[e];
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
