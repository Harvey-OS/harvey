/*
 * sheevaplug reboot code
 *
 * R11 is used by the loader as a temporary, so avoid it.
 */
#include "arm.s"

/*
 * Turn off MMU, then copy the new kernel to its correct location
 * in physical memory.  Then jump to the start of the kernel.
 */

/* main(PADDR(entry), PADDR(code), size); */
TEXT	main(SB), 1, $-4
	MOVW	$setR12(SB), R12

	MOVW	R0, p1+0(FP)		/* destination, passed in R0 */

	/* copy in arguments from frame */
	MOVW	R0, R8			/* entry point */
	MOVW	p2+4(FP), R9		/* source */
	MOVW	n+8(FP), R10		/* byte count */

WAVE('R')
	BL	cachesoff(SB)
	/* now back in 29- or 26-bit addressing, mainly for SB */

	/* turn the MMU off */
WAVE('e')
	MOVW	$KSEGM, R7
	MOVW	$PHYSDRAM, R0
	BL	_r15warp(SB)

	BIC	R7, R12			/* SB */
	BIC	R7, R13			/* SP */
	/* don't care about R14 */

WAVE('b')
	BL	mmuinvalidate(SB)
WAVE('o')
	BL	mmudisable(SB)

WAVE('o')
	MOVW	R9, R4			/* restore regs across function calls */
	MOVW	R10, R5
	MOVW	R8, R6

	/* set up a new stack for local vars and memmove args */
	MOVW	R6, SP			/* tiny trampoline stack */
	SUB	$(0x20 + 4), SP		/* back up before a.out header */

	MOVW	R14, -48(SP)		/* store return addr */
	SUB	$48, SP			/* allocate stack frame */

	MOVW	R6, 44(SP)		/* save dest/entry */
	MOVW	R5, 40(SP)		/* save count */

WAVE('t')

	MOVW	R6, 0(SP)
	MOVW	R6, 4(SP)		/* push dest */
	MOVW	R6, R0
	MOVW	R4, 8(SP)		/* push src */
	MOVW	R5, 12(SP)		/* push size */
	BL	memmove(SB)

	MOVW	44(SP), R6		/* restore R6 (dest/entry) */
	MOVW	40(SP), R5		/* restore R5 (count) */
WAVE('-')
	/*
	 * flush caches
	 */
	BL	cacheuwbinv(SB)

WAVE('>')
WAVE('\r');
WAVE('\n');
/*
 * jump to kernel entry point.  Note the true kernel entry point is
 * the virtual address KZERO|R6, but this must wait until
 * the MMU is enabled by the kernel in l.s
 */
	ORR	R6, R6			/* NOP: avoid link bug */
	B	(R6)

/*
 * turn the caches off, double map 0 & KZERO, invalidate TLBs, revert to
 * tiny addresses.  upon return, it will be safe to turn off the mmu.
 */
TEXT cachesoff(SB), 1, $-4
	MOVW	$(PsrDirq|PsrDfiq|PsrMsvc), R0
	MOVW	R0, CPSR
	MOVW	$KADDR(0x100-4), R7		/* just before this code */
	MOVW	R14, (R7)			/* save link */

	BL	cacheuwbinv(SB)

	MRC	CpSC, 0, R0, C(CpCONTROL), C(0)
	BIC	$(CpCwb|CpCicache|CpCdcache|CpCalign), R0
	MCR     CpSC, 0, R0, C(CpCONTROL), C(0)
	BARRIERS

	/* redo double map of 0, KZERO */
	MOVW	$(L1+L1X(PHYSDRAM)), R4		/* address of PTE for 0 */
	MOVW	$PTEDRAM, R2			/* PTE bits */
//	MOVW	$PTEIO, R2			/* PTE bits */
	MOVW	$PHYSDRAM, R3
	MOVW	$512, R5
_ptrdbl:
	ORR	R3, R2, R1		/* first identity-map 0 to 0, etc. */
	MOVW	R1, (R4)
	ADD	$4, R4				/* bump PTE address */
	ADD	$MiB, R3			/* bump pa */
	SUB.S	$1, R5
	BNE	_ptrdbl

	BARRIERS
	MOVW	$0, R0
	MCR	CpSC, 0, R0, C(CpTLB), C(CpTLBinvd), CpTLBinv
	MCR	CpSC, 0, R0, C(CpTLB), C(CpTLBinvu), CpTLBinv
	BARRIERS

	/* back to 29- or 26-bit addressing, mainly for SB */
	MRC	CpSC, 0, R0, C(CpCONTROL), C(0)
	BIC	$(CpCd32|CpCi32), R0
	MCR     CpSC, 0, R0, C(CpCONTROL), C(0)
	BARRIERS

	MOVW	$KADDR(0x100-4), R7		/* just before this code */
	MOVW	(R7), R14			/* restore link */
	RET

TEXT _r15warp(SB), 1, $-4
	BIC	$KSEGM, R14
	ORR	R0, R14
	RET

TEXT mmudisable(SB), 1, $-4
	MRC	CpSC, 0, R0, C(CpCONTROL), C(0)
	BIC	$(CpChv|CpCmmu|CpCdcache|CpCicache|CpCwb), R0
	MCR     CpSC, 0, R0, C(CpCONTROL), C(0)
	BARRIERS
	RET

TEXT mmuinvalidate(SB), 1, $-4			/* invalidate all */
	MOVW	$0, R0
	MCR	CpSC, 0, R0, C(CpTLB), C(CpTLBinvu), CpTLBinv
	BARRIERS
	RET

TEXT cacheuwbinv(SB), 1, $-4			/* D+I writeback+invalidate */
	BARRIERS
	MOVW	CPSR, R3			/* splhi */
	ORR	$(PsrDirq), R3, R1
	MOVW	R1, CPSR

_uwbinv:					/* D writeback+invalidate */
	MRC	CpSC, 0, PC, C(CpCACHE), C(CpCACHEwbi), CpCACHEtest
	BNE	_uwbinv

	MOVW	$0, R0				/* I invalidate */
	MCR	CpSC, 0, R0, C(CpCACHE), C(CpCACHEinvi), CpCACHEall
	/* drain L1 write buffer, also drains L2 eviction buffer on sheeva */
	BARRIERS

	MCR	CpSC, CpL2, R0, C(CpTESTCFG), C(CpTCl2flush), CpTCl2all
	BARRIERS
	MCR	CpSC, CpL2, R0, C(CpTESTCFG), C(CpTCl2inv), CpTCl2all
	BARRIERS

	MOVW	R3, CPSR			/* splx */
	RET
