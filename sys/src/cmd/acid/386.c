#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ctype.h>
#include <mach.h>
#define Extern extern
#include "acid.h"

extern	void	i386excep(void);
extern	void	i386fpprint(int);
extern	int	i386foll(ulong, ulong*);
extern	void	i386printins(Map *, char, int);
extern	void	i386printdas(Map *, int);
extern  void	i386regfix(Reglist*);

static char *excname[] =
{
[0]	"divide error",
[1]	"debug exception",
[4]	"overflow",
[5]	"bounds check",
[6]	"invalid opcode",
[7]	"math coprocessor emulation",
[8]	"double fault",
[9]	"math coprocessor overrun",
[10]	"invalid TSS",
[11]	"segment not present",
[12]	"stack exception",
[13]	"general protection violation",
[14]	"page fault",
[16]	"math coprocessor error",
[24]	"clock",
[25]	"keyboard",
[27]	"modem status",
[28]	"serial line status",
[30]	"floppy disk",
[36]	"mouse",
[37]	"math coprocessor",
[38]	"hard disk",
[64]	"system call",
};

Machdata i386mach =
{
	{0xCC, 0, 0, 0},	/* break point: INT 3 */
	1,			/* break point size */

	0,			/* init */
	leswab,			/* convert short to local byte order */
	leswal,			/* convert long to local byte order */
	ctrace,			/* print C traceback */
	findframe,		/* frame finder */
	0,			/* print fp registers */
	i386regfix,			/* grab registers */
	0,			/* restore registers */
	i386excep,		/* print exception */
	0,			/* breakpoint fixup */
	0,			/* single precision float printer */
	0,			/* double precision float printer */
	i386foll,		/* following addresses */
	i386printins,		/* print instruction */
	0,			/* dissembler */
};


void
i386regfix(Reglist *rp)
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
i386excep(void)
{
	ulong c;
	static char buf[32];

	c = strc.cause;
	if(c > 64 || excname[c] == 0) {
		sprint(buf, "exception %d", c);
		strc.excep = buf;
	}
	else
		strc.excep = excname[c];
}
