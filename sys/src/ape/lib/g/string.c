#include <libg.h>

typedef unsigned char uchar;
typedef unsigned short ushort;

Point
string(Bitmap *d, Point p, Font *f, char *s, Fcode fc)
{
	int n, wid;
	uchar *b;
	enum { Max = 128 };
	ushort cbuf[Max], *c, *ec;

	while(*s){
		n = cachechars(f, &s, cbuf, Max, &wid);
		b = bneed(17+2*n);
		b[0] = 's';
		BPSHORT(b+1, d->id);
		BPLONG(b+3, p.x);
		BPLONG(b+7, p.y);
		BPSHORT(b+11, f->id);
		BPSHORT(b+13, fc);
		BPSHORT(b+15, n);
		b += 17;
		ec = &cbuf[n];
		for(c=cbuf; c<ec; c++){
			BPSHORT(b, *c);
			b += 2;
		}
		p.x += wid;
		agefont(f);
	}
	return p;
}
