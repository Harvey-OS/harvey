#include <u.h>
#include <libc.h>
#include <stdio.h>
#include "dat.h"
#include "fns.h"
extern int qflag;

int min_sort(Minterm *, Minterm *, int);
int min_cov(Minterm *, Minterm *, int);
void min_cpy(Minterm *, Minterm *, int);
int is_cov(int, int, Minterm*, Minterm*);
#ifdef PLAN9
cmpcare(Minterm *mt1, Minterm *mt2);
#endif
#ifndef PLAN9
cmpcare(void *mt1, void *mt2);
#endif

#define COVERED 2
#define DELETE 4
#define SKIP 4
extern int allprimes;

char *
getdep(unsigned int l)
{
	int i;
	for(i = 0; l ; l >>= 1, i += 1);
	return(depends[i]->name);
}

Minterm *
cover(Minterm * start, Minterm * end, Minterm * out)
{
	Minterm *mt1;
	qsort(start, end-start, sizeof(Minterm), cmpcare);
	for(mt1 = start; mt1 < end; ++mt1) {
		if(mt1->type & DELETE) continue;
		mt1->type |= DELETE;
		if(!is_cov(mt1->care, mt1->true, start, end)) {
			mt1->type &= ~DELETE;
		}
	}
	for(mt1 = start; mt1 < end; ++mt1)
		if(!(mt1->type & DELETE))
			*out++ = *mt1;
	return(out);
}
Minterm *
doimpq(Minterm *start, Minterm *end, int mask, int lvl, Minterm *out)
{
	USED(start, end, mask, lvl, out);
	return((Minterm *) 0 );
}

Minterm *
reduce(Minterm* start, Minterm* end, int mask, Minterm* out)
{
	Minterm *mt;
	for(mt = start; mt < end; ++mt) mt->type = 0;
	if(qflag) {
		mt = doimpq(start, end, mask,
			1, out);
	}
	else
		mt = doimp(start, end, mask, 1, out);
	return(cover(start, mt, start));
}

#define SWAP(x,y) {tmp = *(x); *(x) = *(y); *(y) = tmp;}

Minterm *
doimp(Minterm *start, Minterm *end, int mask, int lvl, Minterm *out)
{
	int carea, truea, careb, trueb, l, trueab, careab, msk;
	Minterm *zp, *op, *dp;
	Minterm *p, *a, *b;
	Minterm tmp;
	int f;

/*	int typea;

fprintf(stderr, "doimp %x - %x = %d %x %x\n", start, end, end-start, mask, lvl);
printmt(start, end); 
*/
	for(msk = mask; msk;  msk &= ~l) {
		l = msk & -msk;
		f = 0;
		if(l < lvl)  continue;
		end = cover(start, end, start);
/*
fprintf(stderr, "	 %x - %x = %d %x %x %x\n", start, end, end-start, l, lvl, oput);
printmt(start, end); 
*/
		p = end;
		zp = op = start;
		dp = end-1;
		while(op <= dp) {
			if(!(op->care & l)) {
				while(!(dp->care & l)) {
					--dp;
					if(op > dp) break;
				}
				if(op > dp) break;
				SWAP(op, dp);
				dp--;
			}
			if(zp->true & l) {
				if(!(op->true & l)) SWAP(zp, op)
				else {
					++op;
					continue;
				}
			}
			op++;
			zp++;
	 	}
		dp = op;
		op = zp;
		zp = start;
		careab = 0;
		for(a=zp; a<op; a++) {
			carea = a->care & ~l;
			truea = a->true & ~l;
/*			typea = a->type & COVERED; */
			for(b = op; b < dp; ++b) {
				if(b->type & DELETE) continue;
				careb = b->care & ~l;
				trueb = b->true & ~l;
				trueab = carea & careb & truea;
				if(trueab != (carea & careb & trueb))
					continue;
				trueab |= carea & ~careb & truea;
				trueab |= ~carea & careb & trueb;
				if(p < out) {
					p->true = trueab;
					p->care = careab = (carea | careb);
/*					if(typea & b->type) 
						p->type = COVERED;
					else { */
						p->type = 0;
						f = 1;
		/*			} */
					++p;
				}
				else {
					fprintf(stderr, "NCOMP to small\n");
					exits("error");;
				}
				if(careb == careab) 
					b->type |= DELETE;
				if(carea == careab) {
					a->type |= DELETE;
					break;
				}
			}
		}		
		if(f) {
			p = doimp(dp, p,
				mask & ~l, l<<1, out); 
			end = cover(start, p, start);
		}
			
	}
/*	for(a=start; a<end; a++)
		a->type |= COVERED; */
	return(end);
}


#ifdef PLAN9
int
cmpcare(Minterm *mt1, Minterm *mt2)
#endif
#ifndef PLAN9
int
cmpcare(void *mmt1, void *mmt2)
#endif
{
#ifndef PLAN9
	Minterm *mt1, *mt2;
#endif
	unsigned int i, care;
#ifndef PLAN9
	mt1 = (Minterm *) mmt1;	
	mt2 = (Minterm *) mmt2;	
#endif
	for(care = mt2->care, i = 0; care; care >>= 1)
		if(care & 1) ++i;
	for(care = mt1->care; care; care >>= 1)
		if(care & 1) --i;
	return(i);
}

int
is_cov(int care, int true, Minterm * mt1, Minterm * mt2)
{
	Minterm *mt;
	int care1, true1, diff, bit;
	for(mt = mt2-1; mt >= mt1; mt--) {
		if(mt->type & DELETE) continue;
		care1 = mt->care;
		true1 = mt->true;
		if(care & care1 & (true ^ true1)) continue;
		diff = care1  & ~care;
		if(allprimes) {
			if(diff)
				continue;
			else
				return(1);
		}
		for(;diff; diff &= ~bit) {
			bit = diff & -diff;
			if(!is_cov(care | bit, true | (bit & ~true1), mt1, mt))
					return(0);}
		return(1);
	}
	return(0);
}


void
min_cpy(Minterm *a, Minterm *b, int n)
{
	while(n--) *(--a) = *(--b);
}
