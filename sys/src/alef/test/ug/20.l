#include	<alef.h>

void stateA(chan(int));
void stateB(chan(int));
void stateC(chan(int));

int readcpid;

void
readc(chan(int) c)
{
	int fd, fd2;
	byte ch;

	readcpid = getpid();
	fd = open("/dev/cons", OREAD);
	fd2 = open("/dev/consctl", OWRITE);
	check fd >= 0 && fd2 >= 0, "keyboard open";
	write(fd2, "rawon", 5);
	for(;;) {
		check read(fd, &ch, 1) == 1, "read error";
		c <-= ch;
	}
}

void
stateA(chan(int) c)
{
	print("A");
	switch(<-c) {
	default:
	case '1':	become stateA(c);
	case '2':	become stateB(c);
	case '3':	become stateC(c);
	}
}

void
stateB(chan(int) c)
{
	print("B");
	switch(<-c) {
	default:
	case '3':
	case '1':	become stateB(c);
	case '2':	become stateA(c);
	}
}

void
stateC(chan(int) c)
{
	print("C");
	switch(<-c) {
	default:	become stateC(c);
	case '1':	become stateA(c);
	case '2':	become stateB(c);
	case '3':	print("\n");		/* terminal state */
			if(readcpid)
				postnote(PNPROC,readcpid, "kill");
			exits(nil);
	}
}

void
main(void)
{
	chan(int) c;

	alloc c;
	proc readc(c);
	stateA(c);
}
