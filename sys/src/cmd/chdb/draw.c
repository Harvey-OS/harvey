#include	"ddb.h"

static
struct
{
	char*	sq1;
	char*	sq2;
	long	sidesq;
	int	side;
} w;


void	doline(char*, int, int, int, int);
void	doflood(char*);
void	dofill(char*, int, int);
Bitmap*	dobitmap(char*, char*);
void	dopict(char*);

Bitmap*
draw(short *vec, int side)
{
	int i, j;
	int x1, x2, y1, y2;
	char *p, *q;

	/*
	 * allocate
	 */
	if(w.side != side) {
		w.side = side;
		w.sidesq = side*side;
		if(w.sq1 == 0) {
			w.sq1 = malloc(w.sidesq);
			w.sq2 = malloc(w.sidesq);
		} else {
			w.sq1 = realloc(w.sq1, w.sidesq);
			w.sq2 = realloc(w.sq2, w.sidesq);
		}
		if(!w.sq1 || !w.sq2) {
			fprint(2, "malloc of square failed\n");
			exits("malloc");
		}
	}
	memset(w.sq1, 0, w.sidesq);

	for(i=0; vec[i]!=End; i=j+1) {
		/*
		 * scale the vector
		 * draw the outline in sq2 (value=1)
		 */
		memset(w.sq2, 0, w.sidesq);

		x1 = vec[i+0];
		x1 = (x1*w.side + 256) / 512;
		y1 = 511-vec[i+1];
		y1 = (y1*w.side + 256) / 512;

		for(j=i+2; vec[j]!=End; j+=2) {
			x2 = vec[j+0];
			x2 = (x2*w.side + 256) / 512;
			y2 = 511-vec[j+1];
			y2 = (y2*w.side + 256) / 512;
			doline(w.sq2, x1, y1, x2, y2);
			x1 = x2;
			y1 = y2;
		}

		/*
		 * fill outside outline in sq2 (value=2)
		 */
		doflood(w.sq2);

		/*
		 * xor the boxes together to make piece
		 */
		p = w.sq1;
		q = w.sq2;
		for(i=0; i<w.sidesq; i++) {
			if(*q != 2)
				*p ^= 1;
			p++;
			q++;
		}
	}
	
	return dobitmap(w.sq1, w.sq2);
}

void
doline(char *sq, int x1, int y1, int x2, int y2)
{
	int x, y, ix, iy, dx, dy, side;
	long a1, a2, a3;

	side = w.side;
	x = x1;
	y = y1;

	dx = x2 - x1;
	ix = 1;
	if(dx < 0)
		ix = -1;
	iy = 1;

	dy = y2 - y1;
	if(dy < 0)
		iy = -1;

	for(;;) {

		a1 = x; if(a1 < 0) a1 = 0; if(a1 >= side) a1 = side-1;
		a2 = y; if(a2 < 0) a2 = 0; if(a2 >= side) a2 = side-1;
		sq[a2*side + a1] = 1;

		if(x == x2 && y == y2)
			break;
		a1 = dy * (x2-x-ix) - dx * (y2-y);
		if(a1 < 0)
			a1 = -a1;
		a2 = dy * (x2-x) - dx * (y2-y-iy);
		if(a2 < 0)
			a2 = -a2;
		a3 = dy * (x2-x-ix) - dx * (y2-y-iy);
		if(a3 < 0)
			a3 = -a3;
		if(a1 < a2) {
			x += ix;
			if(a3 <= a1)
				y += iy;
		} else {
			y += iy;
			if(a3 <= a2)
				x += ix;
		}
	}
}

void
doflood(char *sq)
{
	char *p, *q;
	int j, side;

	/*
	 * gross fill
	 */
	side = w.side;
	p = sq;
	for(j=0; j<side; j++) {
		q = memchr(p, 1, side);
		if(q == 0) {
			memset(p, 2, side);
			p += side;
			continue;
		}
		memset(p, 2, q-p);
		p += side;
		q = p;
		for(;;) {
			q--;
			if(*q != 0)
				break;
			*q = 2;
		}
	}

	/*
	 * pick up the knooks
	 */
	p = sq;
	q = p;
	for(;;) {
		q = memchr(q, 0, side - (q-p));
		if(q == 0)
			break;
		if(q[side] == 2)
			dofill(sq, q-p, 0);
		q++;
	}
	p += side;
	for(j=2; j<side; j++) {
		q = p;
		for(;;) {
			q = memchr(q, 0, side - (q-p));
			if(q == 0)
				break;
			if(q[-side] == 2 || q[side] == 2)
				dofill(sq, q-p, j-1);
			q++;
		}
		p += side;
	}
	q = p;
	for(;;) {
		q = memchr(q, 0, side - (q-p));
		if(q == 0)
			break;
		if(q[-side] == 2)
			dofill(sq, q-p, side-1);
		q++;
	}
}

void
dofill(char *sq, int x, int y)
{
	int side;
	long v;

	side = w.side;
	v = y*side + x;
	sq[v] = 2;
	if(x > 0)
		if(sq[v-1] == 0)
			dofill(sq, x-1, y);
	if(x < side-1)
		if(sq[v+1] == 0)
			dofill(sq, x+1, y);
	if(y > 0)
		if(sq[v-side] == 0)
			dofill(sq, x, y-1);
	if(y < side-1)
		if(sq[v+side] == 0)
			dofill(sq, x, y+1);
}

void
dopict(char *sq)
{
	int i, j;
	char x[500];

	for(i=0; i<w.side; i++) {
		for(j=0; j<w.side; j++) {
			x[j] = ' ';
			if(*sq)
				x[j] = '*';
			sq++;
		}
		while(j > 0 && x[j-1] == ' ')
			j--;
		x[j] = 0;
		print("%s\n", x);
	}
	print("\n");
}

Bitmap*
dobitmap(char *p, char *t)
{
	Bitmap *b;
	Rectangle r;
	char *q;
	int i, j, b1, b0;

	r.min.x = 0;
	r.min.y = 0;
	r.max.x = w.side;
	r.max.y = w.side;
	b = balloc(r, 0);
	if(b == 0) {
		fprint(2, "balloc failed\n");
		exits("balloc");
	}

	q = t;
	for(i=0; i<w.side; i++) {
		b0 = 0;
		b1 = 1<<7;
		for(j=0; j<w.side; j++) {
			if(*p != 0)
				b0 |= b1;
			b1 >>= 1;
			if(b1 == 0) {
				*q++ = b0;
				b0 = 0;
				b1 = 1<<7;
			}
			p++;
		}
		if(b1 != (1<<7))
			*q++ = b0;
	}
	wrbitmap(b, 0, w.side, (uchar*)t);
	return b;
}
