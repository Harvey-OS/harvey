/*
 * ti omap3530 SoC machine assist
 * arm cortex-a8 processor
 *
 * loader uses R11 as scratch.
 * R9 and R10 are used for `extern register' variables.
 *
 * ARM v7 arch. ref. man. §B1.3.3 that we don't need barriers
 * around moves to CPSR.
 */

#include "arm.s"

/*
 * MCR and MRC are counter-intuitively named.
 *	MCR	coproc, opcode1, Rd, CRn, CRm[, opcode2]	# arm -> coproc
 *	MRC	coproc, opcode1, Rd, CRn, CRm[, opcode2]	# coproc -> arm
 */

/*
 * Entered here from Das U-Boot or another Plan 9 kernel with MMU disabled.
 * Until the MMU is enabled it is OK to call functions provided
 * they are within ±32MiB relative and do not require any
 * local variables or more than one argument (i.e. there is
 * no stack).
 */
TEXT _start(SB), 1, $-4
	MOVW	$setR12(SB), R12		/* load the SB */
	SUB	$KZERO, R12
	ADD	$PHYSDRAM, R12

	/* SVC mode, interrupts disabled */
	MOVW	$(PsrDirq|PsrDfiq|PsrMsvc), R1
	MOVW	R1, CPSR
	BARRIERS

	DELAY(printloopret, 1)
PUTC('\r')
	DELAY(printloopnl, 1)
PUTC('\n')
	/*
	 * work around errata
	 */
	MRC	CpSC, 0, R1, C(CpCONTROL), C(0), CpAuxctl
	ORR	$(CpACissue1|CpACldstissue1), R1  /* fight omap35x errata 3.1.1.9 */
	ORR	$CpACibe, R1			/* enable cp15 invalidate */
	ORR	$CpACl1pe, R1			/* enable l1 parity checking */
	ORR	$CpCalign, R1			/* catch alignment errors */
	BIC	$CpACasa, R1			/* no speculative accesses */
	/* go faster with fewer restrictions */
	BIC	$(CpACcachenopipe|CpACcp15serial|CpACcp15waitidle|CpACcp15pipeflush), R1
	MCR	CpSC, 0, R1, C(CpCONTROL), C(0), CpAuxctl
	ISB

	MRC	CpSC, 1, R1, C(CpCLD), C(CpCLDl2), CpCLDl2aux
	ORR	$CpCl2nowralloc, R1		/* fight cortex errata 460075 */
	ORR	$(CpCl2ecc|CpCl2eccparity), R1
#ifdef TEDIUM
	/*
	 * I don't know why this clobbers the system, but I'm tired
	 * of arguing with this fussy processor.  To hell with it.
	 */
	MCR	CpSC, 1, R1, C(CpCLD), C(CpCLDl2), CpCLDl2aux
	ISB
#endif
	DELAY(printloops, 1)
PUTC('P')
	/*
	 * disable the MMU & caches
	 */
	MRC	CpSC, 0, R1, C(CpCONTROL), C(0), CpMainctl
	BIC	$(CpCdcache|CpCicache|CpCmmu), R1
	ORR	$CpCsbo, R1
	BIC	$CpCsbz, R1
	MCR	CpSC, 0, R1, C(CpCONTROL), C(0), CpMainctl
	ISB

	MRC	CpSC, 0, R1, C(CpCONTROL), C(0), CpAuxctl
	BIC	$CpACl2en, R1			/* turn l2 cache off */
	MCR	CpSC, 0, R1, C(CpCONTROL), C(0), CpAuxctl
	ISB

PUTC('l')
	DELAY(printloop3, 1)

PUTC('a')
	/* clear Mach */
	MOVW	$PADDR(MACHADDR), R4		/* address of Mach */
	MOVW	$0, R0
_machZ:
	MOVW	R0, (R4)
	ADD	$4, R4
	CMP.S	$PADDR(L1+L1X(0)), R4	/* end at top-level page table */
	BNE	_machZ

	/*
	 * set up the MMU page table
	 */

PUTC('n')
	/* clear all PTEs first, to provide a default */
//	MOVW	$PADDR(L1+L1X(0)), R4		/* address of PTE for 0 */
_ptenv0:
	ZEROPTE()
	CMP.S	$PADDR(L1+16*KiB), R4
	BNE	_ptenv0

	DELAY(printloop4, 2)
PUTC(' ')
	/*
	 * set up double map of PHYSDRAM, KZERO to PHYSDRAM for first few MBs,
	 * but only if KZERO and PHYSDRAM differ.
	 */
	MOVW	$PTEDRAM, R2			/* PTE bits */
	MOVW	$PHYSDRAM, R3			/* pa */
	CMP	$KZERO, R3
	BEQ	no2map
	MOVW	$PADDR(L1+L1X(PHYSDRAM)), R4  /* address of PTE for PHYSDRAM */
	MOVW	$DOUBLEMAPMBS, R5
_ptdbl:
	FILLPTE()
	SUB.S	$1, R5
	BNE	_ptdbl
no2map:

	/*
	 * back up and fill in PTEs for memory at KZERO.
	 * beagle has 1 bank of 256MB of SDRAM at PHYSDRAM;
	 * igepv2 has 1 bank of 512MB at PHYSDRAM.
	 * Map the maximum (512MB).
	 */
PUTC('9')
	MOVW	$PTEDRAM, R2			/* PTE bits */
	MOVW	$PHYSDRAM, R3
	MOVW	$PADDR(L1+L1X(KZERO)), R4	/* start with PTE for KZERO */
	MOVW	$512, R5			/* inner loop count (MBs) */
_ptekrw:					/* set PTEs */
	FILLPTE()
	SUB.S	$1, R5				/* decrement inner loop count */
	BNE	_ptekrw

	/*
	 * back up and fill in PTEs for MMIO
	 * stop somewhere after uarts
	 */
PUTC(' ')
	MOVW	$PTEIO, R2			/* PTE bits */
	MOVW	$PHYSIO, R3
	MOVW	$PADDR(L1+L1X(VIRTIO)), R4	/* start with PTE for VIRTIO */
_ptenv2:
	FILLPTE()
	CMP.S	$PADDR(L1+L1X(PHYSIOEND)), R4
	BNE	_ptenv2

	/* mmu.c sets up the trap vectors later */

	/*
	 * set up a temporary stack; avoid data & bss segments
	 */
	MOVW	$(PHYSDRAM | (128*1024*1024)), R13

	/* invalidate caches */
	BL	cachedinv(SB)
	MOVW	$KZERO, R0
	MCR	CpSC, 0, R0, C(CpCACHE), C(CpCACHEinvi), CpCACHEall
	ISB
	MCR	CpSC, 0, R0, C(CpCACHE), C(CpCACHEwb), CpCACHEwait
	BARRIERS

PUTC('f')
	/*
	 * turn caches on
	 */
	MRC	CpSC, 0, R1, C(CpCONTROL), C(0), CpAuxctl
	ORR	$CpACl2en, R1			/* turn l2 cache on */
	MCR	CpSC, 0, R1, C(CpCONTROL), C(0), CpAuxctl
	BARRIERS

	MRC	CpSC, 0, R1, C(CpCONTROL), C(0), CpMainctl
	ORR	$(CpCdcache|CpCicache), R1
	MCR	CpSC, 0, R1, C(CpCONTROL), C(0), CpMainctl
	BARRIERS

PUTC('r')
	/* set the domain access control */
	MOVW	$Client, R0
	BL	dacput(SB)

	DELAY(printloop5, 2)
PUTC('o')
	/* set the translation table base */
	MOVW	$PADDR(L1), R0
	BL	ttbput(SB)

	MOVW	$0, R0
	BL	pidput(SB)		/* paranoia */

PUTC('m')
	/*
	 * the little dance to turn the MMU on
	 */
	BL	cacheuwbinv(SB)
	BL	mmuinvalidate(SB)
	BL	mmuenable(SB)

PUTC(' ')
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

PUTC('B')
	MOVW	$PHYSDRAM, R3			/* pa */
	CMP	$KZERO, R3
	BEQ	no2unmap
	/* undo double map of PHYSDRAM, KZERO & first few MBs */
	MOVW	$(L1+L1X(PHYSDRAM)), R4		/* addr. of PTE for PHYSDRAM */
	MOVW	$0, R0
	MOVW	$DOUBLEMAPMBS, R5
_ptudbl:
	ZEROPTE()
	SUB.S	$1, R5
	BNE	_ptudbl
no2unmap:
	BARRIERS
	MOVW	$KZERO, R0
	MCR	CpSC, 0, R0, C(CpTLB), C(CpTLBinvu), CpTLBinv
	BARRIERS

#ifdef HIGH_SECURITY				/* i.e., not GP omap */
	/* hack: set `secure monitor' vector base addr for cortex */
//	MOVW	$HVECTORS, R0
	MOVW	$PADDR(L1), R0
	SUB	$(MACHSIZE+(2*1024)), R0
	MCR	CpSC, 0, R0, C(CpVECS), C(CpVECSbase), CpVECSmon
	ISB
#endif

	/*
	 * call main in C
	 * pass Mach to main and set up the stack in it
	 */
	MOVW	$(MACHADDR), R0			/* Mach */
	MOVW	R0, R13
	ADD	$(MACHSIZE), R13		/* stack pointer */
	SUB	$4, R13				/* space for link register */
	MOVW	R0, R10				/* m = MACHADDR */
PUTC('e')
	BL	main(SB)			/* void main(Mach*) */
	/*FALLTHROUGH*/

/*
 * reset the system
 */

TEXT _reset(SB), 1, $-4
	MOVW	$(PsrDirq|PsrDfiq|PsrMsvc), R0
	MOVW	R0, CPSR
	BARRIERS

	DELAY(printloopr, 2)
PUTC('!')
PUTC('r')
PUTC('e')
PUTC('s')
PUTC('e')
PUTC('t')
PUTC('!')
PUTC('\r')
PUTC('\n')

	/* turn the caches off */
	BL	cacheuwbinv(SB)

	MRC	CpSC, 0, R0, C(CpCONTROL), C(0), CpMainctl
	BIC	$(CpCicache|CpCdcache|CpCalign), R0
	ORR	$CpCsw, R0			/* enable SWP */
	MCR	CpSC, 0, R0, C(CpCONTROL), C(0), CpMainctl
	BARRIERS

	/* redo double map of PHYSDRAM, KZERO & first few MBs */
	MOVW	$PTEDRAM, R2			/* PTE bits */
	MOVW	$PHYSDRAM, R3			/* pa */
	MOVW	$(L1+L1X(PHYSDRAM)), R4		/* address of PHYSDRAM's PTE */
	MOVW	$DOUBLEMAPMBS, R5
_ptrdbl:
	FILLPTE()
	SUB.S	$1, R5
	BNE	_ptrdbl

	MOVW	$PHYSDRAM, R0
	MCR	CpSC, 0, R0, C(CpTLB), C(CpTLBinvu), CpTLBinv
	BARRIERS

	/* turn the MMU off */
	MOVW	$PHYSDRAM, R0
	BL	_r15warp(SB)
	BL	mmuinvalidate(SB)
	BL	mmudisable(SB)

	/* set new reset vector */
	MOVW	$HVECTORS, R2
	MOVW	$0xe59ff018, R3			/* MOVW 0x18(R15), R15 */
	MOVW	R3, (R2)
	BARRIERS

//	MOVW	$PHYSFLASH, R3			/* TODO */
//	MOVW	R3, 0x20(R2)			/* where $0xe59ff018 jumps to */

	/* ...and jump to it */
//	MOVW	R2, R15				/* software reboot */
_limbo:						/* should not get here... */
	BL	idlehands(SB)
	B	_limbo				/* ... and can't get out */
	BL	_div(SB)			/* hack to load _div, etc. */

TEXT _r15warp(SB), 1, $-4
	BIC	$KSEGM, R14			/* link reg, will become PC */
	ORR	R0, R14
	BIC	$KSEGM, R13			/* SP too */
	ORR	R0, R13
	RET

/*
 * `single-element' cache operations.
 * in arm arch v7, they operate on all cache levels, so separate
 * l2 functions are unnecessary.
 */

TEXT cachedwbse(SB), $-4			/* D writeback SE */
	MOVW	R0, R2

	MOVW	CPSR, R3
	CPSID					/* splhi */

	BARRIERS			/* force outstanding stores to cache */
	MOVW	R2, R0
	MOVW	4(FP), R1
	ADD	R0, R1				/* R1 is end address */
	BIC	$(CACHELINESZ-1), R0		/* cache line start */
_dwbse:
	MCR	CpSC, 0, R0, C(CpCACHE), C(CpCACHEwb), CpCACHEse
	/* can't have a BARRIER here since it zeroes R0 */
	ADD	$CACHELINESZ, R0
	CMP.S	R0, R1
	BGT	_dwbse
	B	_wait

TEXT cachedwbinvse(SB), $-4			/* D writeback+invalidate SE */
	MOVW	R0, R2

	MOVW	CPSR, R3
	CPSID					/* splhi */

	BARRIERS			/* force outstanding stores to cache */
	MOVW	R2, R0
	MOVW	4(FP), R1
	ADD	R0, R1				/* R1 is end address */
	BIC	$(CACHELINESZ-1), R0		/* cache line start */
_dwbinvse:
	MCR	CpSC, 0, R0, C(CpCACHE), C(CpCACHEwbi), CpCACHEse
	/* can't have a BARRIER here since it zeroes R0 */
	ADD	$CACHELINESZ, R0
	CMP.S	R0, R1
	BGT	_dwbinvse
_wait:						/* drain write buffer */
	BARRIERS
	/* drain L1 write buffer, also drains L2 eviction buffer on sheeva */
	MCR	CpSC, 0, R0, C(CpCACHE), C(CpCACHEwb), CpCACHEwait
	ISB

	MOVW	R3, CPSR			/* splx */
	RET

TEXT cachedinvse(SB), $-4			/* D invalidate SE */
	MOVW	R0, R2

	MOVW	CPSR, R3
	CPSID					/* splhi */

	BARRIERS			/* force outstanding stores to cache */
	MOVW	R2, R0
	MOVW	4(FP), R1
	ADD	R0, R1				/* R1 is end address */
	BIC	$(CACHELINESZ-1), R0		/* cache line start */
_dinvse:
	MCR	CpSC, 0, R0, C(CpCACHE), C(CpCACHEinvd), CpCACHEse
	/* can't have a BARRIER here since it zeroes R0 */
	ADD	$CACHELINESZ, R0
	CMP.S	R0, R1
	BGT	_dinvse
	B	_wait

/*
 *  enable mmu and high vectors
 */
TEXT mmuenable(SB), 1, $-4
	MRC	CpSC, 0, R0, C(CpCONTROL), C(0), CpMainctl
	ORR	$(CpChv|CpCmmu), R0
	MCR	CpSC, 0, R0, C(CpCONTROL), C(0), CpMainctl
	BARRIERS
	RET

TEXT mmudisable(SB), 1, $-4
	MRC	CpSC, 0, R0, C(CpCONTROL), C(0), CpMainctl
	BIC	$(CpChv|CpCmmu), R0
	MCR	CpSC, 0, R0, C(CpCONTROL), C(0), CpMainctl
	BARRIERS
	RET

/*
 * If one of these MCR instructions crashes or hangs the machine,
 * check your Level 1 page table (at TTB) closely.
 */
TEXT mmuinvalidate(SB), $-4			/* invalidate all */
	MOVW	CPSR, R2
	CPSID					/* interrupts off */

	BARRIERS
	MOVW	PC, R0				/* some valid virtual address */
	MCR	CpSC, 0, R0, C(CpTLB), C(CpTLBinvu), CpTLBinv
	BARRIERS
	MOVW	R2, CPSR			/* interrupts restored */
	RET

TEXT mmuinvalidateaddr(SB), $-4			/* invalidate single entry */
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
	MRC	CpSC, 0, R0, C(CpCONTROL), C(0), CpMainctl
	RET

TEXT ttbget(SB), 1, $-4				/* translation table base */
	MRC	CpSC, 0, R0, C(CpTTB), C(0), CpTTB0
	RET

TEXT ttbput(SB), 1, $-4				/* translation table base */
	MCR	CpSC, 0, R0, C(CpTTB), C(0), CpTTB0
	MCR	CpSC, 0, R0, C(CpTTB), C(0), CpTTB1	/* cortex has two */
	ISB
	RET

TEXT dacget(SB), 1, $-4				/* domain access control */
	MRC	CpSC, 0, R0, C(CpDAC), C(0)
	RET

TEXT dacput(SB), 1, $-4				/* domain access control */
	MCR	CpSC, 0, R0, C(CpDAC), C(0)
	ISB
	RET

TEXT fsrget(SB), 1, $-4				/* data fault status */
	MRC	CpSC, 0, R0, C(CpFSR), C(0), CpDFSR
	RET

TEXT ifsrget(SB), 1, $-4			/* instruction fault status */
	MRC	CpSC, 0, R0, C(CpFSR), C(0), CpIFSR
	RET

TEXT farget(SB), 1, $-4				/* fault address */
	MRC	CpSC, 0, R0, C(CpFAR), C(0x0)
	RET

TEXT getpsr(SB), 1, $-4
	MOVW	CPSR, R0
	RET

TEXT getscr(SB), 1, $-4
	MRC	CpSC, 0, R0, C(CpCONTROL), C(CpCONTROLscr), CpSCRscr
	RET

TEXT pidget(SB), 1, $-4				/* address translation pid */
	MRC	CpSC, 0, R0, C(CpPID), C(0x0)
	RET

TEXT pidput(SB), 1, $-4				/* address translation pid */
	MCR	CpSC, 0, R0, C(CpPID), C(0x0)
	ISB
	RET

TEXT splhi(SB), 1, $-4
	MOVW	CPSR, R0
	CPSID					/* turn off interrupts */

	MOVW	$(MACHADDR+4), R2		/* save caller pc in Mach */
	MOVW	R14, 0(R2)
	RET

TEXT spllo(SB), 1, $-4			/* start marker for devkprof.c */
	MOVW	CPSR, R0
	CPSIE
	RET

TEXT splx(SB), 1, $-4
	MOVW	$(MACHADDR+0x04), R2		/* save caller pc in Mach */
	MOVW	R14, 0(R2)

	MOVW	CPSR, R3
	MOVW	R0, CPSR			/* reset interrupt level */
	MOVW	R3, R0				/* must return old CPSR */
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
	SWPW	R0,(R1)			/* fix: deprecated in armv7 */
	RET

TEXT clz(SB), $-4
	CLZ(0, 0)			/* 0 is R0 */
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
	BARRIERS
	WFI
	RET

TEXT coherence(SB), $-4
	BARRIERS
	RET

#include "cache.v7.s"
