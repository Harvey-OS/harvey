#include <u.h>
#include <libc.h>
#include <ctype.h>

enum {
	Soh=	0x1,
	Eot=	0x4,
	Ack=	0x6,
	Nak=	0x15,
	Cancel=	0x18,
};

int send(uchar*, int);
int notifyf(void*, char*);

int debug;

void
errorout(int ctl, int bytes)
{
	uchar buf[2];

	buf[0] = Cancel;
	write(1, buf, 1);
	fprint(2, "\nxms: gave up after %d bytes\n", bytes);
	write(ctl, "rawoff", 6);
	exits("cancel");
}

void
main(int argc, char **argv)
{
	uchar c;
	uchar buf[128+4];
	uchar seqno;
	int fd, ctl;
	long n;
	int sum;
	uchar *p;
	int bytes;

	ARGBEGIN{
	case 'd':
		debug = 1;
		break;
	}ARGEND

	if(argc != 1){
		fprint(2, "usage: xms filename\n");
		exits("usage");
	}
	fd = open(argv[0], OREAD);
	if(fd < 0){
		perror("xms");
		exits("open");
	}

	ctl = open("/dev/consctl", OWRITE);
	if(ctl >= 0)
		write(ctl, "rawon", 5);

	/* give the other side a 30 seconds to signal ready */
	atnotify(notifyf, 1);
	alarm(30*1000);
	for(;;){
		if(read(0, &c, 1) != 1){
			fprint(2, "xms: timeout\n");
			exits("timeout");
		}
		c = c & 0x7f;
		if(c == Nak)
			break;
	}
	alarm(0);

	/* send the file in 128 byte chunks */
	for(bytes = 0, seqno = 1; ; bytes += n, seqno++){
		n = read(fd, buf+3, 128);
		if(n < 0)
			exits("read");
		if(n == 0)
			break;
		if(n < 128)
			memset(&buf[n+3], 0, 128-n);
		buf[0] = Soh;
		buf[1] = seqno;
		buf[2] = 255 - seqno;

		/* calculate checksum and stuff into last byte */
		sum = 0;
		for(p = buf; p < &buf[128+3]; p++)
			sum += *p;
		buf[131] = sum;

		if(send(buf, 132) < 0)
			errorout(ctl, bytes);
	}

	/* tell other side we're done */
	buf[0] = Eot;
	if(send(buf, 1) < 0)
		errorout(ctl, bytes);

	fprint(2, "xms: %d bytes transmitted\n", bytes);
	write(ctl, "rawoff", 6);
	exits(0);
}

/*
 *  send a message till it's acked or we give up
 */
int
send(uchar *buf, int len)
{
	int tries;
	int n;
	uchar c;

	for(tries = 0;; tries++, sleep(2*1000)){
		if(tries == 10)
			return -1;
		if(write(1, buf, len) != len)
			return -1;
		
		alarm(30*1000);
		n = read(0, &c, 1);
		alarm(0);
		if(debug) switch(c){
		case Soh:
			fprint(2, " Soh");
			break;
		case Eot:
			fprint(2, " Eot");
			break;
		case Ack:
			fprint(2, " Ack");
			break;
		case Nak:
			fprint(2, " Nak");
			break;
		case Cancel:
			fprint(2, "\nremote Cancel\n");
			return -1;
		default:
			if(isprint(c))
				fprint(2, "%c", c);
			else
				fprint(2, " \\0%o", c);
		}
		c = c & 0x7f;
		if(n == 1 && c == Ack)
			break;
	}
	return 0;
}

int
notifyf(void *a, char *msg)
{
	USED(a);
	if(strcmp(msg, "alarm") == 0)
		return 1;
	return 0;
}
