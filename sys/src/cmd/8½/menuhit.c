#include <u.h>
#include <libc.h>
#include <libg.h>
#include <frame.h>
#include <layer.h>
#include "dat.h"
#include "fns.h"

enum{
	Margin=3,	/* outside to text */
	Border=2,	/* outside to selection boxes */
	Blackborder=1,	/* width of outlining border */
	Vspacing=1	/* extra spacing between lines of text */
};

Rectangle	menurect(Rectangle, int);
int		menusel(Rectangle, Point);

int
lmenuhit(int but, Menu *menu)
{
	int i, nitem, maxwid, lasti;
	Rectangle r, menur;
	Point pt;
	Layer *l;
	char *item;

	maxwid = 0;
	for(nitem=0;
	    item=menu->item? menu->item[nitem] : (*menu->gen)(nitem);
	    nitem++){
		i = strwidth(font, item);
		if(i > maxwid)
			maxwid = i;
	}
	if(menu->lasthit<0 || menu->lasthit>=nitem)
		menu->lasthit = 0;
	r = inset(Rect(0, 0, maxwid, nitem*(font->height+Vspacing)), -Margin);
	r = rsubp(r, Pt(maxwid/2, menu->lasthit*(font->height+Vspacing)+font->height/2));
	r = raddp(r, proc.p->mouse.xy);
	pt = Pt(0, 0);
	if(r.min.x < screen.r.min.x)
		pt.x = screen.r.min.x-r.min.x;
	if(r.min.y < screen.r.min.y)
		pt.y = screen.r.min.y-r.min.y;
	if(r.max.x > screen.r.max.x)
		pt.x = screen.r.max.x-r.max.x;
	if(r.max.y > screen.r.max.y)
		pt.y = screen.r.max.y-r.max.y;
	menur = raddp(r, pt);
	l = lalloc(&cover, menur);
	if(l == 0)
		l = (Layer*)&screen;
	border(l, menur, Blackborder, F);
	pt = Pt(menur.min.x+menur.max.x, menur.min.y+Margin);
	for(i=0; i<nitem; i++, pt.y+=font->height+Vspacing){
		item = menu->item? menu->item[i] : (*menu->gen)(i);
		string(l, Pt((pt.x-strwidth(font, item))/2, pt.y), font, item, S);
	}
	lasti = menu->lasthit;
	r = menurect(menur, lasti);
	cursorset(div(add(r.min, r.max), 2));
	bitblt(l, r.min, l, r, F&~D);
	while(proc.p->mouse.buttons & (1<<(but-1))){
		frgetmouse();
		i = menusel(menur, proc.p->mouse.xy);
		if(i == lasti)
			continue;
		bitblt(l, r.min, l, r, F&~D);
		r = menurect(menur, i);
		bitblt(l, r.min, l, r, F&~D);
		lasti = i;
	}
	if(l != &screen)
		lfree(l);
	if(lasti >= 0)
		menu->lasthit = lasti;
	return lasti;
}

Rectangle
menurect(Rectangle r, int i)
{
	if(i < 0)
		return Rect(0, 0, 0, 0);
	r = inset(r, Margin);
	r.min.y += (font->height+Vspacing)*i;
	r.max.y = r.min.y+font->height+Vspacing;
	return inset(r, Border-Margin);
}

int
menusel(Rectangle r, Point p)
{
	r = inset(r, Margin);
	if(!ptinrect(p, r))
		return -1;
	return (p.y-r.min.y)/(font->height+Vspacing);
}
