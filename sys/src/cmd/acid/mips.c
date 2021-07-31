#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ctype.h>
#include <mach.h>
#define Extern extern
#include "acid.h"

void	mipsprintins(Map*, char, int);
void	mipsctrace(int);
int	mipsfoll(ulong, ulong*);
void	mipsexcep(void);
ulong	mipsfindframe(ulong);

#define STARTSYM	"_main"
#define PROFSYM		"_mainp"
#define	FRAMENAME	".frame"

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
 *	Machine description - lots of cruft from db
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
	0, 			/* print floating registers */
	0,			/* grab registers */
	0,			/* restore registers */
	mipsexcep,		/* print exception */
	0,			/* breakpoint fixup */
	0, /* beieeesfpout,		/* single precision float printer */
	0, /* beieeedfpout,		/* double precisioin float printer */
	mipsfoll,		/* following addresses */
	mipsprintins,		/* print instruction */
	0, 			/* dissembler */
};

void
mipsexcep(void)
{
	int e;
	long c;

	c = strc.cause;
	if(c & 0x00002000)	/* INTR3 */
		e = 16;		/* Floating point exception */
	else
		e = (c>>2)&0x0F;
	strc.excep = excname[e];
}

void
mipsctrace(int modif)
{
	int i;
	Symbol s, f;
	List **tail, *q, *l;

	USED(modif);

	strc.l = al(TLIST);
	tail = &strc.l;

	i = 0;
	while(findsym(strc.pc, CTEXT, &s)) {
		if(strcmp(STARTSYM, s.name) == 0 || strcmp(PROFSYM, s.name) == 0)
			break;

		if(strc.pc == s.value)	/* at first instruction */
			f.value = 0;
		else if(findlocal(&s, FRAMENAME, &f) == 0)
			break;

		if(s.type == 'L' || s.type == 'l' || strc.pc <= s.value+mach->pcquant)
			get4(cormap, mach->kbase+mach->link, SEGDATA, (long*)&strc.pc);
		else
			get4(cormap, strc.sp, SEGDATA, (long *) &strc.pc);

		strc.sp += f.value;

		q = al(TLIST);
		*tail = q;
		tail = &q->next;

		l = al(TINT);			/* Function address */
		q->l = l;
		l->ival = s.value;
		l->fmt = 'X';

		l->next = al(TINT);		/* called from address */
		l = l->next;
		l->ival = strc.pc-8;
		l->fmt = 'X';

		l->next = al(TLIST);		/* make list of params */
		l = l->next;
		l->l = listparams(&s, strc.sp);

		l->next = al(TLIST);		/* make list of locals */
		l = l->next;
		l->l = listlocals(&s, strc.sp);

		if(++i > 40)
			break;
	}
	if(i == 0)
		error("no frames found");
}

ulong
mipsfindframe(ulong addr)
{
	long pc, fp;
	Symbol s, f;

	get4(cormap, mach->pc+mach->kbase, SEGDATA, &pc);
	get4(cormap, mach->sp+mach->kbase, SEGDATA, &fp);

	while (findsym(pc, CTEXT, &s)) {
		if (strcmp(STARTSYM, s.name) == 0 || strcmp(PROFSYM, s.name) == 0)
			break;
		if (findlocal(&s, FRAMENAME, &f) == 0)
			break;
		fp += f.value;
		if (s.value == addr)
			return fp;
		if (s.type == 'L' || s.type == 'l')
			get4(cormap, mach->link+mach->kbase, SEGDATA, &pc);
		else
			get4(cormap, fp-f.value, SEGDATA, (long *)&pc);
	}
	return 0;
}
