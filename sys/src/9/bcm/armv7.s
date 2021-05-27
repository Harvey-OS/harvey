/*
 * Broadcom bcm2836 SoC, as used in Raspberry Pi 2
 * 4 x Cortex-A7 processor (armv7)
 */

#include "arm.s"

#define CACHELINESZ 	64
#define ICACHELINESZ	32

#undef DSB
#undef DMB
#undef ISB
#define DSB	WORD	$0xf57ff04f	/* data synch. barrier; last f = SY */
#define DMB	WORD	$0xf57ff05f	/* data mem. barrier; last f = SY */
#define ISB	WORD	$0xf57ff06f	/* instr. sync. barrier; last f = SY */
#define WFI	WORD	$0xe320f003	/* wait for interrupt */
#define WFI_EQ	WORD	$0x0320f003	/* wait for interrupt if eq */
#define ERET	WORD	$0xe160006e	/* exception return from HYP */
#define SEV	WORD	$0xe320f004	/* send event */

/* tas/cas strex debugging limits; started at 10000 */
#define MAXSC 1000000

TEXT armstart(SB), 1, $-4

	/*
	 * if not cpu0, go to secondary startup
	 */
	CPUID(R1)
	BNE	reset

	/*
	 * go to SVC mode, interrupts disabled
	 */
	BL	svcmode(SB)

	/*
	 * disable the mmu and caches
	 */
	MRC	CpSC, 0, R1, C(CpCONTROL), C(0), CpMainctl
	BIC	$(CpCdcache|CpCicache|CpCmmu), R1
	ORR	$(CpCsbo|CpCsw), R1
	BIC	$CpCsbz, R1
	MCR	CpSC, 0, R1, C(CpCONTROL), C(0), CpMainctl
	BARRIERS

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
	 * turn SMP on
	 * invalidate tlb
	 */
	MRC	CpSC, 0, R1, C(CpCONTROL), C(0), CpAuxctl
	ORR	$CpACsmp, R1		/* turn SMP on */
	MCR	CpSC, 0, R1, C(CpCONTROL), C(0), CpAuxctl
	BARRIERS
	MCR	CpSC, 0, R0, C(CpTLB), C(CpTLBinvu), CpTLBinv
	BARRIERS

	/*
	 * start stack at top of mach (physical addr)
	 * set up page tables for kernel
	 */
	MOVW	$PADDR(MACHADDR+MACHSIZE-4), R13
	MOVW	$PADDR(L1), R0
	BL	mmuinit(SB)

	/*
	 * set up domain access control and page table base
	 */
	MOVW	$Client, R1
	MCR	CpSC, 0, R1, C(CpDAC), C(0)
	MOVW	$PADDR(L1), R1
	ORR	$(CpTTBs|CpTTBowba|CpTTBiwba), R1
	MCR	CpSC, 0, R1, C(CpTTB), C(0)
	MCR	CpSC, 0, R1, C(CpTTB), C(0), CpTTB1	/* cortex has two */

	/*
	 * invalidate my caches before enabling
	 */
	BL	cachedinv(SB)
	BL	cacheiinv(SB)
	BL	l2cacheuinv(SB)
	BARRIERS

	/*
	 * enable caches, mmu, and high vectors
	 */

	MRC	CpSC, 0, R0, C(CpCONTROL), C(0), CpMainctl
	ORR	$(CpChv|CpCdcache|CpCicache|CpCmmu), R0
	MCR	CpSC, 0, R0, C(CpCONTROL), C(0), CpMainctl
	BARRIERS

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
	MOVW	$(1<<31), R1
	MCR	CpSC, 0, R1, C(CpCLD), C(CpCLDena), CpCLDenacyc
	MOVW	$1, R1
	MCR	CpSC, 0, R1, C(CpCLD), C(CpCLDena), CpCLDenapmnc

	/*
	 * call main and loop forever if it returns
	 */
	BL	,main(SB)
	B	,0(PC)

	BL	_div(SB)		/* hack to load _div, etc. */

/*
 * startup entry for cpu(s) other than 0
 */
TEXT cpureset(SB), 1, $-4
reset:
	/*
	 * load physical base for SB addressing while mmu is off
	 * keep a handy zero in R0 until first function call
	 */
	MOVW	$setR12(SB), R12
	SUB	$KZERO, R12
	ADD	$PHYSDRAM, R12
	MOVW	$0, R0

	/*
	 * SVC mode, interrupts disabled
	 */
	BL	svcmode(SB)

	/*
	 * disable the mmu and caches
	 */
	MRC	CpSC, 0, R1, C(CpCONTROL), C(0), CpMainctl
	BIC	$(CpCdcache|CpCicache|CpCmmu), R1
	ORR	$(CpCsbo|CpCsw), R1
	BIC	$CpCsbz, R1
	MCR	CpSC, 0, R1, C(CpCONTROL), C(0), CpMainctl
	BARRIERS

	/*
	 * turn SMP on
	 * invalidate tlb
	 */
	MRC	CpSC, 0, R1, C(CpCONTROL), C(0), CpAuxctl
	ORR	$CpACsmp, R1		/* turn SMP on */
	MCR	CpSC, 0, R1, C(CpCONTROL), C(0), CpAuxctl
	BARRIERS
	MCR	CpSC, 0, R0, C(CpTLB), C(CpTLBinvu), CpTLBinv
	BARRIERS

	/*
	 * find Mach for this cpu
	 */
	MRC	CpSC, 0, R2, C(CpID), C(CpIDidct), CpIDmpid
	AND	$(MAXMACH-1), R2	/* mask out non-cpu-id bits */
	SLL	$2, R2			/* convert to word index */
	MOVW	$machaddr(SB), R0
	BIC		$KSEGM, R0
	ADD	R2, R0			/* R0 = &machaddr[cpuid] */
	MOVW	(R0), R0		/* R0 = machaddr[cpuid] */
	CMP	$0, R0
	BEQ	0(PC)			/* must not be zero */
	SUB	$KZERO, R0, R(MACH)	/* m = PADDR(machaddr[cpuid]) */

	/*
	 * start stack at top of local Mach
	 */
	ADD	$(MACHSIZE-4), R(MACH), R13

	/*
	 * set up domain access control and page table base
	 */
	MOVW	$Client, R1
	MCR	CpSC, 0, R1, C(CpDAC), C(0)
	MOVW	12(R(MACH)), R1	/* m->mmul1 */
	SUB	$KZERO, R1		/* phys addr */
	ORR	$(CpTTBs|CpTTBowba|CpTTBiwba), R1
	MCR	CpSC, 0, R1, C(CpTTB), C(0)
	MCR	CpSC, 0, R1, C(CpTTB), C(0), CpTTB1	/* cortex has two */

	/*
	 * invalidate my caches before enabling
	 */
	BL	cachedinv(SB)
	BL	cacheiinv(SB)
	BARRIERS

	/*
	 * enable caches, mmu, and high vectors
	 */
	MRC	CpSC, 0, R0, C(CpCONTROL), C(0), CpMainctl
	ORR	$(CpChv|CpCdcache|CpCicache|CpCmmu), R0
	MCR	CpSC, 0, R0, C(CpCONTROL), C(0), CpMainctl
	BARRIERS

	/*
	 * switch MACH, SB, SP, and PC into KZERO space
	 */
	ADD	$KZERO, R(MACH)
	MOVW	$setR12(SB), R12
	ADD	$KZERO, R13
	MOVW	$_startpg2(SB), R15

TEXT _startpg2(SB), 1, $-4

	/*
	 * enable cycle counter
	 */
	MOVW	$(1<<31), R1
	MCR	CpSC, 0, R1, C(CpCLD), C(CpCLDena), CpCLDenacyc
	MOVW	$1, R1
	MCR	CpSC, 0, R1, C(CpCLD), C(CpCLDena), CpCLDenapmnc

	/*
	 * call cpustart and loop forever if it returns
	 */
	MRC	CpSC, 0, R0, C(CpID), C(CpIDidct), CpIDmpid
	AND	$(MAXMACH-1), R0		/* mask out non-cpu-id bits */
	BL	,cpustart(SB)
	B	,0(PC)

/*
 * get into SVC mode with interrupts disabled
 * raspberry pi firmware since 29 Sept 2015 starts in HYP mode
 */
TEXT svcmode(SB), 1, $-4
	MOVW	CPSR, R1
	AND	$PsrMask, R1
	MOVW	$PsrMhyp, R2
	CMP	R2, R1
	MOVW	$(PsrDirq|PsrDfiq|PsrMsvc), R1
	BNE	nothyp
	MSR(1, 1, 1, 0xe)	/* MOVW	R1, SPSR_HYP */
	MSR(0, 14, 1, 0xe)	/* MOVW	R14, ELR_HYP */
	ERET
nothyp:
	MOVW	R1, CPSR
	RET

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

TEXT cpctget(SB), 1, $-4			/* cache type */
	MRC	CpSC, 0, R0, C(CpID), C(CpIDidct), CpIDct
	RET

TEXT lcycles(SB), 1, $-4
	MRC	CpSC, 0, R0, C(CpCLD), C(CpCLDcyc), 0
	RET

TEXT splhi(SB), 1, $-4
	MOVW	R14, 4(R(MACH))		/* save caller pc in m->splpc */

	MOVW	CPSR, R0			/* turn off irqs (but not fiqs) */
	ORR	$(PsrDirq), R0, R1
	MOVW	R1, CPSR
	RET

TEXT splfhi(SB), 1, $-4
	MOVW	R14, 4(R(MACH))		/* save caller pc in m->splpc */

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
	MOVW	$0, R1
	CMP.S	R1, R(MACH)
	MOVW.NE	R1, 4(R(MACH))			/* clear m->splpc */
	BIC	$(PsrDirq|PsrDfiq), R0, R1
	MOVW	R1, CPSR
	RET

TEXT splx(SB), 1, $-4
	MOVW	R14, 4(R(MACH))		/* save caller pc in m->splpc */

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
TEXT	_tas(SB), $-4			/* _tas(ulong *) */
	/* returns old (R0) after modifying (R0) */
	MOVW	R0,R5
	DMB

	MOVW	$1,R2		/* new value of (R0) */
	MOVW	$MAXSC, R8
tas1:
	LDREX(5,7)		/* LDREX 0(R5),R7 */
	CMP.S	$0, R7		/* old value non-zero (lock taken)? */
	BNE	lockbusy	/* we lose */
	SUB.S	$1, R8
	BEQ	lockloop2
	STREX(2,5,4)		/* STREX R2,(R5),R4 */
	CMP.S	$0, R4
	BNE	tas1		/* strex failed? try again */
	DMB
	B	tas0
lockloop2:
	BL	abort(SB)
lockbusy:
	CLREX
tas0:
	MOVW	R7, R0		/* return old value */
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
	WFI_EQ
	DSB

	MOVW	R3, CPSR			/* splx */
	RET


TEXT coherence(SB), $-4
	BARRIERS
	RET

TEXT sev(SB), $-4
	SEV
	RET

/*
 * invalidate tlb
 */
TEXT mmuinvalidate(SB), 1, $-4
	DSB
	MOVW	$0, R0
	MCR	CpSC, 0, R0, C(CpTLB), C(CpTLBinvu), CpTLBinv
	BARRIERS
	RET

/*
 * mmuinvalidateaddr(va)
 *   invalidate tlb entry for virtual page address va, ASID 0
 */
TEXT mmuinvalidateaddr(SB), 1, $-4
	DSB
	MCR	CpSC, 0, R0, C(CpTLB), C(CpTLBinvu), CpTLBinvse
	BARRIERS
	RET

/*
 * `single-element' cache operations.
 * in arm arch v7, if effective to PoC, they operate on all cache levels, so separate
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

/*
 * TLB on armv7 loads from cache, so no need for writeback
 */
TEXT cachedwbtlb(SB), $-4
	DSB
	ISB
	RET

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

#include "cache.v7.s"
