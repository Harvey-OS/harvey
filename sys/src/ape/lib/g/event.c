#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <stdio.h>
#include <libg.h>

enum{ MAXSLAVE = 32 };

typedef struct Slave	Slave;
typedef struct Ebuf	Ebuf;

struct Slave{
	int	pid;
	int	ack;		/* fd for sending flow-control ack to slave */
	int	count;		/* bytes received but not acked */
	Ebuf	*head;		/* queue of messages for this descriptor */
	Ebuf	*tail;
};

struct Ebuf{
	Ebuf		*next;
	int		n;		/* number of bytes in buf */
	unsigned char	buf[EMAXMSG];
};

static	Slave	eslave[MAXSLAVE];
static	int	nslave;
static	int	isslave = 0;
static	int	Smouse = -1;
static	int	Skeyboard = -1;
static	int	Stimer = -1;
static	int	epipe[2];
static	Ebuf	*ebread(Slave*);
static	int	eforkslave(unsigned long);
static	void	extract(void);
static	void	eerror(char*);
static	void	ekill(void);
static	void	enote(int);

Mouse
emouse(void)
{
	Mouse m;
	Ebuf *eb;

	if(Smouse < 0)
		eerror("mouse not initialzed");
	eb = ebread(&eslave[Smouse]);
	m.buttons = eb->buf[1] & 7;
	m.xy.x = BGLONG(eb->buf+2);
	m.xy.y = BGLONG(eb->buf+6);
	free(eb);
	return m;
}

int
ekbd(void)
{
	Ebuf *eb;
	int c;

	if(Skeyboard < 0)
		eerror("keyboard not initialzed");
	eb = ebread(&eslave[Skeyboard]);
	c = eb->buf[0];
	free(eb);
	return c;
}

unsigned long
event(Event *e)
{
	return eread(~0UL, e);
}

unsigned long
eread(unsigned long keys, Event *e)
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
					eb = ebread(&eslave[i]);
					e->n = eb->n;
					memmove(e->data, eb->buf, e->n);
					free(eb);
				}
				return 1<<i;
			}
		extract();
	}
}

int
ecanmouse(void)
{
	if(Smouse < 0)
		eerror("mouse not initialzed");
	return ecanread(Emouse);
}

int
ecankbd(void)
{
	if(Skeyboard < 0)
		eerror("keyboard not initialzed");
	return ecanread(Ekeyboard);
}

int
ecanread(unsigned long keys)
{
	extern int _FSTAT(int, void*);
	Slave *s;
	char st[256];
	char *p;
	int i;

	for(;;){
		for(i=0; i<nslave; i++)
			if((keys & (1<<i)) && eslave[i].head)
				return 1;
		if(_FSTAT(epipe[0], &st) < 0)
			eerror("ecanread: stat error");
		p = st;
		p += 3*28+5*4;
		if(p[0] == 0 && p[1] == 0 && p[2] == 0 && p[3] == 0)
			return 0;
		extract();
	}
}

unsigned long
estart(unsigned long key, int fd, int n)
{
	char buf[EMAXMSG+1];
	int i, r, count, ack[2];

	if(fd < 0)
		eerror("bad file descriptor");
	if(n <= 0 || n > EMAXMSG)
		n = EMAXMSG;
	if(pipe(ack) < 0)
		eerror("estart pipe");
	i = eforkslave(key);
	if(i < MAXSLAVE){
		eslave[i].ack = ack[1];
		close(ack[0]);
		return 1<<i;
	}
	close(ack[1]);
	buf[0] = i - MAXSLAVE;
	count = 0;
	while((r = read(fd, buf+1, n))>0 && write(epipe[1], buf, r+1)==r+1){
		count += r;
		if(count > EMAXMSG){
			read(ack[0], buf+1, 1);
			count -= EMAXMSG;
		}
	}
	buf[0] = MAXSLAVE;
	write(epipe[1], buf, 1);
	_exit(0);
}

unsigned long
etimer(unsigned long key, int n)
{
	char t[2];
	extern void _nap(unsigned int);

	if(Stimer != -1)
		eerror("timer started twice\n");
	Stimer = eforkslave(key);
	if(Stimer < MAXSLAVE)
		return 1<<Stimer;
	if(n <= 0)
		n = 1000;
	t[0] = t[1] = Stimer - MAXSLAVE;
	do
		_nap(n);
	while(write(epipe[1], t, 2) == 2);
	t[0] = MAXSLAVE;
	write(epipe[1], t, 1);
	_exit(0);
}

void
einit(unsigned long keys)
{
	int ctl, fd;

	if(pipe(epipe) < 0)
		eerror("einit pipe");
	atexit(ekill);
	signal(SIGHUP, enote);
	signal(SIGINT, enote);
	signal(SIGTERM, enote);
	if(keys&Ekeyboard){
		fd = open("/dev/cons", O_RDONLY);
		ctl = open("/dev/consctl", O_WRONLY);
		if(fd < 0 || ctl < 0)
			berror("events: can't open /dev/cons");
		write(ctl, "holdoff", 7);
		write(ctl, "rawon", 5);
		estart(Ekeyboard, fd, 1);
		for(Skeyboard=0; Ekeyboard & ~(1<<Skeyboard); Skeyboard++)
			;
		close(ctl);	/* keyboard child holds it open */
		close(fd);
	}
	if(keys&Emouse){
		fd = open("/dev/mouse", O_RDONLY);
		if(fd < 0)
			eerror("can't open /dev/mouse");
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

	while(s->head == 0)
		extract();
	eb = s->head;
	s->head = s->head->next;
	if(s->head == 0)
		s->tail = 0;
	s->count += eb->n;
	if(s->count > EMAXMSG){
		write(s->ack, "a", 1);
		s->count -= EMAXMSG;
	}
	return eb;
}

static void
extract(void)
{
	Slave *s;
	Ebuf *eb;
	int i, n;
	unsigned char ebuf[EMAXMSG+1];

	bflush();
	if((n=read(epipe[0], (char *)ebuf, EMAXMSG+1)) <= 0
	|| ebuf[0] >= MAXSLAVE)
		exit(1);
	i = ebuf[0];
	if(i >= nslave || n <= 1)
		eerror("protocol error");
	s = &eslave[i];
	if(i == Stimer){
		s->head = (Ebuf *)1;
		return;
	}
	if(i == Skeyboard && n != 2)
		eerror("protocol error");
	if(i == Smouse){
		if(n!=1+10 || ebuf[1]!='m')
			eerror("protocol error");
		if(ebuf[2] & 0x80)
			ereshaped(bscreenrect());
		/* squash extraneous mouse events */
		if(s->head)
			free(ebread(s));
	}
	/* try to save space by only alloacting as much buffer as we need */
	eb = malloc(sizeof(*eb) - sizeof(eb->buf) + n - 1);
	if(eb == 0)
		eerror("protocol error");
	eb->n = n - 1;
	memmove(eb->buf, &ebuf[1], n - 1);
	eb->next = 0;
	if(s->head)
		s->tail = s->tail->next = eb;
	else
		s->head = s->tail = eb;
}

static int
eforkslave(unsigned long key)
{
	int i;

	for(i=0; i<MAXSLAVE; i++)
		if((key & ~(1<<i)) == 0 && eslave[i].pid == 0){
			if(nslave <= i)
				nslave = i + 1;
			switch(eslave[i].pid = fork()){
			case 0:
				close(epipe[0]);
				isslave = 1;
				return MAXSLAVE+i;
			case -1:
				fprintf(stderr, "events: fork error\n");
				exit(1);
			}
			eslave[i].count = 0;
			eslave[i].head = eslave[i].tail = 0;
			return i;
		}
	eerror("bad slave assignment");
}

static void
eerror(char *s)
{
	fprintf(stderr, "events: %s\n", s);
	exit(1);
}

static void
enote(int sig)
{
	char buf[32];
	int fd, i;

	if(isslave)
		return;
	for(i=0; i<nslave; i++){
		sprintf(buf, "/proc/%d/note", eslave[i].pid);
		fd = open(buf, O_WRONLY);
		if(fd > 0){
			write(fd, "die", 3);
			close(fd);
		}
	}
}

static void
ekill(void)
{
	enote(SIGTERM);
}
