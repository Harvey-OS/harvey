#include <u.h>
#include <libc.h>
#include <stdio.h>
#include "dat.h"
#include "fns.h"
#include "y.tab.h"

Minterm* and(int, Minterm *, Minterm * , Minterm *);
void min_cpy(Minterm *, Minterm *, int);
Minterm * reduce(Minterm*, Minterm*, int, Minterm*, int oput);

Minterm *
comterm(Minterm * mt0, Minterm * mt1) 
{
	int care, true;
	unsigned int i;
	care = mt1->care;
	true = mt1->true;
	for(i = 1; care; i <<= 1) 
		if(i & care) {
			mt0->true = i ^ true;
			mt0->care = i;
			mt0++;
			care &= ~i;
		}
	return(mt0);
}
	
Minterm *
invert(Minterm *start, Minterm *end, Minterm *out, int mask)
{
	Minterm *mt0, *mta, *mtb;
	min_cpy(out, end, end - start);
	mt0 = out - (end - start);
	mta = comterm(end, mt0++);
	while(mt0 < out) {
		mtb = comterm(mta, mt0++);
		mta = and(0, end, mta, mtb);
		if((mta - end) > 1000) {
			mta = reduce(end, mta, mask, mt0, 0);
			if((mta - end) > 1000) {
				fprintf(stderr, "invert:too many terms\n");
				exits("error");
			}
		}
		if(mta > mt0) {
			fprintf(stderr, "out of minterms");
			exits("error");
		}
	}
	return(mta);
}

Minterm *
xor_mt(Minterm *start, Minterm *mid, Minterm *end, int bit)
{
	Minterm *mt0, *mt;
	for(mt0 = mt = start; mt < mid; ++mt) {
		if((bit & mt->care) == 0) {
			mt0->care = mt->care | bit;
			mt0->true = mt->true & ~bit;
			++mt0;
		}
		else if((bit & mt->true) == 0) {
			mt0->care = mt->care;
			mt0->true = mt->true;
			++mt0;
		}
	}
	for(; mt < end; ++mt) {
		if((bit & mt->care) == 0) {
			mt0->care = mt->care | bit;
			mt0->true = mt->true | bit;
			++mt0;
		}
		else if(bit & mt->true) {
			mt0->care = mt->care;
			mt0->true = mt->true;
			++mt0;
		}
	}
	return(mt0);
}
