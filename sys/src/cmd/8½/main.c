#include <u.h>
#include <libc.h>
#include <libg.h>
#include <frame.h>
#include <layer.h>
#include "dat.h"
#include "fns.h"

int	cfd;		/* client end of pipe */
int	sfd;		/* service end of pipe */
int	clockfd;	/* /dev/time */
long	clicktime;	/* ms since last click */
Proc	*pmouse;
Proc	*pkbd;
int	mouseslave;
int	kbdslave;
int	sweeping;
int	defscroll;
Cover	cover;
char	srv[128];
Bitmap	*lightgrey;
Bitmap	*darkgrey;
Cursor	*cursorp=0;
int	boxup;
Rectangle box;
Window	*input;
ulong	wqid;
uchar	kbdc[NKBDC];
int	kbdcnt;
Subfont	*subfont;

#define	CLICKTIME	500	/* milliseconds for double click */

int	mkslave(char*, int, int);
void	killslaves(void);
int	intr(void*, char*);
Proc	*wakemouse;

void	mousectl(void);
void	kbdctl(void);
void	windowctl(void);

int	whichbut[] = {	0, 1, 2, 2, 3, 1, 2, 3 };

uchar lightgreybits[] = {
	0x10,
	0x40,
	0x10,
	0x40,
};

uchar darkgreybits[] = {
	0xD0,
	0x70,
	0xD0,
	0x70,
};

Cursor sweep0 = {
	{-7, -7},
	{0x03, 0xC0, 0x03, 0xC0, 0x03, 0xC0, 0x03, 0xC0,
	 0x03, 0xC0, 0x03, 0xC0, 0xFF, 0xFF, 0xFF, 0xFF,
	 0xFF, 0xFF, 0xFF, 0xFF, 0x03, 0xC0, 0x03, 0xC0,
	 0x03, 0xC0, 0x03, 0xC0, 0x03, 0xC0, 0x03, 0xC0},
	{0x00, 0x00, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80,
	 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x7F, 0xFE,
	 0x7F, 0xFE, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80,
	 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x00, 0x00}
};

Cursor boxcurs = {
	{-7, -7},
	{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	 0xFF, 0xFF, 0xF8, 0x1F, 0xF8, 0x1F, 0xF8, 0x1F,
	 0xF8, 0x1F, 0xF8, 0x1F, 0xF8, 0x1F, 0xFF, 0xFF,
	 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
	{0x00, 0x00, 0x7F, 0xFE, 0x7F, 0xFE, 0x7F, 0xFE,
	 0x70, 0x0E, 0x70, 0x0E, 0x70, 0x0E, 0x70, 0x0E,
	 0x70, 0x0E, 0x70, 0x0E, 0x70, 0x0E, 0x70, 0x0E,
	 0x7F, 0xFE, 0x7F, 0xFE, 0x7F, 0xFE, 0x00, 0x00}
};

Cursor sight = {
	{-7, -7},
	{0x1F, 0xF8, 0x3F, 0xFC, 0x7F, 0xFE, 0xFB, 0xDF,
	 0xF3, 0xCF, 0xE3, 0xC7, 0xFF, 0xFF, 0xFF, 0xFF,
	 0xFF, 0xFF, 0xFF, 0xFF, 0xE3, 0xC7, 0xF3, 0xCF,
	 0x7B, 0xDF, 0x7F, 0xFE, 0x3F, 0xFC, 0x1F, 0xF8,},
	{0x00, 0x00, 0x0F, 0xF0, 0x31, 0x8C, 0x21, 0x84,
	 0x41, 0x82, 0x41, 0x82, 0x41, 0x82, 0x7F, 0xFE,
	 0x7F, 0xFE, 0x41, 0x82, 0x41, 0x82, 0x41, 0x82,
	 0x21, 0x84, 0x31, 0x8C, 0x0F, 0xF0, 0x00, 0x00,}
};

Cursor whitearrow = {
	{0, 0},
	{0xFF, 0xE0, 0xFF, 0xE0, 0xFF, 0xC0, 0xFF, 0x00,
	 0xFF, 0x00, 0xFF, 0x80, 0xFF, 0xC0, 0xFF, 0xE0,
	 0xE7, 0xF0, 0xE3, 0xF8, 0xC1, 0xFC, 0x00, 0xFE,
	 0x00, 0x7F, 0x00, 0x3E, 0x00, 0x1C, 0x00, 0x08,},
	{0xFF, 0xE0, 0xFF, 0xE0, 0xC1, 0xC0, 0xC3, 0x00,
	 0xC3, 0x00, 0xC1, 0x80, 0xD8, 0xC0, 0xFC, 0x60,
	 0xE6, 0x30, 0xE3, 0x18, 0xC1, 0x8C, 0x00, 0xC6,
	 0x00, 0x63, 0x00, 0x36, 0x00, 0x1C, 0x00, 0x08,}
};

void
main(int argc, char *argv[])
{
	int fd, n, p[2];
	Bitmap *b;
	char *fn, *init, user[NAMELEN], buf[32];

	atnotify(intr, 1);
	init = 0;
	fn = 0;
	ARGBEGIN{
	case 'i':
		init = ARGF();
		break;
	case 's':
		defscroll = 1;
		break;
	case 'f':
		fn = ARGF();
		break;
	}ARGEND
	USED(argv);
	if(argc != 0){
		fprint(2, "usage: 8½ [-s] [-i startfile] [-f fontfile]\n");
		exits("usage");
	}
	binit(error, fn, "8½");
	if(font->nsub == 0)
		error("no subfonts in main font");
	/*
	 * Ugly: arrange for subfont to be 0th subfont of main font
	 */
	strwidth(font, " ");	/* Force calculation of file name of subfont */
	if(font->sub[0]->name){
		/* must load subfont separately as font's version may be freed */
		fd = open(font->sub[0]->name, OREAD);
		if(fd<0 || (b=rdbitmapfile(fd))==0
			|| (subfont=rdsubfontfile(fd, b))==0)
			error("can't build subfont");
		close(fd);
	}else
		subfont = font->subf[0].f;
	if(fn){
		fd = create("/env/font", 1, 0664);
		if(fd >= 0){
			write(fd, fn, strlen(fn));
			close(fd);
		}
	}
	lightgrey = balloc(Rect(0, 0, 4, 4), 0);
	darkgrey = balloc(Rect(0, 0, 4, 4), 0);
	if(lightgrey==0 || darkgrey==0)
		error("can't initialize textures");
	wrbitmap(lightgrey, 0, 4, lightgreybits);
	wrbitmap(darkgrey, 0, 4, darkgreybits);
	texture(&screen, screen.r, lightgrey, S);
	bflush();

	/*
	 * Layers
	 */
	cover.layer = (Layer*)&screen;
	cover.front = 0;
	cover.ground = lightgrey;

	/*
	 * File server
	 */
	if((clockfd=open("/dev/time", OREAD)) < 0)
		error("/dev/time");
	if(pipe(p) < 0)
		error("pipe");
	cfd = p[0];
	sfd = p[1];

	/*
	 * Post service
	 */
	fd = open("/dev/user", OREAD);
	if(fd < 0)
		error("/dev/user");
	n = read(fd, user, NAMELEN-1);
	close(fd);
	if(n <= 0)
		error("/dev/user");
	user[n] = 0;
	sprint(srv, "/srv/8½.%s.%d", user, getpid());
	fd = create(srv, OWRITE, 0666);
	if(fd < 0)
		error(srv);
	sprint(buf, "%d", cfd);
	if(write(fd, buf, strlen(buf)) != strlen(buf))
		error("srv write");
	close(fd);
	fd = create("/env/8½srv", OWRITE, 0666);
	if(fd > 0){
		write(fd, srv, strlen(srv));
		close(fd);
	}

	/*
	 * Slaves
	 */
	mouseslave = mkslave("/dev/mouse", sizeof(mouse.data), 0);
	kbdslave = mkslave("/dev/cons", 3, 1);
	atexit(killslaves);

	/*
	 * Initialization commands
	 */
	if(init)
		switch(fork2(0)){
		case 0:
			close(sfd);
			close(cfd);
			close(clockfd);
			bclose();
			execl("/bin/rc", "rc", "-c", init, 0);
			exits(0);
	
		case -1:
			error("fork");
		}

	/*
	 * Start processes
	 */
	run(newproc(windowctl, 0));
	pkbd = newproc(kbdctl, 0);
	pmouse = newproc(mousectl, 0);
	proc.p = pmouse;
	longjmp(pmouse->label, 1);
}

int
intr(void *a, char *msg)
{
	int i;
	Window *w;

	USED(a);
	if(strncmp(msg, "interrupt", 9) == 0)
		return 1;
	if(strncmp(msg, "hangup", 6)==0 || strncmp(msg, "exit", 4)==0){
		for(w=&window[1],i=1; i<NSLOT; i++,w++){
			if(w->busy)
				termnote(w, msg);
		}
		killslaves();
		return 0;
	}
	return 0;
}

int
fork2(ulong flag)
{
	int i, j;
	char buf[20];
	Waitmsg w;
	char *p;

	i = rfork(RFPROC|RFFDG|flag);
	if(i != 0){
		if(i < 0)
			return -1;
		do; while((j=wait(&w))!=-1 && j!=i);
		if(j == i){
			p = utfrune(w.msg, ':');
			if(p)
				return atoi(p+1); /* pid of final child */
		}
		return -1;
	}
	i = rfork(RFPROC);
	if(i != 0){
		sprint(buf, "%d", i);
		_exits(buf);
	}
	return 0;
}

int
mkslave(char *file, int count, int raw)
{
	int fd1, fd2, fd3, n, pid;
	char buf[64];

	fd1 = open(file, OREAD);
	if(fd1 < 0)
		error(file);
	switch(pid = fork2(RFNAMEG|RFENVG|RFNOTEG)){
	case 0:
		close(sfd);
		close(clockfd);
		bclose();
		if(raw){
			fd3 = open("/dev/consctl", OWRITE);
			if(fd3 < 0)
				error("slave consctl open");
			write(fd3, "rawon", 5);
		}
		if(mount(cfd, "/dev", MBEFORE, "slave", "") < 0)
			error("slave mount");
		fd2 = open(file, OWRITE);
		if(fd2 < 0)
			error("slave open");
		while((n=read(fd1, buf, count)) >= 0){
			int i;
			/*
			 * bug: shouldn't need this loop but system demands it.
			 * apparently caused by write error putting message on
			 * client end of mount point and corrupting others' access
			 */
			i = 0;
			while(write(fd2, buf, n) != n)
				if(++i > 3)
					error("slave write");
		}
		fprint(2, "%s slave shut down", file);
		exits(0);

	case -1:
		error("fork");
	}
	close(fd1);
	return pid;
}

int
spawn(int slot)
{
	long pid;
	char buf[32];
	int p[2];

	sprint(buf, "%d", slot);
	if(pipe(p) < 0)
		return -1;
	switch(pid = fork2(RFNAMEG|RFENVG|RFNOTEG)){	/* assign = */
	case 0:
		atexitdont(killslaves);
		close(sfd);
		close(clockfd);
		bclose();
		write(p[1], "start", 5);
		if(mount(cfd, "/mnt/8½", MREPL, buf, "") < 0)
			error("client mount");
		if(bind("/mnt/8½", "/dev", MBEFORE) < 0)
			error("client bind");
		close(p[0]);
		close(p[1]);
		close(0);
		close(1);
		if(open("/dev/cons", OREAD) != 0)
			error("/dev/cons 0 client open");
		if(open("/dev/cons", OWRITE) != 1)
			error("/dev/cons 1 client open");
		dup(1, 2);
		execl("/bin/rc", "rc", 0);
		error("exec /bin/rc");

	case -1:
		return -1;
	}
	close(p[1]);
	if(read(p[0], buf, 5) != 5){	/* wait for mount etc. */
		fprint(2, "8½: fork read fail\n");
		return -1;
	}
	close(p[0]);
	return pid;
}

void
killslaves(void)
{
	char buf[32];
	int fd;

	sprint(buf, "/proc/%d/note", mouseslave);
	fd = open(buf, OWRITE);
	if(fd > 0){
		write(fd, "die", 3);
		close(fd);
	}
	sprint(buf, "/proc/%d/note", kbdslave);
	fd = open(buf, OWRITE);
	if(fd > 0){
		write(fd, "die", 3);
		close(fd);
	}
	remove(srv);
}

void
mousectl(void)
{
	uchar *d = mouse.data;

	/*
	 * Mouse is bootstrap process.  Just call sched to get things going.
	 */

	for(;;){
		sched();
		if(d[0] != 'm')
			error("mouse not m");
		if(d[1] & 0x80)
			termreshapeall();
		mouse.buttons = d[1] & 7;
		mouse.xy.x = BGLONG(d+2);
		mouse.xy.y = BGLONG(d+6);
		mouse.msec = BGLONG(d+10);
		if(wakemouse){
			run(wakemouse);
			wakemouse->mouse = mouse;
			wakemouse = 0;
		}
	}
}

void
kbdctl(void)
{
	Rune r;
	int w;

	for(;;){
		while(!fullrune((char*)kbdc, kbdcnt))
			sched();
		if(input && input->kbdc>=0)
			sched();
		w = chartorune(&r, (char*)kbdc);
		memmove(kbdc, kbdc+w, kbdcnt-w);
		kbdcnt -= w;
		clicktime = 0;
		if(input && r){
			input->kbdc = r;
			run(input->p);
		}
	}
}

long
min(long a, long b)
{
	if(a < b)
		return a;
	return b;
}

long
max(long a, long b)
{
	if(a > b)
		return a;
	return b;
}

enum
{
	New3,
	Reshape3,
	Move3,
	Delete3,
	Hide3,
	Nmenu3,			/* Must be LAST! */
};

char *menu3str[Nmenu3+NSLOT] = {
	[New3]		"New",
	[Reshape3]	"Reshape",
	[Move3]		"Move",
	[Delete3]	"Delete",
	[Hide3]		"Hide",
			0
};

Menu menu3 = {
	menu3str
};

enum
{
	Cut2,
	Paste2,
	Snarf2,
	Send2,
	Scroll2,
};

char *menu2str[] = {
	[Cut2]		"cut",
	[Paste2]	"paste",
	[Snarf2]	"snarf",
	[Send2]		"send",
	[Scroll2]	"scroll",
			0,
};

Menu menu2 = {
	menu2str
};

Rectangle
sweep(void)
{
	Mouse *m;
	Point p1, p2;

	sweeping = 1;
	cursorswitch(&sweep0);
	m = &proc.p->mouse;
	while(m->buttons == 0)
		frgetmouse();
	if(m->buttons != 4){
		box = Rect(0, 0, 0, 0);
		goto ret;
	}
	p1 = m->xy;
	box = Rpt(p1, p1);
	cursorswitch(&boxcurs);
	boxup = 1;
	while(m->buttons == 4){
		p2 = m->xy;
		box.min.x = min(p1.x, p2.x);
		box.min.y = min(p1.y, p2.y);
		box.max.x = max(p1.x, p2.x);
		box.max.y = max(p1.y, p2.y);
		frgetmouse();
	}
	boxup = 0;
	if(m->buttons){
		box = Rect(0, 0, 0, 0);
		cursorswitch(0);
		while(m->buttons)
			frgetmouse();
	}
    ret:
	sweeping = 0;
	checkcursor(1);
	return box;
}

void
checkcursor(int sure)
{
	Window *w;
	Cursor *c;

	w = input;
	if(w == 0)
		c = 0;
	else
		c = ptinrect(mouse.xy, w->l->r)?  w->cursorp : 0;
	if(!sweeping && (sure || c!=cursorp))
		cursorswitch(cursorp = c);
}

void
current(Window *w)
{
	Window *oinput;

	oinput = input;
	if(w && w->l)
		input = w;
	else
		input = 0;
	if(oinput)
		termborder(oinput);
	if(w)
		termborder(w);
	checkcursor(0);
	bflush();
}

int
okrect(Rectangle r)
{
	if(rectclip(&r, screen.r) == 0)
		return 0;
	return Dx(r)>=100 && Dy(r)>=50;
}

void
windowctl(void)
{
	Mouse *m;
	Rectangle r;
	Window *w;
	long cc;
	Point p1, p2, p3;
	int i, n, buttons;

	m = &proc.p->mouse;
	buttons = 0;
	for(;;){
		frgetmouse();
		checkcursor(0);
		if(input && input->mouseopen){
			if(ptinrect(m->xy, input->l->r) || buttons){
				input->p->mouse = *m;
				input->mousechanged = 1;
				buttons = m->buttons;
				run(input->p);
				continue;
			}
			buttons = 0;
		}
		if(m->buttons){
			if(input && ptinrect(m->xy, input->scrollr)){
				scroll(input, whichbut[m->buttons], m);
				continue;
			}
			switch(m->buttons){
			case 1:
				w = termtop(m->xy);
				if(w && w!=input){
					current(w);
					clicktime = 0;
				}else if(w && w==input){
					frselect(&w->f, m);
					if(w->f.p0==w->f.p1){
						cc = mouse.msec;
						if(w->q0==w->org+w->f.p0
						&& clicktime
						&& cc-clicktime<CLICKTIME){
							doubleclick(w, w->org+w->f.p0);
							termhighlight(w, w->q0, w->q1);
							clicktime = 0;
						}else
							clicktime = cc;
					}else
						clicktime = 0;
					w->q0 = w->org+w->f.p0;
					w->q1 = w->org+w->f.p1;
				}
				break;

			case 2:
				clicktime = 0;
				if(input){
					if(input->mouseopen)
						break;
					menu2str[Scroll2] = input->scroll?
						"noscroll" : "scroll";
					switch(lmenuhit(2, &menu2)){
					default:
						break;
					case Cut2:
						if(input)
							termcut(input, 1);
						break;
					case Snarf2:
						if(input)
							termsnarf(input);
						break;
					case Paste2:
						if(input)
							termpaste(input);
						break;
					case Send2:
						if(input)
							termsend(input);
						break;
					case Scroll2:
						if(input)
							termscroll(input);
						break;
					}
				}
				break;

			case 4:
				clicktime = 0;
				switch(n = lmenuhit(3, &menu3)){
				default:
					for(i = 0; i < NSLOT; i++)
						if(window[i].label == menu3str[n])
							break;
					if(i != NSLOT){
						w = &window[i];
						w->reshape = w->onscreen;
						w->hidemenu = -1;
						run(w->p);
					}
					break;
				case New3:
					r = sweep();
					if(cfd>=0 && okrect(r))
						newterm(r, 1, 0);
					break;

				case Reshape3:
					cursorswitch(&sight);
					while(m->buttons == 0)
						frgetmouse();
					if(m->buttons==4 && (w=termwhich(m->xy))){
						cursorswitch(&sweep0);
						while(m->buttons == 4)
							frgetmouse();
						if(m->buttons == 0){
							r = sweep();
							if(Dx(r)>100 && Dy(r)>50){
								w->reshape = r;
								run(w->p);
							}
						}
					}
					cursorswitch(0);
					break;

				case Move3:
					cursorswitch(&sight);
					while(m->buttons == 0)
						frgetmouse();
					if(m->buttons==4 && (w=termwhich(m->xy))){
						p1 = div(add(w->r.min, w->r.max), 2);
						p2 = m->xy;
						cursorswitch(&boxcurs);
						boxup = 1;
						while(m->buttons == 4){
							p3 = m->xy;
							if(p1.x < p2.x)
								p3.x -= Dx(w->l->r);
							if(p1.y < p2.y)
								p3.y -= Dy(w->l->r);
							box = raddp(w->l->r, sub(p3, w->l->r.min));
							frgetmouse();
						}
						boxup = 0;
						if(m->buttons==0 && rectXrect(box, screen.r)){
							w->reshape = box;
							run(w->p);
						}
					}
					cursorswitch(0);
					break;

				case Delete3:
					cursorswitch(&sight);
					while(m->buttons == 0)
						frgetmouse();
					cursorswitch(0);
					if(m->buttons==4 && (w=termwhich(m->xy))){
						w->dodelete = 1;
						run(w->p);
					}
					break;

				case Hide3:
					cursorswitch(&sight);
					while(m->buttons == 0)
						frgetmouse();
					cursorswitch(0);
					if(m->buttons==4 && (w=termwhich(m->xy))){
						w->onscreen = w->l->r;
						/*
						 * Put window so corner is {0,0}.
						 * Assumes screen is positive
						 */
						w->reshape = rsubp(w->l->r, w->l->r.max);
						w->hidemenu = 1;
						run(w->p);
					}
					cursorswitch(0);
					break;

				}
				break;

			default:
				clicktime = 0;
				break;
			}
			while(m->buttons)
				frgetmouse();
		}
	}
}

void
hidemenu(Window *w, int hide)
{
	char **ptr;

	if(hide > 0){	/* hiding */
		for(ptr=&menu3str[Nmenu3]; *ptr; ptr++)
			;
		*ptr++ = w->label;
		*ptr = 0;
	}else
		for(ptr=&menu3str[Nmenu3]; *ptr; ptr++)
			if(*ptr == w->label)
				while(*ptr){
					ptr[0] = ptr[1];
					ptr++;
				}
}

void
frgetmouse(void)
{
	bflush();
	wakemouse = proc.p;
	sched();
}

void
error(char *s)
{
	char err[ERRLEN];

	errstr(err);
	fprint(2, "8½: %s: %s\n", s, err);
	exits(s);
}

#define	PRINTSIZE	256
char	printbuf[PRINTSIZE];
int			/* special print to keep stacks small */
print(char *fmt, ...)
{
	int n;

	n = doprint(printbuf, printbuf+sizeof(printbuf), fmt, (&fmt+1)) - printbuf;
	write(1, printbuf, n);
	return n;
}

int			/* special fprint to keep stacks small */
fprint(int fd, char *fmt, ...)
{
	int n;

	n = doprint(printbuf, printbuf+sizeof(printbuf), fmt, (&fmt+1)) - printbuf;
	write(fd, printbuf, n);
	return n;
}

int
sprint(char *s, char *fmt, ...)
{
	return doprint(s, s+PRINTSIZE, fmt, (&fmt+1)) - s;
}

void *
emalloc(ulong n)
{
	void *p;

	p = malloc(n);
	if(p == 0){
		error("malloc: out of memory");
	}
	return p;
}

void *
erealloc(void *p, ulong n)
{
	p = realloc(p, n);
	if(p == 0)
		error("realloc: out of memory");
	return p;
}
