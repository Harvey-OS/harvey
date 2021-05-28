/* virtex5 ppc440x5 initial tlb entries */
#include	"mem.h"
#define	MB	(1024*1024)

/*
 * TLB prototype entries, loaded once-for-all at startup,
 * remaining unchanged thereafter.
 * Limit the table size to ensure it fits in small TLBs.
 */
#define TLBE(hi, md, lo)    WORD    $(hi);  WORD $(md);   WORD    $(lo)

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
 * The low 4 bits of the middle word are high bits (33-36) of the RPN;
 * we set them to zero.
 * On the virtex 5, TLBM doesn't do anything, alas.
 */

TEXT	tlbtab(SB), 1, $-4
	/* DRAM, at most 512MB */
	TLBE(KZERO | PHYSDRAM | TLB256MB | TLBVALID, PHYSDRAM,
		TLBSR | TLBSX | TLBSW)
	TLBE(KZERO | PHYSDRAM+(256*MB) | TLB256MB | TLBVALID, PHYSDRAM+(256*MB),
		TLBSR | TLBSX | TLBSW)

	/* memory-mapped IO, 1MB, 0xf0000000—0xf00fffff */
	TLBE(PHYSMMIO | TLB1MB | TLBVALID, PHYSMMIO,
		TLBSR | TLBSW | TLBI | TLBG)

	/*
	 * SRAM, 128K. put vectors here.
	 */
	TLBE(PHYSSRAM | TLB64K | TLBVALID, PHYSSRAM,
		TLBSR | TLBSX | TLBSW)
	TLBE(PHYSSRAM+64*1024 | TLB64K | TLBVALID, PHYSSRAM+64*1024,
		TLBSR | TLBSX | TLBSW)

TEXT	tlbtabe(SB), 1, $-4
	RETURN
