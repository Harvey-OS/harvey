/*
 * code to keep track of registers
 */

#include "defs.h"
#include "fns.h"

#define STARTSYM	"_main"
#define PROFSYM		"_mainp"
#define	FRAMENAME	".frame"

extern	ulong	hobbitfindframe(ulong);
extern	void	hobbitctrace(int);
extern	void	hobbitexcep(void);
extern	int	hobbitfoll(ulong, ulong*);
extern	void	hobbitprintins(Map *, char, int);
extern	void	hobbitprintdas(Map *, int);
extern	ulong	hobbitbpfix(ulong);

static char *events[16] = {
	"syscall",
	"exception",
	"niladic trap",
	"unimplemented instruction",
	"non-maskable interrupt",
	"interrupt 1",
	"interrupt 2",
	"interrupt 3",
	"interrupt 4",
	"interrupt 5",
	"interrupt 6",
	"timer 1 interrupt",
	"timer 2 interrupt",
	"FP exception",
	"event 0x0E",
	"event 0x0F",
};

static char *exceptions[16] = {
	"exception 0x00",
	"integer zero-divide",
	"trace",
	"illegal instruction",
	"alignment fault",
	"privilege violation",
	"unimplemented register",
	"fetch fault",
	"read fault",
	"write fault",
	"text fetch I/O bus error",
	"data access I/O bus error",
	"exception 0x0C",
	"exception 0x0D",
	"exception 0x0E",
	"exception 0x0F",
};

Machdata hobbitmach =
{
	{0x00, 0x00},			/* break point: KCALL $0 */
	2,				/* break point size */

	0,				/* init */
	leswab,				/* convert short to local byte order */
	leswal,				/* convert long to local byte order */
	hobbitctrace,			/* print C traceback */
	hobbitfindframe,		/* frame finder */
	0,				/* print fp registers */
	rsnarf4,			/* grab registers */
	rrest4,				/* restore registers */
	hobbitexcep,			/* print exception */
	hobbitbpfix,			/* breakpoint fixup */
	leieeesfpout,
	leieeedfpout,
	hobbitfoll,			/* following addresses */
	hobbitprintins,			/* print instruction */
	hobbitprintdas,			/* dissembler */
};

void
hobbitexcep(void)
{
	ulong e;

	e = rget(rname("CAUSE")) & 0x0F;
	if (e == 1)
		dprint("%s\n", exceptions[rget(rname("ID")) & 0x0F]);
	else
		dprint("%s\n", events[e]);
}

ulong
hobbitbpfix(ulong bpaddr)
{
	return bpaddr ^ 0x02;
}

ulong
hobbitfindframe(ulong addr)
{
	ulong pc, fp;
	Symbol s, f;

	pc = rget(mach->pc);
	fp = rget(mach->sp);
	while (findsym(pc, CTEXT, &s)) {
		if (strcmp(STARTSYM, s.name) == 0 || strcmp(PROFSYM, s.name) == 0)
			break;
		if (findlocal(&s, FRAMENAME, &f) == 0)
			break;
		fp += f.value - sizeof(WORD);
		if (s.value == addr)
			return fp;
		if (get4(cormap, fp, SEGDATA, (long *)&pc) == 0)
			break;
	}
	return 0;
}
void
hobbitctrace(int modif)
{
	ADDR pc, sp;
	Symbol s, f;
	int i;

	pc = rget(mach->pc);
	sp = rget(mach->sp);
	i = 0;
	while (findsym(pc, CTEXT, &s)) {
		if (strcmp(STARTSYM, s.name) == 0) {
			dprint("%s() at %lux\n", s.name, s.value);
			break;
		}
		if (pc != s.value) {		/* not at first instruction */
			if (findlocal(&s, FRAMENAME, &f) == 0)
				break;
			sp += f.value - mach->szaddr;	/* minus sizeof return addr */
		}
		if (get4(cormap, sp, SEGDATA, (long *)&pc) == 0)
			break;
		dprint("%s(", s.name);
		printparams(&s, sp);
		dprint(") ");
		printsource(s.value);
		dprint(" called from ");
		psymoff(pc, SEGTEXT, " ");
		printsource(pc);
		dprint("\n");
		if (modif == 'C')
			printlocals(&s, sp);
		if (++i > 40){
			dprint("(trace truncated)\n");
			break;
		}
	}
}
