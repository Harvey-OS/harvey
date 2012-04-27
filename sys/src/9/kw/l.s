/*
 * sheevaplug machine assist
 * arm926ej-s processor at 1.2GHz
 *
 * loader uses R11 as scratch.
 * R9 and R10 are used for `extern register' variables.
 *
 * ARM v7 arch. ref. man. (I know, this is v5) §B1.3.3 that
 * we don't need barriers around moves to CPSR.  The ARM v6 manual
 * seems to be silent on the subject.
 */
#include "arm.s"

/*
 * MCR and MRC are counter-intuitively named.
 *	MCR	coproc, opcode1, Rd, CRn, CRm[, opcode2]	# arm -> coproc
 *	MRC	coproc, opcode1, Rd, CRn, CRm[, opcode2]	# coproc -> arm
 */

/*
 * Entered here from Das U-Boot with MMU disabled.
 * Until the MMU is enabled it is OK to call functions provided
 * they are within ±32MiB relative and do not require any
 * local variables or more than one argument (i.e. there is
 * no stack).
 */
TEXT _start(SB), 1, $-4
	MOVW	$setR12(SB), R12		/* load the SB */
_main:
	/* SVC mode, interrupts disabled */
	MOVW	$(PsrDirq|PsrDfiq|PsrMsvc), R1
	MOVW	R1, CPSR
	BARRIERS

	/*
	 * disable the MMU & caches,
	 * switch to system permission & 32-bit addresses.
	 */
	MOVW	$(CpCsystem|CpCd32|CpCi32), R1
	MCR     CpSC, 0, R1, C(CpCONTROL), C(0)
	ISB

	/*
	 * disable the Sheevaplug's L2 cache, invalidate all caches
	 */

	/* flush caches.  926ejs manual says we have to do it iteratively. */
_dwbinv0:
	MRC	CpSC, 0, PC, C(CpCACHE), C(CpCACHEwbi), CpCACHEtest
	BNE	_dwbinv0
	/* drain L1 write buffer, also drains L2 eviction buffer on sheeva */
	BARRIERS

	/* make the l2 cache pay attention */
	MOVW	$(PHYSIO+0x20100), R1	/* CPUCSREG */
	MOVW	(4*10)(R1), R2
	ORR	$(1<<3), R2		/* cpu->l2cfg |= L2exists */
	MOVW	R2, (4*10)(R1)
	ISB

	/* invalidate l2 cache */
	MCR	CpSC, CpL2, R0, C(CpTESTCFG), C(CpTCl2inv), CpTCl2all
	ISB

	/* disable l2 cache.  do this while l1 caches are off */
	MRC	CpSC, CpL2, R1, C(CpTESTCFG), C(CpTCl2cfg), CpTCl2conf
	/* disabling write allocation is probably for cortex-a8 errata 460075 */
	/* l2 off, no wr alloc, no streaming */
	BIC	$(CpTCl2ena | CpTCl2wralloc | CpTCldcstream), R1
	MCR	CpSC, CpL2, R1, C(CpTESTCFG), C(CpTCl2cfg), CpTCl2conf
	BARRIERS

	/* flush caches.  926ejs manual says we have to do it iteratively. */
_dwbinv1:
	MRC	CpSC, 0, PC, C(CpCACHE), C(CpCACHEwbi), CpCACHEtest
	BNE	_dwbinv1
	BARRIERS

WAVE('\r')
	/* clear Mach */
	MOVW	$PADDR(MACHADDR), R4		/* address of Mach */
_machZ:
	MOVW	R0, (R4)
	ADD	$4, R4				/* bump PTE address */
	CMP.S	$PADDR(L1+L1X(0)), R4
	BNE	_machZ

	/*
	 * set up the MMU page table
	 */

	/* clear all PTEs first, to provide a default */
WAVE('\n')
	MOVW	$PADDR(L1+L1X(0)), R4		/* address of PTE for 0 */
_ptenv0:
	ZEROPTE()
	CMP.S	$PADDR(L1+16*KiB), R4
	BNE	_ptenv0

	/* double map of PHYSDRAM, KZERO to PHYSDRAM for first few MBs */
	MOVW	$PTEDRAM, R2			/* PTE bits */
	MOVW	$PHYSDRAM, R3			/* pa */
	MOVW	$PADDR(L1+L1X(PHYSDRAM)), R4  /* address of PTE for PHYSDRAM */
	MOVW	$16, R5
_ptdbl:
	FILLPTE()
	SUB.S	$1, R5
	BNE	_ptdbl

	/*
	 * back up and fill in PTEs for memory at KZERO
	 * there is 1 bank of 512MB of SDRAM at PHYSDRAM
	 */
	MOVW	$PTEDRAM, R2			/* PTE bits */
	MOVW	$PHYSDRAM, R3
	MOVW	$PADDR(L1+L1X(KZERO)), R4	/* start with PTE for KZERO */
	MOVW	$512, R5			/* inner loop count */
_ptekrw:					/* set PTEs for 512MiB */
	FILLPTE()
	SUB.S	$1, R5
	BNE	_ptekrw

	/*
	 * back up and fill in PTE for MMIO
	 */
	MOVW	$PTEIO, R2			/* PTE bits */
	MOVW	$PHYSIO, R3
	MOVW	$PADDR(L1+L1X(VIRTIO)), R4	/* start with PTE for VIRTIO */
	FILLPTE()

	/* mmu.c sets up the vectors later */

	/*
	 * set up a temporary stack; avoid data & bss segments
	 */
	MOVW	$(PHYSDRAM | (128*1024*1024)), R13

WAVE('P')
	/* set the domain access control */
	MOVW	$Client, R0
	BL	dacput(SB)

	/* set the translation table base */
	MOVW	$PADDR(L1), R0
	BL	ttbput(SB)

	MOVW	$0, R0
	BL	pidput(SB)		/* paranoia */

	/* the little dance to turn the MMU & caches on */
WAVE('l')
	BL	cacheuwbinv(SB)
	BL	mmuinvalidate(SB)
	BL	mmuenable(SB)

WAVE('a')
	/* warp the PC into the virtual map */
	MOVW	$KZERO, R0
	BL	_r15warp(SB)

	/*
	 * now running at KZERO+something!
	 */

	MOVW	$setR12(SB), R12		/* reload the SB */

	/*
	 * set up temporary stack again, in case we've just switched
	 * to a new register set.
	 */
	MOVW	$(KZERO|(128*1024*1024)), R13

	/* can now execute arbitrary C code */

	BL	cacheuwbinv(SB)

WAVE('n')
	/* undo double map of 0, KZERO */
	MOVW	$PADDR(L1+L1X(0)), R4		/* address of PTE for 0 */
	MOVW	$0, R0
	MOVW	$16, R5
_ptudbl:
	MOVW	R0, (R4)
	ADD	$4, R4				/* bump PTE address */
	ADD	$MiB, R0			/* bump pa */
	SUB.S	$1, R5
	BNE	_ptudbl
	BARRIERS
	MCR	CpSC, 0, R0, C(CpTLB), C(CpTLBinvd), CpTLBinvse
	MCR	CpSC, 0, R0, C(CpTLB), C(CpTLBinvu), CpTLBinv
	BARRIERS

WAVE(' ')
	/* pass Mach to main and set up the stack */
	MOVW	$(MACHADDR), R0			/* Mach */
	MOVW	R0, R13
	ADD	$(MACHSIZE), R13		/* stack pointer */
	SUB	$4, R13				/* space for link register */

	BL	main(SB)			/* void main(Mach*) */
	/* fall through */


/* not used */
TEXT _reset(SB), 1, $-4
	/* turn the caches off */
	MOVW	$(PsrDirq|PsrDfiq|PsrMsvc), R0
	MOVW	R0, CPSR
	BARRIERS
	BL	cacheuwbinv(SB)
	MRC	CpSC, 0, R0, C(CpCONTROL), C(0)
	BIC	$(CpCwb|CpCicache|CpCdcache|CpCalign), R0
	MCR     CpSC, 0, R0, C(CpCONTROL), C(0)
	BARRIERS
WAVE('R')

	/* redo double map of 0, KZERO */
	MOVW	$(L1+L1X(0)), R4		/* address of PTE for 0 */
	MOVW	$PTEDRAM, R2			/* PTE bits */
	MOVW	$0, R3
	MOVW	$16, R5
_ptrdbl:
	ORR	R3, R2, R1		/* first identity-map 0 to 0, etc. */
	MOVW	R1, (R4)
	ADD	$4, R4				/* bump PTE address */
	ADD	$MiB, R3			/* bump pa */
	SUB.S	$1, R5
	BNE	_ptrdbl

	BARRIERS
WAVE('e')
	MOVW	$0, R0
	MCR	CpSC, 0, R0, C(CpTLB), C(CpTLBinvd), CpTLBinv
	MCR	CpSC, 0, R0, C(CpTLB), C(CpTLBinvu), CpTLBinv
	BARRIERS

	/* back to 29- or 26-bit addressing, mainly for SB */
	MRC	CpSC, 0, R0, C(CpCONTROL), C(0)
	BIC	$(CpCd32|CpCi32), R0
	MCR     CpSC, 0, R0, C(CpCONTROL), C(0)
	BARRIERS

	/* turn the MMU off */
	MOVW	$PHYSDRAM, R0
	BL	_r15warp(SB)
	BL	mmuinvalidate(SB)
	BL	mmudisable(SB)

WAVE('s')
	/* set new reset vector */
	MOVW	$0, R2
	MOVW	$0xe59ff018, R3			/* MOVW 0x18(R15), R15 */
	MOVW	R3, (R2)
WAVE('e')

	MOVW	$PHYSBOOTROM, R3
	MOVW	R3, 0x20(R2)			/* where $0xe59ff018 jumps to */
	BARRIERS
WAVE('t')
WAVE('\r')
WAVE('\n')

	/* ...and jump to it */
	MOVW	R2, R15				/* software reboot */
_limbo:						/* should not get here... */
	B	_limbo				/* ... and can't get out */
	BL	_div(SB)			/* hack to load _div, etc. */

TEXT _r15warp(SB), 1, $-4
	BIC	$KSEGM, R14
	ORR	R0, R14
	BIC	$KSEGM, R13
	ORR	R0, R13
	RET

/* clobbers R1, R6 */
TEXT myputc(SB), 1, $-4
	MOVW	$PHYSCONS, R6
_busy:
	MOVW	20(R6), R1
	BIC.S	$~(1<<5), R1			/* (x->lsr & LSRthre) == 0? */
	BEQ	_busy
	MOVW	R3, (R6)			/* print */
	ISB
	RET

/*
 * l1 caches
 */

TEXT l1cacheson(SB), 1, $-4
	MOVW	CPSR, R5
	ORR	$(PsrDirq|PsrDfiq), R5, R4
	MOVW	R4, CPSR			/* splhi */

	MRC	CpSC, 0, R0, C(CpCONTROL), C(0)
	ORR	$(CpCdcache|CpCicache|CpCwb), R0
	MCR     CpSC, 0, R0, C(CpCONTROL), C(0)
	BARRIERS

	MOVW	R5, CPSR			/* splx */
	RET

TEXT l1cachesoff(SB), 1, $-4
	MOVM.DB.W [R14], (SP)			/* save lr on stack */

	MOVW	CPSR, R5
	ORR	$(PsrDirq|PsrDfiq), R5, R4
	MOVW	R4, CPSR			/* splhi */

	BL	cacheuwbinv(SB)

	MRC	CpSC, 0, R0, C(CpCONTROL), C(0)
	BIC	$(CpCdcache|CpCicache|CpCwb), R0
	MCR     CpSC, 0, R0, C(CpCONTROL), C(0)
	BARRIERS

	MOVW	R5, CPSR			/* splx */
	MOVM.IA.W (SP), [R14]			/* restore lr */
	RET

/*
 * cache* functions affect only the L1 caches, which are VIVT.
 */

TEXT cachedwb(SB), 1, $-4			/* D writeback */
	MOVW	CPSR, R3			/* splhi */
	ORR	$(PsrDirq), R3, R1
	MOVW	R1, CPSR

	BARRIERS			/* force outstanding stores to cache */
	/* keep writing back dirty cache lines until no more exist */
_dwb:
	MRC	CpSC, 0, PC, C(CpCACHE), C(CpCACHEwb), CpCACHEtest
	BNE	_dwb
	/* drain L1 write buffer, also drains L2 eviction buffer on sheeva */
	BARRIERS

	MOVW	R3, CPSR			/* splx */
	RET

TEXT cachedwbse(SB), 1, $-4			/* D writeback SE */
	MOVW	R0, R2				/* first arg: address */

	MOVW	CPSR, R3			/* splhi */
	ORR	$(PsrDirq), R3, R1
	MOVW	R1, CPSR

	BARRIERS			/* force outstanding stores to cache */
	MOVW	4(FP), R1			/* second arg: size */

//	CMP.S	$(4*1024), R1
//	BGT	_dwb
	ADD	R2, R1
	BIC	$(CACHELINESZ-1), R2
_dwbse:
	MCR	CpSC, 0, R2, C(CpCACHE), C(CpCACHEwb), CpCACHEse
	ADD	$CACHELINESZ, R2
	CMP.S	R2, R1
	BGT	_dwbse
	/* drain L1 write buffer, also drains L2 eviction buffer on sheeva */
	BARRIERS

	MOVW	R3, CPSR			/* splx */
	RET

TEXT cachedwbinv(SB), 1, $-4			/* D writeback+invalidate */
	MOVW	CPSR, R3			/* splhi */
	ORR	$(PsrDirq), R3, R1
	MOVW	R1, CPSR

	BARRIERS			/* force outstanding stores to cache */
	/* keep writing back dirty cache lines until no more exist */
_dwbinv:
	MRC	CpSC, 0, PC, C(CpCACHE), C(CpCACHEwbi), CpCACHEtest
	BNE	_dwbinv
	/* drain L1 write buffer, also drains L2 eviction buffer on sheeva */
	BARRIERS

	MOVW	R3, CPSR			/* splx */
	RET

TEXT cachedwbinvse(SB), 1, $-4			/* D writeback+invalidate SE */
	MOVW	R0, R2				/* first arg: address */

	MOVW	CPSR, R3			/* splhi */
	ORR	$(PsrDirq), R3, R1
	MOVW	R1, CPSR

	BARRIERS			/* force outstanding stores to cache */
	MOVW	4(FP), R1			/* second arg: size */

	DSB
//	CMP.S	$(4*1024), R1
//	BGT	_dwbinv
	ADD	R2, R1
	BIC	$(CACHELINESZ-1), R2
_dwbinvse:
	MCR	CpSC, 0, R2, C(CpCACHE), C(CpCACHEwbi), CpCACHEse
	ADD	$CACHELINESZ, R2
	CMP.S	R2, R1
	BGT	_dwbinvse
	/* drain L1 write buffer, also drains L2 eviction buffer on sheeva */
	BARRIERS

	MOVW	R3, CPSR			/* splx */
	RET

TEXT cachedinvse(SB), 1, $-4			/* D invalidate SE */
	MOVW	R0, R2				/* first arg: address */

	MOVW	CPSR, R3			/* splhi */
	ORR	$(PsrDirq), R3, R1
	MOVW	R1, CPSR

	MOVW	4(FP), R1			/* second arg: size */

	DSB
//	CMP.S	$(4*1024), R1
//	BGT	_dinv
	ADD	R2, R1
	BIC	$(CACHELINESZ-1), R2
_dinvse:
	MCR	CpSC, 0, R2, C(CpCACHE), C(CpCACHEinvd), CpCACHEse
	ADD	$CACHELINESZ, R2
	CMP.S	R2, R1
	BGT	_dinvse
	/* drain L1 write buffer, also drains L2 eviction buffer on sheeva */
	BARRIERS

	MOVW	R3, CPSR			/* splx */
	RET

TEXT cacheuwbinv(SB), 1, $-4			/* D+I writeback+invalidate */
	MOVW	CPSR, R3			/* splhi */
	ORR	$(PsrDirq), R3, R1
	MOVW	R1, CPSR

	BARRIERS			/* force outstanding stores to cache */
	/* keep writing back dirty cache lines until no more exist */
_uwbinv:					/* D writeback+invalidate */
	MRC	CpSC, 0, PC, C(CpCACHE), C(CpCACHEwbi), CpCACHEtest
	BNE	_uwbinv
	/* drain L1 write buffer, also drains L2 eviction buffer on sheeva */
	BARRIERS

	MOVW	$0, R0				/* I invalidate */
	MCR	CpSC, 0, R0, C(CpCACHE), C(CpCACHEinvi), CpCACHEall
	/* drain L1 write buffer, also drains L2 eviction buffer on sheeva */
	BARRIERS

	MOVW	R3, CPSR			/* splx */
	RET

TEXT cacheiinv(SB), 1, $-4			/* I invalidate */
	BARRIERS
	MCR	CpSC, 0, R0, C(CpCACHE), C(CpCACHEinvi), CpCACHEall
	/* drain L1 write buffer, also drains L2 eviction buffer on sheeva */
	BARRIERS
	RET

TEXT cachedinv(SB), 1, $-4			/* D invalidate */
_dinv:
	BARRIERS
	MCR	CpSC, 0, R0, C(CpCACHE), C(CpCACHEinvd), CpCACHEall
	/* drain L1 write buffer, also drains L2 eviction buffer on sheeva */
	BARRIERS
	RET

/*
 * l2 cache
 *
 * these functions assume that the necessary l1 cache operations have been
 * or will be done explicitly by the caller.
 */

/* enable l2 cache in config coproc. reg.  do this while l1 caches are off. */
TEXT l2cachecfgon(SB), 1, $-4
	BARRIERS
	MCR	CpSC, CpL2, R0, C(CpTESTCFG), C(CpTCl2inv), CpTCl2all
	BARRIERS

	MRC	CpSC, CpL2, R1, C(CpTESTCFG), C(CpTCl2cfg), CpTCl2conf
	ORR	$(CpTCl2ena | CpTCl2prefdis), R1  /* l2 on, prefetch off */
	MCR	CpSC, CpL2, R1, C(CpTESTCFG), C(CpTCl2cfg), CpTCl2conf
	BARRIERS
	RET

/* disable l2 cache in config coproc. reg.  do this while l1 caches are off. */
TEXT l2cachecfgoff(SB), 1, $-4
	BARRIERS
	MRC	CpSC, CpL2, R1, C(CpTESTCFG), C(CpTCl2cfg), CpTCl2conf
	BIC	$CpTCl2ena, R1
	MCR	CpSC, CpL2, R1, C(CpTESTCFG), C(CpTCl2cfg), CpTCl2conf
	BARRIERS

	MCR	CpSC, CpL2, R0, C(CpTESTCFG), C(CpTCl2inv), CpTCl2all
	BARRIERS
	RET

TEXT l2cacheuwb(SB), 1, $-4			/* L2 unified writeback */
	MCR	CpSC, CpL2, R0, C(CpTESTCFG), C(CpTCl2flush), CpTCl2all
	ISB
	RET

TEXT l2cacheuwbse(SB), 1, $-4			/* L2 unified writeback SE */
	MOVW	R0, R2				/* first arg: address */

	MOVW	CPSR, R3			/* splhi */
	ORR	$(PsrDirq), R3, R1
	MOVW	R1, CPSR

	MOVW	4(FP), R1			/* second arg: size */

	ADD	R2, R1
	BIC	$(CACHELINESZ-1), R2
_l2wbse:
	MCR	CpSC, CpL2, R2, C(CpTESTCFG), C(CpTCl2flush), CpTCl2seva
	ADD	$CACHELINESZ, R2
	CMP.S	R2, R1
	BGT	_l2wbse
	ISB

	MOVW	R3, CPSR			/* splx */
	RET

TEXT l2cacheuwbinv(SB), 1, $-4		/* L2 unified writeback+invalidate */
	MOVW	CPSR, R3			/* splhi */
	ORR	$(PsrDirq), R3, R1
	MOVW	R1, CPSR

	MCR	CpSC, CpL2, R0, C(CpTESTCFG), C(CpTCl2flush), CpTCl2all
	ISB
	MCR	CpSC, CpL2, R0, C(CpTESTCFG), C(CpTCl2inv), CpTCl2all
	ISB

	MOVW	R3, CPSR			/* splx */
	RET

TEXT l2cacheuwbinvse(SB), 1, $-4	/* L2 unified writeback+invalidate SE */
	MOVW	R0, R2				/* first arg: address */

	MOVW	CPSR, R3			/* splhi */
	ORR	$(PsrDirq), R3, R1
	MOVW	R1, CPSR

	MOVW	4(FP), R1			/* second arg: size */

	ADD	R2, R1
	BIC	$(CACHELINESZ-1), R2
_l2wbinvse:
	MCR	CpSC, CpL2, R2, C(CpTESTCFG), C(CpTCl2flush), CpTCl2seva
	ISB
	MCR	CpSC, CpL2, R2, C(CpTESTCFG), C(CpTCl2inv), CpTCl2seva
	ADD	$CACHELINESZ, R2
	CMP.S	R2, R1
	BGT	_l2wbinvse
	ISB

	MOVW	R3, CPSR			/* splx */
	RET

TEXT l2cacheuinv(SB), 1, $-4			/* L2 unified invalidate */
	MCR	CpSC, CpL2, R0, C(CpTESTCFG), C(CpTCl2inv), CpTCl2all
	ISB
	RET

TEXT l2cacheuinvse(SB), 1, $-4			/* L2 unified invalidate SE */
	MOVW	R0, R2				/* first arg: address */

	MOVW	CPSR, R3			/* splhi */
	ORR	$(PsrDirq), R3, R1
	MOVW	R1, CPSR

	MOVW	4(FP), R1			/* second arg: size */

	ADD	R2, R1
	BIC	$(CACHELINESZ-1), R2
_l2invse:
	MCR	CpSC, CpL2, R2, C(CpTESTCFG), C(CpTCl2inv), CpTCl2seva
	ADD	$CACHELINESZ, R2
	CMP.S	R2, R1
	BGT	_l2invse
	ISB

	MOVW	R3, CPSR			/* splx */
	RET

/*
 *  enable mmu, i and d caches, and high vector
 */
TEXT mmuenable(SB), 1, $-4
	MRC	CpSC, 0, R0, C(CpCONTROL), C(0)
	ORR	$(CpChv|CpCmmu|CpCdcache|CpCicache|CpCwb|CpCsystem), R0
	BIC	$(CpCrom), R0
	MCR     CpSC, 0, R0, C(CpCONTROL), C(0)
	BARRIERS
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

TEXT mmuinvalidateaddr(SB), 1, $-4		/* invalidate single entry */
	MCR	CpSC, 0, R0, C(CpTLB), C(CpTLBinvu), CpTLBinvse
	BARRIERS
	RET

TEXT cpidget(SB), 1, $-4			/* main ID */
	MRC	CpSC, 0, R0, C(CpID), C(0), CpIDid
	RET

TEXT cpctget(SB), 1, $-4			/* cache type */
	MRC	CpSC, 0, R0, C(CpID), C(0), CpIDct
	RET

TEXT controlget(SB), 1, $-4			/* control */
	MRC	CpSC, 0, R0, C(CpCONTROL), C(0)
	RET

TEXT ttbget(SB), 1, $-4				/* translation table base */
	MRC	CpSC, 0, R0, C(CpTTB), C(0)
	RET

TEXT ttbput(SB), 1, $-4				/* translation table base */
	MCR	CpSC, 0, R0, C(CpTTB), C(0)
	ISB
	RET

TEXT dacget(SB), 1, $-4				/* domain access control */
	MRC	CpSC, 0, R0, C(CpDAC), C(0)
	RET

TEXT dacput(SB), 1, $-4				/* domain access control */
	MCR	CpSC, 0, R0, C(CpDAC), C(0)
	ISB
	RET

TEXT fsrget(SB), 1, $-4				/* fault status */
	MRC	CpSC, 0, R0, C(CpFSR), C(0)
	RET

TEXT farget(SB), 1, $-4				/* fault address */
	MRC	CpSC, 0, R0, C(CpFAR), C(0x0)
	RET

TEXT pidget(SB), 1, $-4				/* address translation pid */
	MRC	CpSC, 0, R0, C(CpPID), C(0x0)
	RET

TEXT pidput(SB), 1, $-4				/* address translation pid */
	MCR	CpSC, 0, R0, C(CpPID), C(0x0)
	ISB
	RET

TEXT splhi(SB), 1, $-4
	MOVW	$(MACHADDR+4), R2		/* save caller pc in Mach */
	MOVW	R14, 0(R2)

	MOVW	CPSR, R0			/* turn off interrupts */
	ORR	$(PsrDirq), R0, R1
	MOVW	R1, CPSR
	RET

TEXT spllo(SB), 1, $-4
	MOVW	CPSR, R0
	BIC	$(PsrDirq), R0, R1
	MOVW	R1, CPSR
	RET

TEXT splx(SB), 1, $-4
	MOVW	$(MACHADDR+0x04), R2		/* save caller pc in Mach */
	MOVW	R14, 0(R2)

	MOVW	R0, R1				/* reset interrupt level */
	MOVW	CPSR, R0
	MOVW	R1, CPSR
	RET

TEXT splxpc(SB), 1, $-4				/* for iunlock */
	MOVW	R0, R1
	MOVW	CPSR, R0
	MOVW	R1, CPSR
	RET

TEXT spldone(SB), 1, $0
	RET

TEXT islo(SB), 1, $-4
	MOVW	CPSR, R0
	AND	$(PsrDirq), R0
	EOR	$(PsrDirq), R0
	RET

TEXT splfhi(SB), $-4
	MOVW	CPSR, R0
	ORR	$(PsrDfiq|PsrDirq), R0, R1
	MOVW	R1, CPSR
	RET

//TEXT splflo(SB), $-4
//	MOVW	CPSR, R0
//	BIC	$(PsrDfiq), R0, R1
//	MOVW	R1, CPSR
//	RET

TEXT	tas(SB), $-4
TEXT	_tas(SB), $-4
	MOVW	R0,R1
	MOVW	$1,R0
	SWPW	R0,(R1)			/* fix: deprecated in armv7 */
	RET

//TEXT tas32(SB), 1, $-4
//	MOVW	R0, R1
//	MOVW	$0xDEADDEAD, R0
//	MOVW	R0, R3
//	SWPW	R0, (R1)
//	CMP.S	R0, R3
//	BEQ	_tasout
//	EOR	R3, R3			/* R3 = 0 */
//	CMP.S	R0, R3
//	BEQ	_tasout
//	MOVW	$1, R15			/* abort: lock != 0 && lock != $0xDEADDEAD */
//_tasout:
//	RET

TEXT clz(SB), 1, $-4
	CLZ(0, 0)			/* 0 is R0 */
	RET

TEXT setlabel(SB), 1, $-4
	MOVW	R13, 0(R0)		/* sp */
	MOVW	R14, 4(R0)		/* pc */
	BARRIERS
	MOVW	$0, R0
	RET

TEXT gotolabel(SB), 1, $-4
	MOVW	0(R0), R13		/* sp */
	MOVW	4(R0), R14		/* pc */
	BARRIERS
	MOVW	$1, R0
	RET

TEXT getcallerpc(SB), 1, $-4
	MOVW	0(R13), R0
	RET

TEXT _idlehands(SB), 1, $-4
	MOVW	CPSR, R3
//	ORR	$PsrDirq, R3, R1		/* splhi */
	BIC	$PsrDirq, R3, R1		/* spllo */
	MOVW	R1, CPSR

	MOVW	$0, R0				/* wait for interrupt */
	MCR	CpSC, 0, R0, C(CpCACHE), C(CpCACHEintr), CpCACHEwait
	ISB

	MOVW	R3, CPSR			/* splx */
	RET

TEXT barriers(SB), 1, $-4
	BARRIERS
	RET
