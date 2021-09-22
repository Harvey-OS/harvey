/*
 * changes from pc/mem.h for expand.
 * undef unused symbols to avoid inadvertent use.
 */

/*
 *  Address spaces
 */
#define KZERO 0x80000000	/* base of kernel address space */
#define KSEGM 0xF0000000

#include "../pc/mem.h"

#undef KTZERO			/* see dat.h */

#undef MAXMACH
#undef KSTACK
#undef MACHSIZE

/*
 * pxe loading starts us at 0x7c00 and non-pxe loading starts us at 0x10000.
 * undef everything from 0x7c00 to CPU0END, since we can't use that range.
 */
#undef RMUADDR
#undef RMCODE
#undef RMBUF
#undef IDTADDR
#undef REBOOTADDR
#undef CPU0START
#undef CPU0PDB
#undef CPU0PTE
#undef CPU0GDT
#undef MACHADDR
#undef CPU0MACH
#undef CPU0END

#define v_flag 0
#define EXPAND
