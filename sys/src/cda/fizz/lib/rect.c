#include <u.h>
#include	<libc.h>
#include	<cda/fizz.h>

typedef struct Rlist
{
	Chip *c;
	struct Rlist *next;
} Rlist;
static Rlist *pool;
#ifndef	BSIZE
#define		BSIZE		10
#endif
#define		N		(1<<(16-BSIZE))
#define		B(x)		(((x)>>BSIZE)&(N-1))
static Rlist *rhash[N][N];
static radded, rbkts;

void rectfree(Rlist *r)
{
	r->next = pool;
	pool = r;
}

Rlist *
rectmalloc(Chip *c, Rlist *n)
{
	register Rlist *r;

	if(r = pool)
		pool = pool->next;
	else
		r = NEW(Rlist);
	r->c = c;
	r->next = n;
	return(r);
}

void
rectinit(void)
{
	register Rlist *s, *ss;
	register x, y;

	for(x = 0; x < N; x++)
		for(y = 0; y < N; y++){
			for(s = rhash[x][y]; s; s = ss){
				ss = s->next;
				rectfree(s);
			}
			rhash[x][y] = 0;
		}
	radded = 0;
	rbkts = 0;
}

Chip *
rectadd(Chip *c)
{
	register x, y;
	int x0, y0, x1, y1;
	register Rlist *s;

	if(c->flags&IGRECT)
		return(0);
	x0 = B(c->r.min.x);
	y0 = B(c->r.min.y);
	x1 = B((c->r.max.x-1));
	y1 = B((c->r.max.y-1));

	rbkts += (x1-x0+1)*(y1-y0+1);
	for(x = x0; x <= x1; x++)
		for(y = y0; y <= y1; y++)
			for(s = rhash[x][y]; s; s = s->next)
				if(((s->c->flags&IGRECT) == 0) && fg_rXr(s->c->r, c->r))
					if((s->c->flags&WSIDE) == (c->flags&WSIDE))
						return(s->c);
	for(x = x0; x <= x1; x++)
		for(y = y0; y <= y1; y++)
			rhash[x][y] = rectmalloc(c, rhash[x][y]);
	radded++;
	return((Chip *)0);
}

void
rectdel(Chip *c)
{
	register x, y;
	int x0, y0, x1, y1;
	register Rlist *s, *ss;

	x0 = B(c->r.min.x);
	y0 = B(c->r.min.y);
	x1 = B((c->r.max.x-1));
	y1 = B((c->r.max.y-1));

	for(x = x0; x <= x1; x++)
		for(y = y0; y <= y1; y++){
			for(s = rhash[x][y], ss = 0; s; ss = s, s = s->next)
				if(s->c == c)
					break;
			if(s){
				if(ss)
					ss->next = s->next;
				else
					rhash[x][y] = s->next;
				rectfree(s);
			}
		}
}

#define		POOT		1000

void
rectstat(void)
{
	int cnt[POOT];
	register Rlist *s;
	register x, y, n, nn, nnn;

	for(x = 0; x < POOT; x++)
		cnt[x] = 0;
	for(x = 0; x < N; x++)
		for(y = 0; y < N; y++){
			for(n = 0, s = rhash[x][y]; s; s = s->next)
				n++;
			cnt[n]++;
		}
	print("BSIZE=%d\n", BSIZE);
	for(nn = nnn = x = 0; x < POOT; x++)
		if(cnt[x]){
			print("%d: %d\n", x, cnt[x]);
			if(x) nnn += cnt[x];
			nn += cnt[x]*x;
		}
	if(radded == 0)
		print("no rects!\n");
	else{
		print("%d rects, %.2f ave\n", radded, nn/(float)nnn);
		print("%d bkts, %.2f ave\n", rbkts, rbkts/(float)radded);
		print("ave*ave = %.2f\n", (nn*rbkts)/(float)(nnn*radded));
	}
}
