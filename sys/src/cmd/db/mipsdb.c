/*
 * MipsCo register stuff
 */

#include "defs.h"
#include "fns.h"


#define STARTSYM	"_main"
#define	FRAMENAME	".frame"

extern	ulong	mipsfindframe(ulong);
extern	void	mipsctrace(int);
extern	void	mipsfpprint(int);
extern	void	mipsexcep(void);
extern	int	mipsfoll(ulong, ulong*);
extern	void	mipsprintins(Map*, char, int);
extern	void	mipsprintdas(Map*, int);

static char *excname[] =
{
	"external interrupt",
	"TLB modification",
	"TLB miss (load or fetch)",
	"TLB miss (store)",
	"address error (load or fetch)",
	"address error (store)",
	"bus error (fetch)",
	"bus error (data load or store)",
	"system call",
	"breakpoint",
	"reserved instruction",
	"coprocessor unusable",
	"arithmetic overflow",
	"undefined 13",
	"undefined 14",
	"system call",
	/* the following is made up */
	"floating point exception"		/* FPEXC */
};
/*
 *	Machine description
 */
Machdata mipsmach =
{
	{0, 0, 0, 0xD},		/* break point */
	4,			/* break point size */

	0,			/* init */
	beswab,			/* convert short to local byte order */
	beswal,			/* convert long to local byte order */
	mipsctrace,		/* print C traceback */
	mipsfindframe,		/* Frame finder */
	mipsfpprint,		/* print floating registers */
	rsnarf4,		/* grab registers */
	rrest4,			/* restore registers */
	mipsexcep,		/* print exception */
	0,			/* breakpoint fixup */
	beieeesfpout,		/* single precision float printer */
	beieeedfpout,		/* double precisioin float printer */
	mipsfoll,		/* following addresses */
	mipsprintins,		/* print instruction */
	mipsprintdas,		/* dissembler */
};

void
mipsexcep(void)
{
	int e;
	long c;

	c = rget(rname("CAUSE"));
	if(c & 0x00002000)	/* INTR3 */
		e = 16;		/* Floating point exception */
	else
		e = (c>>2)&0x0F;
	dprint("%s\n", excname[e]);
}

void
mipsfpprint(int modif)
{
	int i;
	char buf[10];
	char reg[8];

	if(modif == 'f'){
		for(i=0; i<32; i++){
			sprint(buf, "F%d", i);
			get1(cormap, rname(buf), SEGREGS, (uchar *)reg, 4);
			if((i&1) == 0)
				dprint("\t");
			dprint("F%-3d", i);
			beieeesfpout(reg);
			if(i&1)
				dprint("\n");
			else
				dprint("%40t");
		}
	}else{
		for(i=0; i<32; i+=2){
			sprint(buf, "F%d", i);
			get1(cormap, rname(buf), SEGREGS, (uchar *)reg, 4);
			sprint(buf, "F%d", i+1);
			get1(cormap, rname(buf), SEGREGS, (uchar *)reg+4, 4);
			if((i&2) == 0)
				dprint("\t");
			dprint("F%-3d", i);
			beieeedfpout(reg);
			if(i&2)
				dprint("\n");
			else
				dprint("%40t");
		}
	}
}

ulong
mipsfindframe(ulong addr)
{
	ulong pc, fp;
	Symbol s, f;

	pc = rget(mach->pc);
	fp = rget(mach->sp);
	while (findsym(pc, CTEXT, &s)) {
		if (strcmp(STARTSYM, s.name) == 0)
			break;
		if (findlocal(&s, FRAMENAME, &f) == 0)
			break;
		fp += f.value;
		if (s.value == addr)
			return fp;
		if (s.type == 'L' || s.type == 'l')
			pc = rget(mach->link);
		else
			get4(cormap, fp-f.value, SEGDATA, (long *)&pc);
	}
	return 0;
}

#define	EVEN(x)	((x)&~1)
void
mipsctrace(int modif)
{
	ADDR pc, sp;
	Symbol s, f;
	int i;

	if (adrflg) {	/* trace from jmpbuf for multi-threaded code */
		if (get4(cormap, EVEN(adrval), SEGDATA, (long *) &sp) == 0) {
			dprint("%s\n", errflg);
			return;
		}
		if (get4(cormap, EVEN(adrval+4), SEGDATA, (long *) &pc) == 0) {
			dprint("%s\n", errflg);
			return;
		}
	} else {
		sp = EVEN(rget(mach->sp));
		pc = rget(mach->pc);
	}
	i = 0;
	while (findsym(pc, CTEXT, &s)) {
		if(strcmp(STARTSYM, s.name) == 0) {
			dprint("%s() at %lux\n", s.name, s.value);
			break;
		}
		if (pc == s.value)	/* at first instruction */
			f.value = 0;
		else if (findlocal(&s, FRAMENAME, &f) == 0)
			break;
		if (s.type == 'L' || s.type == 'l' || pc <= s.value+mach->pcquant)
			pc = rget(mach->link);
		else if (get4(cormap, sp, SEGDATA, (long *) &pc) == 0)
			dprint ("%s\n", errflg);
		sp += f.value;
		dprint("%s(", s.name);
		printparams(&s, sp);
		dprint(") ");
		printsource(s.value);
		dprint(" called from ");
		psymoff(pc-8, SEGTEXT, " ");
		printsource(pc-8);
		dprint("\n");
		if(modif == 'C')
			printlocals(&s, sp);
		if(++i > 40){
			dprint("(trace truncated)\n");
			break;
		}
	}
}
