#include <u.h>
#include <libc.h>
#include <stdio.h>
#include "dat.h"
#include "fns.h"
#include "y.tab.h"

Minterm * invert(Minterm *, Minterm *, Minterm *, int);
void printes(int, Minterm *, Minterm *);
void reorder(Minterm*, Minterm*);
int xlate[32];
extern int logicflag, espresso, invflag;
Minterm * reduce(Minterm*, Minterm*, int, Minterm*);
void pout(char *,  int, Minterm *, Minterm *, unsigned int);
Minterm *xor_mt(Minterm *, Minterm *, Minterm *, int);

int
get_mask(Minterm * start, Minterm * end)
{
	unsigned int mask;
	Minterm *mt;
	mask = 0;
	for(mt = start; mt < end; ++mt)
		mask |= mt->care;
	return(mask);
}

void
out(Node *tp)
{
	int i, j, o, mask;
	Minterm *mt1, *mt2;
	fflush(stderr);
	if (treeflag) {
		treeprint(tp);
		exits("");
	}
	if(espresso) {
		fprintf(stdout, ".i %d\n.o %d\n.type f\n", nleaves, noutputs);
	}
	else {
		if(chipname)
			fprintf(stdout, ".x %s\n", chipname->name);
		Ceval(tp);
	}
	mt2 = 0;
	for(o=1; o<=noutputs; o++) {
		ndepends = 0;
		mt1 = compile(o);
		if(!mt1)
			continue; 
if(yydebug) {
	printmt(minterms, mt1);
}
		mask = get_mask(minterms, mt1);
		if(espresso > 0) {
			printes(o, minterms, mt1);
			continue;
		} 
		mt1 = reduce(minterms, mt1, mask, &minterms[NCOMP]);
		mask = get_mask(minterms, mt1);
		pout(".o", o, minterms, mt1, mask);
		if(logicflag > 0) {
			fprintf(stderr, "%s = ", outputs[o]->name);
			printmt(minterms, mt1);
		} 
		if(invflag | xorflag) {
			mt2 = invert(minterms, mt1, &minterms[NCOMP], mask);
			mt2 = reduce(mt1, mt2, mask, &minterms[NCOMP]);
			if(invflag) pout(".I", o, mt1, mt2, mask);
			if(logicflag > 0) {
				fprintf(stderr, "~%s = ", outputs[o]->name);
				printmt(mt1, mt2);
			}
		}
		if(xorflag) {
			for(j = 1, i = 1; j <= ndepends; ++j, i <<= 1)
				if(depends[j] == outputs[o])
					break;
			if(mask & i) {
				mt2 = xor_mt(minterms, mt1, mt2, i);
				mt2 = reduce(minterms, mt2, mask, &minterms[NCOMP]);
				pout(".X", o, minterms, mt2, get_mask( minterms, mt2));
				if(logicflag > 0) {
					fprintf(stderr, "%s ^= ", outputs[o]->name);
					printmt(minterms, mt2);
				}
			}
		}
	}
	if(espresso) fprintf(stdout, ".end\n");

}


unsigned int
squash(unsigned int value, unsigned int mask)
{
	unsigned int ret, i , j;
	ret = 0;
	for(i = 1, j = 1; mask; i <<= 1) 
		if(i & mask) {
			if(i & value)
				ret |= j;
			j <<= 1;
			mask &= ~i;
		}
	return(ret);
}
			
char * pofmts[] = {
	" %u%c%u",
	" %x%c%x",
	" %o%c%o"
};

void
pout(char * linestrt, int o, Minterm * mt0, Minterm * mt1, unsigned int mask)
{
		int j, c;
		Minterm * mt;
		poline(linestrt, o, mask);
		j = 0;
		for(mt = mt0; mt < mt1; ++mt) {
			c = ':';
			fprintf(stdout, pofmts[octflag ? 2 : hexflag ? 1 : 0],
				squash(mt->true,mask), c,
				squash(mt->care,mask));
			j++;
			if(j >= PERLINE) {
				j = 0;
				fprintf(stdout, "\n");
			}
		}
		if(j)
			fprintf(stdout, "\n");
}

void
printes(int o, Minterm *mt0, Minterm *mt1)
{
	Minterm * mt;
	char line[NLEAVES];
	int i, j, k;
	for(i = 0; i < nleaves; ++i) line[i] = '-';
	line[0] = ' ';
	line[nleaves+1] = ' ';
	line[nleaves + noutputs + 2] = '\n'; 
	line[nleaves + noutputs + 3] = 0;
	line[nleaves + o + 1] = '1';
	for(mt = mt0; mt < mt1; ++mt) {
		for(i = 1; i <= nleaves; ++i) line[i] = '-';
		for(j = 1, i = 1; j ; j = j << 1, ++i) {
			if(mt->care & j) {
				for(k = 1; k <= nleaves; ++k) 
					if(depends[i] == leaves[k]) break;
				line[k] = (mt->true & j) ? '1' : '0';
			}
		}
		fprintf(stdout, "%s", line);
	}
}


void
printmt(Minterm *mt0, Minterm *mt1)
{
	Minterm * mt;
	int i, j, k;
fprintf(stderr, "<<<<\n"); 
	if(mt1 == mt0) {
		fprintf(stderr, "0\n");
		fprintf(stderr, ">>>>>\n"); 
		return;
	}
	if((mt0->care == 0) && (mt0->true == 0)) {
		fprintf(stderr, "1\n");
		fprintf(stderr, ">>>>>\n"); 
		return;
	}
	for(mt = mt0; mt < mt1; ++mt) {
		if(mt != mt0) fprintf(stderr, "	| ");
		if(mt->care == 0) {
			if(mt->true == ~0) fprintf(stderr, "	0\n");
			else if(mt->true == 0) fprintf(stderr, "	1\n");
			else fprintf(stderr, "	%x\n", mt->true);
			continue;
		}
		k = 0;
		for(j = 1, i = 1; j ; j = j << 1, ++i) {
			if(mt->care & j) {
				if(k != 0) fprintf(stderr, " & ");
				k = 1;
				if((mt->true & j) == 0) fprintf(stderr, "!");
				fprintf(stderr, "%s", depends[i]->name);
			}
		}
		fprintf(stderr, "\n");
	}
fprintf(stderr, ">>>>>\n"); 
}

int
finddepend(Hshtab *hp)
{
	int i;
	for(i = 1; i <= ndepends; ++i)
		if(depends[i] == hp)
			return(1<<(i-1));
	depends[i] = hp;
	++ndepends;
	return(1<<(i-1));
}


int unsigned
getmsk(Hshtab *hp, Node *tp)
{
	Hshtab * hp1;
	hp1 = (Hshtab *) tp->t1;
	if(hp1->code != FIELD)
		return(1);
	return(1 << (hp->fref - LO(hp1)));
}

Minterm *
compile(int o)
{
	Hshtab *hp;
	Node *tp;
	hp = outputs[o];
	if(!(tp = hp->assign))
			return((Minterm *) 0);
/*	if(tp = hp->dontcare) tree_walk(0, getmsk(hp, tp), tp->code,
			tp->t1, tp->t2, 0, 0, minterms, (void *) 0, (void *) 0 ); */
	return(tree_walk(0, getmsk(hp, tp), tp->code,
			tp->t1, tp->t2, 0, 0, minterms, (void *) 0, (void *) 0 ));
}



void
poline(char *key, int o, unsigned int mask)
{
	char buf[NCPS];
	unsigned int i, j, msk;
	strcpy(buf, outputs[o]->name);
	if(outputs[o]->type & CTLNCN)
		strcat(buf, "@b");
	if(outputs[o]->type & CTLINV)
		strcat(buf, "@i"); 
	fprintf(stdout, "%s %s", key, buf);
	for(i=1,j=1, msk=mask; msk; i <<= 1, j+= 1) {
		if(mask & i)
			fprintf(stdout, " %s", depends[j]->name);
		msk &= ~i;
	}
	fprintf(stdout, "\n");
}
