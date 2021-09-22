#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>
#define Extern extern
#include "arm64.h"

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
printlocals(Symbol *fn, uvlong fp)
{
	int i;
	Symbol s;

	s = *fn;
	for (i = 0; localsym(&s, i); i++) {
		if (s.class != CAUTO)
			continue;
		Bprint(bioout, "\t%s=#%llux\n", s.name, getmem_v(fp-s.value));
	}
}

void
printparams(Symbol *fn, uvlong fp)
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
		Bprint(bioout, "%s=#%llux", s.name, getmem_v(fp+s.value));
	}
	Bprint(bioout, ") ");
}

#define STARTSYM	"_main"
#define	FRAMENAME	".frame"

void
stktrace(int modif)
{
	uvlong pc, sp;
	Symbol s, f;
	int i;
	char buf[512];

	pc = reg.pc;
	sp = reg.r[31];
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
			pc = reg.r[30];
		else pc = getmem_v(sp);
		sp += f.value;
		Bprint(bioout, "%s(", s.name);
		printparams(&s, sp);
		printsource(s.value);
		Bprint(bioout, "\n\tcalled from ");
		symoff(buf, sizeof(buf), pc-4, CTEXT);
		Bprint(bioout, buf);
		Bprint(bioout, " ");
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
