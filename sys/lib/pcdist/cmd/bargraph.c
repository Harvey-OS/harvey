#include <u.h>
#include <libc.h>
#include <draw.h>
#include <bio.h>
#include <event.h>

enum {PNCTL=3};

static char* rdenv(char*);
void newwin(char*);
Rectangle screenrect(void);

int nokill;
char *title;

Image *light;
Image *dark;
Image *text;

void
initcolor(void)
{
	text = display->black;
	light = allocimagemix(display, DPalegreen, DWhite);
	dark = allocimage(display, Rect(0,0,1,1), CMAP8, 1, DDarkgreen);
}

Rectangle rbar;
Point ptext;
long n, d;
int last;
int lastp;

void
drawbar(void)
{
	int i;
	int p;
	char buf[10];

	if(n > d || n < 0 || d <= 0)
		return;

	i = (Dx(rbar)*(vlong)n)/d;
	p = (n*100LL)/d;

	if(lastp == p && last == i)
		return;

	if(lastp != p){
		sprint(buf, "%d%%", p);
		
		stringbg(screen, addpt(screen->r.min, Pt(Dx(rbar)-30, 4)), text, ZP, display->defaultfont, buf, light, ZP);
		lastp = p;
	}

	if(last != i){
		draw(screen, Rect(rbar.min.x+last, rbar.min.y, rbar.min.x+i, rbar.max.y),
			dark, nil, ZP);
		last = i;
	}
	flushimage(display, 1);
}

void
eresized(int new)
{
	Point p, q;
	Rectangle r;

	if(new && getwindow(display, Refnone) < 0)
		fprint(2,"can't reattach to window");

	r = screen->r;
	draw(screen, r, light, nil, ZP);
	p = string(screen, addpt(r.min, Pt(4,4)), text, ZP,
		display->defaultfont, title);

	p.x = r.min.x+4;
	p.y += display->defaultfont->height+4;

	q = subpt(r.max, Pt(4,4));
	rbar = Rpt(p, q);

	ptext = Pt(r.max.x-4-stringwidth(display->defaultfont, "100%"), r.min.x+4);
	border(screen, rbar, -2, dark, ZP);
	last = 0;
	lastp = -1;

	drawbar();
}

void
bar(Biobuf *b)
{
	char *p, *f[2];
	Event e;
	int k, die, parent, child;

	parent = getpid();

	die = 0;
	switch(child = rfork(RFMEM|RFPROC)) {
	case 0:
		sleep(1000);
		while(!die && (k = eread(Ekeyboard|Emouse, &e))) {
			if(nokill==0 && k == Ekeyboard && (e.kbdc == 0x7F || e.kbdc == 0x03)) { /* del, ctl-c */
				die = 1;
				postnote(PNPROC, parent, "interrupt");
				_exits("interrupt");
			}
		}
		_exits(0);
	}

	while(!die && (p = Brdline(b, '\n'))) {
		p[Blinelen(b)-1] = '\0';
		if(tokenize(p, f, 2) != 2)
			continue;
		n = atol(f[0]);
		d = atol(f[1]);
		drawbar();
	}
	postnote(PNCTL, child, "kill");
	die = 1;
}


void
usage(void)
{
	fprint(2, "usage: bargraph [-w minx,miny,maxx,maxy] 'title'\n");
	exits("usage");
}

void
main(int argc, char **argv)
{
	Biobuf b;
	char *p, *q;
	int lfd;

	p = "0,0,200,40";
	
	ARGBEGIN{
	case 'w':
		p = ARGF();
		break;
	case 'k':
		nokill = 1;
		break;
	default:
		usage();
	}ARGEND;

	if(argc != 1)
		usage();

	title = argv[0];

	lfd = dup(0, -1);

	while(q = strchr(p, ','))
		*q = ' ';
	newwin(p);

	initdraw(0, 0, "bar");
	initcolor();

	einit(Emouse|Ekeyboard);

	Binit(&b, lfd, OREAD);
	eresized(0);
	bar(&b);
}


/* all code below this line should be in the library, but is stolen from colors instead */
static char*
rdenv(char *name)
{
	char *v;
	int fd, size;

	fd = open(name, OREAD);
	if(fd < 0)
		return 0;
	size = seek(fd, 0, 2);
	v = malloc(size+1);
	if(v == 0){
		fprint(2, "page: can't malloc: %r\n");
		exits("no mem");
	}
	seek(fd, 0, 0);
	read(fd, v, size);
	v[size] = 0;
	close(fd);
	return v;
}

void
newwin(char *win)
{
	char *srv, *mntsrv;
	char spec[100];
	int srvfd, cons, pid;

	switch(rfork(RFFDG|RFPROC|RFNAMEG|RFENVG|RFNOTEG|RFNOWAIT)){
	case -1:
		fprint(2, "page: can't fork: %r\n");
		exits("no fork");
	case 0:
		break;
	default:
		exits(0);
	}

	srv = rdenv("/env/wsys");
	if(srv == 0){
		mntsrv = rdenv("/mnt/term/env/wsys");
		if(mntsrv == 0){
			fprint(2, "page: can't find $wsys\n");
			exits("srv");
		}
		srv = malloc(strlen(mntsrv)+10);
		sprint(srv, "/mnt/term%s", mntsrv);
		free(mntsrv);
		pid  = 0;			/* can't send notes to remote processes! */
	}else
		pid = getpid();
	srvfd = open(srv, ORDWR);
	free(srv);
	if(srvfd == -1){
		fprint(2, "page: can't open %s: %r\n", srv);
		exits("no srv");
	}
	sprint(spec, "new -r %s", win);
	if(mount(srvfd, "/mnt/wsys", 0, spec) == -1){
		fprint(2, "page: can't mount /mnt/wsys: %r (spec=%s)\n", spec);
		exits("no mount");
	}
	close(srvfd);
	unmount("/mnt/acme", "/dev");
	bind("/mnt/wsys", "/dev", MBEFORE);
	cons = open("/dev/cons", OREAD);
	if(cons==-1){
	NoCons:
		fprint(2, "page: can't open /dev/cons: %r");
		exits("no cons");
	}
	dup(cons, 0);
	close(cons);
	cons = open("/dev/cons", OWRITE);
	if(cons==-1)
		goto NoCons;
	dup(cons, 1);
	dup(cons, 2);
	close(cons);
//	wctlfd = open("/dev/wctl", OWRITE);
}

Rectangle
screenrect(void)
{
	int fd;
	char buf[12*5];

	fd = open("/dev/screen", OREAD);
	if(fd == -1)
		fd=open("/mnt/term/dev/screen", OREAD);
	if(fd == -1){
		fprint(2, "page: can't open /dev/screen: %r\n");
		exits("window read");
	}
	if(read(fd, buf, sizeof buf) != sizeof buf){
		fprint(2, "page: can't read /dev/screen: %r\n");
		exits("screen read");
	}
	close(fd);
	return Rect(atoi(buf+12), atoi(buf+24), atoi(buf+36), atoi(buf+48));
}

int
postnote(int group, int pid, char *note)
{
	char file[128];
	int f, r;

	switch(group) {
	case PNPROC:
		sprint(file, "/proc/%d/note", pid);
		break;
	case PNGROUP:
		sprint(file, "/proc/%d/notepg", pid);
		break;
	case PNCTL:
		sprint(file, "/proc/%d/ctl", pid);
		break;
	default:
		return -1;
	}

	f = open(file, OWRITE);
	if(f < 0)
		return -1;

	r = strlen(note);
	if(write(f, note, r) != r) {
		close(f);
		return -1;
	}
	close(f);
	return 0;
}
