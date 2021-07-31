#include <libg.h>

typedef unsigned short ushort;

long
strwidth(Font *f, char *s)
{
	int wid, twid, n;
	enum { Max = 128 };
	ushort cbuf[Max];

	twid = 0;
	while(*s){
		n = 0;
		while(cachechars(f, &s, cbuf, Max, &wid) <= 0)
			if(++n > 10)
				berror("strwidth");
		agefont(f);
		twid += wid;
	}
	return twid;
}

Point
strsize(Font *f, char *s)
{
	return Pt(strwidth(f, s), f->height);
}
