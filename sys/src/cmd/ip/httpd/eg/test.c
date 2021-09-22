#include	<u.h>
#include	<libc.h>
#include	"gen.h"

void	wsearch1(int);
void	bsearch2(int);
void	wsearch3(int);
void	bsearch4(int);
void	wsearch5(int);
void	wsearch(int);
void	bsearch(int);
typedef	struct	Hash	Hash;
struct	Hash
{
	ushort	w1, b1, w2, b2;
	uchar	ep;
	long	count;
};

ushort	m1, m2, m3, m4;
Hash	hash[100129];
long	count;
vlong	vcount;
int	hits;

// depth: 0   count: 1
// depth: 1   count: 20
// depth: 2   count: 400
// depth: 3   count: 8902
// depth: 4   count: 197281
// depth: 5   count: 4865609 seconds: 15 4.13r
// depth: 6   count: 119060324 seconds: 190 93.34r
// depth: 7   count: 3195901860 seconds: 2717 2542.91r
// depth: 8   count: 84998978956 seconds: 49537
// depth: 9   count: 2439530234167 seconds: 1321218

void
main(int argc, char *argv[])
{
	int d;

	d = 5;
	ARGBEGIN {
	default:
		fprint(2, "usage: a.out\n");
		exits(0);
	case 'd':
		d = atoi(ARGF());
		break;
	} ARGEND

	if(argc > 0)
		d = atoi(argv[0]);

	ginit(&initp);
	if(d < 4) {
		count = 0;
		wsearch(d);
		vcount = count;
	} else
		wsearch1(d);
	print("count = %lld; hits %d\n", vcount, hits);
	exits(0);
}

void
wsearch1(int d)
{
	short mv[100];
	int j;

	lmp = mv;
	wgen();
	*lmp = 0;
	for(j=0; m1=mv[j]; j++) {
		wmove(m1);
		if(battack(wkpos))
			bsearch2(d);
		wremove();
	}
}

void
bsearch2(int d)
{
	short mv[100];
	int j;

	lmp = mv;
	bgen();
	*lmp = 0;
	for(j=0; m2=mv[j]; j++) {
		bmove(m2);
		if(wattack(bkpos))
			wsearch3(d);
		bremove();
	}
}

void
wsearch3(int d)
{
	short mv[100];
	int j;

	lmp = mv;
	wgen();
	*lmp = 0;
	for(j=0; m3=mv[j]; j++) {
		wmove(m3);
		if(battack(wkpos))
			bsearch4(d);
		wremove();
	}
}

void
bsearch4(int d)
{
	short mv[100];
	int j;

	lmp = mv;
	bgen();
	*lmp = 0;
	for(j=0; m4=mv[j]; j++) {
		bmove(m4);
		if(wattack(bkpos))
			wsearch5(d);
		bremove();
	}
}

void
wsearch5(int d)
{
	ulong h;
	Hash *p;
	int w1, w2, b1, b2;
	int ep;

	w1 = m1;
	w2 = m3;
	if(w1 > w2) {
		w1 = w2;
		w2 = m1;
	}
	b1 = m2;
	b2 = m4;
	if(b1 > b2) {
		b1 = b2;
		b2 = m2;
	}

	ep = eppos;
	if(ep != 0 && board[ep-1] == 0 && board[ep+1] == 0)
		ep = 0;

	h = w1*7 + b1*281 + w2*1069 + b2*3011;
	p = &hash[h%nelem(hash)];
	if(p->w1 == w1 && p->b1 == b1 && p->w2 == w2 && p->b2 == b2)
	if((w1^b1) & 077 && (w2^b1) & 077 && (w1^b2) & 077 && (w2^b2) & 077)
	if(ep == p->ep) {
		hits++;
		vcount += p->count;
		return;
	}
	count = 0;
	wsearch(d-4);
	p->w1 = w1;
	p->b1 = b1;
	p->w2 = w2;
	p->b2 = b2;
	p->ep = ep;
	p->count = count;
	vcount += p->count;
}

void
wsearch(int d)
{
	short mv[100];
	int j, m;

	if(d == 0) {
		count++;
		return;
	}
	lmp = mv;
	wgen();
	*lmp = 0;
	for(j=0; m=mv[j]; j++) {
		wmove(m);
		if(battack(wkpos))
			bsearch(d-1);
		wremove();
	}
}

void
bsearch(int d)
{
	short mv[100];
	int j, m;

	if(d == 0) {
		count++;
		return;
	}
	lmp = mv;
	bgen();
	*lmp = 0;
	for(j=0; m=mv[j]; j++) {
		bmove(m);
		if(wattack(bkpos))
			wsearch(d-1);
		bremove();
	}
}
