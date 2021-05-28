/*
 * ppc440x5 `microboot': immediately after reset, initialise the machine,
 * notably TLB entries, sufficiently that we can get out of the last 4K of
 * memory.  these nonsense constraints appears to be specific to the 440x5.
 */
#include	"mem.h"

#define MB	(1024*1024)

#define SPR_PID		0x30		/* Process ID (not the same as 405) */
#define SPR_MMUCR	0x3B2		/* mmu control */
#define SPR_CCR0	0x3b3		/* Core Configuration Register 0 */
#define SPR_CCR1	0x378		/* core configuration register 1 */
#define SPR_SRR0	0x01a		/* Save/Restore Register 0 */
#define SPR_SRR1	0x01b		/* Save/Restore Register 1 */

#define	ICCCI(a,b)	WORD	$((31<<26)|((a)<<16)|((b)<<11)|(966<<1))
#define	DCCCI(a,b)	WORD	$((31<<26)|((a)<<16)|((b)<<11)|(454<<1))
#define	MSYNC		WORD	$((31<<26)|(598<<1))

#define	TLBWELO(s,a)	WORD	$((31<<26)|((s)<<21)|((a)<<16)|(2<<11)|(978<<1))
#define	TLBWEMD(s,a)	WORD	$((31<<26)|((s)<<21)|((a)<<16)|(1<<11)|(978<<1))
#define	TLBWEHI(s,a)	WORD	$((31<<26)|((s)<<21)|((a)<<16)|(0<<11)|(978<<1))

	NOSCHED

	TEXT	_main(SB), 1, $-4
fakestart:
	CMP	R2, R3
	BEQ	lastword		/* force loading of last word */
start:
	/* we can't issue a synchronising instr. until tlbs are all loaded */
	MOVW	$setSB(SB), R2

	MOVW	$0, R0
	DCCCI(0, 2) /* this flush invalidates the dcache of a 440 (must not be in use) */

	/* see l.s */
	MOVW	$((1<<30)|(0<<21)|(1<<15)|(0<<8)|(0<<2)), R3
	MOVW	R3, SPR(SPR_CCR0)
	MOVW	$(0<<7), R3	/* TCS=0 */
	MOVW	R3, SPR(SPR_CCR1)

	/* allocate cache on store miss, disable U1 as transient, disable U2 as SWOA, no dcbf or icbi exception, tlbsx search 0 */
	MOVW	R0, SPR(SPR_MMUCR)
	ICCCI(0, 2) /* this flushes the icache of a 440; the errata reveals that EA is used; we'll use SB */

	MOVW	R0, CTR
	MOVW	R0, XER

	/* make following TLB entries shared, TID=PID=0 */
	MOVW	R0, SPR(SPR_PID)

	/* last two tlb entries cover 128K sram */
	MOVW	$63, R3
	MOVW	$(PHYSSRAM | TLB64K | TLBVALID), R5	/* TLBHI */
	TLBWEHI(5,3)
	MOVW	$(PHYSSRAM), R5				/* TLBMD */
	TLBWEMD(5,3)
	MOVW	$(TLBSR | TLBSX | TLBSW | TLBI), R5	/* TLBLO */
	TLBWELO(5,3)
	SUB	$1, R3

	MOVW	$(PHYSSRAM+(64*1024) | TLB64K | TLBVALID), R5	/* TLBHI */
	TLBWEHI(5,3)
	MOVW	$(PHYSSRAM+(64*1024)), R5		/* TLBMD */
	TLBWEMD(5,3)
	MOVW	$(TLBSR | TLBSX | TLBSW | TLBI), R5	/* TLBLO */
	TLBWELO(5,3)
	SUB	$1, R3

	/* cover DRAM in case we're going straight to kernel */
	MOVW	$(PHYSDRAM | TLB256MB | TLBVALID), R5	/* TLBHI */
	TLBWEHI(5,3)
	MOVW	$(PHYSDRAM), R5				/* TLBMD */
	TLBWEMD(5,3)
	MOVW	$(TLBSR | TLBSX | TLBSW | TLBW), R5	/* TLBLO */
	TLBWELO(5,3)
	SUB	$1, R3

	MOVW	$(PHYSDRAM+(256*MB) | TLB256MB | TLBVALID), R5	/* TLBHI */
	TLBWEHI(5,3)
	MOVW	$(PHYSDRAM+(256*MB)), R5		/* TLBMD */
	TLBWEMD(5,3)
	MOVW	$(TLBSR | TLBSX | TLBSW | TLBW), R5	/* TLBLO */
	TLBWELO(5,3)
	SUB	$1, R3

	/* and I/O registers too.  sigh. */
	MOVW	$(PHYSMMIO | TLB1MB | TLBVALID), R5	/* TLBHI */
	TLBWEHI(5,3)
	MOVW	$(PHYSMMIO), R5				/* TLBMD */
	TLBWEMD(5,3)
	MOVW	$(TLBSR | TLBSW | TLBI | TLBG), R5	/* TLBLO */
	TLBWELO(5,3)
	SUB	$1, R3

	/* invalidate the other TLB entries for now */
	MOVW	R0, R5
ztlb:
	/* can't use 0 (R0) as first operand */
	TLBWEHI(5,3)
	TLBWEMD(5,3)
	TLBWELO(5,3)
	SUB	$1, R3
	CMP	R3, $0
	BGE	ztlb

	/*
	 * we're currently relying on the shadow I/D TLBs.  to switch to
	 * the new TLBs, we need a synchronising instruction.
	 */
	MOVW	bootstart(SB), R3
	MOVW	R3, SPR(SPR_SRR0)
	MOVW	R0, SPR(SPR_SRR1)	/* new MSR */
	RFI

TEXT	bootstart(SB), 1, $-4
	WORD	$0xfffe2100
lastword:
	/* this instruction must land at 0xfffffffc */
/* this jump works for addresses within 32MB of zero (1st & last 32MB) */
/*	WORD	$((18 << 26) | (0x03FFFFFC & 0xfffe2100) | 2) */
	BR	start
