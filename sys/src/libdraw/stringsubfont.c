/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <draw.h>

Point
stringsubfont(Image *b, Point p, Image *color, Subfont *f, char *cs)
{
	int w, width;
	uchar *s;
	Rune c;
	Fontchar *i;

	s = (uchar*)cs;
	for(; c=*s; p.x+=width){
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
		draw(b, Rect(p.x+i->left, p.y+i->top, p.x+i->left+(i[1].x-i[0].x), p.y+i->bottom),
			color, f->bits, Pt(i->x, i->top));
	}
	return p;
}

Point
strsubfontwidth(Subfont *f, char *cs)
{
	Rune c;
	Point p;
	uchar *s;
	Fontchar *i;
	int w, width;

	p = Pt(0, f->height);
	s = (uchar*)cs;
	for(; c=*s; p.x+=width){
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
