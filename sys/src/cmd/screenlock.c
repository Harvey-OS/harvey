/* screenlock - lock a terminal */
#include <u.h>
#include <libc.h>
#include <libsec.h>
#include <draw.h>
#include <thread.h>
#include <auth.h>

char pic[] = "/lib/bunny.bit";

int vgactl;
int debug;
int doblank;
int chatty = 0;

char user[256];

void
blankscreen(int blank)
{
	if(vgactl < 0)
		return;
	seek(vgactl, 0, 0);
	if(fprint(vgactl, blank? "blank": "unblank") < 0)
		fprint(2, "blankscreen: can't blank: %r\n");
}

void
error(char *fmt, ...)
{
	Fmt f;
	char buf[64];
	va_list arg;

	fmtfdinit(&f, 1, buf, sizeof buf);
	fmtprint(&f, "screenlock: ");
	va_start(arg, fmt);
	fmtvprint(&f, fmt, arg);
	va_end(arg);
	fmtprint(&f, "\n");
	fmtfdflush(&f);
	threadexitsall("fatal error");
}

void
usage(void)
{
	fprint(2, "usage: %s\n", argv0);
	exits("usage");
}


void
readfile(char *name, char *buf, int nbuf, int addnul)
{
	int fd;

	fd = open(name, OREAD);
	if(fd == -1)
		error("%s - can't open: %r", name);
	nbuf = read(fd, buf, nbuf-addnul);
	close(fd);
	if(nbuf == -1)
		error("%s - can't can't read: %r", name);
	if(addnul)
		buf[nbuf] = '\0';
}

void
readline(char *buf, int nbuf)
{
	char c;
	int i;

	i = 0;
	while(i < nbuf-1)
		if(read(0, &c, 1) != 1 || c == '\04' || c == '\177'){
			i = 0;
			break;
		} else if(c == '\n')
			break;
		else if(c == '\b' && i > 0)
			--i;
		else if(c == ('u' & 037))
			i = 0;
		else
			buf[i++] = c;
	buf[i] = '\0';
}

void
checkpassword(void)
{
	int fd, consctl, must;
	char buf[256];
	AuthInfo *ai;
	static int opened;

	must = 1;
	if(!opened){
		fd = open("/dev/cons", OREAD);
		if(fd == -1)
			error("can't open cons: %r");
		dup(fd, 0);
		close(fd);
		fd = open("/dev/cons", OWRITE);
		if(fd == -1)
			error("can't open cons: %r");
		dup(fd, 1);
		dup(1, 2);
		close(fd);
		consctl = open("/dev/consctl", OWRITE);
		if(consctl == -1)
			error("can't open consctl: %r");
		if(write(consctl, "rawon", 5) != 5)
			error("can't turn off echo\n");
		opened = 1;
	}

	for(;;){
		if(chatty || !must)
			fprint(2, "%s's screenlock password: ", user);
		memset(buf, 0, sizeof buf);
		readline(buf, sizeof buf);
		blankscreen(0);
		if(chatty || !must)
			fprint(2, "\n");
		if(buf[0] == '\0' || buf[0] == '\04'){
			if(must)
				continue;
			error("no password typed");
		}

		/* authenticate */
		ai = auth_userpasswd(user, buf);
		if(ai != nil && ai->cap != nil)
			break;
		auth_freeAI(ai);

		if(chatty || !must)
			fprint(2, "password mismatch\n");
		doblank = 1;
	}
	memset(buf, 0, sizeof buf);
	blankscreen(0);
}

void
blanker(void *)
{
	int tics;

	tics = 0;
	for(;;){
		if(doblank > 0){
			doblank = 0;
			tics = 10;
		}
		if(tics > 0 && --tics == 0)
			blankscreen(1);
		sleep(1000);
	}
}

void
grabmouse(void*)
{
	int fd, x, y;
	char ibuf[256], obuf[256];

	if(debug)
		return;
	fd = open("/dev/mouse", ORDWR);
	if(fd < 0)
		error("can't open /dev/mouse: %r");

	snprint(obuf, sizeof obuf, "m %d %d",
		screen->r.min.x + Dx(screen->r)/2,
		screen->r.min.y + Dy(screen->r)/2);
	while(read(fd, ibuf, sizeof ibuf) > 0){
		ibuf[12] = 0;
		ibuf[24] = 0;
		x = atoi(ibuf+1);
		y = atoi(ibuf+13);
		if(x != screen->r.min.x + Dx(screen->r)/2 ||
		   y != screen->r.min.y + Dy(screen->r)/2){
			fprint(fd, "%s", obuf);
			doblank = 1;
		}
	}
}

/* lay down text at `p' */
static void
screenstring(Point p, char *s)
{
	string(screen, p, screen->display->white, ZP, font, s);
	flushimage(display, 1);
}

void
lockscreen(void)
{
	enum { Nfld = 5, Fldlen = 12, Cursorlen = 2*4 + 2*2*16, };
	char *s;
	char buf[Nfld*Fldlen], *flds[Nfld], newcmd[128], cbuf[Cursorlen];
	int fd, dx, dy;
	Image *i;
	Point p;
	Rectangle r;
	Tm *tm;

	fd = open("/dev/screen", OREAD);
	if(fd < 0)
		error("can't open /dev/screen: %r");
	if(read(fd, buf, Nfld*Fldlen) != Nfld*Fldlen)
		error("can't read /dev/screen: %r");
	close(fd);
	buf[sizeof buf-1] = 0;
	if(tokenize(buf, flds, Nfld) != Nfld)
		error("can't tokenize /dev/screen header");
	snprint(newcmd, sizeof newcmd, "-r %s %s %d %d",
		flds[1], flds[2], atoi(flds[3]) - 1, atoi(flds[4]) - 1);
	newwindow(newcmd);
	if (initdraw(nil, nil, "screenlock") < 0)
		sysfatal("initdraw failed");
	if(display == nil)
		error("no display");

	/* screen is now open and covered.  grab mouse and hold on tight */
	procrfork(grabmouse, nil, 4096, RFFDG);
	procrfork(blanker, nil, 4096, RFFDG);
	fd = open(pic, OREAD);
	if(fd > 0){
		i = readimage(display, fd, 0);
		if(i){
 			r = screen->r;
			p = Pt(r.max.x / 2, r.max.y * 2 / 3); 
			dx = (Dx(screen->r) - Dx(i->r)) / 2;
			r.min.x += dx;
			r.max.x -= dx;
			dy = (Dy(screen->r) - Dy(i->r)) / 2;
			r.min.y += dy;
			r.max.y -= dy;
			draw(screen, screen->r, display->black, nil, ZP);
			draw(screen, r, i, nil, i->r.min);
			flushimage(display, 1);
		}
		close(fd);

		/* identify the user on screen, centered */
		tm = localtime(time(0));
		s = smprint("user %s at %d:%02.2d", getuser(), tm->hour, tm->min);
		p = subpt(p, Pt(stringwidth(font, "m") * strlen(s) / 2, 0));
		screenstring(p, s);
	}

	/* clear the cursor */
	fd = open("/dev/cursor", OWRITE);
	if(fd > 0){
		memset(cbuf, 0, sizeof cbuf);
		write(fd, cbuf, sizeof cbuf);
		/* leave it open */
	}
}

void
threadmain(int argc, char *argv[])
{
	readfile("#c/user", user, sizeof user, 1);

	if((vgactl = open("/dev/vgactl", OWRITE)) < 0)
		vgactl = open("#v/vgactl", OWRITE);

	ARGBEGIN{
	case 'd':
		debug++;
		break;
	default:
		usage();
	}ARGEND

	if(argc != 0)
		usage();

	doblank = 1;
	lockscreen();
	checkpassword();
	threadexitsall(nil);
}
