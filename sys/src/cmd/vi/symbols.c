#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>
#define Extern extern
#include "mips.h"

/*
 * Print value v as name[+offset] and then the string s.
 */
void
psymoff(ulong v, int type, char *str)
{
	Symbol s;
	int t;
	int r;
	int delta;

	switch(type) {
	case SEGREGS:
		Bprint(bioout, "%%#%lux", v);
		Bprint(bioout, str);
		return;
	case SEGANY:
		t = CANY;
		break;
	case SEGDATA:
		t = CDATA;
		break;
	case SEGTEXT:
		t = CTEXT;
		break;
	case SEGNONE:
	default:
		return;
	}
	r = 0;
	delta = 0x1000;
	if (v) {
		r = findsym(v, t, &s);
		if (r)
			delta = v-s.value;
	}
	if (v == 0 || r == 0 || delta >= 0x1000)
		Bprint(bioout, "#%lux", v);
	else {
		Bprint(bioout, "%s", s.name);
		if (delta)
			Bprint(bioout, "+#%lux", delta);
	}
	Bprint(bioout, str);
}

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

ulong
findframe(ulong addr)
{
	ulong pc, fp;
	Symbol s, f;

	pc = reg.pc;
	fp = reg.r[29];
	while (findsym(pc, CTEXT, &s)) {
		if (strcmp(STARTSYM, s.name) == 0)
			break;
		if (findlocal(&s, FRAMENAME, &f) == 0)
			break;
		fp += f.value;
		if (s.value == addr)
			return fp;
		if (s.type == 'L' || s.type == 'l')
			pc = reg.r[31];
		else
			pc = getmem_4(fp-f.value);
	}
	return 0;
}

void
stktrace(int modif)
{
	ulong pc, sp;
	Symbol s, f;
	int i;

	pc = reg.pc;
	sp = reg.r[29];
	i = 0;
	while (findsym(pc, CTEXT, &s)) {
		if(strcmp(STARTSYM, s.name) == 0) {
			Bprint(bioout, "%s() at #%lux\n", s.name, s.value);
			break;
		}
		if (pc == s.value)	/* at first instruction */
			f.value = 0;
		else if (findlocal(&s, FRAMENAME, &f) == 0)
			break;
		if (s.type == 'L' || s.type == 'l' || pc <= s.value+4)
			pc = reg.r[31];
		else pc = getmem_4(sp);
		sp += f.value;
		Bprint(bioout, "%s(", s.name);
		printparams(&s, sp);
		printsource(s.value);
		Bprint(bioout, " called from ");
		psymoff(pc-8, SEGTEXT, " ");
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
