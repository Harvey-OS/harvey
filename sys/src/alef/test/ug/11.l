#include	<alef.h>

aggr Point {
	int x;
	int y;
};

aggr Mevent {
	Point;
	int	buttons;
};

aggr Alarmlist {
	Alarmlist *next;
	int	msec;
	chan(int)	ch;
};

int		kbdpid, mousepid, alarmpid;

int
eqpt(Point p1, Point p2)
{
	return p1.x == p2.x && p1.y == p2.y;
}

void
consumealarm(chan(int) ch)
{
	<-ch;
	unalloc ch;
}

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
mousetask(chan(Mevent) mc, chan(tuple(int, chan(int))) alarm)
{
	Mevent m, lastm;
	chan(int) dummy, ch;

	alloc dummy;
	ch = dummy;
	for(;;) {
		alt {
		case m = <-mc:
			if((m.buttons&0x07) == 0)
				break;
			if(ch == dummy) { /* no alarm pending */
				alloc ch;
				alarm <-= (500, ch);
				lastm = m;
				print("*");
			} else {
				task consumealarm(ch);
				ch = dummy;
				if(lastm.buttons == m.buttons
				   && eqpt(lastm.Point, m.Point))
					print("$");
				else
					print("@");
			}
			break;
		case <-ch:		/* alarm expired */
			unalloc ch;
			ch = dummy;
			print("@");
			break;
		}
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

int	msecfd = -1;

uint
msec()
{
	byte buf[16];
	int n;

	if(msecfd < 0) {
		msecfd = open("/dev/msec", OREAD);
		check msecfd >= 0;
	}
	seek(msecfd, 0, 0);
	n = read(msecfd, buf, sizeof(buf)-1);
	check n > 0;
	buf[n] = 0;
	return strtoui(buf, nil, 10);
}

void
addalarm(Alarmlist **head, int ms, chan(int) ch)
{
	Alarmlist *ap, **l, *new;

	alloc new;

	new->ch = ch;
	new->msec = ms;
	l = head;
	for(ap = *head; ap; ap = ap->next)
		if(ms < ap->msec)
			break;
		else
			l = &ap->next;
	new->next = *l;
	*l = new;
}

void
alarmproc(chan(tuple(int, chan(int))) alrmch)
{
	Alarmlist *ap, *alarmlist;
	int a, t, dt;
	chan(int)[1] dummy;
	chan(int) reply;

	alarmpid = getpid();
	alloc dummy;
	dummy <-= 1;
	alarmlist = nil;
	t = msec();

	for(;;) {
		if(alarmlist == nil) {
			/* no alarms - get client request */
			(a, reply) = <-alrmch;
			addalarm(&alarmlist, a, reply);
			t = msec();
		} else while(?alrmch) {
			alt {
			case (a, reply) = <-alrmch:
				addalarm(&alarmlist, a, reply);
				break 2;
			case <-dummy:
				dummy <-= 1;
				break;
			}
		}
		sleep(1);		/* sleep 1ms */
		dt = msec()-t;
		t += dt;
		for(ap = alarmlist; ap != nil;) {
			if(ap->msec <= dt) {
				/* send alarm to client */
				ap->ch <-= 1;
				alarmlist = ap->next;
				unalloc ap;
				ap = alarmlist;
			} else {
				ap->msec -= dt;
				ap = ap->next;
			}
		}
	}
}

void
main(void)
{
	chan(int)[100] kbd;
	chan(int) term;
	chan(Mevent) mouse;
	chan(tuple(int, chan(int))) alarm;

	alloc kbd, mouse, term, alarm;
	proc kbdproc(kbd, term), mouseproc(mouse, term),
		alarmproc(alarm);
	task kbdtask(kbd), mousetask(mouse, alarm);

	<-term;		/* main thread blocks here */
	postnote(PNPROC, mousepid, "kill");
	postnote(PNPROC, kbdpid, "kill");
	postnote(PNPROC, alarmpid, "kill");
	exits(nil);
}
