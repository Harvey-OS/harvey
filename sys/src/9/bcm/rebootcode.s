/*
 * armv6 reboot code
 */
#include "arm.s"

/*
 * Turn off MMU, then copy the new kernel to its correct location
 * in physical memory.  Then jump to the start of the kernel.
 */

/* main(PADDR(entry), PADDR(code), size); */
TEXT	main(SB), 1, $-4
	MOVW	$setR12(SB), R12

	/* copy in arguments before stack gets unmapped */
	MOVW	R0, R8			/* entry point */
	MOVW	p2+4(FP), R9		/* source */
	MOVW	n+8(FP), R10		/* byte count */

	/* SVC mode, interrupts disabled */
	MOVW	$(PsrDirq|PsrDfiq|PsrMsvc), R1
	MOVW	R1, CPSR

	/* prepare to turn off mmu  */
	BL	cachesoff(SB)

	/* turn off mmu */
	MRC	CpSC, 0, R1, C(CpCONTROL), C(0), CpMainctl
	BIC	$CpCmmu, R1
	MCR	CpSC, 0, R1, C(CpCONTROL), C(0), CpMainctl

	/* set up a tiny stack for local vars and memmove args */
	MOVW	R8, SP			/* stack top just before kernel dest */
	SUB	$20, SP			/* allocate stack frame */

	/* copy the kernel to final destination */
	MOVW	R8, 16(SP)		/* save dest (entry point) */
	MOVW	R8, R0			/* first arg is dest */
	MOVW	R9, 8(SP)		/* push src */
	MOVW	R10, 12(SP)		/* push size */
	BL	memmove(SB)
	MOVW	16(SP), R8		/* restore entry point */

	/* jump to kernel physical entry point */
	B	(R8)
	B	0(PC)

/*
 * turn the caches off, double map PHYSDRAM & KZERO, invalidate TLBs, revert
 * to tiny addresses.  upon return, it will be safe to turn off the mmu.
 * clobbers R0-R2, and returns with SP invalid.
 */
TEXT cachesoff(SB), 1, $-4

	/* write back and invalidate caches */
	BARRIERS
	MOVW	$0, R0
	MCR	CpSC, 0, R0, C(CpCACHE), C(CpCACHEwbi), CpCACHEall
	MCR	CpSC, 0, R0, C(CpCACHE), C(CpCACHEinvi), CpCACHEall

	/* turn caches off */
	MRC	CpSC, 0, R1, C(CpCONTROL), C(0), CpMainctl
	BIC	$(CpCdcache|CpCicache|CpCpredict), R1
	MCR	CpSC, 0, R1, C(CpCONTROL), C(0), CpMainctl

	/* invalidate stale TLBs before changing them */
	BARRIERS
	MOVW	$KZERO, R0			/* some valid virtual address */
	MCR	CpSC, 0, R0, C(CpTLB), C(CpTLBinvu), CpTLBinv
	BARRIERS

	/* from here on, R0 is base of physical memory */
	MOVW	$PHYSDRAM, R0

	/* redo double map of first MiB PHYSDRAM = KZERO */
	MOVW	$(L1+L1X(PHYSDRAM)), R2		/* address of PHYSDRAM's PTE */
	MOVW	$PTEDRAM, R1			/* PTE bits */
	ORR	R0, R1				/* dram base */
	MOVW	R1, (R2)

	/* invalidate stale TLBs again */
	BARRIERS
	MCR	CpSC, 0, R0, C(CpTLB), C(CpTLBinvu), CpTLBinv
	BARRIERS

	/* relocate SB and return address to PHYSDRAM addressing */
	MOVW	$KSEGM, R1		/* clear segment bits */
	BIC	R1, R12			/* adjust SB */
	ORR	R0, R12
	BIC	R1, R14			/* adjust return address */
	ORR	R0, R14

	RET
