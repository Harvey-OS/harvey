#include <u.h>
#include <libc.h>
#include <libg.h>
#include <fb.h>

#define rectf(b,r,f)	bitblt(b,r.min,b,r,f)	/* stub - remove rectf */

#define	FSIZE		256
#define	RES		255	/* resolution of FSIZE*FSIZE picture */
#define	ISIZE		48	/* ikon size (square) */
#define MAXBPP		2	/* max bits per pixel to bitmapped display */
#define MAXRANGE	254	/* even number for round-off fun */
#define IKONSPACE	(48+4)	/* pixels per gallery entry */

#define MAXPPBYTE	(8*sizeof(uchar)/MAXBPP)		/* pixels per byte */

Mouse mouse;
#define BUTTON(n)	((mouse.buttons >> (n-1)) & 1)
#define BUTTON123()	((int)(mouse.buttons & 0x7))

Bitmap display;
Font *font;

char	name[NAMELEN] = "";
ulong	pic[FSIZE][FSIZE];		/* input picture reduced to FSIZE*FSIZE*RES
						and integrated */
ulong	edited[ISIZE][ISIZE];		/* selected region from srect */


Rectangle Drect;	/* our whole display area */
Rectangle frect;	/* the big face */
Rectangle mrect;	/* the message area */
Rectangle grect;	/* the gallery */
Rectangle srect;	/* edited rectangle on the big face */
int srectok=0;		/* if srect valid */

int rescale=0;		/* if 5/4 ratio pixels */

Bitmap *face=0;
Bitmap *ikon=0;		/* ikon bitmap*/

/* gallery information */
int xpick;		/* x coordinate of current choice */
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
	"reset",
	0
};
enum Menu2{
	In,
	Out,
	Reset,
};

char *menu3text[]={
	"window",
	"depth",
	"write",
	"abort",
	"finish",
	0
};
enum Menu3{
	Window,
	Depth,
	Write,
	Abort,
	Finish
};


Menu menu2={menu2text};
Menu menu3={menu3text};

void	cursorset(Point p);
void	show_data(void);


void
msg(char *s)
{
	rectf(&display, mrect, Zero);
	string(&screen, mrect.min, font, s, S);
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
ulong contrast_table[RES+1];

void
drawface(int lo, int hi, Point facep)
{
	ulong *c, *f;
	int v, h, s, e, temp, i;
	uchar icon[ISIZE*ISIZE];

	if (hi<lo) {
		e=hi; hi=lo; lo=e;
	}
	if(lo<0)
		lo=0;
	else if(RES<lo)
		lo=RES;
	if(hi<0)
		hi=0;
	else if(RES<hi)
		hi=RES;

	temp = hi - lo;
	c = contrast_table;	/* load the contrast table with current values */
	for (v=0; v!=lo; v++)
		*c++ = 0;
	for(; v!=hi; v++)
		*c++ = (v-lo)*RES/temp;
	for(; v<=RES; v++)
		*c++ = RES;

	for(v=0; v!=ISIZE; v++)
		for(h=0, f=floyd[v]; h!=ISIZE; h++)
			*f++ = contrast_table[edited[v][h] & RES];

	for(v=0; v!=ISIZE; v++){
		f = floyd[v];
		for(h=0; h<ISIZE/ppbyte; h++){
			icon[bpl*v + h] = 0;
			for(s=0; s < ppbyte; s++,f++){
				e = f[0];
				i = 0;
				switch (mugdepth) {
				case 1:	if (e > RES/2) {
						i=0;
						e -= RES;
					} else
						i = 1;
					break;
				case 2:	i = ((1<<mugdepth)*e)/RES;
					if (i<0)
						i=0;
					else if (i>maxp)
						i=maxp;
					e -= (i*RES)/3;
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
	wrbitmap(ikon, 0, 48, (uchar *)icon);
	bitblt(&display, facep, ikon, Rect(0,0, 48,48), S);
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
showgallery(void)
{
	int x, y;
	int xv, yv;
	Rectangle r;

	box(inset(galrect(xpick, ypick), -2), notS);	/* clear old box */
	for (y=0; y<ycount; y++)
		for (x=0; x<xcount; x++) {
			xv = VX(x);
			yv = VY(y);
			r = galrect(x ,y);
			if (xv >= yv)
				drawface(xv, yv, r.min);
			else
				rectf(&display, r, Zero);
		}
	box(inset(galrect(xpick, ypick), -2), S);
}

void
pickrect(void)
{
	Point p;
	int x, y;

	if (!ptinrect(mouse.xy, grect))
		return;
	p = sub(mouse.xy, grect.min);
	x = (p.x+2)/IKONSPACE;
	y = (p.y+2)/IKONSPACE;
	if (VX(x) < VY(y))	/* none displayed there */
		return;

	box(inset(galrect(xpick, ypick), -2), notS);	/* clear old box */
	xpick = x;
	ypick = y;
	box(inset(galrect(xpick, ypick), -2), S);
	show_data();
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
	show_data();
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
	Drect = inset(newrect, 3);
	rectf(&display,Drect,Zero);

	mrect.min = Pt(Drect.min.x+2,  Drect.max.y - 2 - font->height);
	mrect.max = Pt(Drect.max.x, Drect.max.y - 2);

	if (Drect.min.y + 2 + 256 + 3 > mrect.min.y ||
	    Drect.min.x + 2 + 256 + 3 > Drect.max.x)
		return 0;	/* no room for the big picture */

	frect.min = Pt(mrect.min.x+2, mrect.min.y-3-256);
	frect.max = add(frect.min, Pt(256, 256));

	if (frect.min.y - Drect.min.y > Drect.max.x - frect.max.x) {
		/* gallery on top */
		grect.min = add(Drect.min, Pt(2, 2));
		grect.max = Pt(Drect.max.x-2, frect.min.y-3);
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

	grect.min = Pt(grect.max.x - IKONSPACE*xcount,
		       grect.max.y - IKONSPACE*ycount);

	show_data();
	bitblt(&screen, frect.min, face, Rect(0,0,256,256), S);
	if (srectok)
		drawborder(raddp(srect, frect.min));
	xpick = xcount/2;	/* stub - this is lazy */
	ypick = 0;
	showgallery();

	return 1;	/* reshape ok */
}

void
show_data(void)
{
	char buf[100];
	sprint(buf, "%s  depth=%d  low=%d/%d range=%d/%d  picked: %d-%d",
		name, mugdepth, xlow, ylow, xrange, yrange, VX(xpick), VY(ypick));
	msg(buf);

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
	Point p, q, r;

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
	if(!ptinrect(q, frect) || BUTTON(1) || BUTTON(2)){
		while(BUTTON123())
			mouse = emouse();
		srectok = 0;
		return 0;
	}
	q=p;
	box(Rpt(Pt(2*p.x-q.x, p.y), q), S^D);
	for(; BUTTON(3); mouse = emouse()){
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
	return 1;
}


int
sample(int x0, int y0, int x1, int y1)
{
	int v, u;

	if(rescale){
		x0/=1.25; x0+=25;
		x1/=1.25; x1+=25;
	}
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
	else if(FSIZE<=x0)
		x0=FSIZE-1;
	if(y0<0)
		y0=0;
	else if(FSIZE<=y0)
		y0=FSIZE-1;
	if(x1<=x0)
		x1=x0+1;
	else if(FSIZE<x1)
		x1=FSIZE;
	if(y1<=y0)
		y1=y0+1;
	else if(FSIZE<y1)
		y1=FSIZE;
	x1-=x0;
	y1-=y0;

	for(y=0;y!=ISIZE;y++)
		for(x=0;x!=ISIZE;x++) {
			edited[y][x] = sample(x0+x1*x/ISIZE, y0+y1*y/ISIZE,
				x0+((x+1)*x1-1)/ISIZE, y0+((y+1)*y1-1)/ISIZE);
		}
}

void
bitmapinit(void)
{
	binit(0, 0, "");
	face = balloc(Rect(0, 0, 256, 256), 1);
	if (face == (Bitmap *)0) {
		perror("Could not alloc `face'");
		exits("alloc face");
	}
}

void
getpic(char *f)
{
	uchar *vp, v[4096*8];
	PICFILE *p;
	int x, y, sum, shrnk;
	int pheight, pwidth;

	p = picopen_r(f);
	if (p == 0) {
		picerror(f);
		exits("open");
	}

	pwidth=PIC_WIDTH(p);
	pheight=PIC_HEIGHT(p);
#define imax(a,b)	((a > b) ? a : b)
	shrnk=imax(1, imax((pheight+FSIZE-1)/FSIZE, (pwidth+FSIZE-1)/FSIZE));

	for(y=0; y!=FSIZE; y++)
		for(x=0; x!=FSIZE; x++)
			pic[y][x] = 0;
	for(y=0; y!=pheight; y++){
		picread(p, (char *)v);
		for(x=0,vp=v; x!=pwidth; x++,vp+=p->nchan)
			pic[y/shrnk][x/shrnk] += p->nchan<3?*vp:(vp[0]+vp[1]+vp[2])/3;
	}

	/* adjust values for shrinkage */
	for(y=0; y!=FSIZE; y++)
		for(x=0; x<FSIZE; x++)
			pic[y][x]/=shrnk*shrnk;
	/* set each row to the sum of the previous rows. */
	for(x=1; x<FSIZE; x++)
		pic[0][x] += pic[0][x-1];
	for(y=1; y!=FSIZE; y++){
		sum=0;
		for(x=0; x<FSIZE; x++){
			sum += pic[y][x];
			pic[y][x] = pic[y-1][x] + sum;
		}
	}
	picclose(p);
	return;
}

void
makepic(void)
{
	int x, y;
	int e, i, t;
	int floyd[2][FSIZE+1];
	uchar picbitmap[FSIZE][FSIZE/MAXPPBYTE];

	for(x=0; x<FSIZE; x++)
		floyd[0][x] = 0;
	for(y=0; y<FSIZE; y++){
		for (x=0; x<FSIZE/MAXPPBYTE; x++)
			picbitmap[y][x] = 0;
		for(x=0; x<FSIZE; x++)
			floyd[(y+1)&1][x] = 0;
		for(x=0; x<FSIZE; x++){
			e = floyd[y&1][x] + sample(x, y, x+1, y+1);
			i = e/64;
			if (i<0)
				i=0;
			else if (i>3)
				i=3;
			e -= (i*RES)/3;
			picbitmap[y][x/MAXPPBYTE] |= (3-i) <<
				(MAXPPBYTE - (x % MAXPPBYTE) - 1)*MAXBPP;
			t = 3*e/8;
			floyd[(y+1)&1][x] += t;
			floyd[(y+1)&1][x+1] += e-2*t;
			floyd[y&1][x+1] += t;
		}
	}
	wrbitmap(face, 0, 256, (uchar *)picbitmap);
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
mugsusage(void)
{
	fprint(2, "usage: mugs [-a] [-1|2] [file]");
	exits("usage");
}

void
setdepth(int newdepth)
{
	if (ikon)
		bfree(ikon);
	mugdepth = newdepth;
	ikon = balloc(Rect(0, 0, 48, 48), mugdepth-1);
	if (ikon == (Bitmap *)0) {
		perror("Could not alloc `ikon'");
		exits("alloc ikon");
	}
	ppbyte = 8/mugdepth;
	maxp = (1<<mugdepth) - 1;
	bpl = ISIZE/ppbyte;
}

void
main(int argc, char *argv[])
{
	int	depth=2;

	ARGBEGIN{
	case 'a':	rescale++;		break;
	case '1':	depth=1;		break;
	case '2':	depth=2;		break;
	case L'ü':	fprint(2, "rob\n");	break;
	default:
		mugsusage();
	}ARGEND;

	if (*argv)
		strncpy(name, *argv++, sizeof(name));
	else
		strncpy(name, "/fd/0", sizeof(name));
	if (*argv)
		mugsusage();

	getpic(name);
	bitmapinit();
	einit(Emouse);
	setdepth(depth);
	makepic();
	reshape(bscreenrect(0));
	srect = rsubp(frect, frect.min);
	getedited(srect);
	xpick = xcount/2;	/* stub - this is lazy */
	ypick = 0;
	showgallery();
	for(;;){
		mouse = emouse();
		if (BUTTON(1)) 
				pickrect();
		if (BUTTON(2)) 
			switch(menuhit(2, &mouse, &menu2)){
			case In:
				zoomin();
				showgallery();
				break;
			case Out:
				zoomout();
				showgallery();
				break;
			case Reset:
				box(inset(galrect(xpick, ypick), -2), notS);
				xpick = xcount/2;	/* stub - this is lazy */
				ypick = 0;
				xrange = yrange = MAXRANGE;
				xlow = ylow = 0;
				showgallery();
				break;
			}
		if (BUTTON(3))
			switch(menuhit(3, &mouse, &menu3)){
			case Window:
				if (srectok)
					drawborder(raddp(srect, frect.min));
				if(editface(&srect)){
					srectok = 1;
					drawborder(raddp(srect, frect.min));
					getedited(srect);
					showgallery();
				}
				break;
			case Depth:
				setdepth(3 - mugdepth);
				show_data();
				showgallery();
				break;
			case Write:
				do_write();
				break;
			case Abort:
				rectf(&display,Drect,Zero);
				exits("aborted");
			case Finish:
				rectf(&display,Drect,Zero);
				exits(0);
			}
	}
	exits("error");
}
