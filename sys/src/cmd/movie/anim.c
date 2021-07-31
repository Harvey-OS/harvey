#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#define _LIBG_EXTENSION
#include <libg.h>

#define	ONES	(~0UL)

#define	xtbinit(a,b,c,d)	binit(a,b, "anim")

typedef unsigned char uchar;

#define	INPUTSCALE	10000	/* inherited from fdevelop */
int	inputscale	= INPUTSCALE;

#define	NZOOM	20
Rectangle zoomr[NZOOM];	/* for zooming in and out */
int	nzoom = 0;	/* current one */

Rectangle origscreen;

FILE	*inf;

/* options for various commands */

#define	Tcenter	10
#define	Tljust	20
#define	Trjust	30
#define	Tabove	40
#define	Tbelow	50
#define	Tsmall	1
#define	Tmedium	2
#define	Tbig	3
#define	Tbigbig	4

#define	Lsolid	10
#define	Lfat	20
#define	Lfatfat	30
#define	Ldotted	40
#define	Ldashed	50
#define	Lline	1
#define	Larrow1	2
#define	Larrow2	3
#define	Larrow3	4

#define	Bnofill	1
#define	Bfill	2

#define	Cnofill	1
#define	Cfill	2


#define	eq(s,t)	(strcmp((char *) s, (char *) t) == 0)
#define	muldiv(a,b,c)	(((a) * (b)) / (c))	/* ought to be inline */

/* holds data for all input objects */

unsigned memsize;		/* bytes */
uchar	*inbuf;			/* input collected here */
uchar	*input;			/* leave a null at front */
uchar	*nextinp;		/* next free slot in input */
int	nobj	= 0;		/* number of objects in input */
int	overflow = 0;		/* 1 => too much input */

long	slot[2000];		/* slots */
int	slotnum;

uchar	*savechar(int);
uchar	*draw_obj(uchar *, int, int);
uchar	*step_obj(uchar *, int, int);
uchar	*prev_obj(uchar *);
uchar	*next_obj(uchar *);
uchar	*refresh(uchar *);
int	zoomin(void);
Rectangle square(Rectangle);

Point	fetchpt(int, uchar *);
Point	scalept(int, Point);

void	putstring(char *);

int	reshaped	= 0;	/* set to 1 in ereshaped */
/* this ought to include Expose events in X... */

Mouse	mouse;
int	buttondown(void);
Cursor	skull, deadmouse;

char	kbdline	[100];
int	do_kbd(void);

#define	MAXVIEW	20
char	*viewname[MAXVIEW];
Rectangle viewpt[MAXVIEW];
int	curview	= 0;
int	nview	= 0;
#define	INSET	4	/* picture inset from frame */

#define	AW	8	/* arrowhead width and height */
#define	AH	10

#define	MARGINPCT	5
int	margin	= MARGINPCT;	/* percent margin around edges */

#define	MAXCLICK	20
char	*clickname[MAXCLICK];		/* click names */
int	clickval[MAXCLICK];		/* 1 => click on this */
int	clicking = 0;			/* number of active clicks */
int	nclick	= 0;

int	boxops[] ={	'n', Bnofill, 'f', Bfill, 0 };
int	circops[] ={	'n', Cnofill, 'f', Cfill, 0 }; 
int	textops[] ={	'c', Tcenter, 'l', Tljust, 'r', Trjust,
			's', Tsmall, 'm', Tmedium, 'b', Tbig, 'B', Tbigbig, 0 };
int	lineops[] ={	's', Lsolid, 'f', Lfat, 'F', Lfatfat, 'o', Ldotted, 'a', Ldashed,
			'-', Lline, '>', Larrow1, '<', Larrow2, 'x', Larrow3, 0 };

enum	{ Fwd = 1, Back = 0 };

enum	{ F_XOR = DxorS, F_CLR = Zero, F_STORE = S, F_OR = DorS };

int	delay		= 1;	/* how long to delay between things */
int	singstep	= 0;	/* single step if 1, cycle if 2 */
int	dir		= Fwd;		/* 1 = fwd, 0 = backward */
int	fatness		= 0;		/* n => draw with 2n+1 lines */
int	xormode		= F_XOR;	/* otherwise OR/CLR */

enum Menuitems {
	Again = 0,		/* Menu items -- must be 0, other in order */
	Faster,
	Slower,
	Step,
	Forward,
	Fatter,
	Thinner,
	Zoomin,
	Zoomout,
	Xor,
	Newfile,
	Quit,
	Backward ,
	Proceed,
	Hit,
	Cycle,
};

char	*m3[]	= { "again", "faster", "slower", "1 step", "backward",
		    "fatter", "thinner", "zoom in", "zoom out",
		    "or mode", "new file", "Quit?", 0 };
char	*stepmenu[]	= { "1 step", "cycle", "run" };
char	*dirmenu[]	= { "forward", "backward" };
char	*modemenu[]	= { "or mode", "xor mode" };

char	*m3gen(int);
char	*m2gen(int);

Menu	mbut3	= { (char **) 0, m3gen, 0 };
Menu	mbut2	= { (char **) 0, m2gen, 0 };

int	last_hit;
int	last_but;
int	Etimer;

extern	int	do_rcv(FILE *);
extern	int	skipline(FILE *);
extern	int	skipbl(FILE *);
extern	int	badfile(FILE *);
extern	int	savect(int);
extern	int	saveint(int);
extern	int	savelong(long);
extern	int	savepair(int, int);
extern	int	sendopt(int[], char *);
extern	int	erase(uchar *);
extern	int	fatline(Point, Point, int, int);
extern	int	fatcircle(Point, int, int, int);
extern	int	arrow(Point, Point, int, int, int);

extern	FILE	*show(FILE *);
extern	int	clear(void);
extern	int	view_setup(int);
extern	int	init_params(void);
extern	int	checkmouse(void);
extern	int	domouse(void);


main(int argc, char *argv[])
{
	FILE *fp = stdin;

	xtbinit(0, 0, &argc, argv);
	einit(Emouse|Ekeyboard);
	Etimer = etimer(0, 75);
	setbuf(stderr, NULL);

	origscreen = screen.r;
	memsize = 1000000;
	while (argc > 1 && argv[1][0] == '-') {
		switch (argv[1][1]) {
		case 'm':
			memsize = atoi(&argv[1][2]);
			break;
		}
		argv++;
		argc--;
	}
	if (inbuf == NULL && (inbuf = (uchar *) malloc(memsize)) == NULL) {
		fprintf(stderr, "can't allocate %u bytes", memsize);
		exit(1);
	}
	if ((fp = fopen(argv[1], "r")) == NULL) {
		fprintf(stderr, "can't open file %s\n", argv[1]);
		exit(1);
	}

	while ((fp = show(fp)) != 0)
		;
}

FILE *show(FILE *fp)
{
	uchar *ip;
	int i, n, c;
	Event tev;

	init_params();
	input = inbuf+1;	/* leave a null at front */
	nextinp = inbuf+1;	/* next free slot in input */

	clear();
	cursorswitch(&deadmouse);
	do_rcv(fp);
	cursorswitch(0);

	dir = Fwd;
	ip = nextinp;	/* pointing at the end */
  again:
	for (; ip; ) {
		checkmouse();	/* waits for a menu selection */
		n = domouse();
		if (n == Quit)
			return 0;
		if (n == Hit)
			continue;	/* wait for another mouse hit */
		if (n == Forward) {	/* change from fwd to back -- fiddle ip */
			ip = next_obj(ip);
			continue;
		}
		if (n == Backward) {
			ip = prev_obj(ip);
			continue;
		}
		if (n == Newfile) {
			char buf[100];
			/* fprintf(stderr, "filename? "); */
			putstring("filename? ");
			if (do_kbd() == 0)
				continue;
			fclose(fp);
			/* open up the new file, return its fp.  ick */
			if ((fp = fopen(kbdline, "r")) != NULL)
				return fp;
			sprintf(buf, "can't open %s", kbdline);
			fprintf(stderr, "%s\n", buf);
 			putstring(buf);
			continue;
		}
		if (n == Again) {
			if (reshaped) {
				view_setup(nview);
				reshaped = 0;
			}
			clear();
			dir = Fwd;
			ip = input;
		}
		if (n == Zoomin || n == Zoomout) {
			clear();
			dir = Fwd;
			ip = refresh(ip);
			continue;
		}
		if (singstep == 1) {
			ip = step_obj(ip, xormode, dir);
		} else {		/* free running */
		   cycle:
			while (ip) {
				ip = step_obj(ip, xormode, dir);
				if (buttondown()) {	/* someone has touched the mouse */
					singstep = 0;
					break;		/* back round main loop */
				}
				for(i = delay-1; i>0; i--)
					eread(Etimer, &tev);
			}
			if (singstep == 2) {
				clear();
				dir = Fwd;
				ip = input;
				goto cycle;
			}

		}
	}
	if (ip == 0)
		ip = dir == Fwd ? nextinp : input;
	goto again;
}

uchar *refresh(uchar *cp)	/* redraw the screen up to cp */
{
	uchar *ip;

	for (ip = input; ip != 0 && ip < cp; )
		ip = step_obj(ip, xormode, Fwd);
	return ip;
}

init_params(void)
{
	int i;

	for (i = 0; i < MAXCLICK; i++)
		if (clickname[i]) {
			free(clickname[i]);
			clickname[i] = 0;
			clickval[i] = 0;
		}
	for (i = 0; i < MAXVIEW; i++)
		if (viewname[i]) {
			free(viewname[i]);
			viewname[i] = 0;
		}
	nview = nclick = curview = nobj = overflow = slotnum = 0;
	screen.r = inset(screen.r, INSET);
	zoomr[0] = screen.r = square(screen.r);
	nzoom = 0;
}

Rectangle shrink(Rectangle r, int pct)	/* shrink rectangle by 2*pct */
{
	int dx = (r.max.x-r.min.x) * pct / 100;
	int dy = (r.max.y-r.min.y) * pct / 100;
	r.min = add(r.min, Pt(dx,dy));
	r.max = sub(r.max, Pt(dx,dy));
	return r;
}

Rectangle square(Rectangle r)	/* return largest square within r */
{
	double ar;

	ar = (double) Dy(r)/Dx(r);	/* y/x aspect ratio */
	if (ar < 0.9 || ar > 1.1)	/* leave it alone */
		return r;
	if (ar <= 1)	/* x is longer than y */
		r.max.x = r.min.x + Dy(r);
	else		/* y is longer */
		r.max.y = r.min.y + Dx(r);
	return r;
}

view_setup(int n)
{
	int i, j, v, dx, dy, r, c;
	Rectangle sr;

	sr = square(screen.r);
	switch (n) {
	case 1: r = 1; c = 1; break;
	case 2: r = 2; c = 1; break;
	case 3: case 4: r = 2; c = 2; break;
	case 5: case 6: r = 3; c = 2; break;
	case 7: case 8: case 9: r = 3; c = 3; break;
	default: r = (n+2)/3; c = 3; break; /* finking out */
	}
	dx = Dx(sr) / c;
	dy = Dy(sr) / r;
	v = 0;
	for (i = 0; i < r && v < n; i++)
		for (j = 0; j < c && v < n; j++) {
			viewpt[v] = sr;
			viewpt[v].min.x = sr.min.x + j * dx;
			viewpt[v].max.x = sr.min.x + (j+1) * dx;
			viewpt[v].min.y = sr.min.y + i * dy;
			viewpt[v].max.y = sr.min.y + (i+1) * dy;
			v++;
		}
	for (i = 0; i < n; i++) {
		viewpt[i] = shrink(viewpt[i], margin);
	}
	zoomr[nzoom] = screen.r;
}

void ereshaped(Rectangle r)
{
	reshaped = 1;
	origscreen = r;
	screen.r = inset(r, INSET);
	zoomr[0] = screen.r = square(screen.r);
	nzoom = 0;
}

drawrect(Rectangle r, int mode)
{
	segment(&screen, r.min, Pt(r.min.x,r.max.y), ONES, mode);
	segment(&screen, r.min, Pt(r.max.x,r.min.y), ONES, mode);
	segment(&screen, r.max, Pt(r.min.x,r.max.y), ONES, mode);
	segment(&screen, r.max, Pt(r.max.x,r.min.y), ONES, mode);
}

domouse(void)
{
	int i, n;
	Rectangle r;

	if (last_but == 1)
		return Proceed;
	if (last_but == 3) {
		switch (last_hit) {
		case Again:
			return Again;
		case Faster:
			if (delay > 1)
				delay /= 2;
			return Hit;
		case Slower:
			delay *= 2;
			return Hit;
		case Step:
			singstep = (singstep+1) % 3;
			return Hit;
		case Forward:
			dir = 1 - dir;
			if (xormode == F_OR)
				xormode = F_CLR;
			else if (xormode == F_CLR)
				xormode = F_OR;
			return dir == Fwd ? Forward : Backward;
		case Fatter:
			fatness++;
			return Hit;
		case Thinner:
			if (fatness > 0)
				fatness--;
			return Hit;
		case Zoomin:
			return zoomin();
		case Zoomout:
			if (nzoom > 0)
				nzoom--;
			return Zoomout;
		case Xor:
			if (xormode == F_OR || xormode == F_CLR)
				xormode = F_XOR;
			else if (dir == Fwd)
				xormode = F_OR;
			else
				xormode = F_CLR;
			return Hit;
		case Newfile:
			return Newfile;
		case Quit:
			return Quit;
		default:
			return Hit;
		}
	} else if (last_but == 2) {
		Rectangle r;
		if (last_hit == -1)
			return Hit;
		else if (last_hit < nview) {
			r = getrect(2, &mouse);		/* really ought to be 2,3 */
			if (r.min.x == 0 && r.max.x == 0)	/* bailed out */
				return Hit;
			if (Dx(r) < 10 || Dy(r) < 10)	/* too small */
				return Hit;
			if (eqpt(r.min, r.max))
				r = inset(origscreen, INSET);
			drawrect(inset(viewpt[last_hit], -(INSET+fatness)), F_CLR);
			drawrect(r, F_OR);
			viewpt[last_hit] = r = inset(r, INSET+fatness);
			return Hit;
		} else {	/* a click */
			if (clickval[last_hit-nview]) {	/* was on, so turn off */
				clickval[last_hit-nview] = 0;
				clicking--;
			} else {
				clickval[last_hit-nview] = 1;
				clicking++;
			}
			return Hit;
		}
	}
}

int zoomin(void)
{
	Rectangle r;

	r = getrect(3, &mouse);
	/* fprintf(stderr, "into zoomin %d, r %d %d %d %d\n", */
	/*	 nzoom, r.min.x, r.min.y, r.max.x, r.max.y); */
	if (r.min.x == 0)	/* no selection */
		return Hit;
	if (Dx(r) < 10 || Dy(r) < 10) {	/* very small */
		if (Dx(zoomr[nzoom+1]) < 10)	/* no previous one */
			return Hit;
		r = zoomr[nzoom+1];	/* use previous one */
	} else {	/* preserve aspect ratio if near square */
		double ar = (double) Dy(r) / Dx(r);
		if (ar > 0.9 && ar < 1.1)	/* force ratio */
			r = square(r);
	}
	/* either new one or zoom back in */
	if (nzoom < NZOOM-1) {
		nzoom++;
		zoomr[nzoom] = r;
	}
	return Zoomin;
}


do_rcv(FILE *fp)
{
	int c, n, b, m, i, v, x1, x2, y1, y2;
	char opts[100];
	char text[100];
	char *p;
	uchar *ip, *oip;

    while ((b = c = getc(fp)) != EOF) {
	switch (c) {
	case ' ':
	case '\t':
	case '\n':
		break;
	case '#':		/* comments */
		skipline(fp);
		break;
	case 'b':		/* blank.  ignore for now */
		skipline(fp);
		break;

	case 'd':	/* definition of some sort */
		switch (c = skipbl(fp)) {
		case 'v':	/* view */
		case 'c':	/* click */
			if (fscanf(fp, "%d %s", &i, text) != 2)
				return badfile(fp);
			if (c == 'c') {
				clickname[i] = malloc(strlen(text)+1);
				strcpy(clickname[i], text);
				nclick++;
			} else {	/* c == 'v' */
				viewname[i] = malloc(strlen(text)+1);
				strcpy(viewname[i], text);
				nview++;
			}
			skipline(fp);	/* might be a title there */
			break;
		case 'p':	/* only pragma is 'e' for end */
			skipline(fp);
			view_setup(nview);
			break;
		default:
			return badfile(fp);
		}
		break;

	case 'c':		/* click */
		if (fscanf(fp, "%d", &i) != 1)
			return badfile(fp);
		oip = savechar(c);
		savechar(i);
		savect(nextinp-oip);
		skipline(fp);
		break;
	case 'e':		/* erase */
		if (fscanf(fp, "%d", &i) != 1)
			return badfile(fp);
		oip = savechar(c);
		savelong(slot[i]);
		savect(nextinp-oip);
		skipline(fp);
		break;

	case 'g':		/* geom:  draw line, box, circle, ... */
		if (fscanf(fp, "%d", &slotnum) != 1)
			return badfile(fp);
		switch (c = skipbl(fp)) {
		case 'l':
		case 'b':
			if (fscanf(fp, "%d %s %d %d %d %d", &v, opts, &x1, &y1, &x2, &y2) != 6)
				return badfile(fp);
			slot[slotnum] = nextinp-input;
			oip = savechar(c);
			savechar(v);
			savechar(sendopt(c=='b' ? boxops : lineops, opts));	/* options */
			savepair(x1, y1);
			savepair(x2, y2);
			savect(nextinp-oip);
			break;

		case 'c':	/* circle */
			if (fscanf(fp, "%d %s %d %d %d", &v, opts, &x1, &y1, &x2) != 5)
				return badfile(fp);;
			slot[slotnum] = nextinp-input;
			oip = savechar('o');	/* 'o' is for circle */
			savechar(v);
			savechar(sendopt(circops, opts));
			savepair(x1, y1);
			saveint(x2);
			savect(nextinp-oip);
			break;

		case 't':	/* text */
			if (fscanf(fp, "%d %s %d %d", &v, opts, &x1, &y1) != 4)
				return badfile(fp);
			slot[slotnum] = nextinp-input;
			oip = savechar('t');
			savechar(v);
			savechar(sendopt(textops, opts));
			savepair(x1, y1);
			getc(fp);	/* skip 1 separator; no quotes */
			for (p = text; (c = getc(fp)) != '\n'; )
				*p++ = c;
			*p = 0;
			ungetc('\n', fp);
			/* slow... */
			p = (char *) nextinp + 1;
			if (eq(text, "bullet"))
				strcpy(p, "*");
			else if (eq(text, "dot"))
				strcpy(p, ".");
			else if (eq(text, "circle"))
				strcpy(p, "o");
			else if (eq(text, "times"))
				strcpy(p, "x");
			else
				strcpy(p, text);
			*nextinp = n = strlen(p);	/* insert count before string */
			nextinp += n + 2;		/* +2 = count before + \0 on end */
			savect(nextinp-oip);
			break;

		default:
			return badfile(fp);
		}
		break;

	}
	if (b == 'c' || b == 'e' || b == 'g')	/* graphical objs */
		draw_obj(oip, F_XOR, Fwd);
	*nextinp = 0;
    }
}

badfile(FILE *fp)
{
	fprintf(stderr, "input file is not in .i format");
	while (getc(fp) != EOF)
		;
	fclose(fp);
}

sendopt(int optvals[], char *opts)
{
	int i, n;

	n = 0;
	for (i = 0; optvals[i] && opts; i += 2)
		if (*opts == optvals[i]) {
			n += optvals[i+1];
			opts++;
		}
	return n;
}

clear(void)	/* screen */
{
	Rectangle r;

	screen.r = origscreen;
	r = inset(screen.r, INSET);
	bitblt(&screen, r.min, &screen, r, 0);
}


uchar *savechar(int c)
{
	*nextinp++ = c;
	return nextinp-1;
}

savect(int n)
{
	if (n > 255) {
		fprintf(stderr, "text string too long");
		n = 255;
	}
	*nextinp++ = n;
}

saveint(int n)
{
	*nextinp++ = n >> 8;
	*nextinp++ = n & 0377;
}

savelong(long n)
{
	*nextinp++ = n >> 24;
	*nextinp++ = n >> 16;
	*nextinp++ = n >> 8;
	*nextinp++ = n;
}

savepair(int x, int y)
{
	saveint(x);
	saveint(y);
}

getpoint(uchar *ip)
{
	return *ip << 8 | *(ip+1);
}

long getlong(uchar *ip)
{
	return *ip << 24 | *(ip+1) << 16 | *(ip+2) << 8 | *(ip+3);
}

Point scalept(int v, Point p)
{
	p.x = muldiv(p.x, Dx(viewpt[v]), inputscale);
	p.y = Dy(viewpt[v]) - muldiv(p.y, Dy(viewpt[v]), inputscale);
	return p;
}

scalex(int v, int x)
{
	int i;
	int nx = muldiv(x, Dx(viewpt[v]), inputscale);

	for (i = 1; i <= nzoom; i++)
		nx = nx * ((double)Dx(screen.r) / Dx(zoomr[i]));
	return nx;
}

Point fetchpt(int v, uchar *ip)
{
	Point pt;
	int i;

	pt.x = *ip << 8 | *(ip+1);
	pt.y = *(ip+2) << 8 | *(ip+3);
	pt = scalept(v, pt);
	pt = add(pt, viewpt[v].min);
	for (i = 1; i <= nzoom; i++) {
		pt.x = (pt.x-zoomr[i].min.x) * (double) Dx(zoomr[0]) / Dx(zoomr[i]);
		pt.y = (pt.y-zoomr[i].min.y) * (double) Dy(zoomr[0]) / Dy(zoomr[i]);
	}
	/* pt.x += screen.r.min.x;
	/* pt.y += screen.r.min.y;
	*/
	return pt;
} 

/*
  Encoding:  type, view#, opts, coords, chars, etc., # = length of group
	bvoxxyyxxyy#
	lvoxxyyxxyy#
	ovoxxyyrr#
	tvoxxyynccc0#
	ennnn#
	cn#

*/

uchar *prev_obj(uchar *ip)
{
	if (ip <= input)
		return 0;
	return ip - ip[-1] - 1;
}

uchar *next_obj(uchar *ip)
{
	if (ip < input || ip >= nextinp)
		return 0;
	switch (*ip) {
	case 0:
		return 0;
	case 'b':
	case 'l':
		return ip + 12;
	case 'o':
		return ip + 10;
	case 't':
		return ip + ip[7] + 10;
	case 'c':
		return ip + 3;
	case 'e':
		return ip + 6;
	default:
		return 0;
	}
}

uchar *step_obj(uchar *ip, int mode, int dir)	/* draw objs until one that changes something */
{
	int c;
	uchar *oip;

	if (clicking) {
		for (;;) {
			oip = ip;
			ip = draw_obj(ip, mode, dir);
			if (ip == 0 || (oip && *oip == 'c' && clickval[oip[1]]))
				return ip;
		}

	} else {	/* stepping */
		while (ip) {
			c = *ip;
			ip = draw_obj(ip, mode, dir);
			if (c == 'b' || c == 'l' || c == 't' || c == 'e' || c == 'o')
				return ip;
		}
		return ip;
	}
}

uchar *draw_obj(uchar *ip, int mode, int dir)	/* draw obj from coords at ip */
{
	int c, r, thick, n, shift, head;
	Point p0, p1, p2;

	if (ip < input || ip >= nextinp)
		return 0;
	switch (c = *ip++) {
	case 'b':
		p0 = fetchpt(*ip, ip+2);
		p1 = fetchpt(*ip, ip+6);
		if (ip[1] == Bfill) {
			if (p0.y < p1.y)
				/* rectf(&screen, Rpt(p0, p1), mode); */
				bitblt(&screen, p0, &screen, Rpt(p0, p1), mode);
			else
				/* rectf(&screen, Rect(p0.x,p1.y,p1.x,p0.y), mode); */
				bitblt(&screen, p0, &screen, Rect(p0.x,p1.y,p1.x,p0.y), mode);
		} else {
			segment(&screen, p0, Pt(p0.x,p1.y), ONES, mode);
			segment(&screen, Pt(p0.x,p1.y), p1, ONES, mode);
			segment(&screen, p1, Pt(p1.x,p0.y), ONES, mode);
			segment(&screen, Pt(p1.x,p0.y), p0, ONES, mode);
		}
		if (dir == Fwd)
			ip += 1+9+1;
		else
			ip -= (*(ip-2) + 2);
		break;
	case 'l':
		p0 = fetchpt(*ip, ip+2);
		p1 = fetchpt(*ip, ip+6);
		thick = ip[1]/10;	/* ought to be a macro! */
		if (thick == Ldotted/10 || thick == Ldashed/10)
			thick = 1;
		thick = 2 * thick - 1;	/* 1,3,5 */
		fatline(p0, p1, mode, thick);
		head = ip[1]%10;	/* ditto */
		if (head == Larrow1 || head == Larrow3)
			arrow(p0, p1, AW, AH, mode);
		if (head == Larrow2 || head == Larrow3)
			arrow(p1, p0, AW, AH, mode);
		if (dir == Fwd)
			ip += 1+9+1;
		else
			ip -= (*(ip-2) + 2);
		break;
	case 'o':
		p0 = fetchpt(*ip, ip+2);
		r = scalex(*ip, getpoint(ip+6));
		/* fprintf(stderr, "draw circle %d at %d,%d\n", r, p0.x, p0.y); */
		if (ip[1] == Cnofill)
			fatcircle(p0, r, mode, 1);
		else
			disc(&screen, p0, r + fatness, ONES, mode);
		if (dir == Fwd)
			ip += 1+7+1;
		else
			ip -= (*(ip-2) + 2);
		break;
	case 't':
		p0 = fetchpt(*ip, ip+2);
		/* fprintf(stderr, "draw text %s at %d,%d\n", ip+7, p0.x, p0.y); */
		n = ip[6];
		shift = (ip[1]/10) * 10;	/* ought to be a macro! */
		if (shift == Tljust)
			shift = 0;
		else if (shift == Tcenter)
			shift = (9 * n) / 2;	/* 9 = char width */
		else
			shift = 9 * n;
 		string(&screen, sub(p0, Pt(shift,6)), font, (char *) ip+7, mode);
		if (dir == Fwd)
			ip += 1+5 + *(ip+6)+2 + 1;
		else
			ip -= (*(ip-2) + 2);
		break;
	case 'e':
		erase(ip-1);
		if (dir == Fwd)
			ip += 5;
		else
			ip -= (*(ip-2) + 2);
		break;
	case 'c':
		if (dir == Fwd)
			ip += 2;
		else
			ip -= (*(ip-2) + 2);
		break;
	default:
		ip = 0;
		break;
	}
	return ip;
}

erase(uchar *ip)
{
	long target = getlong(ip+1);	/* target label index */
	int mode = F_XOR;

	if (xormode == F_OR || xormode == F_CLR)
		mode = dir == Fwd ? F_CLR : F_OR;
	draw_obj(input+target, mode, Fwd);
}

#define abs(x) ((x) >= 0 ? (x) : -(x))

fatline(Point p0, Point p1, int mode, int thick)
{
	int i, fat, beg, nl;

	fat = thick * (2 * fatness + 1);
	beg = fat / 2;
	if (abs(p1.x-p0.x) >= abs(p1.y-p0.y)) {	/* horizontal */
		for (nl = 0, i = -beg; nl < fat; nl++, i++)
			segment(&screen, add(p0, Pt(0,i)), add(p1, Pt(0,i)), ONES, mode);
	} else {
		for (nl = 0, i = -beg; nl < fat; nl++, i++)
			segment(&screen, add(p0, Pt(i,0)), add(p1, Pt(i,0)), ONES, mode);
	} 
}

fatcircle(Point p0, int r, int mode, int thick)
{
	int i, fat, beg, nl;

	fat = thick * (2 * fatness + 1);
	beg = fat / 2;
	for (nl = 0, i = -beg; nl < fat; nl++, i++)
		circle(&screen, p0, r+i, ONES, mode);
}

arrow(Point p1, Point p2, int w, int h, int c)
	/* draw arrow of height,width (h,w) at p2 of segment p1,p2 */
{
	Point d;
	int norm, qx, qy, lx, ly;

	d = sub(p2, p1);
	norm = sqrt((long)d.x*d.x + (long)d.y*d.y);
	if (norm == 0)	/* shouldn't happen, but ... */
		return;
	qx = p2.x - muldiv(h, d.x, norm);
	qy = p2.y - muldiv(h, d.y, norm);
	lx = muldiv(w/2, -d.y, norm);
	ly = muldiv(w/2, d.x, norm);
	/* segment(&screen, p1, p2, c); */
	segment(&screen, Pt(qx+lx, qy+ly), p2, ONES, c);
	segment(&screen, Pt(qx-lx, qy-ly), p2, ONES, c);
}

skipbl(FILE *fp)
{
	int c;

	while ((c = getc(fp)) == ' ' || c == '\t')
		;
	return c;
}

skipline(FILE *fp)
{
	int c;

	while ((c = getc(fp)) != '\n')
		;
	ungetc('\n', fp);
}

char *m3gen(int n)
{
	static char buf[50];

	if (n < 0 || n > Quit)
		return 0;
	else if (n == Faster) {
		sprintf(buf, "faster %d", delay);
		return buf;
	} else if (n == Slower) {
		sprintf(buf, "slower %d", delay);
		return buf;
	} else if (n == Step) {
		return stepmenu[singstep];
	} else if (n == Forward) {
		return dirmenu[dir];
	} else if (n == Fatter) {
		sprintf(buf, "fatter %d", fatness+1);
		return buf;
	} else if (n == Thinner) {
		sprintf(buf, "thinner %d", fatness+1);
		return buf;
	} else if (n == Xor) {
		return xormode == F_XOR ? modemenu[0] : modemenu[1];
	} else
		return m3[n];
}

char *m2gen(int n)
{
	static char buf[50];

	if (n < 0 || n >= nview+nclick)
		return 0;
	else if (n < nview) {
		sprintf(buf, "view %s", viewname[n]);
		return buf;
	} else {
		sprintf(buf, "click %s%s",
			clickname[n-nview], clickval[n-nview] ? "*" : "");
		return buf;
	}
}


#define button3(b)	((b) & 4)
#define button2(b)	((b) & 2)
#define button1(b)	((b) & 1)
#define button23(b)	((b) & 6)
#define button123(b)	((b) & 7)

int buttondown(void)	/* report state of buttons, if any */
{
	if (!ecanmouse())	/* no event pending */
		return 0;
	mouse = emouse();	/* something, but it could be motion */
	return mouse.buttons & 7;
}

int waitdown(void)	/* wait until some button is down */
{
	while (!(mouse.buttons & 7))
		mouse = emouse();
	return mouse.buttons & 7;
}

int waitup(void)
{
	while (mouse.buttons & 7)
		mouse = emouse();
	return mouse.buttons & 7;
}

checkmouse(void)	/* return button touched if any */
{
	int c, b;
	char *p = NULL;
	extern int confirm(int);

	b = waitdown();
	last_but = 0;
	last_hit = -1;
	c = 0;
	if (button3(b)) {
		last_hit = menuhit(3, &mouse, &mbut3);
		last_but = 3;
	} else if (button2(b)) {
		last_hit = menuhit(2, &mouse, &mbut2);
		last_but = 2;
	} else {		/* button1() */
		last_but = 1;
	}
	waitup();
	if (last_but == 3 && last_hit >= 0) {
		p = m3[last_hit];
		c = p[strlen(p) - 1];
	}
	if (c == '?' && !confirm(last_but))
		last_hit = -1;
	return last_but;
}

confirm(int but)	/* ask for confirmation if menu item ends with '?' */
{
	int c;
	static int but_cvt[8] = { 0, 1, 2, 0, 3, 0, 0, 0 };

	cursorswitch(&skull);
	c = waitdown();
	waitup();
	cursorswitch(0);
	return but == but_cvt[c];
}

Cursor deadmouse = {
	{ 0, 0},	/* offset */
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	  0x00, 0x00, 0x00, 0x0C, 0x00, 0x82, 0x04, 0x41,
	  0xFF, 0xE1, 0x5F, 0xF1, 0x3F, 0xFE, 0x17, 0xF0,
	  0x03, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	  0x00, 0x00, 0x00, 0x0C, 0x00, 0x82, 0x04, 0x41,
	  0xFF, 0xE1, 0x5F, 0xF1, 0x3F, 0xFE, 0x17, 0xF0,
	  0x03, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, }
};

Cursor skull ={
	{ 0, 0 },
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x03,
	  0xE7, 0xE7, 0x3F, 0xFC, 0x0F, 0xF0, 0x0D, 0xB0,
	  0x07, 0xE0, 0x06, 0x60, 0x37, 0xEC, 0xE4, 0x27,
	  0xC3, 0xC3, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x03,
	  0xE7, 0xE7, 0x3F, 0xFC, 0x0F, 0xF0, 0x0D, 0xB0,
	  0x07, 0xE0, 0x06, 0x60, 0x37, 0xEC, 0xE4, 0x27,
	  0xC3, 0xC3, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, }
};


do_kbd(void)	/* read a line from keyboard */
{
	char *p;

	for (p = kbdline; (*p = ekbd()) != '\n' && *p != '\r'; p++) {
		/* fprintf(stderr, "%c", *p); */
		if (*p == '\b')
			p -= 2;
		if (p < kbdline)
			p = kbdline;
		p[1] = 0;
 		putstring(kbdline);
		
	}
	*p = 0;
	/* fprintf(stderr, "\n"); */
	/* fprintf(stderr, "\nfilename is [%s]\n", kbdline); */
	return strlen(kbdline);
}

void putstring(char *buf)
{
        Point p;
        static int jmax = 0, l;

	p = add(screen.r.min, Pt(20,20));
	bitblt(&screen, p, &screen, Rect(p.x, p.y, p.x+jmax, p.y+font->height), F_CLR);
        string(&screen, p, font, buf, F_OR);
        if ((l = strwidth(font, buf)) > jmax)
                jmax = l;
}
