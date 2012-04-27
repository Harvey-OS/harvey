#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>
#define Extern extern
#include "power.h"

#define	STRINGSZ	128

/*
 *	print the value of dot as file:line
 */
void
printsource(long dot)
{
	char str[STRINGSZ];

	if (fileline(str, STRINGSZ, dot))
		Bprint(bioout, "%s", str);
}

void
printlocals(Symbol *fn, ulong fp)
{
	int i;
	Symbol s;

	s = *fn;
	for (i = 0; localsym(&s, i); i++) {
		if (s.class != CAUTO)
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
#define	FRAMENAME	".frame"

void
stktrace(int modif)
{
	ulong pc, sp;
	Symbol s, f;
	int i;
	char buf[512];

	pc = reg.pc;
	sp = reg.r[1];
	i = 0;
	while (findsym(pc, CTEXT, &s)) {
		if(strcmp(STARTSYM, s.name) == 0) {
			Bprint(bioout, "%s() at #%llux\n", s.name, s.value);
			break;
		}
		if (pc == s.value)	/* at first instruction */
			f.value = 0;
		else if (findlocal(&s, FRAMENAME, &f) == 0)
			break;
		if (s.type == 'L' || s.type == 'l' || pc <= s.value+4)
			pc = reg.lr;
		else pc = getmem_4(sp);
		sp += f.value;
		Bprint(bioout, "%s(", s.name);
		printparams(&s, sp);
		printsource(s.value);
		Bprint(bioout, " called from ");
		symoff(buf, sizeof(buf), pc-4, CTEXT);
		Bprint(bioout, buf);
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
