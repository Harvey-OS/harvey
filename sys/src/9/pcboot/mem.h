/*
 * changes from pc/mem.h for pcboot.
 */

#define MAXMACH	1		/* max # cpus system can run */

/*
 * we use a larger-than-normal kernel stack in the bootstraps as we have
 * significant arrays (buffers) on the stack.  we typically consume about
 * 4.5K of stack.
 */
#define KSTACK	(4*BY2PG)	/* Size of kernel stack */

/*
 *  Address spaces
 *
 *  Kernel is at 2GB-4GB in the bootstrap kernels.
 */
#define KZERO 0x80000000	/* base of kernel address space */
#define KSEGM 0xF0000000
/* must match mkfile */
#define	KTZERO	(KZERO+0x900000)	/* first address in kernel text */

#define MemMin (20*MB)	/* l*.s allocated PTEs for this; pcboot runs at 9MB */
#define MemMax (256*MB) /* allow for huge 10gbe buffer rings */

#include "../pc/mem.h"

#define BIOSXCHG RMBUF
