#include <stdio.h>
#include <stdlib.h>

#ifdef plan9
#include <bsd.h>
#endif

#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>

#include <sys/socket.h>

extern	int dial_debug;
extern	int	dial(char*, char*, char*, int*);


/* debug = 0 for no debugging */
/* debug = 1 for readprinter debugging */
/* debug = 2 for sendprinter debugging */
/* debug = 3 for full debugging, its hard to read the messages */

int debug = 0;
#define READTIMEOUT	300
#define	RCVSELTIMEOUT	30
#define	SNDSELTIMEOUT	300

void
rdtmout(void) {
	fprintf(stderr, "read timeout occurred, check printer\n");
}

int
getline(int fd, char *buf, int len) {
	char *bp, c;
	int i = 0, n;

	bp = buf;
	while (alarm(READTIMEOUT),(n=read(fd, bp, 1)) == 1) {
		alarm(0);
		if (*bp == '\r') continue;
		i += n;

		c = *bp++;
		if (c == '\n' || c == '\004' || i >= len-1)
			break;
	}
	alarm(0);
	if (n < 0)
		return(n);
	*bp = '\0';
	return(i);
}

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

/* protocol requests and program states */
#define START	'S'
unsigned char Start[] = { START };
#define ID_LE		'L'
unsigned char Id_le[] = { ID_LE };
#define	REQ_STAT	'T'
unsigned char Req_stat[] = { REQ_STAT };
#define	SEND_DATA	'D'
unsigned char Send_data[] = { SEND_DATA };
#define	SENT_DATA	'A'
unsigned char Sent_data[] = { SENT_DATA };
#define	WAIT_FOR_EOJ	'W'
unsigned char Wait_for_eoj[] = { WAIT_FOR_EOJ };
#define END_OF_JOB	'E'
unsigned char End_of_job[] = { END_OF_JOB };
#define FATAL_ERROR	'F'
unsigned char Fatal_error[] = { FATAL_ERROR };
#define	WAIT_FOR_IDLE	'I'
unsigned char Wait_for_idle[] = { WAIT_FOR_IDLE };
#define	OVER_AND_OUT	'O'
unsigned char Over_and_out[] = { OVER_AND_OUT };

Status	statuslist[] = {
	"initializing", INITIALIZING,
	"idle", IDLE,
	"busy", BUSY,
	"waiting", WAITING,
	"printing", PRINTING,
	"printererror", PRINTERERROR,
	"Error", ERROR,
	"flushing", FLUSHING,
	NULL, UNKNOWN
};


/* find returns a pointer to the location of string str2 in string str1,
 * if it exists.  Otherwise, it points to the end of str1.
 */
char *
find(char *str1, char *str2) {
	char *s1, *s2;

	for (; *str1!='\0'; str1++) {
		for (s1=str1,s2=str2; *s2!='\0'&&*s1==*s2; s1++,s2++) ;
		if ( *s2 == '\0' )
	    		break;
	}

	return(str1);
}

#define	MESGSIZE	16384
int blocksize = 1920;		/* 19200/10, with 1 sec delay between transfers
				 * this keeps the queues from building up.
				 */
char	mesg[MESGSIZE];			/* exactly what came back on ttyi */

int
parsmesg(char *buf) {
	static char sbuf[MESGSIZE];
	char	*s;		/* start of printer messsage in mesg[] */
	char	*e;		/* end of printer message in mesg[] */
	char	*key, *val;	/* keyword/value strings in sbuf[] */
	char	*p;		/* for converting to lower case etc. */
	int	i;		/* where *key was found in statuslist[] */

	if (*(s=find(buf, "%[ "))!='\0' && *(e=find(s, " ]%"))!='\0') {
		strcpy(sbuf, s+3);	/* don't change mesg[] */
		sbuf[e-(s+3)] = '\0';	/* ignore the trailing " ]%" */

		for (key=strtok(sbuf, " :"); key != NULL; key=strtok(NULL, " :"))  {
			if (strcmp(key, "Error") == 0)
				return(ERROR);
			if ((val=strtok(NULL, ";")) != NULL && strcmp(key, "status") == 0)
				key = val;

	    		for (; *key == ' '; key++) ;	/* skip any leading spaces */
			for (p = key; *p; p++)		/* convert to lower case */
				if (*p == ':')  {
					*p = '\0';
					break;
				} else if (isupper(*p)) *p = tolower(*p);

			for (i=0; statuslist[i].state != NULL; i++) {
				if (strcmp(statuslist[i].state, key) == 0)
					return(statuslist[i].val);
			}
		}
	}
	return(UNKNOWN);
}

char buf[MESGSIZE];
fd_set readfds, writefds, exceptfds;
struct timeval rcvtimeout = { RCVSELTIMEOUT, 0 };
struct timeval sndtimeout = { SNDSELTIMEOUT, 0 };

int
readprinter(int printerfd, int pipefd)
{
	unsigned char proto;
	int progstate = START;
	int print_wait_msg = 0;
	int tocount = 0;
	int c, printstat, lastprintstat, n, nfds;


	nfds = ((pipefd>printerfd)?pipefd:printerfd) + 1;
	printstat = 0;
	signal(SIGALRM, rdtmout);
	do {

reselect:
		/* ask sending process to request printer status */
		if (write(pipefd, Req_stat, 1) != 1) {
			fprintf(stderr, "request status failed\n");
			progstate = FATAL_ERROR;
			continue;
		}
		FD_ZERO(&readfds);	/* lets be anal */
		FD_SET(printerfd, &readfds);
		FD_SET(pipefd, &readfds);
		FD_ZERO(&exceptfds);
		FD_SET(printerfd, &exceptfds);
		FD_SET(pipefd, &exceptfds);
		n = select(nfds, &readfds, (fd_set *)0, &exceptfds, &rcvtimeout);
		if (debug&0x1) fprintf(stderr, "readprinter select returned %d\n", n);
		if (n == 0) {
			/* a timeout occurred */
			if (++tocount > 4) {
				fprintf(stderr, "printer appears to be offline.\nHP4m printers may be out of paper.\n");
				tocount = 0;
			}
			goto reselect;
		}
		if (n > 0 && FD_ISSET(printerfd, &exceptfds)) {
			/* printer problem */
			fprintf(stderr, "printer exception\n");
			if (write(pipefd, Fatal_error, 1) != 1) {
				fprintf(stderr, "'fatal error' write to pipe failed\n");
			}
			progstate = FATAL_ERROR;
			continue;
		}
		if (n > 0 && FD_ISSET(pipefd, &exceptfds)) {
			/* pipe problem */
			fprintf(stderr, "pipe exception\n");
			progstate = FATAL_ERROR;
			continue;
		}
		if (n > 0 && FD_ISSET(pipefd, &readfds)) {
			/* protocol pipe wants to be read */
			if (debug&0x1) fprintf(stderr, "pipe wants to be read\n");
			if (read(pipefd, &proto, 1) != 1) {
				fprintf(stderr, "read protocol pipe failed\n");
				progstate = FATAL_ERROR;
				continue;
			}
			if (debug&0x1) fprintf(stderr, "readprinter: proto=%c\n", proto);
			/* change state? */
			switch (proto) {
			case SENT_DATA:
				break;
			case WAIT_FOR_EOJ:
				if (!print_wait_msg) {
					print_wait_msg = 1;
					fprintf(stderr, "waiting for end of job\n");
				}
				progstate = proto;
				break;
			default:
				fprintf(stderr, "received unknown protocol request <%c> from sendfile\n", proto);
				break;
			}
			n--;
		}
		if (n > 0 && FD_ISSET(printerfd, &readfds)) {
			/* printer wants to be read */
			if (debug&0x1) fprintf(stderr, "printer wants to be read\n");
			if ((c=getline(printerfd, buf, MESGSIZE)) < 0) {
				fprintf(stderr, "read printer failed\n");
				progstate = FATAL_ERROR;
				continue;
			}
			if (debug&0x1) fprintf(stderr, "%s\n", buf);
			if (c==1 && *buf == '\004') {
				if (progstate == WAIT_FOR_EOJ) {
					if (debug&0x1) fprintf(stderr, "progstate=%c, ", progstate);
					fprintf(stderr, "%%[ status: endofjob ]%%\n");
/*					progstate = WAIT_FOR_IDLE; */
					progstate = OVER_AND_OUT;
					if (write(pipefd, Over_and_out, 1) != 1) {
						fprintf(stderr, "'fatal error' write to pipe failed\n");
					}
					continue;
				} else {
					if (printstat == ERROR) {
						progstate = FATAL_ERROR;
						continue;
					}
					if (progstate != START && progstate != WAIT_FOR_IDLE)
						fprintf(stderr, "warning: EOF received; program status is '%c'\n", progstate);

				}
				continue;
			}

			/* figure out if it was a status line */
			lastprintstat = printstat;
			printstat = parsmesg(buf);
			if (printstat == UNKNOWN || printstat == ERROR
			    || lastprintstat != printstat) {
				/* print whatever it is that was read */
				fprintf(stderr, buf);
				fflush(stderr);
				if (printstat == UNKNOWN) {
					printstat = lastprintstat;
					continue;
				}
			}
			switch (printstat) {
			case UNKNOWN:
				continue;	/* shouldn't get here */
			case FLUSHING:
			case ERROR:
				progstate = FATAL_ERROR;
				/* ask sending process to die */
				if (write(pipefd, Fatal_error, 1) != 1) {
					fprintf(stderr, "Fatal_error mesg write to pipe failed\n");
				}
				continue;
			case INITIALIZING:
			case PRINTERERROR:
				sleep(1);
				break;
			case IDLE:
				if (progstate == WAIT_FOR_IDLE) {
					progstate = OVER_AND_OUT;
					if (write(pipefd, Over_and_out, 1) != 1) {
						fprintf(stderr, "'fatal error' write to pipe failed\n");
					}
					continue;
				}
				progstate = SEND_DATA;

				goto dowait;
			case BUSY:
			case WAITING:
			default:
				sleep(1);
dowait:
				switch (progstate) {
				case WAIT_FOR_IDLE:
				case WAIT_FOR_EOJ:
				case START:
					sleep(5);
					break;

				case SEND_DATA:
					if (write(pipefd, Send_data, 1) != 1) {
						fprintf(stderr, "send data write to pipe failed\n");
						progstate = FATAL_ERROR;
						continue;
					}
					break;
				default:
					fprintf(stderr, "unexpected program state %c\n", progstate);
					exit(1);
				}
				break;
			}
			n--;
		}
		if (n > 0) {
			fprintf(stderr, "more fds selected than requested!\n");
			exit(1);
		};	
	} while ((progstate != FATAL_ERROR) && (progstate != OVER_AND_OUT));

	if (progstate == FATAL_ERROR)
		return(1);
	else
		return(0);
}

int
sendfile(int infd, int printerfd, int pipefd)
{
	unsigned char proto;
	int progstate = START;
	int i, n, nfds;
	int bytesread,  bytesent = 0; 

	nfds = ((pipefd>printerfd)?pipefd:printerfd) + 1;

	if (write(printerfd, "\004", 1)!=1) {
		perror("sendfile:write:");
		progstate = FATAL_ERROR;
	}
	do {
		FD_ZERO(&readfds);	/* lets be anal */
		FD_SET(pipefd, &readfds);
		n = select(nfds, &readfds, (fd_set *)0, (fd_set *)0, &sndtimeout);
		if (debug&02) fprintf(stderr, "sendfile select returned %d\n", n);
		if (n > 0 && FD_ISSET(pipefd, &readfds)) {
			/* protocol pipe wants to be read */
			if (read(pipefd, &proto, 1) != 1) {
				fprintf(stderr, "read protocol pipe failed\n");
				return(1);
			}
			/* change state? */
			if (debug&02) fprintf(stderr, "sendfile command - <%c>\n", proto);
			switch (proto) {
			case OVER_AND_OUT:
			case END_OF_JOB:
				progstate = proto;
				break;
			case SEND_DATA:
				bytesread = 0;
				do {
					i = read(infd, &buf[bytesread], blocksize-bytesread);
					if (debug&02) fprintf(stderr, "read %d bytes\n", i);
					if (i > 0)
						bytesread += i;
				} while((i > 0) && (bytesread < blocksize));
				if (i < 0) {
					fprintf(stderr, "input file read error\n");
					progstate = FATAL_ERROR;
					break;	/* from switch */
				}
				if (bytesread > 0) {
					if (debug&02) fprintf(stderr, "writing %d bytes\n", bytesread);
					if (write(printerfd, buf, bytesread)!=bytesread) {
						perror("sendfile:write:");
						progstate = FATAL_ERROR;
					} else if (write(pipefd, Sent_data, 1)!=1) {
						perror("sendfile:write:");
						progstate = FATAL_ERROR;
					} else {
						bytesent += bytesread;
					}
					fprintf(stderr, "%d sent\n", bytesent);
					fflush(stderr);

				/* we have reached the end of the input file */
				}
				if (i == 0) {
					if (progstate != WAIT_FOR_EOJ) {
						if (write(printerfd, "\004", 1)!=1) {
							perror("sendfile:write:");
							progstate = FATAL_ERROR;
						} else if (write(pipefd, Wait_for_eoj, 1)!=1) {
							perror("sendfile:write:");
							progstate = FATAL_ERROR;
						} else {
							progstate = WAIT_FOR_EOJ;
						}
					}
				}
				break;
			case REQ_STAT:
				if (write(printerfd, "\024", 1)!=1) {
					fprintf(stderr, "write to printer failed\n");
					progstate = FATAL_ERROR;
				}
				if (debug&02) fprintf(stderr, "^T");
				break;
			case FATAL_ERROR:
				progstate = FATAL_ERROR;
			}
		} else if (n < 0) {
			perror("sendfile:select:");
			progstate = FATAL_ERROR;
		} else if (n == 0) {
			sleep(1);
			fprintf(stderr, "sendfile timeout\n");
			progstate = FATAL_ERROR;
		}
	} while ((progstate != FATAL_ERROR) && (progstate != OVER_AND_OUT));
	if (write(printerfd, "\004", 1)!=1) {
		perror("sendfile:write:");
		progstate = FATAL_ERROR;
	}

	if (debug&02) fprintf(stderr, "%d bytes sent\n", bytesent);
	if (progstate == FATAL_ERROR)
		return(1);
	else
		return(0);
}

void main(int argc, char *argv[]) {
	int c, usgflg=0, infd, printerfd;
	int cpid;
	int pipefd[2];
	char *dialstr;
	unsigned long rprv, sprv;

	dialstr = 0;

	while ((c = getopt(argc, argv, "b:d:")) != -1)
		switch (c) {
		case 'b':
			blocksize = atoi(optarg)/10;
			if (blocksize > MESGSIZE || blocksize < 1)
				blocksize = MESGSIZE;
			break;
		case 'd':
			debug = atoi(optarg);
			dial_debug = debug;
			break;
		case '?':
			fprintf(stderr, "unknown option %c\n", c);
			usgflg++;
		}
	if (optind < argc)
		dialstr = argv[optind++];
	else {
		usgflg++;
	}
	if (usgflg) {
		fprintf(stderr, "usage: %s [-b baudrate] net!host!service [infile]\n", argv[0]);
		exit (2);
	}
	if (optind < argc) {
		infd = open(argv[optind], 0);
		if (infd < 0) {
			fprintf(stderr, "cannot open %s\n", argv[optind]);
			exit(1);
		}
		optind++;
	} else
		infd = 0;

	if (debug & 02) fprintf(stderr, "blocksize=%d\n", blocksize);
	if (debug) fprintf(stderr, "dialing address=%s\n", dialstr);
	printerfd = dial(dialstr, 0, 0, 0);
	if (printerfd < 0) exit(1);

	fprintf(stderr, "printer startup\n");

	if (socketpair(AF_UNIX, SOCK_STREAM, PF_UNSPEC, pipefd) < 0) {
		perror("socketpair");
		exit(1);
	}
	switch(cpid = fork()){
	case -1:
		perror("fork error");
		exit(1);
	case 0:
		close(pipefd[1]);
		sprv = sendfile(infd, printerfd, pipefd[0]);	/* child - to printer */
		if (debug) fprintf(stderr, "to remote - exiting\n");
		exit(sprv);
	default:
		close(pipefd[0]);
		rprv = readprinter(printerfd, pipefd[1]);	/* parent - from printer */
		if (debug) fprintf(stderr, "from remote - exiting\n");
		while(wait(&sprv) != cpid);
		exit(rprv|sprv);
	}
}
