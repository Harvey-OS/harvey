#include	<alef.h>

aggr Point {
	int x;
	int y;
};

aggr Mevent {
	Point;
	int	buttons;
};

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
mouseproc(chan(Mevent) mc)
{
	int fd, n;
	byte buf[1024];
	Mevent m;

	fd = open("/dev/mouse", OREAD);
	for(;;) {
		n = read(fd, buf, sizeof(buf));
		if(n <= 10)
			continue;
		m.buttons = buf[1];
		m.x = buf[5]<<24|buf[4]<<16|buf[3]<<8|buf[2];
		m.y = buf[9]<<24|buf[8]<<16|buf[7]<<8|buf[6];
		mc <-= m;
	}
}
void
main(void)
{
	int r;
	Mevent m;
	chan(int) kbd;
	chan(Mevent) mouse;

	alloc kbd, mouse;
	proc kbdproc(kbd), mouseproc(mouse);

	for(;;) {
		alt {
		case r = <-kbd:
			if(r < 0)
				terminate(nil);
			print("%C", r);
			break;
		case m = <-mouse:
			print("[%x @ (%d, %d)]", m.buttons, m.x, m.y);
			break;
		}
	}
}
