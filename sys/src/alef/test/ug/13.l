#include	<alef.h>

adt Point {
	extern	int x;
	extern	int y;

		int eq(*Point, Point*);
};

adt Mevent {
	extern	Point;
	extern	int	buttons;

		int	fill(*Mevent, byte*, int);
};

adt Mouse {
		Mevent;
	extern	chan(Mevent) 	ch;
		chan(int)	term;
		int		fd;
		int		pid;

		Mouse*		init(byte*, chan(int));
		void		close(*Mouse);
	intern	void		mproc(*Mouse);
};

adt Keyboard {
	extern	chan(int)[100]	ch;
		chan(int)	term;
		int		kbdfd;
		int		ctlfd;
		int		pid;

		Keyboard*	init(byte*, chan(int));
		void		close(*Keyboard);
		int		ctl(*Keyboard, byte*);
	intern	void		kproc(*Keyboard);
};

aggr Alarmlist {
	Alarmlist*	next;
	int		msec;
	chan(int)	ch;

};

adt Alarm {
		int		pid;
	extern	chan(tuple(int, chan(int))) ch;
		Alarmlist*	list;

		Alarm*		init(void);
		void		close(*Alarm);
		void		add(*Alarm, int, chan(int));
	intern	void		aproc(*Alarm);
};

int
Point.eq(Point *p1, Point *p2)
{
	return p1->x == p2->x && p1->y == p2->y;
}

int
Mevent.fill(Mevent *m, byte *buf, int n)
{
	if(n < 10)
		return 0;
	m->buttons = buf[1];
	m->x = buf[5]<<24|buf[4]<<16|buf[3]<<8|buf[2];
	m->y = buf[9]<<24|buf[8]<<16|buf[7]<<8|buf[6];
	return 1;
}

Keyboard*
Keyboard.init(byte *device, chan(int) term)
{
	Keyboard *k;
	byte buf[128];

	alloc k;
	k->kbdfd = open(device, OREAD);
	if(k->kbdfd < 0) {
		unalloc k;
		return nil;
	}
	sprint(buf, "%sctl", device);
	k->ctlfd = open(buf, OWRITE);
	if(k->ctlfd < 0) {
		unalloc k;
		close(k->kbdfd);
		return nil;
	}
	alloc k->ch;
	k->term = term;
	k->ctl("rawon");
	proc k->kproc();
	return k;
}

void
Keyboard.close(Keyboard *k)
{
	if(k->pid)
		postnote(PNPROC, k->pid, "kill");
}

int
Keyboard.ctl(Keyboard *k, byte *msg)
{
	return write(k->ctlfd, msg, strlen(msg));
}

void
Keyboard.kproc(Keyboard *k)
{
	byte buf[UTFmax];
	int i, n;
	Rune r;

	k->pid = getpid();
	i = 0;
	for(;;) {
		n = read(k->kbdfd, buf+i, 1);
		if(n <= 0 || buf[i] == 0x04) {
			k->term <-= -1;
			continue;
		}
		i++;
		if(fullrune(buf, i)) {
			chartorune(&r, buf);
			k->ch <-= r;
			i = 0;
		}
	}
}

Mouse*
Mouse.init(byte *device, chan(int) term)
{
	Mouse *m;

	alloc m;
	m->fd = open(device, OREAD);
	if(m->fd < 0) {
		unalloc m;
		return nil;
	}
	alloc m->ch;
	m->term = term;
	proc m->mproc();
	return m;
}

void
Mouse.close(Mouse *m)
{
	if(m->pid)
		postnote(PNPROC, m->pid, "kill");
}

void
Mouse.mproc(Mouse *m)
{
	int n;
	byte buf[1024];

	m->pid = getpid();
	for(;;) {
		n = read(m->fd, buf, sizeof(buf));
		if(n < 0) {
			m->term <-= -1;
			continue;
		}
		if(m->fill(buf, n))
			m->ch <-= m->Mevent;
	}
}

Alarm*
Alarm.init(void)
{
	Alarm *a;

	alloc a, a->ch;

	proc a->aproc();
	return a;
}

void
Alarm.close(Alarm *a)
{
	if(a->pid)
		postnote(PNPROC, a->pid, "kill");
}

void
Alarm.add(Alarm *a, int ms, chan(int) ch)
{
	Alarmlist *ap, **l, *new;

	alloc new;

	new->ch = ch;
	new->msec = ms;
	l = &a->list;
	for(ap = a->list; ap; ap = ap->next)
		if(ms < ap->msec)
			break;
		else
			l = &ap->next;
	new->next = *l;
	*l = new;
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
Alarm.aproc(Alarm *a)
{
	Alarmlist *ap;
	uint t;
	int ms, dt;
	chan(int)[1] dummy;
	chan(int) reply;

	a->pid = getpid();
	alloc dummy;
	dummy <-= 1;
	t = msec();

	for(;;) {
		if(a->list == nil) {
			/* no alarms - get client request */
			(ms, reply) = <-a->ch;
			a->add(ms, reply);
			t = msec();
		} else while(?a->ch) {
			alt {
			case (ms, reply) = <-a->ch:
				a->add(ms, reply);
				break 2;
			case <-dummy:
				dummy <-= 1;
				break;
			}
		}
		sleep(1);		/* sleep 1ms */
		dt = msec()-t;
		t += dt;
		for(ap = a->list; ap != nil;) {
			if(ap->msec <= dt) {
				/* send alarm to client */
				ap->ch <-= 1;
				a->list = ap->next;
				unalloc ap;
				ap = a->list;
			} else {
				ap->msec -= dt;
				ap = ap->next;
			}
		}
	}
}

void
consumealarm(chan(int) ch)
{
	<-ch;
	unalloc ch;
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
				if(lastm.buttons == m.buttons && m.eq(&lastm))
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
kbdtask(chan(int) kbdc)
{
	int r;

	for(;;) {
		r = <-kbdc;
		print("%C", r);
	}
}

void
main(void)
{
	Mouse *m;
	Keyboard *k;
	Alarm *a;
	chan(int) term;

	alloc term;

	m = .Mouse.init("/dev/mouse", term);
	if(m == nil)
		exits("mouse");

	k = .Keyboard.init("/dev/cons", term);
	if(k == nil) {
		m->close();
		exits("keyboard");
	}

	a = .Alarm.init();
	if(k == nil) {
		m->close();
		k->close();
		exits("alarm");
	}

	task kbdtask(k->ch), mousetask(m->ch, a->ch);

	<-term;			/* main thread blocks here */
	k->close();
	m->close();
	a->close();
	exits(nil);
}
