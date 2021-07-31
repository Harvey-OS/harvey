#include <u.h>
#include <libc.h>

void setspeed(int, int);
void godial(int, int, char*);
int readmsg(int, int);
int wasintr(void);
void punt(char*);
char* syserr(void);

int pulsed;

char *msgs[] =
{
	"OK",
	"CONNECT",
	"RING",
	"NO CARRIER",
	"ERROR",
	"CONNECT 1200",
	"NO DIALTONE",
	"BUSY",
	"NO ANSWER",
	"CONNECT 2400",
	"CONNECT 0300/REL",
	"CONNECT 1200/REL",
	"CONNECT 2400/REL",
	"CONNECT 1200/LAPM",
	"CONNECT 2400/LAPM",
	0
};


void
usage(void)
{
	punt("usage: hayes [-p] telno [device]");
}

void
main(int argc, char **argv)
{
	int data = -1;
	int ctl = -1;
	char cname[3*NAMELEN];

	ARGBEGIN{
	case 'p':
		pulsed = 1;
		break;
	default:
		usage();
	}ARGEND

	switch(argc){
	case 1:
		data = 1;
		break;
	case 2:
		data = open(argv[1], ORDWR);
		if(data < 0){
			fprint(2, "hayes: %s opening %s\n", syserr(), argv[1]);
			exits("hayes");
		}
		sprint(cname, "%sctl", argv[1]);
		ctl = open(cname, ORDWR);
		break;
	default:
		usage();
	}

	setspeed(ctl, 2400);
	godial(data, ctl, argv[0]);
	exits(0);
}

void
setspeed(int ctl, int baud)
{
	char buf[32];

	if(ctl < 0)
		return;
	sprint(buf, "b%d", baud);
	write(ctl, buf, strlen(buf));
}

#define SAY(f, x) (write(f, x, strlen(x)) == strlen(x))

void
godial(int data, int ctl, char *number)
{
	char dialstr[2*NAMELEN];
	int m;
	int baud;

	/* get the modem's attention */
	if(!SAY(data, "\r+++\r"))
		punt("failed write");
	readmsg(data, 2);
	sleep(1000);

	/* tell it to disable command echo and give wordy result codes */
	if(!SAY(data, "ATQ0V1E1\r"))
		punt("failed write");
	m = readmsg(data, 2);
	if(m < 0)
		punt("can't get modem's attention");
	sleep(1000);

	/* tell it to change baud rate once it connects */
	if(!SAY(data, "AT\\J1\r"))
		punt("failed write");
	m = readmsg(data, 2);
	if(m < 0)
		punt("can't get modem's attention");
	sleep(1000);

	/* godial */
	sprint(dialstr, "ATD%c%s\r", pulsed ? 'P' : 'T', number);
	if(!SAY(data, dialstr))
		punt("failed write");
	m = readmsg(data, 30);
	if(m < 0)
		punt("can't get modem's attention");
	switch(m){
	case 5:
	case 11:
	case 13:
		baud = 1200;
		break;
	case 9:
	case 12:
	case 14:
		baud = 2400;
		break;
	case 10:
		baud = 300;
		break;
	default:
		baud = 0;
		fprint(2, "hayes: dial failed - %s\n", msgs[m]);
		exits("hayes");
	}
	setspeed(ctl, baud);
	fprint(2, "hayes: connected at %d baud\n", baud);
}

/*
 *  read till we see a message or we time out
 */
int
readmsg(int f, int secs)
{
	char listenbuf[2*NAMELEN];
	ulong start;
	char *p;
	int len;
	Dir d;
	char **pp;

	p = listenbuf;
	len = sizeof(listenbuf) - 1;
	for(start = time(0); time(0) <= start+secs;){
		if(dirfstat(f, &d) < 0)
			punt("failed read");
		if(d.length == 0)
			continue;
		if(read(f, p, 1) <= 0)
			punt("failed read");
		if(*p == '\n' || *p == '\r' || len == 0){
			*p = 0;
/*			if(p != listenbuf)
				fprint(2, "%s\n", listenbuf); /**/
			for(pp = msgs; *pp; pp++)
				if(strcmp(*pp, listenbuf) == 0){
					return pp - msgs;
				}
			p = listenbuf;
			len = sizeof(listenbuf) - 1;
			continue;
		}
		len--;
		p++;
	}
	return -1;
}

int
wasintr(void)
{
	char err[ERRLEN];

	errstr(err);
	return strcmp(err, "interrupted") == 0;
}

void
punt(char *msg)
{
	fprint(2, "hayes: %s\n", msg);
	exits(msg);
}

char*
syserr(void)
{
	static char err[ERRLEN];
	errstr(err);
	return err;
}

