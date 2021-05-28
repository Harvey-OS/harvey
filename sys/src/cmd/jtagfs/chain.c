#include <u.h>
#include <libc.h>
#include <bio.h>
#include "lebo.h"
#include "bebo.h"
#include "debug.h"
#include "chain.h"

/* functions to deal with bit chains, weird bit orderings an such */

#define Upper(byte, nbits)	((byte)>>(nbits))
#define Lower(byte, nbits)	((bytes)&MSK(nbits))
#define IsCut(bbits, ebits)		(((ebits)/8 - (bbits)/8) > 0)

/* 8: H L | 8: Next */
/* Put at most 8 bits*/
static void
put8bits(Chain *ch, void *p, int nbits)
{
	int e, nbye, nbie, nle;
	uchar low, high, byte;

	byte = *(uchar *)p;
	e = ch->e+nbits-1;	/* offset of last bit to put in */
	nbie = ch->e%8;
	nbye = ch->e/8;
	nle = 8 - nbie;


	if(IsCut(ch->e, e))
		ch->buf[nbye + 1] = byte >> nle;

	high = (byte & MSK(nle)) << nbie;
	low = ch->buf[nbye]&MSK(nbie);


	ch->buf[nbye] = low|high;
	ch->e += nbits;
}

/* Get at most 8 bits*/
static uchar
get8bits(Chain *ch, int nbits)
{
	int b, nbyb, nbib, nlb;
	uchar low, high, res;

	b = ch->b+nbits-1;
	nbib = ch->b % 8;
	nbyb = ch->b / 8;
	nlb = 8 - nbib;
	if(nlb > nbits)
		nlb = nbits;

	low = MSK(nlb) & (ch->buf[nbyb] >> nbib);
	if(IsCut(ch->b, b))
		high = (ch->buf[nbyb + 1] & MSK(nbib)) << nlb;
	else
		high = 0;
	res = MSK(nbits)&(high | low);
	ch->b += nbits;
	return res;
}

/*
 * Putbits and getbits could be made more efficient
 * simply by using memmove in the special case where
 * a bunch of bytes is aligned, but I don't care.
 * Packing in words would also help the so inclined.
 */
void
putbits(Chain *ch, void *p, int nbits)
{
	int nby, nbi, i;
	uchar *vp;

	vp = p;
	nby = nbits / 8;
	nbi = nbits % 8;

	for(i = 0; i < nby; i++)
		put8bits(ch, vp++, 8);
	
	if(nbi != 0)
		put8bits(ch, vp, nbi);
}

void
getbits(void *p, Chain *ch, int nbits)
{
	int nby, nbi, i;
	uchar *vp;

	assert(ch->e >= ch->b);
	nby = nbits / 8;
	nbi = nbits % 8;

	vp = p;
	for(i = 0; i < nby; i++)
		*vp++ = get8bits(ch, 8);
	
	if(nbi != 0)
		*vp = get8bits(ch, nbi);
}

static void
revbych(Chain *ch)
{
	int i, nb;

	nb = (ch->e-ch->b+7)/8;

	for(i = 0; i < nb; i++)
		ch->buf[i] = rtab[ch->buf[i]];
}

void
printchain(Chain *ch)
{
	int i, ng, nb;
	uchar msk, c;
	Chain *ch2;

	fprint(2, "chain buf:%#p b:%d e:%d\n", ch->buf, ch->b, ch->e);
	ch2 = malloc(sizeof(Chain));
	if(ch2 == nil)
		sysfatal("no memory");
	memmove(ch2, ch, sizeof(Chain));


	ng = 8;
	nb = (ch2->e-ch2->b+7)/8;

	for(i = 0; i < nb; i++){
		if(ch2->e - ch2->b < ng)
			ng = ch2->e - ch2->b;
		getbits(&c, ch2, ng);
		if(i != 0 && i%15 == 0)	
			fprint(2, "\n");
		if(ng < 8){
			msk = MSK(ng);
			fprint(2, "%#2.2x ", c&msk);
		}
		else
			fprint(2, "%#2.2x ", c);
		
	}
	if(i%16 != 0)
		fprint(2, "\n");

	free(ch2);
}

/* Good for debugging */
static void
printchslice(Chain *ch, int b, int e)
{
	Chain *ch2;

	fprint(2, "Slice [%d, %d], b:%d e:%d\n", b, e, ch->b, ch->e);
	ch2 = malloc(sizeof(Chain));
	if(ch2 == nil)
		sysfatal("memory");
	memmove(ch2, ch, sizeof(Chain));
	if(b > e){
		fprint(2, "bad args in region\n");
		return;
	}
	if(b < ch2->b || b > ch2->e || e > ch2-> e || e < ch2->b){
		fprint(2, "bad region\n");
		return;
	}
	ch2->b = b;
	ch2->e = e;

	fprint(2, "Slice - ");
	printchain(ch2);
	free(ch2);
}


static u32int
revbytes(u32int *v)
{
	u32int rv;
	uchar *a, *b;

	a = (uchar *)v;
	b = (uchar *)&rv;

	b[3] = rtab[a[3]];
	b[2] = rtab[a[2]];
	b[1] = rtab[a[1]];
	b[0] = rtab[a[0]];

	return rv;
}

u32int
hmsbputl(u32int *v)
{
	u32int rv, bev;

	hbeputl(&bev, *v);
	rv = revbytes(&bev);

	return rv;
}

u32int
msbhgetl(u32int *v)
{
	u32int rv, rev;

	rev = revbytes(v);
	rv = lehgetl(&rev);
	return rv;
}

