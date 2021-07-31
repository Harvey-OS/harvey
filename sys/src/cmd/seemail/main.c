#include <u.h>
#include <libc.h>
#include <libg.h>
#include "dat.h"

#define	INSET	5		/* inset from screen.r to written area */
#define	DATEX	(INSET+5)	/* coordinates of date string */
#define	DATEY	(INSET+1)
#define	XOFF	(INSET+5)	/* offset from corner to picture */
#define	YOFF	(INSET+20)
#define	FCWID	(MAXX+5)	/* width of face, incl. strings */
int	FCHT;			/* height of face, incl. strings */

Point	dp;			/* upper left corner of first picture */
Bitmap	*b;
SRC	old, new;
int	aflag;	/* start at end of logfile */
int	sflag;	/* collapse equal faces */

#define	ALARM	15000

void
alarmf(void *a, char *s)
{
	USED(a);
	if(strcmp(s, "alarm") == 0)
		noted(NCONT);
	noted(NDFLT);
}

void
main(int argc, char *argv[])
{
	int n, lastb;
	uchar buf[14];
	char err[ERRLEN];
	int fd;
	Event e;
	char *log;

	binit(0, 0, "seemail");

	log = "/sys/log/mail";
	ARGBEGIN{
	case 's':
		sflag++;
		break;
	case 'a':
		aflag = 1;
		break;
	case 'f':
		log = ARGF();
		break;
	default:
		error("usage: seemail [-a] [-s] [-fFILE]");
	}ARGEND
	if(argc > 0)
		log = argv[0];

	b = balloc(Rect(0, 0, MAXX, MAXY), 3);	/* byte map */

	srand(time(0));
	start_trail(log);
	FCHT = (MAXY - 2 + 2*medifont->height);

	fd = open("/dev/mouse", OREAD);
	if(fd < 0)
		error("can't open /dev/mouse");

	lastb = 0;
	notify(alarmf);
	alarm(ALARM);
	redraw();
	for(;;){
		alarm(ALARM);
		n = read(fd, buf, sizeof buf);
		alarm(0);
		if(n == sizeof buf){
			if(lastb!=buf[1] && (buf[1]&0x81))
				redraw();
			lastb = e.mouse.buttons;
		}else{
			errstr(err);
			if(strcmp(err, "interrupted") != 0)
			if(strcmp(err, "no error") != 0)
				error(err);
			/* else alarm timeout */
			Date(0);
			trail(log);
		}
	}
	exits(0);
}

void
error(char *s)
{
	fprint(2, "seemail: %s\n", s);
	exits(s);
}

void
redraw(void)
{
	screen.r = bscreenrect(0);
	bitblt(&screen, screen.r.min, &screen, screen.r, 0);	/* cls */
	border(&screen, screen.r, 1, F);
	dp.x = screen.r.min.x + XOFF;
	dp.y = screen.r.min.y + YOFF;
	Same = First = 1;
	Date(1);
}

void
showimage(SRC *From, int Shift)
{
	int nfaceh_1, nfacew_1;		/* # faces high and wide, minus 1 */
	Rectangle sr;
	Point dp2;

	wrbitmap(b, 0, MAXY, &From->pix[0][0]);
	overwrite(b);

	if(Shift){
		sr = screen.r;
		sr.min.x += XOFF;
		sr.min.y += YOFF;
		sr.max.x -= XOFF;
		sr.max.y -= YOFF;
	
		nfacew_1 = Dx(sr)/FCWID - 1;
		if(nfacew_1 < 0)
			nfacew_1 = 0;
		nfaceh_1 = Dy(sr)/FCHT - 1;
		if(nfaceh_1 < 0)
			nfaceh_1 = 0;
	
		sr.max.x = sr.min.x + nfacew_1*FCWID;
		sr.min.y += nfaceh_1*FCHT;
		sr.max.y = sr.min.y + FCHT;
	
		dp2.x = sr.min.x + FCWID;
		dp2.y = sr.min.y;

		do{	/* shift faces */
			bitblt(&screen, dp2, &screen, sr, S);
			sr.max.y = sr.min.y;
			sr.min.y -= FCHT;
			if(dp2.y > dp.y){
				Rectangle nr;
				nr = sr;
				nr.min.x += nfacew_1*FCWID;
				nr.max.x = nr.min.x+FCWID;
				dp2.x -= FCWID;
				bitblt(&screen, dp2, &screen, nr, S);
				dp2.x += FCWID;
			}
			dp2.y -= FCHT;
		}while(dp2.y >= dp.y);
		nomessage();
	}
	bitblt(&screen, dp, b, b->r, S);	/* add new face */
	bflush();
}

void
nomessage(void)
{
	Rectangle dr;

	dr.min.x = dp.x;
	dr.min.y = dp.y + MAXY;
	dr.max.x = dp.x + FCWID;
	dr.max.y = dp.y + FCHT;
	bitblt(&screen, dr.min, &screen, dr, 0);
}

void
message(char *buf, char *tm)
{
	itag(0, buf);
	itag(medifont->height - 2, tm);
}

void
itag(int m, char *buf)
{
	Point p;
	char *z;

	z = &buf[strlen(buf)-1];
	while(strwidth(medifont,buf) > FCWID)
		*z-- = '\0';
	p.x = dp.x + (FCWID-strwidth(medifont, buf))/2;
	p.y = dp.y + MAXY + m;
	string(&screen, p, medifont, buf, S|D);
}

void
Date(int force)
{
	static char last[32];
	char *r, *q;
	Point p;

	q = ctime(time(0L));
	if((r=strrchr(q, ':')) == 0){
		print("bad date %s\n", q);
		return;
	}
	*r = '\0';
	p.x = screen.r.min.x + DATEX;
	p.y = screen.r.min.y + DATEY;
	if(force || strcmp(q, last)!=0)
		string(&screen, p, font, q, S);
	strcpy(last, q);
	bflush();
}
