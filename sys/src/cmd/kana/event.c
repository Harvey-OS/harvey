#include <u.h>
#include <libc.h>
#include <libg.h>

enum{ MAXSLAVE = 32 };

typedef struct Slave	Slave;
typedef struct Ebuf	Ebuf;

struct Slave{
	int	pid;
	Ebuf	*head;		/* queue of messages for this descriptor */
	Ebuf	*tail;
};

struct Ebuf{
	Ebuf	*next;
	int	n;		/* number of bytes in buf */
	uchar	buf[EMAXMSG];
};

static	Slave	eslave[MAXSLAVE];
static	int	nslave;
static	int	Smouse = -1;
static	int	Skeyboard = -1;
static	int	Stimer = -1;
static	int	epipe[2];
static	Ebuf	*ebread(Slave*);
static	int	eforkslave(ulong);
static	void	extract(void);
static	void	ekill(void);
static	int	enote(void *, char *);
int logfid;

Mouse
emouse(void)
{
	Mouse m;
	Ebuf *eb;
	static but[2];
	int b;

	if(Smouse < 0)
		berror("events: mouse not initialzed");
	eb = ebread(&eslave[Smouse]);
	b = eb->buf[1] & 15;
	but[b>>3] = b & 7;
	m.buttons = but[0] | but[1];
	m.xy.x = BGLONG(eb->buf+2);
	m.xy.y = BGLONG(eb->buf+6);
	if (b & 8)		/* window relative */
		m.xy = add(m.xy, screen.r.min);
	if (logfid)
		fprint(logfid, "b: %d xy: %d %d\n", m.buttons, m.xy);
	free(eb);
	return m;
}

int
ekbd(void)
{
	Ebuf *eb;
	int c;

	if(Skeyboard < 0)
		berror("events: keyboard not initialzed");
	eb = ebread(&eslave[Skeyboard]);
	c = eb->buf[0] + (eb->buf[1]<<8);
	free(eb);
	return c;
}

ulong
event(Event *e)
{
	return eread(~0UL, e);
}

ulong
eread(ulong keys, Event *e)
{
	Ebuf *eb;
	int i;

	if(keys) for(;;){
		for(i=0; i<nslave; i++)
			if((keys & (1<<i)) && eslave[i].head){
				if(i == Smouse)
					e->mouse = emouse();
				else if(i == Skeyboard)
					e->kbdc = ekbd();
				else if(i == Stimer)
					eslave[i].head = 0;
				else{
					eb = ebread(&eslave[i]);
					e->n = eb->n;
					memmove(e->data, eb->buf, e->n);
					free(eb);
				}
				return 1<<i;
			}
		extract();
	}
	return 0;
}

int
ecanmouse(void)
{
	if(Smouse < 0)
		berror("events: mouse not initialzed");
	return ecanread(Emouse);
}

int
ecankbd(void)
{
	if(Skeyboard < 0)
		berror("events: keyboard not initialzed");
	return ecanread(Ekeyboard);
}

int
ecanread(ulong keys)
{
	Dir d;
	int i;

	for(;;){
		for(i=0; i<nslave; i++)
			if((keys & (1<<i)) && eslave[i].head)
				return 1;
		if(dirfstat(epipe[0], &d) < 0)
			berror("events: ecanread stat error");
		if(d.length == 0)
			return 0;
		extract();
	}
	return -1;
}

ulong
estart(ulong key, int fd, int n)
{
	char buf[EMAXMSG+1];
	int i, r;

	if(fd < 0)
		berror("events: bad file descriptor");
	if(n <= 0 || n > EMAXMSG)
		n = EMAXMSG;
	i = eforkslave(key);
	if(i < MAXSLAVE)
		return 1<<i;
	buf[0] = i - MAXSLAVE;
	while((r = read(fd, buf+1, n))>=0)
		if(write(epipe[1], buf, r+1)!=r+1)
			break;
	buf[0] = MAXSLAVE;
	write(epipe[1], buf, 1);
	_exits(0);
	return 0;
}

ulong
etimer(ulong key, int n)
{
	char t[2];

	if(Stimer != -1)
		berror("events: timer started twice");
	Stimer = eforkslave(key);
	if(Stimer < MAXSLAVE)
		return 1<<Stimer;
	if(n <= 0)
		n = 1000;
	t[0] = t[1] = Stimer - MAXSLAVE;
	do
		sleep(n);
	while(write(epipe[1], t, 2) == 2);
	t[0] = MAXSLAVE;
	write(epipe[1], t, 1);
	_exits(0);
	return 0;
}

static void
ekeyslave(int fd)
{
	Rune r;
	char t[3], k[10];
	int kr, kn, w;

	if(eforkslave(Ekeyboard) < MAXSLAVE)
		return;
	kn = 0;
	t[0] = Skeyboard;
	for(;;){
		while(!fullrune(k, kn)){
			kr = read(fd, k+kn, sizeof k - kn);
			if(kr < 0){
				t[0] = MAXSLAVE;
				write(epipe[1], t, 1);
				_exits(0);
			}
			kn += kr;
		}
		w = chartorune(&r, k);
		kn -= w;
		memmove(k, &k[w], kn);
		t[1] = r;
		t[2] = r>>8;
		write(epipe[1], t, 3);
	}
}

void
einit(ulong keys)
{
	int ctl, fd;

	if(pipe(epipe) < 0)
		berror("events: einit pipe");
	atexit(ekill);
	atnotify(enote, 1);
	if(keys&Ekeyboard){
		fd = open("/dev/cons", OREAD);
		ctl = open("/dev/consctl", OWRITE);
		if(fd < 0 || ctl < 0)
			berror("events: can't open /dev/cons");
		write(ctl, "holdoff", 7);
		write(ctl, "rawon", 5);
		for(Skeyboard=0; Ekeyboard & ~(1<<Skeyboard); Skeyboard++)
			;
		ekeyslave(fd);
		close(ctl);	/* keyboard child holds it open */
		close(fd);
	}
	if(keys&Emouse){
		fd = open("/dev/mouse", OREAD);
		if(fd < 0)
			berror("events: can't open /dev/mouse");
		estart(Emouse, fd, 10);
		for(Smouse=0; Emouse & ~(1<<Smouse); Smouse++)
			;
		close(fd);
	}
}

static Ebuf*
ebread(Slave *s)
{
	Ebuf *eb;
	Dir d;

	for(;;){
		if(dirfstat(epipe[0], &d) < 0)
			berror("events: eread stat error");
		if(s->head && d.length == 0)
			break;
		extract();
	}
	eb = s->head;
	s->head = s->head->next;
	if(s->head == 0)
		s->tail = 0;
	return eb;
}

static void
extract(void)
{
	Slave *s;
	Ebuf *eb;
	int i, n;
	uchar ebuf[EMAXMSG+1];

	bflush();
loop:
	if((n=read(epipe[0], ebuf, EMAXMSG+1)) < 0
	|| ebuf[0] >= MAXSLAVE)
		exits("eof on event pipe");
	if(n == 0)
		goto loop;
	i = ebuf[0];
	if(i >= nslave || n <= 1)
		berror("events: protocol error");
	s = &eslave[i];
	if(i == Stimer){
		s->head = (Ebuf *)1;
		return;
	}
	if(i == Skeyboard && n != 3)
		berror("events: protocol error");
	if(i == Smouse){
		if(n!=1+10 || ebuf[1]!='m')
			berror("events: protocol error");
		if(ebuf[2] & 0x80)
			ereshaped(bscreenrect(0));
		/* squash extraneous mouse events */
		if(s->head){
			free(s->head);
			s->head = s->tail = 0;
		}
	}
	/* try to save space by only alloacting as much buffer as we need */
	eb = malloc(sizeof(*eb) - sizeof(eb->buf) + n - 1);
	if(eb == 0)
		berror("events: protocol error");
	eb->n = n - 1;
	memmove(eb->buf, &ebuf[1], n - 1);
	eb->next = 0;
	if(s->head)
		s->tail = s->tail->next = eb;
	else
		s->head = s->tail = eb;
}

static int
eforkslave(ulong key)
{
	int i;

	for(i=0; i<MAXSLAVE; i++)
		if((key & ~(1<<i)) == 0 && eslave[i].pid == 0){
			if(nslave <= i)
				nslave = i + 1;
			switch(eslave[i].pid = fork()){
			case 0:
				atexitdont(bexit);
				atnotify(enote, 0);
				close(epipe[0]);
				return MAXSLAVE+i;
			case -1:
				fprint(2, "events: fork error\n");
				exits("fork");
			}
			eslave[i].head = eslave[i].tail = 0;
			return i;
		}
	berror("events: bad slave assignment");
	return -1;
}

static int
enote(void *v, char *s)
{
	char buf[32];
	int fd, i;

	USED(v, s);
	for(i=0; i<nslave; i++){
		sprint(buf, "/proc/%d/note", eslave[i].pid);
		fd = open(buf, OWRITE);
		if(fd > 0){
			write(fd, "die", 3);
			close(fd);
		}
	}
	return 0;
}

static void
ekill(void)
{
	enote(0, 0);
}
