#include <u.h>
#include <libc.h>
#include <libg.h>
#include "menu.h"
#include "thing.h"

extern Font *defont;
extern Bitmap screen;
extern Mouse mouse;
int getmouse(void);

#define rectf(b,r,f)	bitblt(b,r.min,b,r,f)
#define scale(x, inmin, inmax, outmin, outmax)\
	(outmin + (x-inmin)*(outmax-outmin)/(inmax-inmin))
#define button(but)	(mouse.buttons&(1<<(but-1)))

#define MIN(a,b)	((a < b) ? a : b)
#define MAX(a,b)	((a > b) ? a : b)
#define bound(x, low, high) MIN(high, MAX( low, x ))

#define SPACING		(defont->height+1)
#define DISPLAY		16
#define CHARWIDTH	(strwidth(defont," "))
#define DELTA		6
#define BARWIDTH	18
#define	MAXDEPTH	16	/* don't use too much stack */
#define ARROWIDTH	20
#define XMAX	screen.r.max.x
#define YMAX	screen.r.max.y

static Texture tarrow =
{
	0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0xC0, 0x00, 0xC0, 0x00, 0xE0, 0x00, 0xF8, 0xFF, 0xFE,
	0xFF, 0xFE, 0x00, 0xF8, 0x00, 0xE0, 0x00, 0xC0, 0x00, 0xC0, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00,
};

static NMitem *
tablegen(int i, NMitem *table)
{
	return &table[i];
}

static char fill[64];

NMitem *
mhit(NMenu *m, int but, int depth)
{
	register int width, mwidth, i, j, top, newtop, hit, newhit;
	register int items, lines, length, w, x;
	Point p, q, savep, baro, barc;
	Rectangle sr, tr, mr;	/* scroll, text, menu */
	Rectangle rside, rhit;
	register Bitmap *b;
	register char *from, *to;
	Bitmap *bhelp = 0, *arrow;
	NMitem *(*generator)(int,NMitem *), *mi, *table, *ret = 0;
	int dohfn;
	static flip(Rectangle,int);
	static helpon(char *,Rectangle,Bitmap **);
	static helpoff(Bitmap **);

#define sro sr.min
#define src sr.max
#define tro tr.min
#define trc tr.max
#define mro mr.min
#define mrc mr.max

	generator = (table=m->item) ? tablegen : m->generator;
	arrow = balloc(Rect(0, 0, 16, 16),screen.ldepth);
	texture(arrow, arrow->r, maptex(&tarrow), S);
	p = mouse.xy;
	w = x = length = 0;
	for(items = 0; (mi=(*generator)(items, table))->text; ++items) {
		register int s = strlen (mi->text);
		length = MAX(length, s);
		if (mi->next) {
			w = MAX (w, s);
		} else
			x = MAX (x, s);
	}
	if(items == 0)
		return(ret);
	width = length*CHARWIDTH+10;
	w *= CHARWIDTH;
	x *= CHARWIDTH;
	if (x <= w)
		mwidth = w + ARROWIDTH;
	else if (x >= w + 2*ARROWIDTH)
		mwidth = x;
	else
		mwidth = w + ARROWIDTH + (x - w) / 2;
	mwidth += 10;
	sro.x = sro.y = src.x = tro.x = mro.x = mro.y = 0;
	if(items <= DISPLAY)
		lines = items;
	else{
		lines = DISPLAY;
		tro.x = src.x = BARWIDTH;
		sro.x = sro.y = 1;
	}
#define ASCEND 2
	tro.y = ASCEND;
	mrc = trc = add(tro, Pt(mwidth, MIN(items, lines)*SPACING));
	src.y = mrc.y-1;
	newtop = bound(m->prevtop, 0, items-lines);
	p.y -= bound(m->prevhit, 0, lines-1)*SPACING+SPACING/2;
	p.x = bound(p.x-(src.x+mwidth/2), 0, XMAX-mrc.x);
	p.y = bound(p.y, 0, YMAX-mrc.y);
	sr = raddp(sr, p);
	tr = raddp(tr, p);
	mr = raddp(mr, p);
	rside = mr;
	rside.min.x = rside.max.x-20;
	b = balloc(mr,screen.ldepth);
	if(b)
		bitblt(b, mro, &screen, mr, S);
	rectf(&screen, mr, F);
PaintMenu:
	rectf(&screen, inset(mr, 1), 0);
	top = newtop;
	if(items > DISPLAY){
		baro.y = scale(top, 0, items, sro.y, src.y);
		baro.x = sr.min.x;
		barc.y = scale(top+DISPLAY, 0, items, sro.y, src.y);
		barc.x = sr.max.x;
		rectf(&screen, Rpt(baro,barc), F&~D);
	}
	for(p=tro, i=top; i < MIN(top+lines, items); ++i){
		q = p;
		mi = generator(i, table);
		from = mi->text;
		for(to = &fill[0]; *from; ++from)
			if(*from & 0x80)
				for(j=length-(strlen(from+1)+(to-&fill[0])); j-->0;)
					*to++ = *from & 0x7F;
			else
				*to++ = *from;
		*to = '\0';
		q.x += (width-strwidth(defont,fill))/2;
		string(&screen, q, defont, fill, D^S);
		if(mi->next)
			bitblt(&screen, Pt(trc.x-18, p.y-2), arrow, arrow->r, S|D);
		p.y += SPACING;
	}
	savep = mouse.xy;
	mi = 0;
	for(newhit = hit = -1; button(but); getmouse()){
		p = mouse.xy;
		if(depth && ((p.x < mro.x) || button(5-but)))	/* uhh, 5-but?? */
		{
			ret = 0;
			break;
		}
		if(ptinrect(p, sr)){
			p.x = ((p.x << 1) + sr.min.x + sr.max.x)/4;
			if(ptinrect(savep,tr))
				p.y = (baro.y + barc.y)/2;
			/*if (!eqpt(mouse.xy,p))
				cursorset(p);*/
			newtop = scale(p.y, sro.y, src.y, 0, items);
			newtop = bound(newtop-DISPLAY/2, 0, items-DISPLAY);
			if(newtop != top)
/* ->->-> */			goto PaintMenu;
		}
		savep = p;
		newhit = -1;
		if(ptinrect(p, tr)){
			newhit = bound((p.y-tro.y)/SPACING, 0, lines-1);
			if(newhit!=hit && hit>=0
			 && abs(tro.y+SPACING*newhit+SPACING/2-p.y) > SPACING/3)
				newhit = hit;
			rhit = tr;
			rhit.min.y += newhit*SPACING-ASCEND/2;
			rhit.max.y = rhit.min.y + SPACING;
		}
		if(newhit == -1)
			ret = 0, dohfn = 0;
		else
			ret = mi = (*generator)(top+newhit, table), dohfn = 1;
		if(newhit == hit)
		{
			if((newhit != -1) && (bhelp == 0) && button(3) && mi->help)	/* help button hardwired? */
				helpon(mi->help, rhit, &bhelp);
			else if(bhelp && !button(3))
				helpoff(&bhelp);
		}
		else
		{
			flip(tr, hit);
			helpoff(&bhelp);
			flip(tr, newhit);
			hit = newhit;
			if((newhit != -1) && button(3) && mi->help)
				helpon(mi->help, rhit, &bhelp);
		}
		if((newhit != -1) && ptinrect(p, rside))
		{
			if(mi->dfn) (*mi->dfn)(mi);
			if(mi->next && (depth <= MAXDEPTH))
				ret = mhit(mi->next, but, depth+1), dohfn = 0;
			if(mi->bfn) (*mi->bfn)(mi);
		}
		if(newhit==0 && top>0){
			newtop = top-1;
			p.y += SPACING;
			cursorset(p);
			getmouse();
/* ->->-> */		goto PaintMenu;
		}
		if(newhit==DISPLAY-1 && top<items-lines){
			newtop = top+1;
			p.y -= SPACING;
			cursorset(p);
			getmouse();
/* ->->-> */		goto PaintMenu;
		}
	}
	if(bhelp)
		helpoff(&bhelp);
	if(b){
		screenswap(b, b->r, b->r);
		bfree(b);
	}
	if(hit>=0){
		m->prevhit = hit;
		m->prevtop = top;
		if(ret && ret->hfn && dohfn) (*ret->hfn)(mi);
	}
	return(ret);
}

static
flip(Rectangle r,int n)
{
	if(n<0)
		return;
	++r.min.x;
	r.max.y = (r.min.y += SPACING*n-1) + SPACING;
	--r.max.x;
	rectf(&screen, r, F&~D);
}

static
helpon(char *msg, Rectangle r, Bitmap **bhelp)
{
	register Bitmap *b;
	register w;

	w = strwidth(defont, msg)+10;
	if(r.max.x+w < XMAX)
	{
		r.min.x = r.max.x;
		r.max.x += w;
	}
	else
	{
		r.max.x = r.min.x;
		r.min.x -= w;
	}
	if(*bhelp = b = balloc(r = inset(r, -1), screen.ldepth))
	{
		rectf(b, r, F);
		rectf(b, inset(r, 1), F&~D);
		string(b, add(b->r.min, Pt(5, 1)), defont, msg, S^D);
		screenswap(b, b->r, b->r);
	}
}

static
helpoff(Bitmap **bhelp)
{
	Bitmap *bh;

	if(bh = *bhelp)
	{
		screenswap(bh, bh->r, bh->r);
		bfree(bh);
		*bhelp = 0;
	}
}

screenswap(Bitmap *bp, Rectangle rect, Rectangle screenrect)
{
	bitblt(bp, rect.min, &screen, screenrect, D^S);
	bitblt(&screen, screenrect.min, bp, rect, D^S);
	bitblt(bp, rect.min, &screen, screenrect, D^S);
}
