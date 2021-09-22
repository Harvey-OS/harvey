/*
 * interface to sbi (bios/firmware) via assembler sbiecall for rv64
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "riscv.h"

vlong
sbicall(uvlong ext, uvlong func, uvlong arg1, Sbiret *retp, uvlong *xargs)
{
	vlong r;
	Mpl pl;
	Mreg ie;

	if (nosbi)
		return -1;

	coherence();

	/* sbi could change PL in STATUS or SIE */
	pl = splhi();		/* R2, 5, 6, 7 can be changed by sbi */
	ie = getsie();
	if (ext == HSM && func == 3)		/* suspend? */
 		putsie(ie | Superie);		/* wake wfi on super intr */
	else
 		putsie(ie & ~Superie);		/* no super intrs in sbi */

	if (retp)
		memset(retp, 0, sizeof *retp);
	r = sbiecall(ext, func, arg1, retp, xargs);

 	putsie(getsie() & ~Superie | ie & Superie);
	splx(pl);
	return r;
}
