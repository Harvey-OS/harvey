#include <u.h>
#include <libc.h>
#include <draw.h>
#include <thread.h>
#include <cursor.h>
#include <mouse.h>
#include <keyboard.h>
#include <frame.h>
#include <auth.h>
#include <fcall.h>
#include "dat.h"
#include "fns.h"

enum
{
	Margin = 4,		/* outside to text */
	Border = 2,		/* outside to selection boxes */
	Blackborder = 2,	/* width of outlining border */
	Vspacing = 2,		/* extra spacing between lines of text */
};

static	Image	*menutxt;
static	Image	*back;
static	Image	*high;
static	Image	*bord;
static	Image	*text;
static	Image	*htext;

static
void
menucolors(void)
{
	/* Main tone is greenish, with negative selection */
	back = allocimagemix(display, DPalegreen, DWhite);
	high = allocimage(display, Rect(0,0,1,1), RGB24, 1, DDarkgreen);
	bord = allocimage(display, Rect(0,0,1,1), RGB24, 1, DMedgreen);
	text = display->black;
	htext = back;
	if(back==nil || high==nil || bord==nil || text==nil)
		goto Error;
	return;

    Error:
	freeimage(back);
	freeimage(high);
	freeimage(bord);
	back = display->white;
	high = display->black;
	bord = display->black;
	text = display->black;
	htext = display->white;
}

static Rectangle
menurect(Rectangle r, int i)
{
	if(i < 0)
		return Rect(0, 0, 0, 0);
	r.min.y += (font->height+Vspacing)*i;
	r.max.y = r.min.y+font->height+Vspacing;
	return insetrect(r, Border-Margin);
}

static int
menusel(Rectangle r, Point p)
{
	r = insetrect(r, Margin);
	if(!ptinrect(p, r))
		return -1;
	return (p.y-r.min.y)/(font->height+Vspacing);
}

static
void
paintitem(Image *view, Menu *menu, Rectangle textr, int i, int highlight, Image *save, Image *restore)
{
	char *item;
	Rectangle r;
	Point pt;
	Image *h;

	if(i < 0)
		return;
	r = menurect(textr, i);
	if(restore){
		draw(screen, r, restore, nil, restore->r.min);
		return;
	}
	if(save)
		draw(save, save->r, view, nil, r.min);
	if(menu->item)
		item = menu->item[i];
	else
		item = (*menu->gen)(i);
	pt.x = (textr.min.x+textr.max.x-stringwidth(font, item))/2;
	pt.y = textr.min.y+i*(font->height+Vspacing);
	h = back;
	if(highlight)
		h = high;
	draw(screen, r, h, nil, pt);
	h = text;
	if(highlight)
		h = htext;
	string(screen, pt, h, pt, font, item);
}


int
menuselect(Menu *menu, Mouse *mouse, int but, Font *font, Point delta)
{
	int i, nitem, maxwid, lasti;
	Rectangle r, menur, textr;
	Point pt;
	Image *d;
	char *item;
	Image *save;

	if(back == nil)
		menucolors();
	mouse->xy.x += delta.x;
	mouse->xy.y += delta.y;
	maxwid = 0;
	nitem = 0;
	for(;;){
		if(menu->item)
			item = menu->item[nitem];
		else
			item = (*menu->gen)(nitem);
		if(item == nil)
			break;
		i = stringwidth(font, item);
		if(i > maxwid)
			maxwid = i;
		nitem++;
	}
	if(menu->lasthit<0 || menu->lasthit>=nitem)
		menu->lasthit = 0;
	r = Rect(0, 0, maxwid, nitem*(font->height+Vspacing));
	r = insetrect(r, -Margin);
	r = rectsubpt(r, Pt(maxwid/2, menu->lasthit*(font->height+Vspacing)+font->height/2));
	r = rectaddpt(r, mouse->xy);
	pt = Pt(0, 0);
	if(r.max.x>screen->r.max.x)
		pt.x = screen->r.max.x-r.max.x;
	if(r.max.y>screen->r.max.y)
		pt.y = screen->r.max.y-r.max.y;
	if(r.min.x<screen->r.min.x)
		pt.x = screen->r.min.x-r.min.x;
	if(r.min.y<screen->r.min.y)
		pt.y = screen->r.min.y-r.min.y;
	menur = rectaddpt(r, pt);
	textr.max.x = menur.max.x-Margin;
	textr.min.x = textr.max.x-maxwid;
	textr.min.y = menur.min.y+Margin;
	textr.max.y = textr.min.y + nitem*(font->height+Vspacing);
	d = allocwindow(wscreen, menur, Refbackup, DWhite);
	if(d == nil)
		d = view;
	draw(d, menur, back, nil, ZP);
	border(d, menur, Blackborder, bord, ZP);
	lasti = menu->lasthit;
	for(i = 0; i<nitem; i++)
		paintitem(d, menu, textr, i, 0, nil, nil);
	if(lasti < 0)
		lasti = 0;
	r = menurect(textr, 0);
	save = allocimage(display, r, display->chan, 0, DWhite);
	paintitem(d, menu, textr, lasti, 1, save, nil);
	r = menurect(textr, menu->lasthit);
	pt = addpt(r.min, r.max);
	moveto(mousectl, divpt(pt, 2));
	flushimage(display, 1);
	readmouse(mousectl);
	mouse->xy.x += delta.x;
	mouse->xy.y += delta.y;
	while(mouse->buttons & (1<<(but-1))){
		flushimage(display, 1);
		readmouse(mousectl);
		mouse->xy.x += delta.x;
		mouse->xy.y += delta.y;
		i = menusel(menur, mouse->xy);
		if(i == lasti)
			continue;
		paintitem(d, menu, textr, lasti, 0, nil, save);
		paintitem(d, menu, textr, i, 1, save, nil);
		lasti = i;
	}
	if(d != view)
		freeimage(d);
	freeimage(save);
	if(lasti >= 0)
		menu->lasthit = lasti;
	mouse->xy.x -= delta.x;
	mouse->xy.y -= delta.y;
	return lasti;
}
