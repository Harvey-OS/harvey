/*
 * i386 register stuff
 */

#include "defs.h"
#include "fns.h"

extern	void	i386excep(void);
extern	void	i386fpprint(int);
extern	int	i386foll(ulong, ulong*);
extern	void	i386printins(Map *, char, int);
extern	void	i386printdas(Map *, int);

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
	i386fpprint,		/* print fp registers */
	rsnarf4,		/* grab registers */
	rrest4,			/* restore registers */
	i386excep,		/* print exception */
	0,			/* breakpoint fixup */
	leieeesfpout,		/* single precision float printer */
	leieeedfpout,		/* double precision float printer */
	i386foll,		/* following addresses */
	i386printins,		/* print instruction */
	i386printdas,		/* dissembler */
};


void
i386excep(void)
{
	ulong c;

	c = rget(rname("TRAP"));
	if(c > 64 || excname[c] == 0)
		dprint("exception %d\n", c);
	else
		dprint("%s\n", excname[c]);
}

void
i386fpprint(int modif)
{
	int j;
	char buf[10];
	char reg[12];

	USED(modif);
	for(j=0; j<8; j++){
		sprint(buf, "F%dL", j);
		get1(cormap, rname(buf), SEGREGS, (uchar *) reg, 10);
		chkerr();
		/* open hole */
		memmove(reg+10, reg+8, 2);
		memset(reg+8, 0, 2);
		dprint("\tF%d\t", j);
		leieee80fpout(reg);
		dprint("\n");
	}
}
