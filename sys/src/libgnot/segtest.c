#include <u.h>
#include <libc.h>
#include <libg.h>
#include <gnot.h>

extern int _setdda(int, int, Linedesc*);
int check(GBitmap *, Point, Point, int);
int cleared(GBitmap *, Rectangle);
void clear(GBitmap *);
Bitmap *gtob(GBitmap *);
int dbg;

int
main(int argc, char *argv[])
{
	int iters, ld, simple, i, rx, ry, any;
	Point rmin, rmax, p0, p1, pp0, pp1;
	long seed;
	int good;
	Rectangle clipr, gbr;
	GBitmap *gb;
	Linedesc l;
	char c;

	binit(0, 0, 0);
	ld = 0;
	iters = 1000;
	simple = 0;
	ARGBEGIN {
	case 'i':
		iters = atoi(ARGF());
		break;
	case 's':
		simple = 1;
		break;
	} ARGEND
	if(argc > 0)
		ld = atoi(argv[0]);
	if(simple) {
		rmin = Pt(0,0);
		rmax = Pt(500,500);
	} else {
		seed = time(0);
		print("seed %lux\n", seed);
		srand(seed);
		rmin = Pt(nrand(63)-31,nrand(63)-31);
		rmax = Pt(rmin.x+500,rmin.y+500);
	}
	gbr = Rpt(rmin,rmax);
	gb = gballoc(gbr, ld);
	rmin = add(rmin,Pt(5,5));
	rmax = sub(rmax,Pt(5,5));
	rx = rmax.x - rmin.x;
	ry = rmax.y - rmin.y;
	print("unclipped lines\n");
	good = 0;
	for(i = 0; i < iters; i++) {
		p0 = add(rmin,Pt(nrand(rx),nrand(ry)));
		p1 = add(rmin,Pt(nrand(rx),nrand(ry)));
		pp0 = p0;
		pp1 = p1;
		any = _clipline(gb->r, &pp0, &pp1, &l);
		gsegment(gb, p0, p1, ~0, S);
		if(!check(gb, pp0, pp1, any)) {
			print("%d good\n", good);
			print("failure drawing [%d,%d][%d,%d], closed=[%d,%d][%d,%d]; dx %d dy %d\n",
				p0.x, p0.y, p1.x, p1.y,
				pp0.x, pp0.y, pp1.x, pp1.y,
				p1.x-p0.x, p1.y-p0.y);
			good = 0;
		}else
			good++;
		gsegment(gb, p0, p1, 0, Zero);
	}
	print("%d good\n", good);
	print("clipped lines\n");
	good = 0;
	for(i = 0; i < iters; i++) {
		p0 = add(rmin,Pt(nrand(rx),nrand(ry)));
		p1 = add(rmin,Pt(nrand(rx),nrand(ry)));
		pp0 = p0;
		pp1 = p1;
		clipr = rcanon(Rpt(add(rmin,Pt(nrand(rx),nrand(ry))),
				   add(rmin,Pt(nrand(rx),nrand(ry)))));
		any = _clipline(clipr, &pp0, &pp1, &l);
		gb->r = clipr;
		gsegment(gb, p0, p1, ~0, S);
		gb->r = gbr;
		if(!check(gb, pp0, pp1, any)) {
			print("%d good\n", good);
			print("failure drawing [%d,%d][%d,%d], clip rect [[%d,%d][%d,%d]], clipped,closed=[%d,%d][%d,%d]; dx %d dy %d\n",
				p0.x, p0.y, p1.x, p1.y,
				clipr.min.x, clipr.min.y, clipr.max.x, clipr.max.y,
				pp0.x, pp0.y, pp1.x, pp1.y,
				p1.x-p0.x, p1.y-p0.y);
			good = 0;
		}else
			good++;
		/*
		 * Zero out the line without clipping: see if same pixels touched
		 */
		gsegment(gb, p0, p1, 0, Zero);
		if(!cleared(gb, clipr)) {
			if(good) {
				print("%d good\n", good);
				print("failure to clear [%d,%d][%d,%d], clip rect [[%d,%d][%d,%d]], clipped,closed=[%d,%d][%d,%d]; dx %d dy %d\n",
					p0.x, p0.y, p1.x, p1.y,
					clipr.min.x, clipr.min.y,
					clipr.max.x, clipr.max.y,
					pp0.x, pp0.y, pp1.x, pp1.y,
					p1.x-p0.x, p1.y-p0.y);
				good = 0;
			}
			clear(gb);
			bflush();
			read(0, &c, 1);
		}
	}
	print("%d good\n", good);
}

int
pixset(GBitmap *b, Point p)
{
	uchar *d;
	uchar mask;
	int l;
	ulong v;

	if(!ptinrect(p, b->r))
		return 0;
	d = gbaddr(b, p);
	l = b->ldepth;
	mask = (~0UL)<<(8-(1<<l));
	l = (p.x&(0x7>>l))<<l;
	mask >>= l;

	v = (*d & mask);
	if(dbg)
		print("pixset [%d,%d] -> %lux\n", p.x, p.y, v);
	return v;
}

int
check(GBitmap *b, Point p0, Point p1, int any)
{
	int ans, dirx, diry;

	dirx = (p0.x < p1.x)? 1 : -1;
	diry = (p0.y < p1.y)? 1 : -1;
	ans = (any? pixset(b, p0) : !pixset(b, p0)) &&
		!pixset(b, add(p0, Pt(0, -diry))) &&
		!pixset(b, add(p0, Pt(-dirx, 0))) &&
		!pixset(b, add(p0, Pt(-dirx, -diry))) &&
	      (any? pixset(b, p1) : !pixset(b, p1)) &&
		!pixset(b, add(p1, Pt(0, diry))) &&
		!pixset(b, add(p1, Pt(dirx, 0))) &&
		!pixset(b, add(p1, Pt(dirx, diry)));
	if(!ans) {
		dbg = 1;
		pixset(b, p0);
		pixset(b, add(p0, Pt(0, -diry)));
		pixset(b, add(p0, Pt(-dirx, 0)));
		pixset(b, add(p0, Pt(-dirx, -diry)));
		pixset(b, p1);
		pixset(b, add(p1, Pt(0, diry)));
		pixset(b, add(p1, Pt(dirx, 0)));
		pixset(b, add(p1, Pt(dirx, diry)));
		dbg = 0;
	}
	return ans;
}

long
bwords(GBitmap *b)
{
	long lsize;
	int wsize;

	wsize = 1<<(5-b->ldepth);
	if(b->r.max.x >= 0)
		lsize = (b->r.max.x+wsize-1)/wsize;
	else
		lsize = b->r.max.x/wsize;
	if(b->r.min.x >= 0)
		lsize -= b->r.min.x/wsize;
	else
		lsize -= (b->r.min.x-wsize+1)/wsize;
	lsize *= Dy(b->r);
	return lsize;
}

int
cleared(GBitmap *b, Rectangle r)
{
	ulong *p, *pe;
	ulong n;
	int i, j;
	Bitmap *bb;

	p = b->base;
	n = bwords(b);
	pe = b->base + n;
	while(p < pe)
		if(*p++){
			--p;
			for(i=b->r.min.x; i<b->r.max.x; i++)
			for(j=b->r.min.y; j<b->r.max.y; j++)
				if(gaddr(b, Pt(i, j)) == p){
					print("bad pixel [%d,%d] %.8lux\n", i, j, *p);
					bb = gtob(b);
					bitblt(&screen, bb->r.min, bb, bb->r, S);
					border(&screen, inset(r, -5), 5, ~D);
					return 0;
				}
			return 0;
		}
	return 1;
}

void
clear(GBitmap *b)
{
	memset(b->base, 0, bwords(b)*sizeof(long));
}

Bitmap*
gtob(GBitmap *g)
{
	Bitmap *b;
	uchar *u;
	int y;

	b = balloc(g->r, g->ldepth);
	if(b == 0){
		print("can't balloc %R\n", g->r);
		exits(0);
	}
	u = (uchar*)gaddr(g, g->r.min);
	u += (g->r.min.x&31)/8;
	for(y=g->r.min.y; y<g->r.max.y; y++, u+=sizeof(long)*g->width)
		wrbitmap(b, y, y+1, u);
	return b;
}
