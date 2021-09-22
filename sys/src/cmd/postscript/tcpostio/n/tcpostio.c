/*
 * tcpostio - send postscript to a printer via tcp
 */
#include <u.h>
#include <libc.h>
#include <ctype.h>

#define FD_ISSET(a, b)	0	/* make it compile */

/*
 * debug = 0 for no debugging
 * debug = 1 for readprinter debugging
 * debug = 2 for sendprinter debugging
 * debug = 3 for full debugging, it's hard to read the messages
 */
int debug = 0;

enum {
	READTIMEOUT =	300,
	RCVSELTIMEOUT =	30,
	SNDSELTIMEOUT =	300,

	MESGSIZE =	16384,
};

typedef ulong *fd_set;

typedef struct {
	char	*state;			/* printer's current status */
	int	val;			/* value returned by getstatus() */
} Status;

/* printer states */
#define	INITIALIZING	0
#define	IDLE		1
#define	BUSY		2
#define	WAITING		3
#define	PRINTING	4
#define	PRINTERERROR	5
#define	ERROR		6
#define	FLUSHING	7
#define UNKNOWN		8

int	blocksize = 1920;	/* 19200/10, with 1 sec delay between transfers
				 * this keeps the queues from building up.
				 */
char mesg[MESGSIZE];		/* exactly what came back from the printer */
char buf[MESGSIZE];

/* protocol requests and program states */
#define START		'S'
uchar	Start[] = { START };

#define ID_LE		'L'
uchar	Id_le[] = { ID_LE };

#define	REQ_STAT	'T'
uchar	Req_stat[] = { REQ_STAT };

#define	SEND_DATA	'D'
uchar	Send_data[] = { SEND_DATA };

#define	SENT_DATA	'A'
uchar	Sent_data[] = { SENT_DATA };

#define	WAIT_FOR_EOJ	'W'
uchar	Wait_for_eoj[] = { WAIT_FOR_EOJ };

#define END_OF_JOB	'E'
uchar	End_of_job[] = { END_OF_JOB };

#define FATAL_ERROR	'F'
uchar	Fatal_error[] = { FATAL_ERROR };

#define	WAIT_FOR_IDLE	'I'
uchar	Wait_for_idle[] = { WAIT_FOR_IDLE };

#define	OVER_AND_OUT	'O'
uchar	Over_and_out[] = { OVER_AND_OUT };

Status	status[] = {
	"initializing",	INITIALIZING,
	"idle",		IDLE,
	"busy",		BUSY,
	"waiting",	WAITING,
	"printing",	PRINTING,
	"printererror",	PRINTERERROR,
	"Error",	ERROR,
	"flushing",	FLUSHING,
	nil,		UNKNOWN
};

// fd_set: readfds, writefds, exceptfds;
// struct timeval rcvtimeout = { RCVSELTIMEOUT, 0 };
// struct timeval sndtimeout = { SNDSELTIMEOUT, 0 };

void
rdtmout(void)
{
	fprint(2, "read timeout occurred, check printer\n");
}

int
getline(int fd, char *buf, int len)
{
	char *bp, c;
	int i = 0, n;

	bp = buf;
	alarm(READTIMEOUT);
	while ((n = read(fd, bp, 1)) == 1) {
		if (*bp == '\r')
			continue;
		i += n;
		c = *bp++;
		if (c == '\n' || c == '\004' || i >= len - 1)
			break;
	}
	alarm(0);
	if (n < 0)
		return(n);
	*bp = '\0';
	return i;
}


/*
 * find returns a pointer to the location of string str2 in string str1,
 * if it exists.  Otherwise, it points to the end of str1.
 */
char *
find(char *str1, char *str2)
{
	char *s1, *s2;

	for (; *str1 != '\0'; str1++) {
		for (s1 = str1, s2 = str2; *s2 != '\0' && *s1 == *s2; s1++, s2++)
			;
		if (*s2 == '\0')
			break;
	}
	return str1;
}

int
parsmesg(char *buf)
{
	int i;			/* where *key was found in status[] */
	char *s;		/* start of printer messsage in mesg[] */
	char *e;		/* end of printer message in mesg[] */
	char *key, *val;	/* keyword/value strings in sbuf[] */
	char *p;		/* for converting to lower case etc. */
	static char sbuf[MESGSIZE];

	if (*(s = find(buf, "%[ ")) == '\0' || *(e = find(s, " ]%")) == '\0')
		return UNKNOWN;
	strcpy(sbuf, s + 3);	/* don't change mesg[] */
	sbuf[e-(s+3)] = '\0';	/* ignore the trailing " ]%" */

	for (key = strtok(sbuf, " :"); key != nil; key = strtok(nil, " :"))  {
		if (strcmp(key, "Error") == 0)
			return ERROR;
		if ((val = strtok(nil, ";")) != nil && strcmp(key, "status") == 0)
			key = val;

		for (; *key == ' '; key++)
			;			/* skip any leading spaces */
		for (p = key; *p; p++)		/* convert to lower case */
			if (*p == ':')  {
				*p = '\0';
				break;
			} else if (isupper(*p))
				*p = tolower(*p);

		for (i = 0; status[i].state != nil; i++)
			if (strcmp(status[i].state, key) == 0)
				return status[i].val;
	}
	return UNKNOWN;
}

int
readprinter(int prfd, int pipefd)
{
	int state = START, print_wait_msg = 0;
	int tocount = 0, c, prstat, lastprstat, n, nfds;
	uchar proto;

	nfds = (pipefd > prfd? pipefd: prfd) + 1;
	prstat = 0;
//	signal(SIGALRM, rdtmout);	// TODO: atnotify
	do {
reselect:
		/* ask sending process to request printer status */
		if (write(pipefd, Req_stat, 1) != 1) {
			fprint(2, "request status failed\n");
			state = FATAL_ERROR;
			continue;
		}
#ifdef APE				/* TODO */
		FD_ZERO(&readfds);
		FD_SET(prfd, &readfds);
		FD_SET(pipefd, &readfds);
		FD_ZERO(&exceptfds);
		FD_SET(prfd, &exceptfds);
		FD_SET(pipefd, &exceptfds);
		n = select(nfds, &readfds, (fd_set *)0, &exceptfds, &rcvtimeout);
#endif
		if (debug & 1)
			fprint(2, "readprinter select returned %d\n", n);
		if (n == 0) {
			/* a timeout occurred */
			if (++tocount > 4) {
				fprint(2, "printer appears to be offline.\n"
					"HP4m printers may be out of paper.\n");
				tocount = 0;
			}
			goto reselect;
		}
		if (n > 0 && FD_ISSET(prfd, &exceptfds)) {
			/* printer problem */
			fprint(2, "printer exception\n");
			if (write(pipefd, Fatal_error, 1) != 1)
				fprint(2, "'fatal error' write to pipe failed\n");
			state = FATAL_ERROR;
			continue;
		}
		if (n > 0 && FD_ISSET(pipefd, &exceptfds)) {
			/* pipe problem */
			fprint(2, "pipe exception\n");
			state = FATAL_ERROR;
			continue;
		}
		if (n > 0 && FD_ISSET(pipefd, &readfds)) {
			/* protocol pipe wants to be read */
			if (debug & 1)
				fprint(2, "pipe wants to be read\n");
			if (read(pipefd, &proto, 1) != 1) {
				fprint(2, "read protocol pipe failed\n");
				state = FATAL_ERROR;
				continue;
			}
			if (debug & 1)
				fprint(2, "readprinter: proto=%c\n", proto);
			/* change state? */
			switch (proto) {
			case SENT_DATA:
				break;
			case WAIT_FOR_EOJ:
				if (!print_wait_msg) {
					print_wait_msg = 1;
					fprint(2, "waiting for end of job\n");
				}
				state = proto;
				break;
			default:
				fprint(2, "received unknown protocol request "
					"<%c> from sendfile\n", proto);
				break;
			}
			n--;
		}
		if (n > 0 && FD_ISSET(prfd, &readfds)) {
			/* printer wants to be read */
			if (debug & 1)
				fprint(2, "printer wants to be read\n");
			if ((c = getline(prfd, buf, MESGSIZE)) < 0) {
				fprint(2, "read printer failed\n");
				state = FATAL_ERROR;
				continue;
			}
			if (debug & 1)
				fprint(2, "%s\n", buf);
			if (c == 1 && *buf == '\004') {
				if (state == WAIT_FOR_EOJ) {
					if (debug & 1)
						fprint(2, "state=%c, ", state);
					fprint(2, "%%[ status: endofjob ]%%\n");
					/* state = WAIT_FOR_IDLE; */
					state = OVER_AND_OUT;
					if (write(pipefd, Over_and_out, 1) != 1)
						fprint(2, "'fatal error' write to pipe failed\n");
					continue;
				} else {
					if (prstat == ERROR) {
						state = FATAL_ERROR;
						continue;
					}
					if (state != START && state != WAIT_FOR_IDLE)
						fprint(2, "warning: EOF received; program status is '%c'\n", state);

				}
				continue;
			}

			/* figure out if it was a status line */
			lastprstat = prstat;
			prstat = parsmesg(buf);
			if (prstat == UNKNOWN || prstat == ERROR ||
			    lastprstat != prstat) {
				/* print whatever it is that was read */
				fprint(2, "%s", buf);
				if (prstat == UNKNOWN) {
					prstat = lastprstat;
					continue;
				}
			}
			switch (prstat) {
			case UNKNOWN:
				continue;	/* shouldn't get here */
			case FLUSHING:
			case ERROR:
				state = FATAL_ERROR;
				/* ask sending process to die */
				if (write(pipefd, Fatal_error, 1) != 1)
					fprint(2, "Fatal_error mesg write to pipe failed\n");
				continue;
			case INITIALIZING:
			case PRINTERERROR:
				sleep(1);
				break;
			case IDLE:
				if (state == WAIT_FOR_IDLE) {
					state = OVER_AND_OUT;
					if (write(pipefd, Over_and_out, 1) != 1)
						fprint(2, "'fatal error' write to pipe failed\n");
					continue;
				}
				state = SEND_DATA;
				goto dowait;
			case BUSY:
			case WAITING:
			default:
				sleep(1);
dowait:
				switch (state) {
				case WAIT_FOR_IDLE:
				case WAIT_FOR_EOJ:
				case START:
					sleep(5);
					break;
				case SEND_DATA:
					if (write(pipefd, Send_data, 1) != 1) {
						fprint(2, "send data write to pipe failed\n");
						state = FATAL_ERROR;
						continue;
					}
					break;
				default:
					sysfatal("unexpected program state %c",
						state);
				}
				break;
			}
			n--;
		}
		if (n > 0)
			sysfatal("too many fds selected");
	} while (state != FATAL_ERROR && state != OVER_AND_OUT);
	return state == FATAL_ERROR;
}

int
sendfile(int infd, int prfd, int pipefd)
{
	uchar proto;
	int state = START;
	int i, n, bytesread,  bytesent = 0;

	if (write(prfd, "\004", 1) != 1) {
		perror("sendfile:write:");
		state = FATAL_ERROR;
	}
	do {
//		alarm(READTIMEOUT);	// TODO: sndtimeout from select
		n = read(pipefd, &proto, 1);
		alarm(0);
		if (debug & 2)
			fprint(2, "sendfile select returned %d\n", n);
		if (n < 0) {
			perror("sendfile: read:");
			state = FATAL_ERROR;
			continue;
		} else if (n == 0) {
			sleep(1);
			fprint(2, "sendfile timeout\n");
			state = FATAL_ERROR;
			continue;
		}

		/* change state? */
		if (debug & 2)
			fprint(2, "sendfile command - <%c>\n", proto);
		switch (proto) {
		case OVER_AND_OUT:
		case END_OF_JOB:
			state = proto;
			break;
		case SEND_DATA:
			bytesread = 0;
			do {
				i = read(infd, &buf[bytesread],
					blocksize - bytesread);
				if (debug & 02)
					fprint(2, "read %d bytes\n", i);
				if (i > 0)
					bytesread += i;
			} while (i > 0 && bytesread < blocksize);
			if (i < 0) {
				fprint(2, "input file read error\n");
				state = FATAL_ERROR;
				break;
			}
			if (bytesread > 0) {
				if (debug & 2)
					fprint(2, "writing %d bytes\n",
						bytesread);
				if (write(prfd, buf, bytesread) != bytesread) {
					perror("sendfile:write:");
					state = FATAL_ERROR;
				} else if (write(pipefd, Sent_data, 1) != 1) {
					perror("sendfile:write:");
					state = FATAL_ERROR;
				} else
					bytesent += bytesread;
				fprint(2, "%d sent\n", bytesent);
				/* we've reached the end of the input */
			}
			if (i == 0 && state != WAIT_FOR_EOJ)
				if (write(prfd, "\004", 1) != 1) {
					perror("sendfile:write:");
					state = FATAL_ERROR;
				} else if (write(pipefd, Wait_for_eoj, 1) != 1) {
					perror("sendfile:write:");
					state = FATAL_ERROR;
				} else
					state = WAIT_FOR_EOJ;
			break;
		case REQ_STAT:
			/* send a control-t to query the printer */
			if (write(prfd, "\024", 1) != 1) {
				fprint(2, "write to printer failed\n");
				state = FATAL_ERROR;
			}
			if (debug & 2)
				fprint(2, "query via control-T.");
			break;
		case FATAL_ERROR:
			state = FATAL_ERROR;
			break;
		}
	} while (state != FATAL_ERROR && state != OVER_AND_OUT);
	if (write(prfd, "\004", 1) != 1) {
		perror("sendfile:write:");
		state = FATAL_ERROR;
	}

	if (debug & 2)
		fprint(2, "%d bytes sent\n", bytesent);
	return state == FATAL_ERROR;
}

static void
usage(void)
{
	fprint(2, "usage: %s [-b baud] net!host!service [infile]\n", argv0);
	exits("usage");
}

void
main(int argc, char *argv[])
{
	int infd, prfd, cpid;
	int pipefd[2];
	ulong rprv, sprv;
	char *dialstr;

	dialstr = 0;
	ARGBEGIN {
	case 'b':
		blocksize = atoi(EARGF(usage())) / 10;
		if (blocksize > MESGSIZE || blocksize < 1)
			blocksize = MESGSIZE;
		break;
	case 'd':
		debug = atoi(EARGF(usage()));
		break;
	default:
		usage();
	} ARGEND
	if (argc > 0)
		dialstr = *argv++;
	else
		usage();
	if (argc > 0) {
		infd = open(*argv, 0);
		if (infd < 0)
			sysfatal("can't open %s: %r", *argv);
		argv++;
		USED(argv);
	} else
		infd = 0;

	if (debug & 02)
		fprint(2, "blocksize=%d\n", blocksize);
	if (debug)
		fprint(2, "dialing address=%s\n", dialstr);
	prfd = dial(dialstr, 0, 0, 0);
	if (prfd < 0)
		sysfatal("can't dial %s: %r", dialstr);

	fprint(2, "printer startup\n");

	if (pipe(pipefd) < 0)
		sysfatal("can't open pipe: %r");
	switch (cpid = fork()) {
	case -1:
		sysfatal("fork failed: %r");
	case 0:				/* child - to printer */
		close(pipefd[1]);
		sprv = sendfile(infd, prfd, pipefd[0]);
		if (debug)
			fprint(2, "to remote - exiting\n");
		exits(sprv? "foo": nil);
	default:			/* parent - from printer */
		close(pipefd[0]);
		rprv = readprinter(prfd, pipefd[1]);
		if (debug)
			fprint(2, "from remote - exiting\n");
//		while (wait(&sprv) != cpid)	// TODO: 9 wait
//			;
		exits(rprv | sprv? "foo": nil);
	}
}
