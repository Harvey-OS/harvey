#include <u.h>
#include <libc.h>
#include <stdio.h>
#include "dat.h"
#include "fns.h"
#include "gram.h"
extern int nterm, T[], M[], nlook;
void
out(Node *tp)
{
	if(plaflag)
		plaout(tp);
	else if(romflag)
		romout(tp);
}


/*
 * -a option:
 * make <term>:<mask> outputs
 * for use with quine, cover, etc.
 */
void
plaout(Node *tp)
{
	register j, o, c;
	Value x1, x2, d1, mask, li;
	Value ibit, rdepend;

	if (treeflag)
		treeprint(tp);
	if(chipname)
		fprint(1, ".x %s\n", chipname->name); 
	for(o=1; o<=noutputs; o++) {
		clrdepend();
		compile(o);
		getdepend();
		if(ndepends > 31) {
			if(outputs[o]->pin)
				fprint(2, "too many inputs %d for %d\n",
					ndepends, outputs[o]->pin);
			else
				fprint(2, "too many inputs %d for %s\n",
					ndepends, outputs[o]->name);
#ifdef PLAN9
		exits("too many inputs");
#else
		exit(1);
#endif
		}
		mask = ~((Value)0) << ndepends;
		rdepend = 0;
		clearcache();
		for(ibit = 1; (ibit & mask) == 0; ibit <<= 1) {
			mask |= ibit;
			for(li=0;;) {
				x1 = vexec(li);
				d1 = dcval;
				x2 = vexec(li|ibit);
				if(!(d1 & dcval))
				if(x1^x2) {
					rdepend |= ibit;
					break;
				}
				li = ((li | mask) + 1) & ~mask;
				if(!li)
					break;
			}
			mask &= ~ibit;
		}
		poline(o, rdepend);
		mask = ~rdepend;
		j = 0;
		for(li=0; ;) {
			c = 0;
			x1 = vexec(li);
			if(x1)
				c = ':';
			if(dcval)
				c = '%';
			if(c) {
				fprint(1, " %ld%c%ld",
					take(li, rdepend), c,
					take(~(Value) 0, rdepend));
				j++;
				if(j >= PERLINE) {
					j = 0;
					fprint(1, "\n");
				}
			}
			li = ((li | mask) + 1) & ~mask;
			if(!li)
				break;
		}
		if(j)
			fprint(1, "\n");
	}
/*	following is gone 
	 * if output pins are the same,
	 * then make them be dependent on
	 * same inputs. this allows hand-crafted
	 * or gates by separate outputs.
	for(i=1; i<=noutputs; i++) {
		o = outputs[i]->pin;
		if(o)
		for(j=i+1; j<=noutputs; j++)
			if(outputs[j]->pin == o) {
				outputs[i]->depend |= outputs[j]->rdepend;
				outputs[j]->depend = outputs[i]->depend;
			}
	}
	for(o=0; o<noutputs; o++) {
		depend = outputs[o+1]->depend;
		rdepend = outputs[o+1]->rdepend;
		obit = 1L << o;
		clearcache();
	}		*/
}

void
getdepend(void)
{
	int i;
	ndepends = 0;
	for(i = 1; i <= nleaves; ++i)
		if(leaves[i]->iused)
			depends[++ndepends] = leaves[i];
}
void
clrdepend(void) 
{
	int i;
	for(i = 1; i <= nleaves; ++i) {
		leaves[i]->iused = 0;
		leaves[i]->val = 0;
	}
}
/*
 * -r option:
 * write out the eqn evaluation as a table
 * suitable for putting in a rom.
 */
void
romout(Node *tp)
{
	int j;
	Value x, nperms, li;
	int width;	/* # bits per word */
	int npline;	/* # words per line */
	int nline;
	int line;	/* output word # */

	clearcache();
	if (treeflag) {
		treeprint(tp);
		fprint(1, "\n\n\n");
#ifdef PLAN9
		exits((char *) 0);
#else
		exit(1);
#endif
	}
	nperms = 1L<<nleaves;
	width = (hexflag) ? 4:3;
	npline = (hexflag) ? 16:8;
	if (promflag)
		npline = 1;
	if (noutputs > 16)
		npline = 1;
	line = 0;
	nline = npline;
	for(li=0;li<nperms;li++) {
/*		vexec(li);*/
		if (nline==npline && promflag==0) {
			if (nperms < 10)
				fprint(1, "%d: ",line);
			else
			if (nperms < 100)
				fprint(1, "%2d: ",line);
			else
			if (nperms < 1000)
				fprint(1, "%3d: ",line);
			else
			if (nperms < 10000)
				fprint(1, "%4d: ",line);
		}
		x = 0;
		if (hexflag)
			fprint(1, "x"); else
			fprint(1, "0");
		for(j=noutputs-1; j>=0; j--) {
			x <<= 1;
			x += (rom[j] & 01);
			if (j%width==0) {
				if (hexflag)
					fprint(1, "%x",x);
				else
					fprint(1, "%o",x);
				x = 0;
			}
		}
		nline--;
		line++;
		if (nline==0) {
			fprint(1, "\n");
			nline = npline;
		} else {
			fprint(1, " ");
		}
	}
}

Value
vexec(Value in)
{
	int i;
	Value x;

	if(in < CACHE && in >= 0 && cache[in] < 0) {
		dcval = cache[in]&DCBIT;
		return cache[in]&VALBIT;
	}
	x = in;
	for(i=1; i<=ndepends; i++) {
		hp = depends[i];
		hp->val = x & 1;
		x >>= 1;
	}

	x = eval1();
	if(in < CACHE && in >= 0)
		cache[in] = 0x80 | x;

	dcval = (x & 2) ? 1 : 0;
	return (x & 1);
}

Value
take(Value v, Value m)
{
	Value x, bit, val;
	int i;

	x = 0;
	val = 1;
	bit = 1;
	for(i=nleaves; i>=0; i--) {
		if(m & bit) {
			if(v & bit)
				x |= val;
			val <<= 1;
		}
		bit <<= 1;
	}
	return x;
}

void
labels(void)
{
	register int i;

	for(i=1; i<=nleaves; i++) {
		hp = leaves[i];
		fprint(1, "%s ",hp->name);
	}
	fprint(1, "\n");
	for(i=1; i<=noutputs; i++) {
		hp = outputs[i];
		fprint(1, "%s ",hp->name);
	}
	fprint(1, "\n");
}



/*
 * -w option:
 * make a ".w" that defines a pal chip
 * as a macro.
 */
void
cdl(void)
{
	Hshtab *hp;
	char *s1;
	int i;

	s1 = (chipname) ? chipname->name : "lde";
	fprint(1, ".c %s %s\n", "e", s1);
	for(i=1; i<=nleaves; i++) {
		hp = leaves[i];
		if (hp->type==0)
			continue;
		fprint(1, "\t%s\t%d\n", hp->name, hp->pin);
	}
	for(i=1; i<=noutputs; i++) {
		hp = outputs[i];
		if (hp->type==0)
			continue;
		fprint(1, "\t%s\t%d\n", hp->name, hp->pin);
	}
	for(i=1; i<=nleaves; i++) {
		hp = leaves[i];
		if (hp->type==0)
			continue;
		fprint(1, ".m\t%s\t%s\n", hp->name, hp->name);
	}
	for(i=1; i<=noutputs; i++) {
		hp = outputs[i];
		if (hp->type==0)
			continue;
		fprint(1, ".m\t%s\t%s\n", hp->name, hp->name);
	}
}

void
poline(int o, Value rdepend)
{
	char buf[NCPS];
	int i, ibit;
	if(outputs[o]->pin)
		fprint(1, ".o %d", outputs[o]->pin);
	else {
		strcpy(buf, outputs[o]->name);
		if(outputs[o]->type & CTLNCN)
			strcat(buf, "@b");
		if(outputs[o]->type & CTLINV)
			strcat(buf, "@i"); 
		fprint(1, ".o %s", buf);
	}
	for(i=0; i<ndepends; i++) {
		ibit = 1L << i;
		if(rdepend & ibit)
			if(depends[i+1]->pin)
				fprint(1, " %d", depends[i+1]->pin);
			else 
				fprint(1, " %s", depends[i+1]->name);
	}
	fprint(1, "\n");
}
