#include <u.h>
#include <libc.h>
#include <ctype.h>

enum {
	Soh=	0x1,
	Stx=	0x2,
	Eot=	0x4,
	Ack=	0x6,
	Nak=	0x15,
	Cancel=	0x18,
};

int send(uchar*, int);
int notifyf(void*, char*);

int debug, progress, onek;

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

ushort
updcrc(int c, ushort crc)
{
	int count;

	for (count=8; --count>=0;) {
		if (crc & 0x8000) {
			crc <<= 1;
			crc += (((c<<=1) & 0400)  !=  0);
			crc ^= 0x1021;
		}
		else {
			crc <<= 1;
			crc += (((c<<=1) & 0400)  !=  0);
		}
	}
	return crc;	
}

void
main(int argc, char **argv)
{
	uchar c;
	uchar buf[1024+5];
	uchar seqno;
	int fd, ctl;
	long n;
	int sum;
	uchar *p;
	int bytes;
	int crcmode;

	ARGBEGIN{
	case 'd':
		debug = 1;
		break;
	case 'p':
		progress = 1;
		break;
	case '1':
		onek = 1;
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
	if(ctl < 0){
		perror("xms");
		exits("consctl");
	}
	write(ctl, "rawon", 5);

	/* give the other side a 30 seconds to signal ready */
	atnotify(notifyf, 1);
	alarm(30*1000);
	crcmode = 0;
	for(;;){
		if(read(0, &c, 1) != 1){
			fprint(2, "xms: timeout\n");
			exits("timeout");
		}
		c = c & 0x7f;
		if(c == Nak)
			break;
		if(c == 'C') {
			if (debug)
				fprint(2, "crc mode engaged\n");
			crcmode = 1;
			break;
		}
	}
	alarm(0);

	/* send the file in 128/1024 byte chunks */
	for(bytes = 0, seqno = 1; ; bytes += n, seqno++){
		n = read(fd, buf+3, onek ? 1024 : 128);
		if(n < 0)
			exits("read");
		if(n == 0)
			break;
		if(n < (onek ? 1024 : 128))
			memset(&buf[n+3], 0, (onek ? 1024 : 128)-n);
		buf[0] = onek ? Stx : Soh;
		buf[1] = seqno;
		buf[2] = 255 - seqno;

		/* calculate checksum and stuff into last byte */
		if (crcmode) {
			unsigned short crc;
			crc = 0;
			for(p = buf + 3; p < &buf[(onek ? 1024 : 128)+3]; p++)
				crc = updcrc(*p, crc);
			crc = updcrc(0, crc);
			crc = updcrc(0, crc);
			buf[(onek ? 1024 : 128) + 3] = crc >> 8;
			buf[(onek ? 1024 : 128) + 4] = crc;
		}
		else {
			sum = 0;
			for(p = buf + 3; p < &buf[(onek ? 1024 : 128)+3]; p++)
				sum += *p;
			buf[(onek ? 1024 : 128) + 3] = sum;
		}

		if(send(buf, (onek ? 1024 : 128) + 4 + crcmode) < 0)
			errorout(ctl, bytes);
		if (progress && bytes % 10240 == 0)
			fprint(2, "%dK\n", bytes / 1024);
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
