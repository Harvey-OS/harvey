#include <alef.h>
#include "utils.h"
#include "tcpostio.h"

#ifndef plan9
#define seek lseek
#define create(a,b,c)	creat(a,c)
#define TMULT	1
int dial(byte *, byte *, byte *, int *);
extern int errno;
#else
#define TMULT 1000
#endif

/* warn every TWARN seconds without a response from the printer */
#define	TWARN	60
/* die after TDEAD seconds without a response from the printer */
#define TDEAD	300
/* time between dialing address */
#define TDIAL 20

void
tmpnam(byte *s){
	byte buf[64];
	int i, j;
	i = getpid();
	for(j=0; j<100; j++){
		sprint(buf, "/tmp/tcpostio.%d", i+j);
		if(access(buf, 0) < 0){
			strcpy(s, buf);
			return;
		}
	}
	fprint(2, "tcpostio: can't find temp file\n");
	exits("tmpnam");
}

int debug;
int inputfd, printerfd;
int bytes_to_send;
int bytes_sent, blocksize;
prog_states prog_state;

aggr Status {
	byte *str;		/* printer's current status */
	pr_state val;	/* value returned by getstatus() */
};

Status statuslist[] = {
	{ "initializing", PR_INIT },
	{ "idle", PR_IDLE },
	{ "busy", PR_BUSY },
	{ "waiting", PR_WAIT },
	{ "printing", PR_PRINT },
	{ "printererror", PR_PRINTERR },
	{ "Error", PR_ERROR },
	{ "Flushing", PR_FLUSH },
	{ "end of job", PR_EOJ },
	{ "unknown", PR_UNKNOWN }
};

byte *sbuf = nil;

pr_state
parsmesg(byte *buf) {
	byte	*ks, *vs;		/* start of strings */
	byte	*ke, *ve;		/* end of strings */
	int kl, vl;			/* length of strings */
	byte	*p;		/* for converting to lower case etc. */
	int i;			/* where *key was found in statuslist[] */

if (debug) fprint(2, "parsemesg begin\n");
	kl = strlen(buf);
if (debug) {
	fprint(2, "mesg length = %d\n", kl);
	fprint(2, "mesg is <%*s>\n", kl, buf);
}
	sbuf = realloc(sbuf, kl);
	if (sbuf == nil) {
		fprint(2, "realloc failed in parsmesg\n");
		exits("realloc");		/* I SHOULD NOT CALL EXITS HERE! */
	}
	/* mesage is a 'cntrl d' */
	if (*buf == '\004')
		return(PR_EOJ);
	if (*(ks=find(buf, "%%[ "))!='\0' && *(ke=find(ks, " ]%%"))!='\0') {
		strcpy(sbuf, ks+4);	/* ignore the leading "%%[ " and don't change mesg[] */
		sbuf[ke-(ks+4)] = '\0';	/* ignore the trailing " ]%%" and terminate string */

		ks = sbuf;
		ke = find(ks, ":");
		kl = ke - ks - 1;
		for (; kl > 0; ) {
			/* message of the form "Error: condition; ..." */
			if (strncmp(ks, "Error", kl) == 0)
				return(PR_ERROR);
			/* message of the form "Flushing: rest of job ..." */
			if (strncmp(ks, "Flushing", kl) == 0)
				return(PR_FLUSH);
			/* message of the form "... status: condition; ..." */
			vs = ke + 1;
			ve = find(vs, ";");
			vl = ve - vs - 1;
			for (; *ks == ' '; ks++) kl--;	/* skip any leading spaces */
			if (vl > 0 && strncmp(ks, "status", kl) == 0) {
				for (; *vs == ' '; vs++) vl--;	/* skip any leading spaces */
if (debug) fprint(2,"parsemesg, after skip leading spaces <%s>\n", vs);
				for (p = vs; *p; p++)		/* convert to lower case */
					if (*p == ';')  {
						*p = '\0';
						break;
					} else if (*p>='A' && *p<='Z') *p += 0x20;

				for (i=0; statuslist[i].str != nil; i++) {
if (debug) fprint(2, "parsemesg, val=%d <%s> %d\n", statuslist[i].val, vs, vl);
					if (strncmp(statuslist[i].str, vs, vl) == 0) {

						return(statuslist[i].val);
					}
				}
			} else {
				ks = ve + 1;
				ke = find(ks, ":");
				kl = ke - ks -1;
			}
		}
	}
	return(PR_UNKNOWN);
}

int waiting_for_EOJ;
int sendprocpid, timeprocpid, recvprocpid;

void
sendproc(chan (int) send_chan) {
	int request, i;
	byte *buf;

	buf = malloc(blocksize);
	if (buf == nil) {
		fprint(2, "malloc failed\n");
		exits("malloc");
	}
	sendprocpid = getpid();
	while (request = <-send_chan) {
if (debug) fprint(2, "sendproc %d\n", request);
		switch(request) {
		case REQ_STAT:
			if (write(printerfd, "", 1) != 1) {
				fprint(2, "write to printer failed\n");
			}
			break;
		case SEND_DATA:
			if ((i=read(inputfd, buf, blocksize)) < 0) {
#ifdef plan9
				fprint(2, "read from input file returned %d, errstr=%s\n", i, errstr);
#else
				fprint(2, "read from input file returned %d, errno=%d\n", i, errno);
#endif
			} else if (i == 0) {
				if (prog_state != WAIT_FOR_EOJ) {
					if (write(printerfd, "", 1) != 1) {
						fprint(2, "write to printer failed\n");
					}
					prog_state = WAIT_FOR_EOJ;
				}
				break;
			} else if (write(printerfd, buf, i) != i) {
				fprint(2, "write to printer failed\n");
			} else {
				bytes_sent += i;
				break;
			}
			fprint(2, "should exit here\n");
			break;
		case SEND_EOD:
			if (write(printerfd, "", 1) != 1) {
				fprint(2, "write to printer failed\n");
			}
			break;
		}
	}
}

/* time granularity */
#define TGRAN 10

void
timeproc(chan (int) time_chan) {
	timeprocpid = getpid();
	while(1) {
		sleep(TGRAN * TMULT);
		time_chan <-= TGRAN;
	}
}

byte statusbuf[8192];

void
recvproc(chan (pr_state) recv_chan) {
	int i;

	recvprocpid = getpid();
	while((i=getline(printerfd, statusbuf, blocksize)) > 0) {
if (debug) fprint(2, "recvproc\n");
		if (statusbuf[i-1] == '\n')
			statusbuf[i-1] = '\0';
		recv_chan <-= parsmesg(statusbuf);
	}
}

byte tmpfilename[64];

void
cleanup(void) {
#ifdef plan9
	remove(tmpfilename);
#else
	unlink(tmpfilename);
#endif
}

int
copy(int in, int out) {
	byte *buf;
	int i, rv;

	rv = 1;
	buf = malloc(blocksize+1);
	if (buf == nil) {
		fprint(2, "malloc failed\n");
		rv = 0;
	} else {
		while ((i=read(in, buf, blocksize)) > 0) {
			if (write(out, buf, i) != i) {
				fprint(2, "write failed\n");
				rv = 0;
				break;
			}
		}
	}
	return(rv);
}

void
main(int argc, byte **argv) {
	chan(prog_states) send_chan;
	chan(int) time_chan;
	chan (pr_state) recv_chan;
	int t, ac, i;
	pr_state printer_state;
	int time_past, time_out;
	byte *av, *rv;
#ifdef plan9
	Dir iDir;
#else
	Stat statbuf;
#endif

	/*
	 * This splits off the rendezvous tag space for the ALEF runtime
	 * It is necessary because many tcpostio processes may run in
	 * the same namespace
	 */
	rfork(RFNAMEG);

	statusbuf[0] = '\0';
	prog_state = START;
	blocksize = PRBUFSIZ;
	bytes_sent = 0;
	time_past = 0;
	time_out = 0;
	inputfd = 0;
	printerfd = -1;
	rv = nil;

	for (ac=1; ac<argc; ac++) {
		if (argv[ac]!=nil && argv[ac][0]=='-' && argv[ac][1]) {
			switch (argv[ac][1]) {
			case 'b':
				if (argv[ac][2]) av = &argv[ac][2];
				else av = &argv[++ac][0];
				blocksize = atoi(av)/10;
				if (blocksize > PRBUFSIZ || blocksize < 1)
					blocksize = PRBUFSIZ;
				break;
			case 'd':
				debug = 1;
				break;
			default:
				fprint(2, "unknown option %c\n", argv[ac][1]);
				break;
			}
		} else break;
	}
if (debug) fprint(2, "parsed options\n");

	if (ac < argc) {
		if (argv[ac][0] == '/')
			printerfd = open(argv[ac], ORDWR);
		else
			for (i=0; printerfd<0&&i<3; i++) {
				printerfd = dial(argv[ac], nil, nil, nil);
				if (printerfd<0) sleep(TDIAL*TMULT);
			}
		if (printerfd < 0) {
			fprint(2, "connect to (%s) failed\n", argv[ac]);
			exits("dial");
		}
		ac++;
	} else {
		fprint(2, "usage: %s net!addr!service inputfile\n", argv[0]);
		exits("usage");
	}
if (debug) fprint(2, "ac=%d argc=%d\n", ac, argc);
	if (ac >= argc) {
		tmpnam(tmpfilename);
		if ((inputfd=create(tmpfilename, ORDWR, 0600)) < 0) {
			fprint(2, "cannot create temporary file %s\n", tmpfilename);
			exits("create");
		}
		atexit(cleanup);
		if (!copy(0, inputfd)) {
			fprint(2, "copy of input to tmpfile failed\n");
			exits("copy");
		}
#ifndef plan9
		close(inputfd);
		if ((inputfd=open(tmpfilename, OREAD)) < 0) {
			fprint(2, "cannot open temporary file %s\n", tmpfilename);
			exits("open");
		}
		/* atexit doesn't work for UNIX, tell bobf */
		cleanup();
#else
/*		seeks don't work, tell bobf */

		if (seek(inputfd, 0, 0)<0) {
			fprint(2, "seek failed\n");
			exits("seek");
		}
#endif

	} else {
		if ((inputfd=open(argv[ac], OREAD)) < 0) {
			fprint(2, "cannot open input file %s\n", argv[ac]);
			exits("open");
		}
		fprint(2, "opened input file %s\n", argv[ac]);
	}

#ifdef plan9
	if (dirfstat(inputfd, &iDir) < 0) {
		fprint(2, "input or temp file could not be stat'ed\n");
		exits("fstat");
	}
	bytes_to_send = iDir.Length.length;
#else
	if (fstat(inputfd, &statbuf) < 0) {
		fprint(2, "input or temp file could not be stat'ed\n");
		exits("fstat");
	}
	bytes_to_send = statbuf.size;
#endif
	if(bytes_to_send == 0) {
		fprint(2, "no input to send!\n");
		exits("");
	}
if (debug) fprint(2, "about to go mutliprocessing\n");

	alloc send_chan, time_chan, recv_chan;
	proc sendproc(send_chan);
	proc timeproc(time_chan);
	proc recvproc(recv_chan);

	send_chan <-= SEND_EOD;
	send_chan <-= REQ_STAT;
	for(;;) {
		alt {
		case printer_state = <-recv_chan:
			if (time_past == 0)
				fprint(2, ". %5.2f%% sent, %.100s\n", bytes_sent*100.0/bytes_to_send, statusbuf);
			time_out = 0;
			switch(printer_state) {
			case PR_INIT:
				break;
			case PR_IDLE:
				if (prog_state == WAIT_FOR_EOJ)
					goto STOP;
				else {
					prog_state = SEND_DATA;
					send_chan <-= SEND_DATA;
				}
				break;
			case PR_BUSY:
			case PR_WAIT:
			case PR_PRINT:
				if (prog_state == SEND_DATA)
					send_chan <-= SEND_DATA;
				break;
			case PR_PRINTERR:
			case PR_ERROR:
			case PR_FLUSH:
				fprint(2, ". %5.2f%% sent, %.100s\n", bytes_sent*100.0/bytes_to_send, statusbuf);
				if (prog_state != WAIT_FOR_EOJ) {
					send_chan <-= SEND_EOD;
					prog_state = WAIT_FOR_EOJ;
					rv = statuslist[printer_state].str;
				}
				break;
			case PR_UNKNOWN:
				fprint(2, ". %5.2f%% sent, %.100s", bytes_sent*100.0/bytes_to_send, statusbuf);
				break;
			case PR_EOJ:
				if (prog_state == WAIT_FOR_EOJ)
					goto STOP;
				break;
			}
			send_chan <-= REQ_STAT;
			break;
		case t = <-time_chan:
if (debug) fprint(2, "bing!");
			time_past +=t;
			time_out += t;
			if (time_out >= TWARN) {
				if ((time_out%TWARN) == 0)
					fprint(2, ". %5.2f%% sent, out of contact with printer for %d seconds\n", bytes_sent*100.0/bytes_to_send, time_out);
				if (time_out >= TDEAD) {
						fprint(2, "  Aborting job!\n");
						rv = "timeout";
						goto STOP;
				}
			} else 
				fprint(2, ": %5.2f%% sent, %.100s\n", bytes_sent*100.0/bytes_to_send, statusbuf);
			break;
		}

	}
STOP:
	fprint(2, "%d bytes sent, status: %s\n", bytes_sent, statuslist[PR_EOJ].str);
#ifdef plan9
	postnote(PNPROC, sendprocpid, "kill");
	postnote(PNPROC, timeprocpid, "kill");
	postnote(PNPROC, recvprocpid, "kill");
#else
	kill(sendprocpid, 15);
	kill(timeprocpid, 15);
	kill(recvprocpid, 15);
#endif
	exits(rv);
}
