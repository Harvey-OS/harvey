#include <u.h>
#include <libc.h>

enum {
	Soh=	0x1,
	Eot=	0x4,
	Ack=	0x6,
	Nak=	0x15,
	Cancel=	0x18,
};

int notifyf(void*, char*);
int readupto(uchar*, int);
int receive(int, uchar);
void send(int);

int debug, dfd;

void
main(int argc, char **argv)
{
	int fd;
	uchar seqno;
	ulong bytes;

	ARGBEGIN {
	case 'd':
		dfd = 2;
		debug = 1;
		break;
	} ARGEND

	if(argc != 1){
		fprint(2, "usage: xmr filename\n");
		exits("usage");
	}
	fd = open("/dev/consctl", OWRITE);
	if(fd >= 0)
		write(fd, "rawon", 5);
	fd = create(argv[0], ORDWR, 0666);
	if(fd < 0){
		perror("xmr: create");
		exits("create");
	}

	atnotify(notifyf, 1);
	send(Nak);

	/*
	 *  keep receiving till the other side gives up
	 */
	bytes = 0;
	for(seqno = 1; ; seqno++){
		if(receive(fd, seqno) == -1)
			break;
		bytes += 128;
	}
	fprint(2, "xmr: received %ld bytes\n", bytes);
	exits(0);
}

void
send(int byte)
{
	uchar c;

	c = byte;
	if(write(1, &c, 1) != 1){
		fprint(2, "xmr: hungup\n");
		exits("hangup");
	}
}

int
readupto(uchar *a, int len)
{
	int n;
	int sofar;

	for(sofar = 0; sofar < len; sofar += n){
		n = read(0, a, len-sofar);
		if(n <= 0){
			send(Nak);
			return sofar;
		}
		if(*a == Eot || *a == Cancel)
			return sofar + n;
		a += n;
	}
	return sofar;

}

int
receive(int fd, uchar seqno)
{
	uchar buf[128+4];
	uchar sum;
	uchar *p;
	int n;
	int tries;
	int have;

	for(have = 0, tries = 0;; tries++){
		if(debug)
			fprint(dfd, "have == %d\n", have);
		if(tries > 10){
			fprint(2, "xmr: timed out\n");
			if(debug)
				close(dfd);
			exits("timeout");
		}

		/* try to gather up a block */
		alarm(15*1000);
		n = readupto(&buf[have], 132-have);
		alarm(0);
		have += n;
		if(have){
			switch(buf[0]){
			case Eot:
				send(Ack);
				return -1;
			case Cancel:
				fprint(2, "xmr: transfer aborted by sender\n");
				exits("cancel");
			}
		}
		if(have != 132)
			continue;

		/* checksum */
		for(p = buf, sum = 0; p < &buf[128+3]; p++)
			sum += *p;

		/* If invalid block, resynchronize */
		if(buf[0] != Soh || buf[2] != (255-buf[1]) || sum != buf[131]){
			if(debug){
				fprint(dfd, "resync %2.2ux %d %d %ux %ux\n", buf[0],
					buf[1], buf[2], sum, buf[131]);
				write(dfd, (char*)buf+3, 128);
				fprint(dfd, "\n");
			}
			p = memchr(buf+1, Soh, 131);
			if(p){
				have = 132-(p-buf);
				memmove(buf, p, have);
			} else
				have = 0;
			continue;
		}

		/* it's probably a real block, so dump it if there's an error */
		have = 0;

		/* if this is the last block, ack */
		if(buf[1] == seqno-1){
			tries = 0;
			send(Ack);
		}else if(buf[1] == seqno){
			if(debug)
				fprint(dfd, "Ack\n");
			send(Ack);
			if(write(fd, buf+3, 128) != 128){
				fprint(2, "xmr: abort, error writing file\n");
				exits("write");
			}
			return 0;
		} else {
			send(Nak);
		}
	}
}

int
notifyf(void *a, char *msg)
{
	USED(a);
	if(strcmp(msg, "alarm") == 0)
		return 1;
	return 0;
}
