#include <alef.h>
#include <libg.h>

enum
{
	Margin = 3,		/* outside to text */
	Border = 2,		/* outside to selection boxes */
	Blackborder = 1,	/* width of outlining border */
	Vspacing = 1,		/* extra spacing between lines of text */
	Maxunscroll = 25,	/* maximum #entries before scrolling turns on */
	Nscroll = 20,		/* number entries in scrolling part */
	Scrollwid = 14,		/* width of scroll bar */
	Gap = 4,			/* between text and scroll bar */
};

intern	Bitmap	*menutxt;

intern	byte menutxtbits[] = {
	0x22, 0x22, 0x88, 0x88, 0x22, 0x22, 0x88, 0x88,
	0x22, 0x22, 0x88, 0x88, 0x22, 0x22, 0x88, 0x88,
	0x22, 0x22, 0x88, 0x88, 0x22, 0x22, 0x88, 0x88,
	0x22, 0x22, 0x88, 0x88, 0x22, 0x22, 0x88, 0x88,
};

/*
 * r is a rectangle holding the text elements.
 * return the rectangle, including its black edge, holding element i.
 */
intern Rectangle
menurect(Rectangle r, int i)
{
	if(i < 0)
		return Rect(0, 0, 0, 0);
	r.min.y += (font->height+Vspacing)*i;
	r.max.y = r.min.y+font->height+Vspacing;
	return inset(r, Border-Margin);
}

/*
 * r is a rectangle holding the text elements.
 * return the element number containing p.
 */
intern int
menusel(Rectangle r, Point p)
{
	if(!ptinrect(p, r))
		return -1;
	return (p.y-r.min.y)/(font->height+Vspacing);
}

/*
 * menur is a rectangle holding all the highlightable text elements.
 * track mouse while inside the box, return what's selected when button
 * is raised, -1 as soon as it leaves box.
 * invariant: nothing is highlighted on entry or exit.
 */
intern
(int, Mouse)
menuscan(int but, Rectangle menur, int lasti, chan(Mouse)cm, Mouse m)
{
	int i;
	Rectangle r;

	r = menurect(menur, lasti);
	bitblt(&screen, r.min, &screen, r, F&~D);
	bflush();
	m = <-cm;
	while(m.buttons & (1<<(but-1))){
		bflush();
		m = <-cm;
		i = menusel(menur, m.xy);
		if(i == lasti)
			continue;
		bitblt(&screen, r.min, &screen, r, F&~D);
		if(i == -1)
			return (i, m);
		r = menurect(menur, i);
		bitblt(&screen, r.min, &screen, r, F&~D);
		lasti = i;
	}
	return (lasti, m);
}

intern void
menupaint(Menu *menu, Rectangle textr, int off, int nitemdrawn)
{
	int i;
	Point pt;
	Rectangle r;
	byte *item;

	r = inset(textr, Border-Margin);
	bitblt(&screen, r.min, &screen, r, 0);
	pt = Pt(textr.min.x+textr.max.x, textr.min.y);
	for(i = 0; i<nitemdrawn; i++){
		if(menu->item)
			item = menu->item[i+off];
		else
			item = (*menu->gen)(i+off);
		string(&screen,
			Pt((pt.x-strwidth(font, item))/2, pt.y),
			font, item, S);
		pt.y += font->height+Vspacing;
	}
}

intern void
menuscrollpaint(Rectangle scrollr, int off, int nitem, int nitemdrawn)
{
	Rectangle r;

	bitblt(&screen, scrollr.min, &screen, scrollr, 0);
	r.min.x = scrollr.min.x;
	r.max.x = scrollr.max.x;
	r.min.y = scrollr.min.y + (Dy(scrollr)*off)/nitem;
	r.max.y = scrollr.min.y + (Dy(scrollr)*(off+nitemdrawn))/nitem;
	if(r.max.y < r.min.y+2)
		r.max.y = r.min.y+2;
	border(&screen, r, 1, F);
	if(menutxt == nil){
		menutxt = balloc(Rect(0, 0, 16, 16), 0);
		if(menutxt)
			wrbitmap(menutxt, 0, 16, menutxtbits);
	}
	if(menutxt)
		texture(&screen, inset(r, 1), menutxt, S);
}

(int, Mouse)
Menu.hit(Menu *menu, int but, chan(Mouse) cm, Mouse m)
{
	int i, nitem, nitemdrawn, maxwid, lasti, off, noff, wid, screenitem;
	int scrolling;
	Rectangle r, menur, sc, textr, scrollr;
	Bitmap *b;
	Point pt;
	byte *item;

	sc = screen.clipr;
	clipr(&screen, screen.r);
	maxwid = 0;
	nitem = 0;
	for(;;){
		if(menu->item)
			item = menu->item[nitem];
		else
			item = (*menu->gen)(nitem);
		if(item == nil)
			break;
		i = strwidth(font, item);
		if(i > maxwid)
			maxwid = i;
		nitem++;
	}
	if(menu->lasthit<0 || menu->lasthit>=nitem)
		menu->lasthit = 0;
	screenitem = (Dy(screen.r)-10)/(font->height+Vspacing);
	if(nitem>Maxunscroll || nitem>screenitem){
		scrolling = 1;
		nitemdrawn = Nscroll;
		if(nitemdrawn > screenitem)
			nitemdrawn = screenitem;
		wid = maxwid + Gap + Scrollwid;
		off = menu->lasthit - nitemdrawn/2;
		if(off < 0)
			off = 0;
		if(off > nitem-nitemdrawn)
			off = nitem-nitemdrawn;
		lasti = menu->lasthit-off;
	}else{
		scrolling = 0;
		nitemdrawn = nitem;
		wid = maxwid;
		off = 0;
		lasti = menu->lasthit;
	}
	r = inset(Rect(0, 0, wid, nitemdrawn*(font->height+Vspacing)), -Margin);
	r = rsubp(r, Pt(wid/2, lasti*(font->height+Vspacing)+font->height/2));
	r = raddp(r, m.xy);
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
	textr.max.x = menur.max.x-Margin;
	textr.min.x = textr.max.x-maxwid;
	textr.min.y = menur.min.y+Margin;
	textr.max.y = textr.min.y + nitemdrawn*(font->height+Vspacing);
	if(scrolling){
		scrollr = inset(menur, Border);
		scrollr.max.x = scrollr.min.x+Scrollwid;
	}else
		scrollr = Rect(0, 0, 0, 0);

	b = balloc(menur, screen.ldepth);
	if(b == nil)
		b = &screen;
	bitblt(b, menur.min, &screen, menur, S);
	bitblt(&screen, menur.min, &screen, menur, 0);
	border(&screen, menur, Blackborder, F);
	r = menurect(textr, lasti);
	cursorset(div(add(r.min, r.max), 2));
	menupaint(menu, textr, off, nitemdrawn);
	if(scrolling)
		menuscrollpaint(scrollr, off, nitem, nitemdrawn);
	while(m.buttons & (1<<(but-1))){
		(lasti, m) = menuscan(but, textr, lasti, cm, m);
		if(lasti >= 0)
			break;
		while(!ptinrect(m.xy, textr) && (m.buttons & (1<<(but-1)))){
			if(scrolling && ptinrect(m.xy, scrollr)){
				noff = ((m.xy.y-scrollr.min.y)*nitem)/Dy(scrollr);
				noff -= nitemdrawn/2;
				if(noff < 0)
					noff = 0;
				if(noff > nitem-nitemdrawn)
					noff = nitem-nitemdrawn;
				if(noff != off){
					off = noff;
					menupaint(menu, textr, off, nitemdrawn);
					menuscrollpaint(scrollr, off, nitem, nitemdrawn);
				}
			}
			bflush();
			m = <-cm;
		}
	}
	bitblt(&screen, menur.min, b, menur, S);
	if(b != &screen)
		bfree(b);
	clipr(&screen, sc);
	if(lasti >= 0){
		menu->lasthit = lasti+off;
		return (menu->lasthit, m);
	}
	return (-1, m);
}
