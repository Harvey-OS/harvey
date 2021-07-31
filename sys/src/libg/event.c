#include <u.h>
#include <libc.h>
#include <libg.h>

Slave	eslave[MAXSLAVE];
int	Skeyboard = -1;
int	Smouse = -1;
int	Stimer = -1;
int	logfid;

static	int	nslave;
static	int	isparent;
static	int	epipe[2];
static	int	eforkslave(ulong);
static	void	extract(void);
static	void	ekill(void);
static	int	enote(void *, char *);

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

	if(keys == 0)
		return 0;
	for(;;){
		for(i=0; i<nslave; i++)
			if((keys & (1<<i)) && eslave[i].head){
				if(i == Smouse)
					e->mouse = emouse();
				else if(i == Skeyboard)
					e->kbdc = ekbd();
				else if(i == Stimer)
					eslave[i].head = 0;
				else{
					eb = _ebread(&eslave[i]);
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
	while((r = read(fd, buf+1, n))>0)
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
			if(kr <= 0){
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

	isparent = 1;
	if(pipe(epipe) < 0)
		berror("events: einit pipe");
	atexit(ekill);
	atnotify(enote, 1);
	if(keys&Ekeyboard){
		fd = open("/dev/cons", OREAD);
		ctl = open("/dev/consctl", OWRITE);
		if(fd < 0)
			berror("events: can't open /dev/cons");
		if(ctl < 0)
			berror("events: can't open /dev/consctl");
		write(ctl, "rawon", 5);
		for(Skeyboard=0; Ekeyboard & ~(1<<Skeyboard); Skeyboard++)
			;
		ekeyslave(fd);
	}
	if(keys&Emouse){
		fd = open("/dev/mouse", OREAD);
		if(fd < 0)
			berror("events: can't open /dev/mouse");
		estart(Emouse, fd, 14);
		for(Smouse=0; Emouse & ~(1<<Smouse); Smouse++)
			;
	}
}

Ebuf*
_ebread(Slave *s)
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
	char msg[100];

	bflush();
loop:
	if((n=read(epipe[0], ebuf, EMAXMSG+1)) <= 0)
		berror("eof on event pipe");
	if(ebuf[0] >= MAXSLAVE){
		sprint(msg, "bad slave %d on event pipe", ebuf[0]);
		berror(msg);
	}
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
		if(n!=1+14 || ebuf[1]!='m')
			berror("events: protocol error");
		if(ebuf[2] & 0x80)
			ereshaped(bscreenrect(&screen.clipr));
		/* squash extraneous mouse events */
		if((eb=s->tail) && eb->buf[1] == ebuf[2]){
			memmove(eb->buf, &ebuf[1], n - 1);
			return;
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
			/*
			 * share the file descriptors so the last child
			 * out closes all connections to the window server
			 */
			switch(eslave[i].pid = rfork(RFPROC)){
			case 0:
				atexitdont(bexit);
				isparent = 0;
				return MAXSLAVE+i;
			case -1:
				fprint(2, "events: fork error\n");
				exits("fork");
			}
			eslave[i].head = eslave[i].tail = 0;
			return i;
		}
	berror("events: bad slave assignment");
	return 0;
}

static int
enote(void *v, char *s)
{
	int i, pid;

	USED(v, s);
	pid = getpid();
	if(!isparent)
		return 1;
	for(i=0; i<nslave; i++){
		if(pid == eslave[i].pid)
			continue;	/* don't kill myself */
		postnote(PNPROC, eslave[i].pid, "die");
	}
	close(epipe[0]);
	close(epipe[1]);
	return 0;
}

static void
ekill(void)
{
	enote(0, 0);
}
