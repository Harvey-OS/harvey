#include <u.h>
#include <libc.h>
#include <libg.h>
#include <fb.h>
#include "scsicam.h"

#define	ISIZE		48	/* ikon size (square) */
#define MAXRANGE	(PM/*& ~0x01*/)	/* even number for round-off fun */
#define IKONSPACE	(ISIZE+4)	/* pixels per gallery entry */


Mouse mouse;
#define BUTTON(n)	((mouse.buttons >> (n-1)) & 1)
#define BUTTON123()	((int)(mouse.buttons & 0x7))

Bitmap display;
Font *font;

char	*name;
char	*machine;

ulong	pic[GOODDY][DX];	/* integrated input picture */
ulong	edited[ISIZE][ISIZE];	/* selected region from srect */

uchar frame[FRAMESIZE];	/* from the scsicam */


Rectangle Drect;	/* our whole display area */
Rectangle frect;	/* the big face */
Rectangle mrect;	/* the message area */
Rectangle grect;	/* the gallery */
Rectangle srect;	/* edited rectangle on the big face */
Rectangle machrect;	/* machine name */
Rectangle namerect;	/* user name */
Rectangle approved[2];	/* approved ikons */

int srectok=0;		/* if srect valid */

Bitmap *face=0;
Bitmap *camface=0;
Bitmap *ikon=0;		/* ikon bitmap*/
Bitmap *ikons[2]={0, 0};	/*selected ikons*/

/* gallery information */
int gallok = 0;
int xpick = -1;		/* x coordinate of current choice */
int xlow=0;		/* value of lowest x column */
int xrange=MAXRANGE;	/* range covered by each row */
int xcount;		/* number of columns in each row */
int xvalue;		/* value of choice at xpick */

int ypick;		/* y coordinate of current choice */
int ylow=0;		/* value of lowest y row */
int yrange=MAXRANGE;	/* range covered by each column */
int ycount;		/* number of rows in each column */
int yvalue;		/* value of choice at ypick */


int mugdepth;		
int ppbyte;		/* pixels per byte */
int maxp;		/* max pixel value */
int bpl;		/* bytes per line in the bitmap */


char *menu2text[]={
	"in",
	"out",
	0
};
enum Menu2{
	In,
	Out,
};

char *menu3text[]={
	"snap picture",
	"edit face",
	"select 2 bit ikon",
	"select 1 bit ikon",
	"save ikons",
	"quit",
	0
};
enum Menu3{
	Snap,
	Edit,
	Select2,
	Select1,
	Save,
	Quit,
};


Menu menu2={menu2text};
Menu menu3={menu3text};

void	cursorset(Point p);


void
msg(char *s)
{
	bitblt(&display, mrect.min, &display, mrect, Zero);
	string(&screen, mrect.min, font, s, S);
	bflush();
}

void
box(Rectangle r, int f)
{
	r.max=sub(r.max, Pt(1, 1));
	segment(&display, r.min, Pt(r.min.x, r.max.y), ~0, f);
	segment(&display, Pt(r.min.x, r.max.y), r.max, ~0, f);
	segment(&display, r.max, Pt(r.max.x, r.min.y), ~0, f);
	segment(&display, Pt(r.max.x, r.min.y) ,r.min, ~0, f);
}

void
drawborder(Rectangle r)
{
	box(r, DxorS);
}


ulong floyd[ISIZE+1][ISIZE+1];
ulong contrast_table[PM+1];

void
drawface(int lo, int hi, Point facep)
{
	ulong *c, *f;
	int *ep;
	int v, h, s, e, temp, i;
	uchar icon[ISIZE*ISIZE];

	if (hi<lo) {
		int t=hi; hi=lo; lo=t;
	}
	if(lo<0)
		lo=0;
	else if(lo > PM)
		lo=PM;
	if(hi<0)
		hi=0;
	else if(hi > PM)
		hi=PM;

	temp = hi - lo;
	c = contrast_table;	/* load the contrast table with current values */
	for (v=0; v!=lo; v++)
		c[v] = 0;
	for(; v!=hi; v++)
		c[v] = (v-lo)*PM/temp;
	for(; v<=PM; v++)
		c[v] = PM;

	for(v=0; v!=ISIZE; v++)
		for(h=0, f=floyd[v]; h!=ISIZE; h++)
			*f++ = contrast_table[edited[v][h] & PM];

	for(v=0; v!=ISIZE; v++){
		f = floyd[v];
		for(h=0; h<ISIZE/ppbyte; h++){
			icon[bpl*v + h] = 0;
			for(s=0; s < ppbyte; s++,f++){
				e = f[0];
				switch (mugdepth) {
				case 1:	if (e > PM/2) {
						i=0;
						e -= PM;
					} else
						i = 1;
					break;
				case 2:	i = ((1<<mugdepth)*e)/PM;
					if (i<0)
						i=0;
					else if (i>maxp)
						i=maxp;
					e -= (i*PM)/3;
					i = maxp-i;
					break;
				}
				icon[bpl*v + h] |= i << (8*sizeof(uchar) -
					mugdepth - mugdepth*s);
				temp = 3*e/8;
				f[ISIZE+1] += temp;
				f[ISIZE+2] += e-2*temp;
				f[1] += temp;
			}
		}
	}
	wrbitmap(ikon, 0, ISIZE, (uchar *)icon);
	bitblt(&display, facep, ikon, Rect(0,0, ISIZE,ISIZE), S);
}

Rectangle
galrect(int x, int y)
{
	Rectangle r;
	r.min = add(grect.min, Pt(x*IKONSPACE,y*IKONSPACE));
	r.max = add(r.min, Pt(ISIZE, ISIZE));
	return r;
}

#define VX(x)	(xlow + (xrange/xcount)*x)
#define VY(y)	(ylow + (yrange/ycount)*y)

void
show_data(void) {
	char buf[100];

	sprint(buf, "x %d %d %d  y %d %d %d", xlow, xrange, xcount,
		ylow, yrange, ycount);
}

void
showgallery(void)
{
	int x, y;
	int xv, yv;
	Rectangle r;

	bitblt(&display, grect.min, &display, grect, 0);
	for (x=0; x<xcount; x++) {
		for (y=0; y<ycount; y++) {
			xv = VX(x);
			yv = VY(y);
			r = galrect(x ,y);
			if (xv >= yv)
				drawface(xv, yv, r.min);
			else
				bitblt(&display, r.min, &display, r, 0);
		}
	}
	if (xpick >= 0)
		box(inset(galrect(xpick, ypick), -2), S);
	gallok = 1;
	show_data();
}

void
pickrect(void)
{
	Point p;
	int x, y, xv, yv;

	if (!ptinrect(mouse.xy, grect))
		return;
	p = sub(mouse.xy, grect.min);
	x = (p.x+2)/IKONSPACE;
	y = (p.y+2)/IKONSPACE;
	if (VX(x) < VY(y))	/* none displayed there */
		return;

	box(inset(galrect(xpick, ypick), -2), 0);	/* clear old box */
	xpick = x;
	ypick = y;
	box(inset(galrect(xpick, ypick), -2), ~0);
	show_data();
	bflush();
}

void
zoomset(int oldvx, int oldvy)
{
	int tx, xincr = xrange/xcount;
	int ty, yincr = yrange/ycount;

	xpick = xcount / 2;
	ypick = ycount / 2;
	xlow = oldvx - xpick*xincr;
	ylow = oldvy - ypick*yincr;
	if (xlow < 0) {
		tx = ((-xlow) / xincr) + 1;
		xpick -= tx;
		xlow += tx*xincr;
	}
	if (ylow < 0) {
		ty = ((-ylow) / yincr) + 1;
		ypick -= ty;
		ylow += ty*yincr;
	}
}

void
zoomin(void)
{
	int oldvx = VX(xpick);
	int oldvy = VY(ypick);

	box(inset(galrect(xpick, ypick), -2), notS);	/* clear old box */
	xrange /= 2;
	if (xrange < xcount)
		xrange = xcount;
	yrange /= 2;
	if (yrange < ycount)
		yrange = ycount;
	zoomset(oldvx, oldvy);
}

void
zoomout(void)
{
	int oldvx = VX(xpick);
	int oldvy = VY(ypick);

	box(inset(galrect(xpick, ypick), -2), notS);	/* clear old box */
	xrange *= 2;
	if (xrange > MAXRANGE)
		xrange = MAXRANGE;
	yrange *= 2;
	if (yrange > MAXRANGE)
		yrange = MAXRANGE;
	zoomset(oldvx, oldvy);
}

int
reshape(Rectangle newrect)
{
	Point l1, l0;
	char buf[100];

	Rectangle label1, label0;
	Drect = inset(newrect, 3);
	bitblt(&display, Drect.min, &display, Drect, Zero);

	/*
	 * message area
	 */
	mrect.min = Pt(Drect.min.x+20,  Drect.max.y - 2 - font->height);
	mrect.max = Pt(Drect.max.x, Drect.max.y - 2);

	if (Drect.min.y + 2 + GOODDY + 3 > mrect.min.y ||
	    Drect.min.x + 2 + DX + 3 > Drect.max.x)
		return 0;	/* no room for the big picture */

	frect.min = Pt(mrect.min.x+20, mrect.min.y-3-GOODDY);
	frect.max = add(frect.min, Pt(DX, GOODDY));
	box(inset(frect, -2), ~0);

	machrect.max.x = namerect.max.x = frect.max.x;
	machrect.min.x = namerect.min.x = frect.min.x;
	machrect.max.y = frect.min.y-5;
	namerect.max.y = machrect.min.y = machrect.max.y-font->height;
	namerect.min.y = namerect.max.y-font->height;

	approved[0].max.y = approved[1].max.y = namerect.min.y - 4;
	approved[0].min.y = approved[1].min.y = approved[0].max.y - ISIZE;
	approved[1].min.x = frect.min.x;
	approved[1].max.x = approved[1].min.x + ISIZE;
	approved[0].min.x = approved[1].max.x + 20;
	approved[0].max.x = approved[0].min.x + ISIZE;
	l1 = sub(approved[1].min, Pt(0, font->height + 5));
	l0 = sub(approved[0].min, Pt(0, font->height + 5));

	if (l1.y - Drect.min.y > Drect.max.x - frect.max.x) {
		/* gallery on top */
		grect.min = add(Drect.min, Pt(2, 2));
		grect.max = Pt(Drect.max.x-2, l1.y-4);
	} else {
		/* gallery on the right */
		grect.min = Pt(frect.max.x+2, Drect.min.y+2);
		grect.max = Pt(Drect.max.x-2, mrect.min.y-3);
	}

#define IKONCOUNT(r,axis)	((r.max.axis - r.min.axis)/IKONSPACE)
#define MINIKON	3
	if ((xcount=IKONCOUNT(grect, x)) < MINIKON ||
	    (ycount=IKONCOUNT(grect, y)) < MINIKON)
		return 0;	/* no room for useful gallery */
	if (xcount > PM)
		xcount = PM;
	if (ycount > PM)
		ycount = PM;
	grect.min = Pt(grect.max.x - IKONSPACE*xcount,
		       grect.max.y - IKONSPACE*ycount);
	xpick = ypick = -1;

	bitblt(&display, frect.min, face, Rect(0, 0, DX, GOODDY), S);

	drawborder(inset(approved[1], -2));
	drawborder(inset(approved[0], -2));
	string(&screen, sub(approved[1].min, Pt(0, font->height + 5)),
		font, "2 bit", S);
	string(&screen, sub(approved[0].min, Pt(0, font->height + 5)),
		font, "1 bit", S);
	bitblt(&screen, approved[0].min, ikons[0], Rect(0,0,ISIZE,ISIZE), S);
	bitblt(&screen, approved[1].min, ikons[1], Rect(0,0,ISIZE,ISIZE), S);
	sprint(buf, "machine: %s", machine);
	string(&screen, machrect.min, font, buf, S);
	sprint(buf, "   name: %s", name);
	string(&screen, namerect.min, font, buf, S);

	if (srectok) {
		drawborder(raddp(srect, frect.min));
		showgallery();
	}
	bflush();
	return 1;
}

void
ereshaped(Rectangle newrect)
{
	while (!reshape(newrect)) {
		msg("Window too small");
		mouse = emouse();
	}
}

void
xhair(Point p)
{
	if (p.y < frect.max.y && p.y >= frect.min.y)
		segment(&display, Pt(frect.min.x, p.y), Pt(frect.max.x, p.y),
			~0, S^D);
	if (p.x < frect.max.x && p.x >= frect.min.x)
		segment(&display, Pt(p.x, frect.min.y), Pt(p.x, frect.max.y),
			~0, S^D);
}

int
editface(Rectangle *rp)
{
	Point p, q, r, s;

	p=mouse.xy;
	xhair(p);
	mouse = emouse();
	while(!BUTTON123()){
		q=mouse.xy;
		if(!eqpt(p, q)){
			xhair(p);
			p=q;
			xhair(p);
		}
		mouse = emouse();
	}
	xhair(p);
	if(!ptinrect(q, frect)){
		while(BUTTON123())
			mouse = emouse();
		srectok = 0;
		return 0;
	}
	q=p;
	box(Rpt(Pt(2*p.x-q.x, p.y), q), S^D);
	for(; BUTTON123(); mouse = emouse()){
		r=mouse.xy;
		if (r.x<frect.min.x)
			r.x=frect.min.x;
		else if (r.x>frect.max.x)
			r.x=frect.max.x;
		if (r.y<frect.min.y)
			r.y=frect.min.y;
		else if (r.y>frect.max.y)
			r.y=frect.max.y;
		if (2*abs(r.x-p.x)<abs(r.y-p.y)){
			if (r.x>p.x)
				r.x=p.x+abs(r.y-p.y)/2;
			else
				r.x=p.x-abs(r.y-p.y)/2;
		} else if(r.y>p.y)
			r.y=p.y+2*abs(r.x-p.x);
		else r.y=p.y-2*abs(r.x-p.x);
		if(!eqpt(r, q)){
			box(Rpt(Pt(2*p.x-q.x, p.y), q), S^D);
			q=r;
			box(Rpt(Pt(2*p.x-q.x, p.y), q), S^D);
		}
	}
	box(Rpt(Pt(2*p.x-q.x, p.y), q), S^D);
	rp->min=Pt(2*p.x-q.x, p.y);
	rp->max=q;
	*rp = rsubp(rcanon(*rp), frect.min);
	srectok = 1;
	return 1;
}


int
sample(int x0, int y0, int x1, int y1)
{
	int v, u;

	if(x1==x0)
		x1++;
	if(y1==y0)
		y1++;
	if(x0==0){
		if(y0 == 0)
			v = pic[y1-1][x1-1];
		else
			v = pic[y1-1][x1-1] - pic[y0-1][x1-1];
	}
	else if (y0 == 0)
		v = pic[y1-1][x1-1] - pic[y1-1][x0-1];
	else
		v = pic[y1-1][x1-1] - pic[y0-1][x1-1] - 
		    pic[y1-1][x0-1] + pic[y0-1][x0-1];
	u = v/(y1-y0)/(x1-x0);
	return u;
}

void
getedited(Rectangle r)
{
	int x, y, x0, y0, x1, y1;
	x0 = r.min.x;
	y0 = r.min.y;
	x1 = r.max.x;
	y1 = r.max.y;
	if(x0<0)
		x0=0;
	else if(DX<=x0)
		x0=DX-1;
	if(y0<0)
		y0=0;
	else if(DY<=y0)
		y0=DY-1;
	if(x1<=x0)
		x1=x0+1;
	else if(DX<x1)
		x1=DX;
	if(y1<=y0)
		y1=y0+1;
	else if(DY<y1)
		y1=DY;
	x1-=x0;
	y1-=y0;

	for(y=0;y!=ISIZE;y++) {
		for(x=0;x!=ISIZE;x++) {
			edited[y][x] = sample(x0+x1*x/ISIZE, y0+y1*y/ISIZE,
				x0+((x+1)*x1-1)/ISIZE, y0+((y+1)*y1-1)/ISIZE);
		}
	}
}

void
bitmapinit(void)
{
	binit(0, 0, "ikon");
	face = balloc(Rect(0, 0, DX, GOODDY), screen.ldepth);
	camface = balloc(Rect(0, 0, DX, GOODDY), SCAMDEPTH);
	ikons[0] = balloc(Rect(0, 0, ISIZE, ISIZE), 0);
	ikons[1] = balloc(Rect(0, 0, ISIZE, ISIZE), 1);
	if (face == 0 || camface == 0 || ikons[0] == 0 || ikons[1] == 0) {
		fprint(2, "Could not alloc bitmaps\n");
		exits("alloc");
	}
	bitblt(ikons[0], Pt(0,0), ikons[0], Rect(0, 0, ISIZE, ISIZE), 0);
	bitblt(ikons[1], Pt(0,0), ikons[0], Rect(0, 0, ISIZE, ISIZE), 0);
}

void
setuppic(void) {
	int x, y;
	int shift = 8-BPP;
	uchar *cp = frame;

	/*
	 * unpack our frame buffer.
	 */
	for (y=0; y<GOODDY; y++)
		for(x=0; x<DX; x++) {
			pic[y][x] = ((*cp) >> shift) & PM;
			shift -= BPP;
			if (shift < 0) {
				cp++;
				shift = 8-BPP;
			}
		}

	/*
	 * set each row to the sum of the previous rows.
	 */
	for(x=1; x<DX; x++)
		pic[0][x] += pic[0][x-1];
	for(y=1; y<GOODDY; y++){
		int sum=0;
		for(x=0; x<DX; x++){
			sum += pic[y][x];
			pic[y][x] = pic[y-1][x] + sum;
		}
	}
	return;
}

/*
 * Reduce the camera frame to the current display depth,
 * for display purposes only.
 */
void
compute_display_face(void)
{
	int x, y;
	int e, i, t;
	int floyd[2][DX+1];
	uchar picbitmap[GOODDY*DX];	/* stub - check ldepth>3 */
	uchar *cp = picbitmap;
	ulong *w;
	int screen_bpp = (1<<screen.ldepth);
	int screen_pm = (1<<screen_bpp)-1;
	int shift = 8-screen_bpp;
	int reduction = (1<<BPP)/(1<<screen_bpp);
	*cp = 0;

	for(x=0; x<DX; x++)
		floyd[0][x] = 0;
	for(y=0; y<GOODDY; y++){
		for(x=0; x<DX; x++)
			floyd[(y+1)&1][x] = 0;
		for(x=0; x<DX; x++){
			e = floyd[y&1][x] + sample(x,y,x,y);
			i = e/reduction;
			if (i<0)
				i=0;
			else if (i>3)
				i=3;
			e -= (i*(1<<BPP))/3;

			*cp |= (3-i)<<shift;
			shift -= screen_bpp;
			if (shift < 0) {
				*(++cp) = 0;
				shift = 8-screen_bpp;
			}
			t = 3*e/8;
			floyd[(y+1)&1][x] += t;
			floyd[(y+1)&1][x+1] += e-2*t;
			floyd[y&1][x+1] += t;
		}
	}
	wrbitmap(face, 0, GOODDY, (uchar *)picbitmap);
	bitblt(&display, frect.min, face, Rect(0, 0, DX, GOODDY), S);
	bflush();
	return;
}

void
do_write(void)
{
	
	int x, y;

	switch (mugdepth) {
	case 1:	{	ushort icon[ISIZE][ISIZE/(8*sizeof(ushort))];

			bitblt(ikon, Pt(0,0), &screen, galrect(xpick, ypick), S);
			rdbitmap(ikon, 0, ISIZE, (uchar *)icon);
			for (y=0; y<ISIZE; y++) {
				for (x=0; x<ISIZE/(ppbyte*sizeof(ushort)); x++)
					print("0x%-4.4ux, ", icon[y][x]);
				print("\n");
			}
			break;
		}
	case 2:	{	ulong icon[ISIZE][ISIZE/(4*sizeof(ulong))];

			bitblt(ikon, Pt(0,0), &screen, galrect(xpick, ypick), S);
			rdbitmap(ikon, 0, ISIZE, (uchar *)icon);
			for (y=0; y<ISIZE; y++) {
				for (x=0; x<ISIZE/(ppbyte*sizeof(ulong)); x++)
					print("0x%-8.8ulx, ", icon[y][x]);
				print("\n");
			}
			break;
		}
	}
}

void
ikonusage(void)
{
	fprint(2, "usage: ikon machine user\n");
	exits("usage");
}

void
setdepth(int newdepth)
{
	if (ikon)
		bfree(ikon);
	mugdepth = newdepth;
	ikon = balloc(Rect(0, 0, ISIZE, ISIZE), mugdepth-1);
	if (ikon == (Bitmap *)0) {
		perror("Could not alloc `ikon'");
		exits("alloc ikon");
	}
	ppbyte = 8/mugdepth;
	maxp = (1<<mugdepth) - 1;
	bpl = ISIZE/ppbyte;
}

void
init(void) {
	bitmapinit();
	einit(Emouse);
	setdepth(2);
	srectok = 0;
	ereshaped(bscreenrect(0));
}

void
snap(void) {
	int cfd = open("#v/scam", OREAD);
	int n;

	if (cfd < 0) {
		perror("open scam device");
		exits("open scam");
	}
	msg("Click button 1 to take a picture");
	do {
		if ((n=read(cfd, frame, FRAMESIZE)) == FRAMESIZE) {
			wrbitmap(camface, 0, GOODDY, (uchar *) frame);
			bitblt(&screen, frect.min, camface, Rect(0,0,DX,GOODDY),
				notS);
			bflush();
			if (ecanmouse()) {
				mouse = emouse();
				if (BUTTON123())
					break;
			}
		}
	} while(1);
	close(cfd);
	gallok = 1;
	xpick = -1;
}

void
sweepface(void) {
	msg("Sweep face with button 1, starting at the top center");
	srectok = 0;
	while (!editface(&srect))
		;
	drawborder(raddp(srect, frect.min));
	getedited(srect);
}

void
saveikon(void) {
	bitblt(ikons[mugdepth-1], Pt(0,0), &screen, galrect(xpick, ypick), S);
	bitblt(&screen, approved[mugdepth-1].min, &screen,
		galrect(xpick, ypick), S);
}

void
setup_ikon_pick(int depth) {
	setdepth(depth);
	xrange = yrange = MAXRANGE;
	xlow = ylow = 0;
	xpick = -1;
	showgallery();
/* stub	msg("pick ikon with button 2, more detail with button 1");*/
}

void
idlemouse(void) {
	while(BUTTON123())
		mouse = emouse();
}

int
main(int argc, char *argv[]) {
	ARGBEGIN{
	case L'ü':	fprint(2, "rob\n");	break;
	default:
		ikonusage();
	}ARGEND;

	if (argc != 2)
		ikonusage();

	machine = *argv++;
	name = *argv;

	init();
	goto start;
	while (1) {
		if (!BUTTON123())
			mouse = emouse();
		if (BUTTON(1)) {
			if (gallok) {
				pickrect();
				zoomin();
				showgallery();
			}
			idlemouse();
			continue;
		}
		if (BUTTON(2)) {
			if (gallok) {
				pickrect();
				saveikon();
			}
			idlemouse();
			continue;
		}
		if (!BUTTON(3))
			continue;
		switch(menuhit(3, &mouse, &menu3)){
		case Snap:
start:			srectok = 0;
			snap();
			if (!BUTTON(1))
				continue;
			msg("processing...");
			setuppic();
			compute_display_face();
			idlemouse();
		case Edit:
			if (srectok) /* erase current border */
				drawborder(raddp(srect, frect.min));
			sweepface();
			if (BUTTON(2) | BUTTON(3))
				continue;
			idlemouse();
			break;
		case Select2:
			setup_ikon_pick(2);
			idlemouse();
			break;
		case Select1:
			setup_ikon_pick(1);
			idlemouse();
			break;
		case Save:
			break;
		case Quit:
			exits("");
		}
	}
}
