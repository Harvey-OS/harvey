#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>
#define Extern extern
#include "3210.h"

/*
 * Print value v as name[+offset] and then the string s.
 */
int
symoff(char *buf, int n, long v, int space)
{
	Symbol s;
	int r;
	long delta;

	r = delta = 0;		/* to shut compiler up */
	if (v) {
		r = findsym(v, space, &s);
		if (r)
			delta = v-s.value;
		if (delta < 0)
			delta = -delta;
	}
	if (v == 0 || r == 0 || delta >= 4096)
		return snprint(buf, n, "%lux", v);
	else if (delta)
		return snprint(buf, n, "%s+%lux", s.name, delta);
	else
		return snprint(buf, n, "%s", s.name);
}

void
tracecall(ulong npc)
{
	Symbol s;

	findsym(npc, CTEXT, &s);
	Bprint(bioout, "%X %s(", reg.ip, s.name);
	printparams(&s, reg.r[REGSP]);
	Bprint(bioout, "from ");
	printsource(reg.ip);
	Bprint(bioout, " r%d=%X\n", REGARG, reg.r[REGARG]);
	Bflush(bioout);
}

void
traceret(ulong npc, int r)
{
	Symbol s;

	if(r != REGLINK && r != REGRET+1)
		return;

	findsym(npc, CTEXT, &s);
	Bprint(bioout, "%X return to %X %s r%d=%X a%d=%g\n",
		reg.ip, npc, s.name, REGRET, reg.r[REGRET],
		FREGRET, dsptod(reg.f[FREGRET].val));
	Bflush(bioout);
}

#define	STRINGSZ	128

/*
 *	print the value of dot as file:line
 */
void
printsource(long dot)
{
	char str[STRINGSZ];

	if(fileline(str, STRINGSZ, dot))
		Bprint(bioout, "%s", str);
}

void
printlocals(Symbol *fn, ulong fp)
{
	int i;
	Symbol s;

	s = *fn;
	for(i = 0; localsym(&s, i); i++){
		if(s.class != CAUTO)
			continue;
		Bprint(bioout, "\t%s=#%lux\n", s.name, getmem_4(fp-s.value));
	}
}

void
printparams(Symbol *fn, ulong fp)
{
	int i;
	Symbol s;
	int first;

	fp += mach->szreg;			/* skip saved pc */
	s = *fn;
	for (first = i = 0; localsym(&s, i); i++) {
		if (s.class != CPARAM)
			continue;
		if (first++)
			Bprint(bioout, ", ");
		Bprint(bioout, "%s=#%lux", s.name, getmem_4(fp+s.value));
	}
	Bprint(bioout, ") ");
}

#define STARTSYM	"_main"

void
stktrace(int modif)
{
	Symbol s;
	ulong pc, opc, sp;
	char buf[128];
	int i, moved;

	pc = reg.ip;
	sp = reg.r[REGSP];
	i = 0;
	opc = 0;
	while(pc && opc != pc) {
		moved = pc2sp(pc);
		if(moved == -1)
			break;
		if(!findsym(pc, CTEXT, &s))
			break;
		if(strcmp(STARTSYM, s.name) == 0) {
			Bprint(bioout, "%s() at %lux\n", s.name, s.value);
			break;
		}
		sp += moved;
		opc = pc;
		if(s.type == 'L' || s.type == 'l' || pc <= s.value+4)
			pc = reg.r[REGLINK];
		else
			pc = getmem_4(sp);
		Bprint(bioout, "%s(", s.name);
		printparams(&s, sp);
		printsource(s.value);
		symoff(buf, sizeof buf, pc-8, CANY);
		Bprint(bioout, " called from %s", buf);
		printsource(pc-8);
		Bprint(bioout, "\n");
		if(modif == 'C')
			printlocals(&s, sp);
		if(++i > 40){
			Bprint(bioout, "(trace truncated)\n");
			break;
		}
	}
}
