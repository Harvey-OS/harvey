#include <u.h>
#include <libc.h>
#include <libg.h>

#define Min(a, b)	((a) < (b) ? (a) : (b))
int *vector;
int nvec;
int cmax = 1;
Rectangle scroll;
int height;
int lasttype = -1;
int type;
char *mon;
Point pbot, ptop;
int ys[4];

char   *getstr(int);
void	reshape(Rectangle);
int	getstat(void);
int	inittype(int);
void	ignore(void*, char*);
void	mousepkt(int, char*);
int	sysfield(int, int);

enum
{
	Memory=0,
	Ether,
	Swap,
	Contxt,	 /* Code relies on the order of these up to load - Dont move! */
	Intr,
	Syscall,
	Fault,
	Tlbfault,
	Tlbpurge,
	Load,
	Ntype
};

void
main(int argc, char **argv)
{
	int l, t, n, ms, scaled, i;
	char buf[128];
	Point p1;

	ARGBEGIN{
	case 'm':
		type = Memory;
		break;
	case 'w':
		type = Swap;
		break;
	case 'e':
		type = Ether;
		break;
	case 'c':
		type = Contxt;
		break;
	case 'i':
		type = Intr;
		break;
	case 's':
		type = Syscall;
		break;
	case 'f':
		type = Fault;
		break;
	case 't':
		type = Tlbfault;
		break;
	case 'p':
		type = Tlbpurge;
		break;
	case 'l':
		type = Load;
		break;
	default:
		fprint(2, "usage: stats [-mwecisftpl]\n");
		exits("usage");
	}ARGEND

	binit(0, 0, "stats");

	ms = open("/dev/mouse", OREAD);
	if(ms < 0) {
		perror("open mouse");
		exits("mouse");
	}

	reshape(screen.r);

	notify(ignore);

	for(;;) {
		if(vector[nvec-1] == cmax) {
			cmax = 0;
			for(i = 0; i < nvec-1; i++)
				if(vector[i] > cmax)
					cmax = vector[i];
			reshape(screen.r);
		}

		l = vector[0];
		memmove(vector+1, vector, sizeof(int) * (nvec-1));
		vector[0] = getstat();
		vector[0] = l+(vector[0]-l)/2;

		if(vector[0] > cmax) {
			cmax = vector[0];
			reshape(screen.r);
		}
		segment(&screen, ptop, pbot, 1, Zero);
		scaled = (vector[0]*height)/cmax;
		segment(&screen, pbot, sub(pbot, Pt(0, scaled)), 1, F);
		for(i = 0; i < 4; i++)
			point(&screen, Pt(ptop.x, ys[i]), 0xff, F);
		p1 = scroll.min;
		p1.x--;
		bitblt(&screen, p1, &screen, scroll, S);

		sprint(buf, "%d %s", cmax, mon);
		string(&screen, scroll.min, font, buf, S);
		bflush();
		do {
			alarm(1000);
			n = read(ms, buf, sizeof(buf));
			if(n >= 14) {
				t = alarm(0);
				if(t == 0)
					t = 1000;
				mousepkt(ms, buf);
				alarm(t);
			}
		}while(n > 0);
	}
}

void
mousepkt(int ms, char *buf)
{
	if(buf[0] != 'm')
		return;
	if(buf[1] & 0x80) {			/* Reshaped */
		reshape(bscreenrect(0));
		return;
	}
	if(buf[1] & 0x07) {			/* Button */
		type++;
		if(type == Ntype)
			type = 0;
		/* wait for button up */
		while(read(ms, buf, 14) == 14 && (buf[1] & 0x07))
			;
	}
}

void
reshape(Rectangle r)
{
	int *newvector;
	Point p1, p2;
	int q, i, width, scaled;

	if(cmax == 0)
		cmax = 1;

	screen.r = r;
	scroll = inset(r, 4);
	height = scroll.max.y - scroll.min.y;
	width = scroll.max.x - scroll.min.x;

	newvector = malloc(width*sizeof(int));
	memset(newvector, 0, width*sizeof(int));
	if(vector) {
		memmove(newvector, vector, Min(width, nvec)*sizeof(int));
		free(vector);
	}
	nvec = width;
	vector = newvector;
		
	bitblt(&screen, screen.r.min, &screen, screen.r, Zero);
	border(&screen, screen.r, 1, F);

	p1 = scroll.min;
	p2 = Pt(scroll.max.x, scroll.min.y);
	q = height/4;
	for(i = 0; i < 4; i++) {
		segment(&screen, p1, p2, 0xff, F);
		ys[i] = p1.y;
		p1.y += q;
		p2.y += q;
	}

	ptop.x = scroll.max.x-1;
	pbot.x = ptop.x;
	ptop.y = scroll.min.y;
	pbot.y = scroll.max.y;

	p1 = pbot;
	for(i = 0; i < nvec; i++) {
		scaled = (vector[i]*height)/cmax; 
		segment(&screen, p1, sub(p1, Pt(0, scaled)), 1, F);
		p1.x -= 1;
	}
}

int
getstat(void)
{
	static int fd, nfd;
	static int lastval;
	int newval, val;
	char *s;

	if(type != lasttype) {
		for(;;) {
			close(fd);
			fd = inittype(type);
			if(fd >= 0)
				break;
			type++;
			if(type == Ntype)
				type = Memory;
		}
		lasttype = type;
		lastval = 0;
	}

	switch(type) {
	case Memory:
		s = getstr(fd);
		return atoi(s);
	case Swap:
		s = getstr(fd);
		s = strchr(s, 'y');
		s++;
		return atoi(s);
	case Ether:
		s = getstr(fd);
		s = strchr(s, ':');	/* In */
		val = atoi(s+1);
		s = strchr(s, ':');	/* Out */
		val += atoi(s+1);
		if(lastval == 0)
			lastval = val;
		newval = val-lastval;
		lastval = val;
		return newval;
	case Contxt:
	case Intr:
	case Syscall:
	case Fault:
	case Tlbfault:
	case Tlbpurge:
		val = sysfield(fd, type-Contxt+1);
		if(lastval == 0)
			lastval = val;
		newval = val-lastval;
		lastval = val;
		return newval;
	case Load:
		return sysfield(fd, 7);
	}
}

int
inittype(int type)
{
	char *s;
	int f;

	f = -1;
	memset(vector, 0, nvec*sizeof(int));
	switch(type) {
	case Memory:
		mon = "mem";
		f = open("/dev/swap", OREAD);
		s = getstr(f);
		s = strchr(s, '/');
		s++;
		cmax = atoi(s);
		reshape(screen.r);
		break;
	case Swap:
		mon = "swap";
		f = open("/dev/swap", OREAD);
		s = getstr(f);
		s = strchr(s, 'y');
		s = strchr(s, '/');
		s++;
		cmax = atoi(s);
		reshape	(screen.r);
		break;
	case Ether:
		mon = "ether";
		f = open("#l/ether/0/stats", OREAD);
		cmax = 1;
		reshape	(screen.r);
		break;
	case Contxt:
		mon = "contxt";
		f = open("/dev/sysstat", OREAD);
		cmax = 1;
		reshape	(screen.r);
		break;
	case Intr:
		mon = "intr";
		f = open("/dev/sysstat", OREAD);
		cmax = 1;
		reshape	(screen.r);
		break;
	case Syscall:
		mon = "syscall";
		f = open("/dev/sysstat", OREAD);
		cmax = 1;
		reshape	(screen.r);
		break;
	case Fault:
		mon = "fault";
		f = open("/dev/sysstat", OREAD);
		cmax = 1;
		reshape	(screen.r);
		break;
	case Tlbfault:
		mon = "tlbmiss";
		f = open("/dev/sysstat", OREAD);
		cmax = 1;
		reshape	(screen.r);
		break;
	case Tlbpurge:
		mon = "tlbpurge";
		f = open("/dev/sysstat", OREAD);
		cmax = 1;
		reshape	(screen.r);
		break;
	case Load:
		mon = "load";
		f = open("/dev/sysstat", OREAD);
		cmax = 1;
		reshape	(screen.r);
		break;
	}
	if(f < 0) {
		fprint(2, "stats: %s: %r", mon);
		sleep(500);
	}

	return f;
}

int
sysfield(int fd, int nr)
{
	int val = 0;
	char *s;

	s = getstr(fd);
	for(;;) {
		val += atoi(s+(nr*12));
		s = strchr(s, '\n');
		if(s == 0 || *++s == '\0')
			break;
		s++;
	};
	return val;
}

char *
getstr(int fd)
{
	static char buf[1000];
	int n;

	if(seek(fd, 0, 0) < 0)
		exits("getstr: seek");
	n = read(fd, buf, sizeof(buf));
	if(n < 0)
		exits("getstr: read");
	buf[n] = 0;
	return buf;
}

void
ignore(void *a, char *c)
{
	USED(a);
	if(strcmp(c, "alarm") == 0)
		noted(NCONT);

	noted(NDFLT);
}
