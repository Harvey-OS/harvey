#include <string.h>
#include <libg.h>

Point
string(Bitmap *d, Point p, Font *ft, char *s, Fcode f)
{
	unsigned char *buf;
	char *q;
	int n;

	while(*s){
		buf = bneed(15+1024);
		buf[0] = 's';
		BPSHORT(buf+1, d->id);
		BPLONG(buf+7, p.y);
		BPSHORT(buf+11, ft->id);
		BPSHORT(buf+13, f);
		BPLONG(buf+3, p.x);
		q = memchr(s, 0, 1024);
		if(q){
			n = q - s;
			bneed((n+1)-1024);	/* trim */
		}else
			n = 1023;
		memmove(buf+15, s, n);
		buf[15+n] = 0;
		p.x += strwidth(ft, (char*)buf+15);
		s += n;
	}
	return p;
}
