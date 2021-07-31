#include	<u.h>
#include	<libc.h>
#include	<libg.h>
#include	<bio.h>
#include	"proof.h"

int	res;
int	hpos;
int	vpos;
int	DIV = 11;

Point offset;
Point xyoffset = { 0,0 };

Rectangle	view[MAXVIEW];
Rectangle	bound[MAXVIEW];		/* extreme points */
int	nview = 1;

int	lastp;	/* last page number we were on */

#define	NPAGENUMS	200
struct pagenum {
	int	num;
	long	adr;
} pagenums[NPAGENUMS];
int	npagenums;

int	curfont, cursize;

char	*getcmdstr(void);

static void	initpage(void);
static void	view_setup(int);
static Point	scale(Point);
static void	clearview(Rectangle);
static int	addpage(int);
static void	spline(Bitmap *, int, Point *, int);
static int	skipto(int, int);
static void	wiggly(int);
static void	devcntrl(void);
static void	eatline(void);
static int	getn(void);
static int	botpage(int);
static void	getstr(char *);
static void	getutf(char *);

#define Do screen.r.min
#define Dc screen.r.max

/* declarations and definitions of font stuff are in font.c and main.c */

static void
initpage(void)
{
	int i;

	view_setup(nview);
	for (i = 0; i < nview-1; i++)
		bitblt(&screen, view[i].min, &screen, view[i+1], S);
	clearview(view[nview-1]);
	offset = view[nview-1].min;
	vpos = 0;
}

static void
view_setup(int n)
{
	int i, j, v, dx, dy, r, c;

	switch (n) {
	case 1: r = 1; c = 1; break;
	case 2: r = 1; c = 2; break;
	case 3: r = 1; c = 3; break;
	case 4: r = 2; c = 2; break;
	case 5: case 6: r = 2; c = 3; break;
	case 7: case 8: case 9: r = 3; c = 3; break;
	default: r = (n+2)/3; c = 3; break; /* finking out */
	}
	dx = (Dc.x - Do.x) / c;
	dy = (Dc.y - Do.y) / r;
	v = 0;
	for (i = 0; i < r && v < n; i++)
		for (j = 0; j < c && v < n; j++) {
			view[v] = screen.r;
			view[v].min.x = Do.x + j * dx;
			view[v].max.x = Do.x + (j+1) * dx;
			view[v].min.y = Do.y + i * dy;
			view[v].max.y = Do.y + (i+1) * dy;
			v++;
		}
}

static void
clearview(Rectangle r)
{
	bitblt(&screen, r.min, &screen, r, 0);
}

void ereshaped(Rectangle r)
{
	/* this is called if we are reshaped */
	screen.r = r;
	initpage();
}

static Point
scale(Point p)
{
	p.x /= DIV;
	p.y /= DIV;
	return add(xyoffset, add(offset,p));
}

static int
addpage(int n)
{
	int i;

	for (i = 0; i < npagenums; i++)
		if (n == pagenums[i].num)
			return i;
	if (npagenums < NPAGENUMS-1) {
		pagenums[npagenums].num = n;
		pagenums[npagenums].adr = Boffset(&bin);
		npagenums++;
	}
	return npagenums;
}

void
readpage(void)
{
	int c, i;
	static int first = 0;
	int m, n, gonow = 1;
	char buf[300];
	Rune r[32], t;
	Point p,q,qq;

	offset = screen.clipr.min;
	cursorswitch(&deadmouse);
	while (gonow)
	{
		c = BGETC(&bin);
		switch (c)
		{
		case -1:
			cursorswitch(0);
			if (botpage(lastp+1)) {
				initpage();
				break;
			}
			exits(0);
		case 'p':	/* new page */
			lastp = getn();
			addpage(lastp);
			if (first++ > 0) {
				cursorswitch(0);
				botpage(lastp);
				cursorswitch(&deadmouse);
			}
			initpage();
			break;
		case '\n':	/* when input is text */
		case ' ':
		case 0:		/* occasional noise creeps in */
			break;
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			/* two motion digits plus a character */
			hpos += (c-'0')*10 + BGETC(&bin)-'0';

		/* FALLS THROUGH */
		case 'c':	/* single ascii character */
			r[0] = Bgetrune(&bin);
			r[1] = 0;
			dochar(r);
			break;

		case 'C':
			for(i=0; ; i++){
				t = Bgetrune(&bin);
				if(isspace(t))
					break;
				r[i] = t;
			}
			r[i] = 0;
			dochar(r);
			break;

		case 'N':
			r[0] = getn();
			r[1] = 0;
			dochar(r);
			break;

		case 'D':	/* draw function */
			switch (BGETC(&bin))
			{
			case 'l':	/* draw a line */
				n = getn();
				m = getn();
				p = Pt(hpos,vpos);
				q = add(p, Pt(n,m));
				hpos += n;
				vpos += m;
				segment(&screen, scale(p), scale(q), ONES, Mode);
				break;
			case 'c':	/* circle */
				/*nop*/
				m = getn()/2;
				p = Pt(hpos+m,vpos);
				hpos += 2*m;
				circle(&screen, scale(p), m/DIV, ONES, Mode);
				/* p=currentpt; p.x+=dmap(m/2);circle bp,p,a,ONES,Mode*/
				break;
			case 'e':	/* ellipse */
				/*nop*/
				m = getn()/2;
				n = getn()/2;
				p = Pt(hpos+m,vpos);
				hpos += 2*m;
				ellipse(&screen, scale(p), m/DIV, n/DIV, ONES, Mode);
				break;
			case 'a':	/* arc */
				p = scale(Pt(hpos,vpos));
				n = getn();
				m = getn();
				hpos += n;
				vpos += m;
				q = scale(Pt(hpos,vpos));
				n = getn();
				m = getn();
				hpos += n;
				vpos += m;
				qq = scale(Pt(hpos,vpos));
				arc(&screen,q,qq,p,ONES,Mode);
				break;
			case '~':	/* wiggly line */
				wiggly(0);
				break;
			default:
				break;
			}
			eatline();
			break;
		case 's':
			n = getn();	/* ignore fractional sizes */
			if (cursize == n)
				break;
			cursize = n;
			if (cursize >= NFONT)
				cursize = NFONT-1;
			break;
		case 'f':
			curfont = getn();
			break;
		case 'H':	/* absolute horizontal motion */
			hpos = getn();
			break;
		case 'h':	/* relative horizontal motion */
			hpos += getn();
			break;
		case 'w':	/* word space */
			break;
		case 'V':
			vpos = getn();
			break;
		case 'v':
			vpos += getn();
			break;
		case '#':	/* comment */
		case 'n':	/* end of line */
			eatline();
			break;
		case 'x':	/* device control */
			devcntrl();
			break;
		default:
			sprint(buf, "unknown input character %o %c\n", c, c);
			exits("bad char");
		}
	}
	cursorswitch(0);
}

static void
spline(Bitmap *b, int n, Point *pp, int f)
{
	long w, t1, t2, t3, fac=1000; 
	int i, j, steps=10; 
	Point p, q;

	for (i = n; i > 0; i--)
		pp[i] = pp[i-1];
	pp[n+1] = pp[n];
	n += 2;
	p = pp[0];
	for(i = 0; i < n-2; i++)
	{
		for(j = 0; j < steps; j++)
		{
			w = fac * j / steps;
			t1 = w * w / (2 * fac);
			w = w - fac/2;
			t2 = 3*fac/4 - w * w / fac;
			w = w - fac/2;
			t3 = w * w / (2*fac);
			q.x = (t1*pp[i+2].x + t2*pp[i+1].x + 
				t3*pp[i].x + fac/2) / fac;
			q.y = (t1*pp[i+2].y + t2*pp[i+1].y + 
				t3*pp[i].y + fac/2) / fac;
			segment(b, p, q, ONES, f);
			p = q;
		}
	}
}

/* Have to parse skipped pages, to find out what fonts are loaded. */
static int
skipto(int gotop, int curp)
{
	char *p;
	int i;

	if (gotop == curp)
		return 1;
	for (i = 0; i < npagenums; i++)
		if (pagenums[i].num == gotop) {
			if (Bseek(&bin, pagenums[i].adr, 0) == Beof) {
				fprint(2, "can't rewind input\n");
				return 0;
			}
			return 1;
		}
	if (gotop <= curp) {
	    restart:
		if (Bseek(&bin,0,0) == Beof) {
			fprint(2, "can't rewind input\n");
			return 0;
		}
	}
	for(;;){
		p = Brdline(&bin, '\n');
		if (p == 0) {
			if(gotop>curp){
				gotop = curp;
				goto restart;
			}
			return 0;
		} else if (*p == 'p') {
			lastp = curp = atoi(p+1);
			addpage(lastp);	/* maybe 1 too high */
			if (curp>=gotop)
				return 1;
		}
	}
}

static void
wiggly(int skip)
{
	Point p[300];
	int c,i,n;
	for (n = 1; (c = BGETC(&bin)) != '\n' && c>=0; n++) {
		Bungetc(&bin);
		p[n].x = getn();
		p[n].y = getn();
	}
	p[0] = Pt(hpos, vpos);
	for (i = 1; i < n; i++)
		p[i] = add(p[i],p[i-1]);
	hpos = p[n-1].x;
	vpos = p[n-1].y;
	for (i = 0; i < n; i++)
		p[i] = scale(p[i]);
	if (!skip)
		spline(&screen,n,p,Mode);
}

static void
devcntrl(void)	/* interpret device control functions */
{
        char str[80];
	int n;

	getstr(str);
	switch (str[0]) {	/* crude for now */
	case 'i':	/* initialize */
		break;
	case 'T':	/* device name */
		getstr(devname);
		break;
	case 't':	/* trailer */
		break;
	case 'p':	/* pause -- can restart */
		break;
	case 's':	/* stop */
		break;
	case 'r':	/* resolution assumed when prepared */
		res=getn();
		DIV = floor(.5 + res/(100.0*mag));
		if (DIV < 1)
			DIV = 1;
		mag = res/(100.0*DIV); /* adjust mag according to DIV coarseness */
		break;
	case 'f':	/* font used */
		n = getn();
		getstr(str);
		loadfontname(n, str);
		break;
	/* these don't belong here... */
	case 'H':	/* char height */
		break;
	case 'S':	/* slant */
		break;
	case 'X':
		break;
	}
	eatline();
}

int
isspace(int c)
{
	return c==' ' || c=='\t' || c=='\n';
}

static void
getstr(char *is)
{
	uchar *s = (uchar *) is;

	for (*s = BGETC(&bin); isspace(*s); *s = BGETC(&bin))
		;
	for (; !isspace(*s); *++s = BGETC(&bin))
		;
	Bungetc(&bin);
	*s = 0;
}

static void
getutf(char *s)		/* get next utf char, as bytes */
{
	int c, i;

	for (i=0;;) {
		c = BGETC(&bin);
		if (c < 0)
			return;
		s[i++] = c;

		if (fullrune(s, i)) {
			s[i] = 0;
			return;
		}
	}
}

static void
eatline(void)
{
	int c;

	while ((c=BGETC(&bin)) != '\n' && c >= 0)
		;
}

static int
getn(void)
{
	int n, c, sign;

	while (c = BGETC(&bin))
		if (!isspace(c))
			break;
	if(c == '-'){
		sign = -1;
		c = BGETC(&bin);
	}else
		sign = 1;
	for (n = 0; '0'<=c && c<='9'; c = BGETC(&bin))
		n = n*10 + c - '0';
	while (c == ' ')
		c = BGETC(&bin);
	Bungetc(&bin);
	return(n*sign);
}

static int
botpage(int np)	/* called at bottom of page np-1 == top of page np */
{
	char *p;
	int n;

	while (p = getcmdstr()) {
		if (*p == '\0')
			return 0;
		if (*p == 'q')
			exits(p);
		if (*p == 'c')		/* nop */
			continue;
		if (*p == 'm') {
			mag = atof(p+1);
			if (mag <= .1 || mag >= 10)
				mag = DEFMAG;
			allfree();	/* zap fonts */
			DIV = floor(.5 + res/(100.0*mag));
			if (DIV < 1)
				DIV = 1;
			mag = res/(100.0*DIV);
			return skipto(np-1, np);	/* reprint the page */
		}
		if (*p == 'x') {
			xyoffset.x += atoi(p+1)*100;
			skipto(np-1, np);
			return 1;
		}
		if (*p == 'y') {
			xyoffset.y += atoi(p+1)*100;
			skipto(np-1, np);
			return 1;
		}
		if (*p == '/') {	/* divide into n pieces */
			nview = atoi(p+1);
			if (nview < 1)
				nview = 1;
			else if (nview > MAXVIEW)
				nview = MAXVIEW;
			return skipto(np-1, np);
		}
		if (*p == 'p') {
			if (p[1] == '\0')	/* bare 'p' */
				return skipto(np-1, np);
			p++;
		}
		if ('0'<=*p && *p<='9') {
			n = atoi(p);
			return skipto(n, np);
		}
		if (*p == '-' || *p == '+') {
			n = atoi(p);
			if (n == 0)
				n = *p == '-' ? -1 : 1;
			return skipto(np - 1 + n, np);
		}
		if (*p == 'd') {
			dbg = 1 - dbg;
			continue;
		}

		fprint(2, "illegal;  try q, 17, +2, -1, p, m.7, /2, x1, y-.5 or return\n");
	}
	return 0;
}
