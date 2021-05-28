/* virtex4 ppc405 initial tlbs */
#include	"mem.h"
#define	MB	(1024*1024)

/*
 * TLB prototype entries, loaded once-for-all at startup,
 * remaining unchanged thereafter.
 * Limit the table size to ensure it fits in small TLBs.
 */
#define	TLBE(hi, lo)	WORD	$(hi);  WORD	$(lo)

/*
 * WARNING: applying TLBG to instruction pages will cause ISI traps
 * on Xilinx 405s, despite the words of most of the Xilinx manual
 * (e.g., UG011, p. 155).  p. 153 has the (insufficiently bold-faced)
 * warning: ``In virtual mode, attempts to fetch instructions either
 * from guarded storage or from no-execute memory locations normally
 * cause an instruction-storage interrupt [i.e., trap or exception]
 * to occur.''  In other words, WARNING: LARKS' VOMIT!
 *
 * Using write-back caches requires more care with cache flushing
 * and use of memory barriers, but cuts run times to about 40% of
 * what we see with write-through caches (using the TLBW bit).
 */
#define RAMATTRS 0			/* write-back cache */

/* largest page size is 16MB */
#define TLBED16MB(i) \
	TLBE(KZERO | (PHYSDRAM + ((i)*16*MB)) | TLB16MB | TLBVALID, \
		PHYSDRAM + ((i)*16*MB) | TLBZONE(0) | TLBWR | TLBEX | RAMATTRS)

TEXT	tlbtab(SB), 1, $-4
	/* tlbhi tlblo */

	/* DRAM, at most 128MB */
	TLBED16MB(0)
	TLBED16MB(1)
	TLBED16MB(2)
	TLBED16MB(3)

	TLBED16MB(4)
	TLBED16MB(5)
	TLBED16MB(6)
	/* MACs are stored in 113.78—128 MB in the secure-memory design */
	TLBED16MB(7)

	/* memory-mapped IO, 512KB */
	TLBE(PHYSMMIO | TLB256K | TLBVALID,
	     PHYSMMIO | TLBZONE(0) | TLBWR | TLBI | TLBG)
	TLBE(PHYSMMIO+256*1024 | TLB256K | TLBVALID,
	     PHYSMMIO+256*1024 | TLBZONE(0) | TLBWR | TLBI | TLBG)

	/* SRAM, 128K.  put vectors here.  */
	TLBE(PHYSSRAM | TLB64K | TLBVALID,
	     PHYSSRAM | TLBZONE(0) | TLBWR | TLBEX | RAMATTRS)
	TLBE(PHYSSRAM+64*1024 | TLB64K | TLBVALID,
	     PHYSSRAM+64*1024 | TLBZONE(0) | TLBWR | TLBEX | RAMATTRS)

TEXT	tlbtabe(SB), 1, $-4
	RETURN
