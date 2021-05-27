/*
 * Broadcom bcm2835 SoC, as used in Raspberry Pi
 * arm1176jzf-s processor (armv6)
 */

#include "arm.s"

#define CACHELINESZ 32

TEXT armstart(SB), 1, $-4

	/*
	 * SVC mode, interrupts disabled
	 */
	MOVW	$(PsrDirq|PsrDfiq|PsrMsvc), R1
	MOVW	R1, CPSR

	/*
	 * disable the mmu and L1 caches
	 * invalidate caches and tlb
	 */
	MRC	CpSC, 0, R1, C(CpCONTROL), C(0), CpMainctl
	BIC	$(CpCdcache|CpCicache|CpCpredict|CpCmmu), R1
	MCR	CpSC, 0, R1, C(CpCONTROL), C(0), CpMainctl
	MCR	CpSC, 0, R0, C(CpCACHE), C(CpCACHEinvu), CpCACHEall
	MCR	CpSC, 0, R0, C(CpTLB), C(CpTLBinvu), CpTLBinv
	ISB

	/*
	 * clear mach and page tables
	 */
	MOVW	$PADDR(MACHADDR), R1
	MOVW	$PADDR(KTZERO), R2
_ramZ:
	MOVW	R0, (R1)
	ADD	$4, R1
	CMP	R1, R2
	BNE	_ramZ

	/*
	 * start stack at top of mach (physical addr)
	 * set up page tables for kernel
	 */
	MOVW	$PADDR(MACHADDR+MACHSIZE-4), R13
	MOVW	$PADDR(L1), R0
	BL	,mmuinit(SB)

	/*
	 * set up domain access control and page table base
	 */
	MOVW	$Client, R1
	MCR	CpSC, 0, R1, C(CpDAC), C(0)
	MOVW	$PADDR(L1), R1
	MCR	CpSC, 0, R1, C(CpTTB), C(0)

	/*
	 * enable caches, mmu, and high vectors
	 */
	MRC	CpSC, 0, R0, C(CpCONTROL), C(0), CpMainctl
	ORR	$(CpChv|CpCdcache|CpCicache|CpCpredict|CpCmmu), R0
	MCR	CpSC, 0, R0, C(CpCONTROL), C(0), CpMainctl
	ISB

	/*
	 * switch SB, SP, and PC into KZERO space
	 */
	MOVW	$setR12(SB), R12
	MOVW	$(MACHADDR+MACHSIZE-4), R13
	MOVW	$_startpg(SB), R15

TEXT _startpg(SB), 1, $-4

	/*
	 * enable cycle counter
	 */
	MOVW	$1, R1
	MCR	CpSC, 0, R1, C(CpSPM), C(CpSPMperf), CpSPMctl

	/*
	 * call main and loop forever if it returns
	 */
	BL	,main(SB)
	B	,0(PC)

	BL	_div(SB)		/* hack to load _div, etc. */

TEXT cpidget(SB), 1, $-4			/* main ID */
	MRC	CpSC, 0, R0, C(CpID), C(0), CpIDid
	RET

TEXT fsrget(SB), 1, $-4				/* data fault status */
	MRC	CpSC, 0, R0, C(CpFSR), C(0), CpFSRdata
	RET

TEXT ifsrget(SB), 1, $-4			/* instruction fault status */
	MRC	CpSC, 0, R0, C(CpFSR), C(0), CpFSRinst
	RET

TEXT farget(SB), 1, $-4				/* fault address */
	MRC	CpSC, 0, R0, C(CpFAR), C(0x0)
	RET

TEXT lcycles(SB), 1, $-4
	MRC	CpSC, 0, R0, C(CpSPM), C(CpSPMperf), CpSPMcyc
	RET

TEXT splhi(SB), 1, $-4
	MOVW	$(MACHADDR+4), R2		/* save caller pc in Mach */
	MOVW	R14, 0(R2)

	MOVW	CPSR, R0			/* turn off irqs (but not fiqs) */
	ORR	$(PsrDirq), R0, R1
	MOVW	R1, CPSR
	RET

TEXT splfhi(SB), 1, $-4
	MOVW	$(MACHADDR+4), R2		/* save caller pc in Mach */
	MOVW	R14, 0(R2)

	MOVW	CPSR, R0			/* turn off irqs and fiqs */
	ORR	$(PsrDirq|PsrDfiq), R0, R1
	MOVW	R1, CPSR
	RET

TEXT splflo(SB), 1, $-4
	MOVW	CPSR, R0			/* turn on fiqs */
	BIC	$(PsrDfiq), R0, R1
	MOVW	R1, CPSR
	RET

TEXT spllo(SB), 1, $-4
	MOVW	CPSR, R0			/* turn on irqs and fiqs */
	BIC	$(PsrDirq|PsrDfiq), R0, R1
	MOVW	R1, CPSR
	RET

TEXT splx(SB), 1, $-4
	MOVW	$(MACHADDR+0x04), R2		/* save caller pc in Mach */
	MOVW	R14, 0(R2)

	MOVW	R0, R1				/* reset interrupt level */
	MOVW	CPSR, R0
	MOVW	R1, CPSR
	RET

TEXT spldone(SB), 1, $0				/* end marker for devkprof.c */
	RET

TEXT islo(SB), 1, $-4
	MOVW	CPSR, R0
	AND	$(PsrDirq), R0
	EOR	$(PsrDirq), R0
	RET

TEXT	tas(SB), $-4
TEXT	_tas(SB), $-4
	MOVW	R0,R1
	MOVW	$1,R0
	SWPW	R0,(R1)			/* fix: deprecated in armv6 */
	RET

TEXT setlabel(SB), 1, $-4
	MOVW	R13, 0(R0)		/* sp */
	MOVW	R14, 4(R0)		/* pc */
	MOVW	$0, R0
	RET

TEXT gotolabel(SB), 1, $-4
	MOVW	0(R0), R13		/* sp */
	MOVW	4(R0), R14		/* pc */
	MOVW	$1, R0
	RET

TEXT getcallerpc(SB), 1, $-4
	MOVW	0(R13), R0
	RET

TEXT idlehands(SB), $-4
	MOVW	CPSR, R3
	ORR	$(PsrDirq|PsrDfiq), R3, R1		/* splfhi */
	MOVW	R1, CPSR

	DSB
	MOVW	nrdy(SB), R0
	CMP	$0, R0
	MCR.EQ	CpSC, 0, R0, C(CpCACHE), C(CpCACHEintr), CpCACHEwait
	DSB

	MOVW	R3, CPSR			/* splx */
	RET


TEXT coherence(SB), $-4
	BARRIERS
	RET

/*
 * invalidate tlb
 */
TEXT mmuinvalidate(SB), 1, $-4
	MOVW	$0, R0
	MCR	CpSC, 0, R0, C(CpTLB), C(CpTLBinvu), CpTLBinv
	BARRIERS
	MCR CpSC, 0, R0, C(CpCACHE), C(CpCACHEinvi), CpCACHEflushbtc
	RET

/*
 * mmuinvalidateaddr(va)
 *   invalidate tlb entry for virtual page address va, ASID 0
 */
TEXT mmuinvalidateaddr(SB), 1, $-4
	MCR	CpSC, 0, R0, C(CpTLB), C(CpTLBinvu), CpTLBinvse
	BARRIERS
	RET

/*
 * drain write buffer
 * writeback data cache
 */
TEXT cachedwb(SB), 1, $-4
	DSB
	MOVW	$0, R0
	MCR	CpSC, 0, R0, C(CpCACHE), C(CpCACHEwb), CpCACHEall
	RET

/*
 * drain write buffer
 * writeback and invalidate data cache
 */
TEXT cachedwbinv(SB), 1, $-4
	DSB
	MOVW	$0, R0
	MCR	CpSC, 0, R0, C(CpCACHE), C(CpCACHEwbi), CpCACHEall
	RET

/*
 * cachedwbinvse(va, n)
 *   drain write buffer
 *   writeback and invalidate data cache range [va, va+n)
 */
TEXT cachedwbinvse(SB), 1, $-4
	MOVW	R0, R1		/* DSB clears R0 */
	DSB
	MOVW	n+4(FP), R2
	ADD	R1, R2
	SUB	$1, R2
	BIC	$(CACHELINESZ-1), R1
	BIC	$(CACHELINESZ-1), R2
	MCRR(CpSC, 0, 2, 1, CpCACHERANGEdwbi)
	RET

/*
 * cachedwbse(va, n)
 *   drain write buffer
 *   writeback data cache range [va, va+n)
 */
TEXT cachedwbtlb(SB), 1, $-4
TEXT cachedwbse(SB), 1, $-4

	MOVW	R0, R1		/* DSB clears R0 */
	DSB
	MOVW	n+4(FP), R2
	ADD	R1, R2
	BIC	$(CACHELINESZ-1), R1
	BIC	$(CACHELINESZ-1), R2
	MCRR(CpSC, 0, 2, 1, CpCACHERANGEdwb)
	RET

/*
 * cachedinvse(va, n)
 *   drain write buffer
 *   invalidate data cache range [va, va+n)
 */
TEXT cachedinvse(SB), 1, $-4
	MOVW	R0, R1		/* DSB clears R0 */
	DSB
	MOVW	n+4(FP), R2
	ADD	R1, R2
	SUB	$1, R2
	BIC	$(CACHELINESZ-1), R1
	BIC	$(CACHELINESZ-1), R2
	MCRR(CpSC, 0, 2, 1, CpCACHERANGEinvd)
	RET

/*
 * drain write buffer and prefetch buffer
 * writeback and invalidate data cache
 * invalidate instruction cache
 */
TEXT cacheuwbinv(SB), 1, $-4
	BARRIERS
	MOVW	$0, R0
	MCR	CpSC, 0, R0, C(CpCACHE), C(CpCACHEwbi), CpCACHEall
	MCR	CpSC, 0, R0, C(CpCACHE), C(CpCACHEinvi), CpCACHEall
	RET

/*
 * L2 cache is not enabled
 */
TEXT l2cacheuwbinv(SB), 1, $-4
	RET

/*
 * invalidate instruction cache
 */
TEXT cacheiinv(SB), 1, $-4
	MOVW	$0, R0
	MCR	CpSC, 0, R0, C(CpCACHE), C(CpCACHEinvi), CpCACHEall
	RET

/*
 * invalidate range of instruction cache
 */
TEXT cacheiinvse(SB), 1, $-4
	MOVW	R0, R1		/* DSB clears R0 */
	DSB
	MOVW n+4(FP), R2
	ADD	R1, R2
	SUB	$1, R2
	MCRR(CpSC, 0, 2, 1, CpCACHERANGEinvi)
	MCR CpSC, 0, R0, C(CpCACHE), C(CpCACHEinvi), CpCACHEflushbtc
	DSB
	ISB
	RET
