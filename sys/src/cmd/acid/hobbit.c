#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ctype.h>
#include <mach.h>
#define Extern extern
#include "acid.h"

#define STARTSYM	"_main"
#define PROFSYM		"_mainp"
#define	FRAMENAME	".frame"

extern	ulong	hobbitfindframe(ulong);
extern	void	hobbitctrace(int);
extern	void	hobbitexcep(void);
extern	int	hobbitfoll(ulong, ulong*);
extern	void	hobbitprintins(Map *, char, int);
extern	ulong	hobbitbpfix(ulong);
extern	void	hobbitregfix(Reglist*);

static char *events[16] =
{
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

static char *exceptions[16] =
{
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
	hobbitregfix,			/* fixup register address vars registers */
	0,				/* restore registers */
	hobbitexcep,			/* print exception */
	hobbitbpfix,			/* breakpoint fixup */
	0,
	0,
	hobbitfoll,			/* following addresses */
	hobbitprintins,			/* print instruction */
	0,				/* dissembler */
};

void
hobbitregfix(Reglist *rp)
{
	Lsym *l;

	while(rp->rname) {
		l = look(rp->rname);
		if(l == 0)
			print("lost register %s\n", rp->rname);
		else
			l->v->ival = mach->kbase+rp->roffs;
		rp++;
	}
	machdata->rsnarf = 0;	/* Only need to fix once */
}

void
hobbitexcep(void)
{
	ulong e;

	e = strc.cause & 0x0F;
	if (e == 1)
		strc.excep = exceptions[rget("ID")&0x0F];
	else
		strc.excep = events[e];
}

ulong
hobbitbpfix(ulong bpaddr)
{
	return bpaddr ^ 0x02;
}

ulong
hobbitfindframe(ulong addr)
{
	ulong fp;
	Symbol s, f;

	fp = strc.sp;
	while (findsym(strc.pc, CTEXT, &s)) {
		if (strcmp(STARTSYM, s.name) == 0 || strcmp(PROFSYM, s.name) == 0)
			break;
		if (findlocal(&s, FRAMENAME, &f) == 0)
			break;
		fp += f.value - sizeof(long);
		if (s.value == addr)
			return fp;
		if (get4(cormap, fp, SEGDATA, (long *)&strc.pc) == 0)
			break;
	}
	return 0;
}
void
hobbitctrace(int modif)
{
	int i;
	Symbol s, f;
	List **tail, *q, *l;

	USED(modif);
	strc.l = al(TLIST);
	tail = &strc.l;
	i = 0;
	while (findsym(strc.pc, CTEXT, &s)) {
		if(strcmp(STARTSYM, s.name) == 0 || strcmp(PROFSYM, 0) == 0)
			break;

		if (strc.pc != s.value) {		/* not at first instruction */
			if (findlocal(&s, FRAMENAME, &f) == 0)
				break;
			/* minus sizeof return addr */
			strc.sp += f.value - mach->szaddr;
		}
		if(get4(cormap, strc.sp, SEGDATA, (long *)&strc.pc) == 0)
			break;

		q = al(TLIST);
		*tail = q;
		tail = &q->next;

		l = al(TINT);			/* Function address */
		q->l = l;
		l->ival = s.value;
		l->fmt = 'X';

		l->next = al(TINT);		/* called from address */
		l = l->next;
		l->ival = strc.pc;
		l->fmt = 'X';

		l->next = al(TLIST);		/* make list of params */
		l = l->next;
		l->l = listparams(&s, strc.sp);

		l->next = al(TLIST);		/* make list of locals */
		l = l->next;
		l->l = listlocals(&s, strc.sp);

		if (++i > 40)
			break;
	}

	if(i == 0)
		error("no frames found");
}
