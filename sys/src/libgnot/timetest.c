#include	<u.h>
#include	<libc.h>
#include	<libg.h>
#include	"/sys/include/gnot.h"

int
main(int argc, char *argv[])
{
	GBitmap *b;
	int h,v,iters,i,op,fac;
	Rectangle r;
	Point p;

	v = h = 100;
	fac = 1;
	op = S;
	if(argc > 1)
		v = atoi(argv[1]);
	if(argc > 2)
		h = atoi(argv[2]);
	if(argc > 3)
		op = atoi(argv[3]);
	if(argc > 4)
		fac = atoi(argv[4]);
	b = gballoc(Rect(0,0,1024,1024),0);
	r = Rect(48,33,48+h,33+v);
	iters = fac * 100000/(((h+31)/32) * v);
	print("h=%d v=%d op=%d nblits=%d\n", h, v, op, iters*32);
	while(--iters >= 0) {
		p = Pt(31,33);
		for(i=32; --i>=0;){
			gbitblt(b, p, b, r, op);
			p.x--;
		}
	}
}
