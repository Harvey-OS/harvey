#include	<alef.h>

aggr Point {
	int x;
	int y;
};

aggr Mevent {
	Point;
	int	buttons;
};

int		kbdpid, mousepid;

void
kbdtask(chan(int) kbdc)
{
	int r;

	for(;;) {
		r = <-kbdc;
		print("%C", r);
	}
}

void
kbdproc(chan(int) kbdc, chan(int) termc)
{
	byte buf[UTFmax];
	int i, n, fd, ctlfd;
	Rune r;

	kbdpid = getpid();
	fd = open("/dev/cons", OREAD);
	ctlfd = open("/dev/consctl", OWRITE);
	write(ctlfd, "rawon", 5);	/* set raw mode */
	i = 0;
	for(;;) {
		n = read(fd, buf+i, 1);
		if(n <= 0 || buf[i] == 0x04) {
			termc <-= -1;
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
mousetask(chan(Mevent) mc)
{
	Mevent m;

	for(;;) {
		m = <-mc;
		print("[%x @ (%d, %d)]", m.buttons, m.x, m.y);
	}
}

void
mouseproc(chan(Mevent) mc, chan(int) termc)
{
	int fd, n;
	byte buf[1024];
	Mevent m;

	mousepid = getpid();
	fd = open("/dev/mouse", OREAD);
	for(;;) {
		n = read(fd, buf, sizeof(buf));
		if(n < 10) {
			termc <-= 1;
			return;
		}
		m.buttons = buf[1];
		m.x = buf[5]<<24|buf[4]<<16|buf[3]<<8|buf[2];
		m.y = buf[9]<<24|buf[8]<<16|buf[7]<<8|buf[6];
		mc <-= m;
	}
}

void
main(void)
{
	chan(int)[100] kbd;
	chan(int) term;
	chan(Mevent) mouse;

	alloc kbd, mouse, term;
	proc kbdproc(kbd, term), mouseproc(mouse, term);
	task kbdtask(kbd), mousetask(mouse);

	<-term;		/* main thread blocks here */
	postnote(PNPROC, mousepid, "kill");
	postnote(PNPROC, kbdpid, "kill");
	exits(nil);
}
