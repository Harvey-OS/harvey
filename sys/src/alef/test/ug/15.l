#include	<alef.h>

enum {
	BUFSIZE	=	4096,
};

int
decode(int fd, byte *buf, int n)
{
	byte *p;

	for(p = buf; p < buf+n; p++) {
		if(read(fd, p, 1) <= 0)
			break;
		if(*p == '\n')
			return p-buf+1;
	}
	return p-buf;
}

void
display(byte *buf, int n)
{
	write(1, buf, n);
}

void
waitforinput(void)
{
	byte c;

	read(0, &c, 1);
	if(c == 0x7f)
		exits(nil);
}

void
main(int argc, byte **argv)
{
	byte *active, *passive;
	int fd, fd2, n;

	if(argc < 2) {
		fprint(2, "give file name as arg\n");
		exits("usage");
	}
	fd = open(argv[1], OREAD);
	check fd >= 0, "open error";

	fd2 = open("/dev/consctl", OWRITE);
	check fd2 >= 0, "open error";
	write(fd2, "rawon", 5);

	active = malloc(BUFSIZE);
	passive = malloc(BUFSIZE);

	n = decode(fd, active, BUFSIZE);	/* first block */
	check n > 0, "read error";
	while(active != nil) {
		par {
			display(active, n);

			n = decode(fd, passive, BUFSIZE);
			if(n <= 0)
				passive = nil;
		}
		(active, passive) = (passive, active);
		waitforinput();
	}
}
