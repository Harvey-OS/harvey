#include <u.h>
#include <libc.h>

void setspeed(int, int);
int getspeed(char*, int);
void godial(int, int, char*);
int readmsg(int, int);
void punt(char*, ...);

int pulsed;
int verbose;
char msgbuf[128];		/* last message read */

enum
{
	Ok,
	Success,
	Failure,
	Noise,
};

typedef struct Msg	Msg;
struct Msg
{
	char	*text;
	int	type;
};


Msg msgs[] =
{
	{ "OK",			Ok, },
	{ "NO CARRIER", 	Failure, },
	{ "ERROR",		Failure, },
	{ "NO DIALTONE",	Failure, },
	{ "BUSY",		Failure, },
	{ "NO ANSWER",		Failure, },
	{ "CONNECT",		Success, },
	{ 0,			0 },
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
	case 'v':
		verbose = 1;
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
			fprint(2, "hayes: %r opening %s\n", argv[1]);
			exits("hayes");
		}
		sprint(cname, "%sctl", argv[1]);
		ctl = open(cname, ORDWR);
		break;
	default:
		usage();
	}
	godial(data, ctl, argv[0]);
	exits(0);
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

	/* initialize */
	if(!SAY(data, "ATZ\r"))
		punt("failed write");
	m = readmsg(data, 2);
	if(m < 0)
		punt("can't get modem's attention");

	/*
	 *	Q0 = report result codes
	 * 	V1 = full word result codes
	 *	W1 = negotiation progress codes enabled
	 *	E1 = echo commands
	 *	M1 = speaker on until on-line
	 *	\N7 = LAP-M fall back to MNP fall back to no error correction
	 *	C1 = compression requested (MNP or LAP-M)
	 *	\J1 = change baud rate on connection
	 *	\Q6 = CTS/RTS flow control
	 */
	if(!SAY(data, "ATQ0V1W1E1M1\\N7C1\\J1\\Q6\r"))
		punt("failed write");
	m = readmsg(data, 2);
	if(m != Ok)
		punt("can't get modem's attention");
	sleep(1000);

	/* godial */
	sprint(dialstr, "ATD%c%s\r", pulsed ? 'P' : 'T', number);
	if(!SAY(data, dialstr))
		punt("failed write");
	m = readmsg(data, 60);
	if(m != Success)
		punt("dial failed: %s", msgbuf);
	baud = getspeed(msgbuf, 9600);
	setspeed(ctl, baud);
	fprint(2, "hayes: connected at %d baud\n", baud);
}

/*
 *  read till we see a message or we time out
 */
int
readmsg(int f, int secs)
{
	ulong start;
	char *p;
	int len;
	Dir d;
	Msg *pp;

	p = msgbuf;
	len = sizeof(msgbuf) - 1;
	for(start = time(0); time(0) <= start+secs;){
		if(dirfstat(f, &d) < 0)
			punt("failed read");
		if(d.length == 0){
			sleep(100);
			continue;
		}
		if(read(f, p, 1) <= 0)
			punt("failed read");
		if(*p == '\n' || *p == '\r' || len == 0){
			*p = 0;
			if(verbose && p != msgbuf)
				fprint(2, "%s\n", msgbuf);
			for(pp = msgs; pp->text; pp++)
				if(strncmp(pp->text, msgbuf, strlen(pp->text))==0)
					return pp->type;
			start = time(0);
			p = msgbuf;
			len = sizeof(msgbuf) - 1;
			continue;
		}
		len--;
		p++;
	}
	strcpy(msgbuf, "No response from modem");
	return Noise;
}

/*
 *  get baud rate from a connect message
 */
int
getspeed(char *msg, int speed)
{
	char *p;
	int s;

	p = msg + sizeof("CONNECT") - 1;
	while(*p == ' ' || *p == '\t')
		p++;
	s = atoi(p);
	if(s <= 0)
		return speed;
	else
		return s;
}

/*
 *  set speed and RTS/CTS modem flow control
 */
void
setspeed(int ctl, int baud)
{
	char buf[32];

	if(ctl < 0)
		return;
	sprint(buf, "b%d", baud);
	write(ctl, buf, strlen(buf));
	write(ctl, "m1", 2);
}


void
punt(char *fmt, ...)
{
	char buf[256];
	int n;

	strcpy(buf, "hayes: ");
	n = doprint(buf+strlen(buf), buf+sizeof(buf), fmt, (&fmt+1)) - buf;
	buf[n] = '\n';
	write(2, buf, n+1);
	exits("hayes");
}
