/*
 * SPARC register stuff
 */

#include "defs.h"
#include "fns.h"

/*
 * sparc has same stack format as mips
 */
extern	ulong	mipsfindframe(ulong);
extern	void	mipsctrace(int);
extern	void	mipsfpprint(int);
extern	void	sparcexcep(void);
extern	int	sparcfoll(ulong, ulong*);
extern	void	sparcprintins(Map *, char, int);
extern	void	sparcprintdas(Map *, int);

static char *trapname[] =
{
	"reset",
	"instruction access exception",
	"illegal instruction",
	"privileged instruction",
	"fp disabled",
	"window overflow",
	"window underflow",
	"unaligned address",
	"fp exception",
	"data access exception",
	"tag overflow",
};

Machdata sparcmach =
{
	{0x91, 0xd0, 0x20, 0x01},	/* breakpoint: TA $1 */
	4,			/* break point size */

	0,			/* init */
	beswab,			/* convert short to local byte order */
	beswal,			/* convert long to local byte order */
	mipsctrace,		/* print C traceback */
	mipsfindframe,		/* frame finder */
	mipsfpprint,		/* print fp registers */
	rsnarf4,		/* grab registers */
	rrest4,			/* restore registers */
	sparcexcep,		/* print exception */
	0,			/* breakpoint fixup */
	beieeesfpout,		/* single precision float printer */
	beieeedfpout,		/* double precision float printer */
	sparcfoll,		/* following addresses */
	sparcprintins,		/* print instruction */
	sparcprintdas,		/* dissembler */
};

static char*
excname(ulong tbr)
{
	static char buf[32];

	if(tbr < sizeof trapname/sizeof(char*))
		return trapname[tbr];
	if(tbr >= 130)
		sprint(buf, "trap instruction %d", tbr-128);
	else if(17<=tbr && tbr<=31)
		sprint(buf, "interrupt level %d", tbr-16);
	else switch(tbr){
	case 36:
		return "cp disabled";
	case 40:
		return "cp exception";
	case 128:
		return "syscall";
	case 129:
		return "breakpoint";
	default:
		sprint(buf, "unknown trap %d", tbr);
	}
    Return:
	return buf;
}

void
sparcexcep(void)
{
	long tbr;

	tbr = rget(rname("TBR"));
	tbr = (tbr&0xFFF)>>4;
	dprint("last exception: %s\n", excname(tbr));
}
