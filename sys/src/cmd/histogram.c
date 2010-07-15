#include <u.h>
#include <libc.h>
#include <draw.h>
#include <bio.h>
#include <thread.h>
#include <mouse.h>
#include <keyboard.h>

enum {
	STACK 	= 8*1024,

	Dot	= 2,	/* height of dot */
	Lx	= 4,	/* x offset */
	Ly	= 4,	/* y offset */
	Bw	= 2,	/* border width */
};

Image *neutral;
Image *light;
Image *dark;
Image *txtcolor;

char *title = "histogram";
Rectangle hrect;
Point maxvloc;
double *data;
double vmax = 100, scale = 1.0;
uint nval;
int dontdie = 0, col = 1;

int colors[][3] = {
	{ 0xFFAAAAFF,	0xFFAAAAFF,	0xBB5D5DFF },		/* Peach */
	{ DPalebluegreen, DPalegreygreen, DPurpleblue },	/* Aqua */
	{ DPaleyellow,	DDarkyellow,	DYellowgreen },		/* Yellow */
	{ DPalegreen,	DMedgreen,	DDarkgreen },		/* Green */
	{ 0x00AAFFFF,	0x00AAFFFF,	0x0088CCFF },		/* Blue */
	{ 0xEEEEEEFF,	0xCCCCCCFF,	0x888888F },		/* Grey */
};

void
initcolor(int i)
{
	neutral = allocimagemix(display, colors[i][0], DWhite);
	light = allocimage(display, Rect(0,0,1,1), CMAP8, 1, colors[i][1]);
	dark  = allocimage(display, Rect(0,0,1,1), CMAP8, 1, colors[i][2]);
	txtcolor = display->black;
}

void*
erealloc(void *v, ulong sz)
{
	v = realloc(v, sz);
	if(v == nil){
		sysfatal("realloc: %r");
		threadexitsall("memory");
	}
	return v;
}

Point
datapoint(int x, double v)
{
	Point p;
	double y;

	p.x = x;
	y = (v*scale) / vmax;
	p.y = hrect.max.y - Dy(hrect)*y - Dot;
	if(p.y < hrect.min.y)
		p.y = hrect.min.y;
	if(p.y > hrect.max.y - Dot)
		p.y = hrect.max.y - Dot;
	return p;
}

void
drawdatum(int x, double prev, double v)
{
	Point p, q;

	p = datapoint(x, v);
	q = datapoint(x, prev);
	if(p.y < q.y){
		draw(screen, Rect(p.x, hrect.min.y, p.x+1, p.y), neutral,
			nil, ZP);
		draw(screen, Rect(p.x, p.y, p.x+1, q.y+Dot), dark, nil, ZP);
		draw(screen, Rect(p.x, q.y+Dot, p.x+1, hrect.max.y), light,
			nil, ZP);
	}else{
		draw(screen, Rect(p.x, hrect.min.y, p.x+1, q.y), neutral,
			nil, ZP);
		draw(screen, Rect(p.x, q.y, p.x+1, p.y+Dot), dark, nil, ZP);
		draw(screen, Rect(p.x, p.y+Dot, p.x+1, hrect.max.y), light,
			nil, ZP);
	}

}

void
updatehistogram(double v)
{
	char buf[32];

	draw(screen, hrect, screen, nil, Pt(hrect.min.x+1, hrect.min.y));
	if(v * scale > vmax)
		v = vmax / scale;
	drawdatum(hrect.max.x-1, data[0], v);
	memmove(&data[1], &data[0], (nval-1) * sizeof data[0]);
	data[0] = v;
	snprint(buf, sizeof buf, "%0.9f", v);
	stringbg(screen, maxvloc, txtcolor, ZP, display->defaultfont, buf,
		neutral, ZP);
	flushimage(display, 1);
}

void
redrawhistogram(int new)
{
	Point p, q;
	Rectangle r;
	uint onval = nval;
	int i;
	char buf[32];

	if(new && getwindow(display, Refnone) < 0)
		sysfatal("getwindow: %r");

	r = screen->r;
	draw(screen, r, neutral, nil, ZP);
	p = string(screen, addpt(r.min, Pt(Lx, Ly)), txtcolor, ZP,
		display->defaultfont, title);

	p.x = r.min.x + Lx;
	p.y += display->defaultfont->height + Ly;

	q = subpt(r.max, Pt(Lx, Ly));
	hrect = Rpt(p, q);

	maxvloc = Pt(r.max.x - Lx - stringwidth(display->defaultfont,
		"999999999"), r.min.y + Ly);

	nval = abs(Dx(hrect));
	if(nval != onval){
		data = erealloc(data, nval * sizeof data[0]);
		if(nval > onval)
			memset(data+onval, 0, (nval - onval) * sizeof data[0]);
	}

	border(screen, hrect, -Bw, dark, ZP);
	snprint(buf, sizeof buf, "%0.9f", data[0]);
	stringbg(screen, maxvloc, txtcolor, ZP, display->defaultfont, buf,
		neutral, ZP);
	draw(screen, hrect, neutral, nil, ZP);
	for(i = 1; i < nval - 1; i++)
		drawdatum(hrect.max.x - i, data[i-1], data[i]);
	drawdatum(hrect.min.x, data[i], data[i]);
	flushimage(display, 1);
}

void
reader(void *arg)
{
	int fd;
	double v;
	char *p, *f[2];
	uchar buf[512];
	Biobufhdr b;
	Channel *c = arg;

	threadsetname("reader");
	fd = dup(0, -1);
	Binits(&b, fd, OREAD, buf, sizeof buf);

	while((p = Brdline(&b, '\n')) != nil) {
		p[Blinelen(&b) - 1] = '\0';
		if(tokenize(p, f, 1) != 1)
			continue;
		v = strtod(f[0], 0);
		send(c, &v);
	}
	if(!dontdie)
		threadexitsall(nil);
}


void
histogram(char *rect)
{
	int rm;
	double dm;
	Channel *dc;
	Keyboardctl *kc;
	Mouse mm;
	Mousectl *mc;
	Rune km;
	Alt a[] = {
		/* c	v	op */
		{nil,	&dm,	CHANRCV},	/* data from stdin */
		{nil,	&mm,	CHANRCV},	/* mouse message */
		{nil,	&km,	CHANRCV},	/* keyboard runes */
		{nil,	&rm,	CHANRCV},	/* resize event */
		{nil,	nil,	CHANEND},
	};
	static char *mitems[] = {
		"exit",
		nil
	};
	static Menu menu = {
		mitems,
		nil,
		-1
	};

	memset(&mm, 0, sizeof mm);
	memset(&km, 0, sizeof km);
	dm = rm = 0;

	if(newwindow(rect) < 0)
		sysfatal("newwindow: %r");
	if(initdraw(nil, nil, "histogram") < 0)
		sysfatal("initdraw: %r");

	initcolor(col);

	mc = initmouse(nil, screen);
	if(!mc)
		sysfatal("initmouse: %r");
	kc = initkeyboard(nil);
	if(!kc)
		sysfatal("initkeyboard: %r");

	dc = chancreate(sizeof dm, 10);
	if(!dc)
		sysfatal("chancreate: %r");

	a[0].c = dc;
	a[1].c = mc->c;
	a[2].c = kc->c;
	a[3].c = mc->resizec;

	proccreate(reader, a[0].c, STACK + sizeof(Biobuf));

	redrawhistogram(0);
	for(;;)
		switch(alt(a)){
		case 0:
			updatehistogram(dm);
			break;
		case 1:
			if(mm.buttons & 4 && menuhit(3, mc, &menu, nil) == 0)
				goto done;
			break;
		case 2:
			if(km == 0x7F)
				goto done;
			break;
		case 3:
			redrawhistogram(1);
			break;
		default:
			sysfatal("shouldn't happen");
		}
done:
	closekeyboard(kc);
	closemouse(mc);
	chanfree(a[0].c);
	threadexitsall(nil);
}

void
usage(void)
{
	fprint(2, "usage: histogram [-h] [-c index] [-r minx,miny,maxx,maxy] "
		"[-s scale] [-t title] [-v maxv]\n");
	exits("usage");
}

void
threadmain(int argc, char **argv)
{
	char *p, *q;

	p = "-r 0,0,400,150";

	ARGBEGIN{
	case 'v':
		vmax = strtod(EARGF(usage()), 0);
		break;
	case 'r':
		p = smprint("-r %s", EARGF(usage()));
		break;
	case 's':
		scale = strtod(EARGF(usage()), 0);
		if(scale <= 0)
			usage();
		break;
	case 'h':
		dontdie = 1;
		break;
	case 't':
		title = EARGF(usage());
		break;
	case 'c':
		col = atoi(EARGF(usage())) % nelem(colors);
		break;
	default:
		usage();
	}ARGEND;

	while((q = strchr(p, ',')) != nil)
		*q = ' ';

	histogram(p);
}
