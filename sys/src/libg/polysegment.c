#include <u.h>
#include <libc.h>
#include <libg.h>

void
polysegment(Bitmap *d, int n, Point *pp, int v, Fcode f)
{
	int i, m;
	uchar *buf;
	Point *p1, *p2;

	n--;	/* n is now number of lines to draw */
	while(n > 0){
		p1 = pp;
		p2 = pp+1;
		for(m=0; m<n; m++,p1++,p2++){
			if(m == 2000)
				break;
			if(p2->x-p1->x < -128 || p2->x-p1->x > 127)
				break;
			if(p2->y-p1->y < -128 || p2->y-p1->y > 127)
				break;
		}
		/* m is number of short lines to draw */
		if(m == 0){
			segment(d, pp[0], pp[1], v, f);
			pp++;
			n--;
			continue;
		}
		buf = bneed(16+2*m);
		buf[0] = 'e';
		BPSHORT(buf+1, d->id);
		BPLONG(buf+3, pp[0].x);
		BPLONG(buf+7, pp[0].y);
		buf[11] = v;
		BPSHORT(buf+12, f);
		BPSHORT(buf+14, m);
		buf += 16;
		p2 = pp+1;
		for(i=0; i<m; i++,pp++,p2++){
			*buf++ = p2->x - pp->x;
			*buf++ = p2->y - pp->y;
		}
		n -= m;
	}
}
