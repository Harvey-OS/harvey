/* for plan 9 */
#include <u.h>
#include <libc.h>
#include <stdarg.h>

/* not for plan 9 */
/* #include <errno.h> */
/* #include <time.h> */
/* #include <ipc.h> */

#define MIN(a,b)	((a<b)?a:b)

#define	ACK(a)	write(a, "", 1)
#define NAK(a)	write(a, "\001", 1)

#define RDNETIMEOUT	60000
#define WRNETIMEOUT	60000
#define LPDAEMONLOG	"/tmp/lpdaemonl"

#define LNBFSZ	4096
char lnbuf[LNBFSZ];
int dbgstate = 0;
char *dbgstrings[] = {
	"",
	"rcvack1",
	"send",
	"rcvack2",
	"response",
	"done"
};
void
error(int level, char *s1, ...)
{
	long thetime;
	char *chartime;
	va_list ap;
	char *args[8];
	int argno = 0;

	if (level == 0) {
		time(&thetime);
		chartime = ctime(thetime);
		fprint(2, "%.15s ", &(chartime[4]));
	}
	va_start(ap, s1);
	while(args[argno++] = va_arg(ap, char*));
	va_end(ap);
	fprint(2, s1, *args);
	return;
}

int
alarmhandler(void *foo, char *note) {
	USED(foo);
	if(strncmp(note, "alarm", ERRLEN)==0) {
		fprint(2, "alarm at %d - %s\n", dbgstate, dbgstrings[dbgstate]);
		return(1);
	} else return(0);
}

/* get a line from inpfd using nonbuffered input.  The line is truncated if it is too
 * long for the buffer.  The result is left in lnbuf and the number of characters
 * read in is returned.
 */
int
readline(int inpfd)
{
	register char *ap;
	register int i;

	ap = lnbuf;
	i = 0;
	do {
		if (read(inpfd, ap, 1) != 1) {
			error(0, "read error in readline, fd=%d\n", inpfd);
			break;
		}
	} while ((++i < LNBFSZ - 2) && *ap++ != '\n');
	if (i == LNBFSZ - 2) {
		*ap = '\n';
		i++;
	}
	*ap = '\0';
	return(i);
}

#define	RDSIZE 512
char jobbuf[RDSIZE];

int
pass(int inpfd, int outfd, int bsize)
{
	int bcnt, rv;

	for(bcnt=bsize; bcnt > 0; bcnt -= rv) {
		alarm(WRNETIMEOUT);	/* to break hanging */
		if((rv=read(inpfd, jobbuf, MIN(bcnt,RDSIZE))) < 0) {
			error(0, "read error during pass, %d remaining\n", bcnt);
			break;
		} else if((write(outfd, jobbuf, rv)) != rv) {
			error(0, "write error during pass, %d remaining\n", bcnt);
			break;
		}
	}
	alarm(0);
	return(bcnt);
}
	
/* get whatever stdin has and put it into the temporary file.
 * return the file size.
 */
int
prereadfile(int inpfd)
{
	int rv, bsize;

	bsize = 0;
	do {
		if((rv=read(0, jobbuf, RDSIZE))<0) {
			error(0, "read error while making temp file\n");
			exits("read error while making temp file");
		} else if((write(inpfd, jobbuf, rv)) != rv) {
			error(0, "write error while making temp file\n");
			exits("write error while making temp file");
		}
		bsize += rv;
	} while (rv!=0);
	return(bsize);
}

int
tmpfile(void)
{
	static tindx = 0;
	char tmpf[20];
	int tmpfd;

	sprint(tmpf, "/tmp/lp%d.%d", getpid(), tindx++);
	if((tmpfd=create(tmpf, ORDWR|ORCLOSE|OTRUNC, 0666)) < 0) {
		error(0, "cannot create temp file %s\n", tmpf);
		exits("cannot create temp file");
	}
	return(tmpfd);
}

int
recvACK(int netfd)
{
	int rv;

	*jobbuf = '\0';
	alarm(RDNETIMEOUT);
	if (read(netfd, jobbuf, 1)!=1 || *jobbuf!='\0') {
		error(0, "failed to receive ACK, ");
		if (*jobbuf == '\0')
			error(1, "read failed\n");
		else
			error(1, "received <0x%x> instead\n", *jobbuf);
		rv = 0;
	} else rv = 1;
	alarm(0);
	return(rv);
}

void
main(int argc, char *argv[])
{
	char *devdir;
	int i, rv, netfd, bsize, datafd;

	/* make connection */
	if (argc != 4) {
		fprint(2, "usage: %s destination network service\n", argv[0]);
		exits("incorrect number of arguments");
	}
	if ((netfd=dial((devdir=netmkaddr(argv[1], argv[2], argv[3])), 0, 0, 0)) < 0) {
		fprint(2, "dialing %s\n", devdir);
		perror("dial");
		exits("can't dial");
	}

	/* read options line from stdin and send it */
	i = readline(0);
	if (write(netfd, lnbuf, i) != i) {
		error(0, "write error while sending options\n");
		exits("write error while sending options");
	}

	/* read stdin into tmpfile to get size */
	datafd = tmpfile();
	bsize = prereadfile(datafd);

	/* send the size of the file to be sent */
	i = sprint(lnbuf, "%d\n", bsize);
	if ((rv=write(netfd, lnbuf, i)) != i) {
		perror("write error while sending size");
		error(0, "write returned %d\n", rv);
		exits("write error while sending size");
	}

	if (seek(datafd, 0L, 0) < 0) {
		error(0, "error seeking temp file\n");
		exits("seek error");
	}
	/* mirror performance in readfile() in lpdaemon */
	atnotify(alarmhandler, 1);
	dbgstate = 1;
	if(!recvACK(netfd)) {
		error(0, "failed to receive ACK before sending data\n");
		exits("recv ack1 failed");
	}
	dbgstate = 2;
	if ((i=pass(datafd, netfd, bsize)) != 0) {
		NAK(netfd);
		error(0, "failed to send %d bytes\n", i);
		exits("send data failed");
	}
	ACK(netfd);
	dbgstate = 3;
	if(!recvACK(netfd)) {
		error(0, "failed to receive ACK after sending data\n");
		exits("recv ack2 failed");
	}

	/* get response, as from lp -q */
	dbgstate = 4;
	while((rv=read(netfd, jobbuf, RDSIZE)) > 0) {
		if((write(1, jobbuf, rv)) != rv) {
			error(0, "write error while sending to stdout\n");
			exits("write error while sending to stdout");
		}
	}
	dbgstate = 5;
	atnotify(alarmhandler, 0);
	/* close down network connections and go away */
	exits("");
}
