typedef struct Opt	Opt;

int debug;
#define DPRINT if(debug)fprint

enum
{
	/* control characters */
	Se=		240,		/* end subnegotiation */
	NOP=		241,
	Mark=		242,		/* data mark */
	Break=		243,
	Interrupt=	244,
	Abort=		245,		/* TENEX ^O */
	AreYouThere=	246,
	Erasechar=	247,		/* erase last character */
	Eraseline=	248,		/* erase line */
	GoAhead=	249,		/* half duplex clear to send */
	Sb=		250,		/* start subnegotiation */
	Will=		251,
	Wont=		252,
	Do=		253,
	Dont=		254,
	Iac=		255,

	/* options */
	Binary=		0,
	Echo,
	SGA,
	Stat,
	Timing,
	Det,
	Term,
	EOR,
	Uid,
	Outmark,
	Ttyloc,
	M3270,
	Padx3,
	Window,
	Speed,
	Flow,
	Line,
	Xloc,
	Extend,
};

struct Opt
{
	char	*name;
	int	code;
	char	noway;	
	int	(*change)(Biobuf*, int);	/* routine for status change */
	int	(*sub)(Biobuf*, uchar*, int n);	/* routine for subnegotiation */
	char	remote;				/* remote value */
	char	local;				/* local value */
};

Opt opt[] =
{
[Binary]	{ "binary",		0,  0, },
[Echo]		{ "echo",		1,  0, },
[SGA]		{ "suppress Go Ahead",	3,  0, },
[Stat]		{ "status",		5,  1, },
[Timing]	{ "timing",		6,  1, },
[Det]		{ "det",		20, 1, },
[Term]		{ "terminal",		24, 0, },
[EOR]		{ "end of record",	25, 1, },
[Uid]		{ "uid",		26, 1, },
[Outmark]	{ "outmark",		27, 1, },
[Ttyloc]	{ "ttyloc",		28, 1, },
[M3270]		{ "3270 mode",		29, 1, },
[Padx3]		{ "pad x.3",		30, 1, },
[Window]	{ "window size",	31, 1, },
[Speed]		{ "speed",		32, 1, },
[Flow]		{ "flow control",	33, 1, },
[Line]		{ "line mode",		34, 1, },
[Xloc]		{ "X display loc",	35, 0, },
[Extend]	{ "Extended",		255, 1, },
};

int	control(Biobuf*, int);
Opt*	findopt(int);
int	will(Biobuf*);
int	wont(Biobuf*);
int	doit(Biobuf*);
int	dont(Biobuf*);
int	sub(Biobuf*);
int	send2(int, int, int);
int	send3(int, int, int, int);
int	sendnote(int, char*);
void	fatal(char*, void*, void*);
char*	syserr(void);
int	wasintr(void);
long	iread(int, void*, int);
long	iwrite(int, void*, int);
void	binit(Biobuf*, int);
void	berase(Biobuf*);
void	bkill(Biobuf*);

/*
 *  parse telnet control messages
 */
int
control(Biobuf *bp, int c)
{
	if(c < 0)
		return -1;
	switch(c){
	case AreYouThere:
		fprint(Bfildes(bp), "Plan 9 telnet, version 1\r\n");
		break;
	case Sb:
		return sub(bp);
	case Will:
		return will(bp);
	case Wont:
		return wont(bp);
	case Do:
		return doit(bp);
	case Dont:
		return dont(bp);
	case Se:
		fprint(2, "telnet: SE without an SB\n");
		break;
	default:
		break;
	}
	return 0;
}

Opt*
findopt(int c)
{
	Opt *o;

	for(o = opt; o <= &opt[Extend]; o++)
		if(o->code == c)
			return o;
	return 0;
}

int
will(Biobuf *bp)
{
	Opt *o;
	int c;
	int rv = 0;

	c = Bgetc(bp);
	if(c < 0)
		return -1;
	DPRINT(2, "will %d\n", c);
	o = findopt(c);
	if(o == 0){
		send3(Bfildes(bp), Iac, Dont, c);
		return 0;
	}
	if(o->noway)
		send3(Bfildes(bp), Iac, Dont, c);
	else if(o->remote == 0)
		rv |= send3(Bfildes(bp), Iac, Do, c);
	if(o->remote == 0){
		if(o->change)
			rv |= (*o->change)(bp, Will);
	}
	o->remote = 1;
	return rv;
}

int
wont(Biobuf *bp)
{
	Opt *o;
	int c;
	int rv = 0;

	c = Bgetc(bp);
	if(c < 0)
		return -1;
	DPRINT(2, "wont %d\n", c);
	o = findopt(c);
	if(o == 0)
		return 0;
	if(o->remote){
		if(o->change)
			rv |= (*o->change)(bp, Wont);
		rv |= send3(Bfildes(bp), Iac, Dont, c);
	}
	o->remote = 0;
	return rv;
}

int
doit(Biobuf *bp)
{
	Opt *o;
	int c;
	int rv = 0;

	c = Bgetc(bp);
	if(c < 0)
		return -1;
	DPRINT(2, "do %d\n", c);
	o = findopt(c);
	if(o == 0 || o->noway){
		send3(Bfildes(bp), Iac, Wont, c);
		return 0;
	}
	if(o->noway)
		return 0;
	if(o->local == 0){
		if(o->change)
			rv |= (*o->change)(bp, Do);
		rv |= send3(Bfildes(bp), Iac, Will, c);
	}
	o->local = 1;
	return rv;
}

int
dont(Biobuf *bp)
{
	Opt *o;
	int c;
	int rv = 0;

	c = Bgetc(bp);
	if(c < 0)
		return -1;
	DPRINT(2, "dont %d\n", c);
	o = findopt(c);
	if(o == 0)
		return 0;
	if(o->noway)
		return 0;
	if(o->local){
		o->local = 0;
		if(o->change)
			rv |= (*o->change)(bp, Dont);
		rv |= send3(Bfildes(bp), Iac, Wont, c);
	}
	o->local = 0;
	return rv;
}

/* read in a subnegotiation message and pass it to a routine for that option */
int
sub(Biobuf *bp)
{
	uchar subneg[128];
	uchar *p;
	Opt *o;
	int c;

	p = subneg;
	for(;;){
		c = Bgetc(bp);
		if(c == Iac){
			c = Bgetc(bp);
			if(c == Se)
				break;
			if(p < &subneg[sizeof(subneg)])
				*p++ = Iac;
		}
		if(c < 0)
			return -1;
		if(p < &subneg[sizeof(subneg)])
			*p++ = c;
	}
	if(p == subneg)
		return 0;
	DPRINT(2, "sub %d %d n = %d\n", subneg[0], subneg[1], (int)(p - subneg - 1));
	o = findopt(subneg[0]);
	if(o == 0 || o->sub == 0)
		return 0;
	return (*o->sub)(bp, subneg+1, p - subneg - 1);
}

void
sendd(int c0, int c1)
{
	char *t = 0;

	switch(c0){
	case Will:
		t = "Will";
		break;
	case Wont:
		t = "Wont";
		break;
	case Do:
		t = "Do";
		break;
	case Dont:
		t = "Dont";
		break;
	}
	if(t)
		DPRINT(2, "r %s %d\n", t, c1);
}

int
send2(int f, int c0, int c1)
{
	uchar buf[2];

	buf[0] = c0;
	buf[1] = c1;
	return iwrite(f, buf, 2) == 2 ? 0 : -1;
}

int
send3(int f, int c0, int c1, int c2)
{
	uchar buf[3];

	buf[0] = c0;
	buf[1] = c1;
	buf[2] = c2;
	sendd(c1, c2);
	return iwrite(f, buf, 3) == 3 ? 0 : -1;
}

int
sendnote(int pid, char *msg)
{
	int fd;
	char name[128];

	sprint(name, "/proc/%d/note", pid);
	fd = open(name, OWRITE);
	if(fd < 0)
		return -1;
	if(write(fd, msg, strlen(msg))!=strlen(msg))
		return -1;
	return close(fd);
}

void
fatal(char *fmt, void *a0, void *a1)
{
	char buf[128];

	sprint(buf, fmt, a0, a1);
	fprint(2, "%s: %s\n", argv0, buf);
	exits(buf);
}

char*
syserr(void)
{
	static char err[ERRMAX];

	errstr(err, sizeof err);
	return err;
}

int
wasintr(void)
{
	return strcmp(syserr(), "interrupted") == 0;
}

long
iread(int f, void *a, int n)
{
	long m;

	for(;;){
		m = read(f, a, n);
		if(m >= 0 || !wasintr())
			break;
	}
	return m;
}

long
iwrite(int f, void *a, int n)
{
	long m;

	m = write(f, a, n);
	if(m < 0 && wasintr())
		return n;
	return m;
}
