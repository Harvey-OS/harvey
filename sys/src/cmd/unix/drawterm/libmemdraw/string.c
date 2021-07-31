#include "../lib9.h"

#include "../libdraw/draw.h"
#include "../libmemdraw/memdraw.h"
#include "../libmemlayer/memlayer.h"

Point
memimagestring(Memimage *b, Point p, Memimage *color, Memsubfont *f, char *cs)
{
	int w, width;
	uchar *s;
	Rune c;
	Fontchar *i;

	s = (uchar*)cs;
	for(; (c=*s) != 0; p.x+=width){
		width = 0;
		if(c < Runeself)
			s++;
		else{
			w = chartorune(&c, (char*)s);
			if(w == 0){
				s++;
				continue;
			}
			s += w;
		}
		if(c >= f->n)
			continue;
		i = f->info+c;
		width = i->width;
		memdraw(b, Rect(p.x+i->left, p.y+i->top, p.x+i->left+(i[1].x-i[0].x), p.y+i->bottom),
			color, Pt(i->x, i->top), f->bits, Pt(i->x, i->top));
	}
	return p;
}

Point
memsubfontwidth(Memsubfont *f, char *cs)
{
	Rune c;
	Point p;
	uchar *s;
	Fontchar *i;
	int w, width;

	p = Pt(0, f->height);
	s = (uchar*)cs;
	for(; (c=*s) != 0; p.x+=width){
		width = 0;
		if(c < Runeself)
			s++;
		else{
			w = chartorune(&c, (char*)s);
			if(w == 0){
				s++;
				continue;
			}
			s += w;
		}
		if(c >= f->n)
			continue;
		i = f->info+c;
		width = i->width;
	}
	return p;
}
