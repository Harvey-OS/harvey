#include <u.h>
#include <libc.h>
#include <libg.h>
#include <gnot.h>

Point
gsubfstrsize(GSubfont *f, char *cs)
{
	int i, l, n;
	Fontchar *info;
	uchar *s;
	Rune c;

	s = (uchar*)cs;
	l = 0;
	n = f->n;
	info = f->info;
	if(s)
		while(c = *s){
			if(c < Runeself)
				s++;
			else{
				i = chartorune(&c, (char*)s);
				if(i == 0){
					s++;
					continue;
				}
				s += i;
			}
			if(c < n)
				l += info[c].width;
		}
	return Pt(l, f->height);
}

long
gsubfstrwidth(GSubfont *f, char *s)
{
	Point p;

	p = gsubfstrsize(f,s);
	return p.x;
}
