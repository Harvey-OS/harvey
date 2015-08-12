/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>
#include "arm.h"

#define STRINGSZ 128

/*
 *	print the value of dot as file:line
 */
void
printsource(int32_t dot)
{
	char str[STRINGSZ];

	if(fileline(str, STRINGSZ, dot))
		Bprint(bioout, "%s", str);
}

void
printlocals(Symbol *fn, uint32_t fp)
{
	int i;
	Symbol s;

	s = *fn;
	for(i = 0; localsym(&s, i); i++) {
		if(s.class != CAUTO)
			continue;
		Bprint(bioout, "\t%s=#%lux\n", s.name, getmem_4(fp - s.value));
	}
}

void
printparams(Symbol *fn, uint32_t fp)
{
	int i;
	Symbol s;
	int first;

	fp += mach->szreg; /* skip saved pc */
	s = *fn;
	for(first = i = 0; localsym(&s, i); i++) {
		if(s.class != CPARAM)
			continue;
		if(first++)
			Bprint(bioout, ", ");
		Bprint(bioout, "%s=#%lux", s.name, getmem_4(fp + s.value));
	}
	Bprint(bioout, ") ");
}
#define STARTSYM "_main"
#define FRAMENAME ".frame"

void
stktrace(int modif)
{
	uint32_t pc, sp;
	Symbol s, f;
	int i;
	char buf[512];

	pc = reg.r[15];
	sp = reg.r[13];
	i = 0;
	while(findsym(pc, CTEXT, &s)) {
		if(strcmp(STARTSYM, s.name) == 0) {
			Bprint(bioout, "%s() at #%llux\n", s.name, s.value);
			break;
		}
		if(pc == s.value) /* at first instruction */
			f.value = 0;
		else if(findlocal(&s, FRAMENAME, &f) == 0)
			break;
		if(s.type == 'L' || s.type == 'l' || pc <= s.value + 4)
			pc = reg.r[14];
		else
			pc = getmem_4(sp);
		sp += f.value;
		Bprint(bioout, "%s(", s.name);
		printparams(&s, sp);
		printsource(s.value);
		Bprint(bioout, " called from ");
		symoff(buf, sizeof(buf), pc - 8, CTEXT);
		Bprint(bioout, buf);
		printsource(pc - 8);
		Bprint(bioout, "\n");
		if(modif == 'C')
			printlocals(&s, sp);
		if(++i > 40) {
			Bprint(bioout, "(trace truncated)\n");
			break;
		}
	}
}
