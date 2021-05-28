/* virtex5 ppc440x5 bootstrap tlb entries */
#include	"mem.h"

#define MB (1024*1024)

/*
 * TLB prototype entries, loaded once-for-all at startup,
 * remaining unchanged thereafter.
 * Limit the table size to ensure it fits in small TLBs.
 * First entry will be tlb entry #63 and we count down from there.
 *
 * Add TLBW for write-through caching.
 * TLBM doesn't help on the Xilinx virtex 5, alas.
 * The low 4 bits of the middle word are high bits (33-36) of the RPN;
 * we set them to zero.
 */
#define TLBE(hi, md, lo)    WORD    $(hi);  WORD $(md);   WORD    $(lo)

TEXT    tlbtab(SB), 1, $-4

	/*
	 * SRAM, 128K.  put vectors here.
	 * the `microboot' at the end of SRAM has already installed these
	 * TLB entries for SRAM.
	 */
	TLBE(PHYSSRAM | TLB64K | TLBVALID, PHYSSRAM,
		TLBSR | TLBSX | TLBSW)
	TLBE(PHYSSRAM+(64*1024) | TLB64K | TLBVALID, PHYSSRAM+(64*1024),
		TLBSR | TLBSX | TLBSW)

	/* DRAM, 512MB */
	TLBE(PHYSDRAM | TLB256MB | TLBVALID, PHYSDRAM,
		TLBSR | TLBSX | TLBSW)
	TLBE(PHYSDRAM+(256*MB) | TLB256MB | TLBVALID, PHYSDRAM+(256*MB),
		TLBSR | TLBSX | TLBSW)

	/* memory-mapped IO, 1MB */
	TLBE(PHYSMMIO | TLB1MB | TLBVALID, PHYSMMIO,
		TLBSR | TLBSW | TLBI | TLBG)

TEXT    tlbtabe(SB), 1, $-4
	RETURN
