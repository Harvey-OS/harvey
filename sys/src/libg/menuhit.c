#include <u.h>
#include <libc.h>
#include <libg.h>

enum
{
	Margin = 3,		/* outside to text */
	Border = 2,		/* outside to selection boxes */
	Blackborder = 1,	/* width of outlining border */
	Vspacing = 1,		/* extra spacing between lines of text */
};

static Rectangle
menurect(Rectangle r, int i)
{
	if(i < 0)
		return Rect(0, 0, 0, 0);
	r = inset(r, Margin);
	r.min.y += (font->height+Vspacing)*i;
	r.max.y = r.min.y+font->height+Vspacing;
	return inset(r, Border-Margin);
}

static int
menusel(Rectangle r, Point p)
{
	r = inset(r, Margin);
	if(!ptinrect(p, r))
		return -1;
	return (p.y-r.min.y)/(font->height+Vspacing);
}

int
menuhit(int but, Mouse *m, Menu *menu)
{
	int i, nitem, maxwid = 0, lasti;
	Rectangle r, menur, sc;
	Point pt;
	Bitmap *b;
	char *item;

	sc = screen.clipr;
	clipr(&screen, screen.r);
	for(nitem = 0;
	    item = menu->item? menu->item[nitem] : (*menu->gen)(nitem);
	    nitem++){
		i = strwidth(font, item);
		if(i > maxwid)
			maxwid = i;
	}
	if(menu->lasthit<0 || menu->lasthit>=nitem)
		menu->lasthit = 0;
	r = inset(Rect(0, 0, maxwid, nitem*(font->height+Vspacing)), -Margin);
	r = rsubp(r,
	    Pt(maxwid/2, menu->lasthit*(font->height+Vspacing)+font->height/2));
	r = raddp(r, m->xy);
	pt = Pt(0, 0);
	if(r.max.x>screen.r.max.x)
		pt.x = screen.r.max.x-r.max.x;
	if(r.max.y>screen.r.max.y)
		pt.y = screen.r.max.y-r.max.y;
	if(r.min.x<screen.r.min.x)
		pt.x = screen.r.min.x-r.min.x;
	if(r.min.y<screen.r.min.y)
		pt.y = screen.r.min.y-r.min.y;
	menur = raddp(r, pt);
	b = balloc(menur, screen.ldepth);
	if(b == 0)
		b = &screen;
	bitblt(b, menur.min, &screen, menur, S);
	bitblt(&screen, menur.min, &screen, menur, 0);
	border(&screen, menur, Blackborder, F);
	pt = Pt(menur.min.x+menur.max.x, menur.min.y+Margin);
	for(i = 0; i<nitem; i++, pt.y += font->height+Vspacing){
		item = menu->item? menu->item[i] : (*menu->gen)(i);
		string(&screen,
			Pt((pt.x-strwidth(font, item))/2, pt.y),
			font, item, S);
	}
	lasti = menusel(menur, m->xy);
	r = menurect(menur, menu->lasthit);
	cursorset(div(add(r.min, r.max), 2));
	bitblt(&screen, r.min, &screen, r, F&~D);
	*m = emouse();
	while(m->buttons & (1<<(but-1))){
		*m = emouse();
		i = menusel(menur, m->xy);
		if(i == lasti)
			continue;
		bitblt(&screen, r.min, &screen, r, F&~D);
		r = menurect(menur, i);
		bitblt(&screen, r.min, &screen, r, F&~D);
		lasti = i;
	}
	bitblt(&screen, menur.min, b, menur, S);
	if(b != &screen)
		bfree(b);
	if(lasti >= 0)
		menu->lasthit = lasti;
	clipr(&screen, sc);
	return lasti;
}
