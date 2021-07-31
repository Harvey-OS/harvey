#include <u.h>
#include <libc.h>
#include <libg.h>
#include <gnot.h>

Point
gsubfstring(GBitmap *b, Point p, GSubfont *f, char *cs, Fcode fc)
{
	int w, width;
	int full;
	uchar *s;
	Rectangle r;
	Rune c;
	Fontchar *i;

	s = (uchar*)cs;
	fc &= F;
	full = fc==S || fc==notS;	/* for reverse-video */
	if(full){
		r.min.y = 0;
		r.max.y = f->height;
	}
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
		if(!full){
			r.min.y = i->top;
			r.max.y = i->bottom;
		}
		r.min.x = i[0].x;
		r.max.x = i[1].x;
		gbitblt(b, Pt(p.x+i->left, p.y+r.min.y), f->bits, r, fc);
	}
	return p;
}
