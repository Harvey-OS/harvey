/*
 * i960 register stuff
 */

#include "defs.h"
#include "fns.h"

/*
 * i960 has an as yet unknown stack format - presotto
 */
extern	void	i960excep(void);
extern	void	i960printins(Map*, char, int);
extern	void	i960printdas(Map*, int);
extern	int	i960foll(ulong, ulong*);


Machdata i960mach =
{
	{0x66, 0, 0x06, 0},	/* break point: fmark */
	4,			/* break point size */

	0,			/* init */
	leswab,			/* convert short to local byte order */
	leswal,			/* convert long to local byte order */
	ctrace,			/* print C traceback */
	findframe,		/* frame finder */
	0,			/* print fp registers */
	rsnarf4,		/* grab registers */
	rrest4,			/* restore registers */
	i960excep,		/* print exception */
	0,			/* breakpoint fixup */
	leieeesfpout,		/* single precision float printer */
	leieeedfpout,		/* double precision float printer */
	i960foll,		/* following addresses */
	i960printins,		/* print instruction */
	i960printdas,		/* dissembler */
};

void
i960excep(void)
{
	dprint("intel machine exceptions are too hard to decode\n");
}
