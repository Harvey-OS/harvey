#include	"type.h"

#define	MDIST	(150*MILE)
#define	VDIST	(50*MILE)
#define	GDIST	(25*MILE)
#define	NDIST	(70*MILE)
#define	BDIST	(1*MILE)
static	Nav	*npp;

void
nvinit(void)
{

	npp = &nav[0];
	plane.v1 = NNULL;
	plane.g1 = NNULL;
	plane.v2 = NNULL;
	plane.g2 = NNULL;
	plane.ad = NNULL;
	plane.bk = NNULL;
}

/*
 * tune the radios
 * look for near stations
 */
void
nvupdat(void)
{
	Nav *np;
	int f, q, n;
	long d1, d2;

	n = 0;
	np = npp;

loop:
	n++;
	if(n >= 200) {
		npp = np;
		return;
	}
	np++;
	if(np->z == 0) {
		if(plane.v1 != NNULL)
			if(plane.v1->freq != plane.vor1)
				plane.v1 = NNULL;
		if(plane.v2 != NNULL)
			if(plane.v2->freq != plane.vor2)
				plane.v2 = NNULL;
		if(plane.g1 != NNULL)
			if(plane.g1->freq != plane.vor1)
				plane.g1 = NNULL;
		if(plane.g2 != NNULL)
			if(plane.g2->freq != plane.vor2)
				plane.g2 = NNULL;
		if(plane.ad != NNULL)
			if(plane.ad->freq != plane.adf)
				plane.ad = NNULL;
		np = &nav[0];
	}

	q = np->freq;
	if(q != plane.vor1 &&
		q != plane.vor2 &&
		q != plane.adf &&
		q != 75)
			goto loop;
	f = np->flag;

	d1 = np->x - plane.x;
	if(d1 < 0)
		d1 = -d1;
	d2 = np->y - plane.y;
	if(d2 < 0)
		d2 = -d2;
	if(d2 > d1)
		d1 = d2 + d1/2;
	else
		d1 += d2/2;
/* botch d2 set, not used here
	if(d2 < MDIST)
		d2 = fdist(np->x, np->y);
 */


	if(q == plane.vor1) {
		while(f & (VOR|ILS)) {
			if(((f & VOR) && d1 > VDIST) ||
			   ((f & ILS) && d1 > GDIST)) {
				if(np == plane.v1)
					plane.v1 = NNULL;
				break;
			}
			if(plane.v1 == NNULL)
				plane.v1 = np;
			if(np == plane.v1)
				break;
			d2 = fdist(plane.v1->x, plane.v1->y);
			if(d1 < d2)
				plane.v1 = np;
			break;
		}
		while(f & GS) {
			if(d1 > GDIST) {
				if(np == plane.g1)
					plane.g1 = NNULL;
				break;
			}
			if(plane.g1 == NNULL)
				plane.g1 = np;
			if(np == plane.g1)
				break;
			d2 = fdist(plane.g1->x, plane.g1->y);
			if(d1 < d2)
				plane.g1 = np;
			break;
		}
	}

	if(q == plane.vor2) {
		while(f & (VOR|ILS)) {
			if(((f & VOR) && d1 > VDIST) ||
			   ((f & ILS) && d1 > GDIST)) {
				if(np == plane.v2)
					plane.v2 = NNULL;
				break;
			}
			if(plane.v2 == NNULL)
				plane.v2 = np;
			if(np == plane.v2)
				break;
			d2 = fdist(plane.v2->x, plane.v2->y);
			if(d1 < d2)
				plane.v2 = np;
			break;
		}
		while(f & GS) {
			if(d1 > GDIST) {
				if(np == plane.g2)
					plane.g2 = NNULL;
				break;
			}
			if(plane.g2 == NNULL)
				plane.g2 = np;
			if(np == plane.g2)
				break;
			d2 = fdist(plane.g2->x, plane.g2->y);
			if(d1 < d2)
				plane.g2 = np;
			break;
		}
	}

	if(q == plane.adf)
	while(f & NDB) {
		if(d1 > NDIST) {
			if(np == plane.ad)
				plane.ad = NNULL;
			break;
		}
		if(plane.ad == NNULL)
			plane.ad = np;
		if(np == plane.ad)
			break;
		d2 = fdist(plane.ad->x, plane.ad->y);
		if(d1 < d2)
			plane.ad = np;
		break;
	}

	if(q == 75)
	while(f & (OM|MM|IM)) {
		if(d1 > BDIST) {
			if(np == plane.bk)
				plane.bk = NNULL;
			break;
		}
		if(plane.bk == NNULL)
			plane.bk = np;
		if(np == plane.bk)
			break;
		d2 = fdist(plane.bk->x, plane.bk->y);
		if(d1 < d2)
			plane.bk = np;
		break;
	}
	goto loop;
}
