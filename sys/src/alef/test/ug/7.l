#include	<alef.h>

void
kbdproc(chan(int) kbdc)
{
	byte buf[UTFmax];
	int i, n, fd, ctlfd;
	Rune r;

	fd = open("/dev/cons", OREAD);
	ctlfd = open("/dev/consctl", OWRITE);
	write(ctlfd, "rawon", 5);	/* set raw mode */
	i = 0;
	for(;;) {
		n = read(fd, buf+i, 1);
		if(n <= 0 || buf[i] == 0x04) {
			kbdc <-= -1;
			return;
		}
		i++;
		if(fullrune(buf, i)) {
			chartorune(&r, buf);
			kbdc <-= r;
			i = 0;
		}
	}
}

void
main(void)
{
	int r;
	chan(int) kbd;

	alloc kbd;
	proc kbdproc(kbd);

	for(;;) {
		r = <-kbd;
		if(r < 0)
			terminate(nil);
		print("%C", r);
	}
}
