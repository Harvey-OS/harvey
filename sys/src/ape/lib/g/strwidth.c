#include <libg.h>

Point
strsize(Font *f, char *cs)
{
	int c, l, n;
	Fontchar *info;
	unsigned char *s;

	s = (unsigned char*)cs;
	l = 0;
	n = f->n;
	info = f->info;
	if(s)
		while(c = *s++)
			if(c < n)
				l += info[c].width;
	return Pt(l,f->height);
}

long
strwidth(Font *f, char *s)
{
	return strsize(f,s).x;
}
