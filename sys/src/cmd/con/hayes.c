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
	char *cname;

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
		cname = malloc(strlen(argv[1])+4);
		sprint(cname, "%sctl", argv[1]);
		ctl = open(cname, ORDWR);
		free(cname);
		break;
	default:
		usage();
	}
	godial(data, ctl, argv[0]);
	exits(0);
}

int
send(int fd, char *x)
{
	return write(fd, x, strlen(x));
}

void
godial(int data, int ctl, char *number)
{
	char *dialstr;
	int m;
	int baud;

	/* get the modem's attention */
	if(send(data, "\r+++\r") < 0)
		punt("failed write");
	readmsg(data, 2);
	sleep(1000);

	/* initialize */
	if(send(data, "ATZ\r") < 0)
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
	 */
	if(send(data, "ATQ0V1E1M1\r") < 0)
		punt("failed write");
	m = readmsg(data, 2);
	if(m != Ok)
		punt("can't get modem's attention");
	if(send(data, "ATW1\r") < 0)
		punt("failed write");
	readmsg(data, 2);
	sleep(1000);

	/* godial */
	dialstr = malloc(6+strlen(number));
	sprint(dialstr, "ATD%c%s\r", pulsed ? 'P' : 'T', number);
	if(send(data, dialstr) < 0) {
		free(dialstr);
		punt("failed write");
	}
	free(dialstr);
	m = readmsg(data, 60);
	if(m != Success)
		punt("dial failed: %s", msgbuf);
	baud = getspeed(msgbuf, 9600);
	setspeed(ctl, baud);
	fprint(2, "hayes: connected at %d baud\n", baud);
}

/*
 *  read until we see a message or we time out
 */
int
readmsg(int f, int secs)
{
	ulong start;
	char *p;
	int len;
	Dir *d;
	Msg *pp;

	p = msgbuf;
	len = sizeof(msgbuf) - 1;
	for(start = time(0); time(0) <= start+secs;){
		if((d = dirfstat(f)) == nil)
			punt("failed read");
		if(d->length == 0){
			free(d);
			sleep(100);
			continue;
		}
		free(d);
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
	va_list arg;
	int n;

	strcpy(buf, "hayes: ");
	va_start(arg, fmt);
	n = vseprint(buf+strlen(buf), buf+sizeof(buf), fmt, arg) - buf;
	va_end(arg);
	buf[n] = '\n';
	write(2, buf, n+1);
	exits("hayes");
}
