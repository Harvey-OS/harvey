#include	"mem.h"
#define	MB	(1024*1024)

/*
 * TLB prototype entries, loaded once-for-all at startup,
 * remaining unchanged thereafter.
 * Limit the table size to ensure it fits in small TLBs.
 */
#define TLBE(hi, md, lo)    WORD    $(hi);  WORD $(md);   WORD    $(lo)

TEXT    tlbtab(SB), $-4

        /* tlbhi tlblo */

        /* DRAM, 512MB */
        TLBE(KZERO|PHYSDRAM|TLB256MB|TLBVALID, PHYSDRAM, TLBSR|TLBSX|TLBSW)
        TLBE(KZERO|(PHYSDRAM+256*MB)|TLB256MB|TLBVALID, (PHYSDRAM+256*MB), TLBSR|TLBSX|TLBSW)

        /* memory-mapped IO, 4K */
        TLBE(PHYSMMIO|TLB4K|TLBVALID, REALMMIO|1, TLBSR|TLBSW|TLBI|TLBG)	/* note ERPN */

		/* flash (eval board) */
		/* to map 32mbytes at once, we must map 256mbytes */
		TLBE(0xF0000000|TLB256MB|TLBVALID, 0xF0000000|1, TLBSR|TLBSW|TLBSX|TLBI|TLBG)

//        TLBE(PHYSEMAC4|TLB4K|TLBVALID, PHYSEMAC4|1, TLBSR|TLBSW|TLBI|TLBG)	/* note ERPN */

TEXT    tlbtabe(SB), $-4
        RETURN
