/*
 * tegra 2 SoC machine assist
 * dual arm cortex-a9 processors
 *
 * LDREX/STREX use an exclusive monitor, which is part of the data cache unit
 * for the L1 cache, so they won't work right if the L1 cache is disabled.
 */

#include "arm.s"

/* tas/cas strex debugging limits; started at 10000 */
#define MAXSC 1000000

/*
 * Entered here from Das U-Boot or another Plan 9 kernel with MMU disabled
 * and running on cpu0, in the zero segment.  We quickly set up a stack.
 */
TEXT _start(SB), 1, $-4
	/* get all cpus into known states */
	MOVW	$(PsrMsvc|PsrDirq|PsrDfiq), CPSR  /* svc mode, interrupts off */
	BARRIERS
	CLREX
	SETZSB

	/* invalidate i-cache and branch-target cache */
	MTCP	CpSC, 0, PC, C(CpCACHE), C(CpCACHEinviis), CpCACHEall
	BARRIERS

	/*
	 * cpu0 step 1 from a9 mpcore manual: disable my MMU & D caches.
	 *	invalidate caches below after learning cache characteristics.
	 */
	MFCP	CpSC, 0, R1, C(CpCONTROL), C(0), CpMainctl
	ORR	$CpCsbo, R1
	BIC	$(CpCsbz|CpCmmu|CpCdcache|CpCpredict), R1
	MTCP	CpSC, 0, R1, C(CpCONTROL), C(0), CpMainctl
	BARRIERS

	/*
	 * establish a temporary stack in high memory so we can call
	 * assembler routines and some simple C functions.
	 * leave room for one arg + LR.
	 */
	MOVW	$((PHYSDRAM | TMPSTACK) - ARGLINK), SP
	BL	resetcpu1(SB)		/* sanity check: are we cpu0? */
PUTC('\r'); PUTC('\n')
 	MOVW	$PADDR(MACHADDR), R(MACH)
	MOVW	$0, R(USER)		/* up = nil */

	BL	errata(SB)		/* work around cortex-a9 bugs */
	BL	datasegok(SB)		/* another sanity check */
	BL	zerolow(SB)		/* zero bss, cpu0 Mach & L1CPU0 */

	/* put cpu0's kernel stack in newly-zeroed Mach */
	ADD	$(MACHSIZE-4-ARGLINK), R(MACH), SP /* leave room for arg + LR */

	CPUID(R0)
	BL	main(SB)		/* no return */

wfispin:
	BARRIERS
	WFI
	B	wfispin

	BL	_div(SB)		/* hack to load _div, etc. */


TEXT resetcpu1(SB), $-4
	CPUID(R1)
	CMP	$0, R1
	BEQ	cpu0
	/* we're cpu n for n != 0; how did we get here? */
	PUTC('?'); PUTC('c'); PUTC('n')
	/* fall through into reset code */
cpu0:
	/*
	 * get cpu1 into reset to stop it doing any further damage.
	 */
	MOVW	$(1<<1), R0			/* cpu1 bit to reset */
	MOVW	$(PHYSCLKRST+0x340), R1		/* &clkrst->cpuset */
	MOVW	(R1), R2
	AND	R0, R2, R3
	CMP	$0, R3
	BNE	inreset				/* cpu1 already in reset */

	MOVW	R0, (R1)			/* put cpu1 into reset */
	BARRIERS

	/* it will take about 1 µs. until cpu1 is actually in reset. */
	DELAY(nrst0, 1)
	PUTC('c'); PUTC('p'); PUTC('u'); PUTC('1')
	PUTC(' '); PUTC('w'); PUTC('a'); PUTC('s')
	DELAY(nrst1, 1)
	PUTC(' '); PUTC('n'); PUTC('o'); PUTC('t')
	PUTC(' '); PUTC('i'); PUTC('n'); PUTC(' ')
	DELAY(nrst2, 1)
	PUTC('r'); PUTC('e'); PUTC('s'); PUTC('e')
	PUTC('t'); PUTC('!'); PUTC('\r'); PUTC('\n')
	DELAY(nrst3, 1)
inreset:
	RET

/* zero a word at a time.  R1 has start, R2 has end */
TEXT zero(SB), $-4
	MOVW	$0, R0
_zero:
	MOVW	R0, (R1)
	ADD	$BY2WD, R1
	CMP.S	R1, R2
	BNE	_zero
	DSB
	RET

TEXT zerolow(SB), $-4
	MOVM.DB.W [R14], (SP)	/* push LR */

	MOVW	$PADDR(LOWZSTART), R1	/* zero cpu0 Mach & L1CPU0 */
	MOVW	$PADDR(LOWZEND), R2
	BL	zero(SB)

	MOVW	$edata-KZERO(SB), R1	/* zero bss */
	MOVW	$end-KZERO(SB), R2
	BL	zero(SB)

	MOVM.IA.W (SP), [R14]	/* pop LR */
	RET

TEXT hivecs(SB), $-4
	MFCP	CpSC, 0, R1, C(CpCONTROL), C(0), CpMainctl
	ORR	$CpChv, R1
	MTCP	CpSC, 0, R1, C(CpCONTROL), C(0), CpMainctl
_barrret:
	BARRIERS
_ret:
	RET

	GLOBL	vfy(SB), $4
	DATA	vfy+0(SB)/4, $0xcafebabe

/*
 * verify data segment alignment
 */
TEXT datasegok(SB), $-4
	MOVW	$0xcafebabe, R0
	MOVW	vfy(SB), R1
	CMP.S	R0, R1
	BEQ	_ret
	PUTC('?'); PUTC('d'); PUTC('s');
	B	wfispin

/* turn my L1 caches on.  */
TEXT cachedon(SB), $-4
	FLBTC			/* before enabling branch prediction */
	BARRIERS
	MFCP	CpSC, 0, R0, C(CpCONTROL), C(0), CpMainctl
	/* CpCrr is for erratum 716044 */
	ORR	$(CpCdcache|CpCicache|CpCalign|CpCpredict|CpCrr), R0
	MTCP	CpSC, 0, R0, C(CpCONTROL), C(0), CpMainctl
	B	_barrret

/*
 * converts virtual address in R0 to a physical address.
 */
TEXT k2paddr(SB), 1, $-4
	BIC	$KSEGM, R0
	ADD	$PHYSDRAM, R0
	RET

/*
 * converts physical address in R0 to a virtual address.
 */
TEXT p2kaddr(SB), 1, $-4
	BIC	$KSEGM, R0
	ORR	$KZERO, R0
	RET

/* switch PC, SP & SB, R(MACH) to new segment in R0 */
TEXT warpregs(SB), 1, $-4
	MOVW	$KSEGM, R1
	BIC	R1, SP
	ORR	R0, SP
	BIC	R1, R12
	ORR	R0, R12
	BIC	R1, R14			/* link reg, will become PC */
	ORR	R0, R14
	BIC	R1, R(MACH)
	ORR	R0, R(MACH)
	RET


/* modifies R0, R3—R6 */
TEXT printhex(SB), 1, $-4
	MOVW	R0, R3
	PUTC('0')
	PUTC('x')
	MOVW	$(BI2BY*BY2WD-4), R5	/* bits to shift right */
nextdig:
	SRA	R5, R3, R4
	AND	$0xf, R4
	ADD	$'0', R4
	CMP.S	$'9', R4
	BLE	nothex		/* if R4 <= 9, jump */
	ADD	$('a'-('9'+1)), R4
nothex:
	PUTC(R4)
	SUB.S	$4, R5
	BGE	nextdig

	PUTC('\r')
	PUTC('\n')
	DELAY(proct, 50)
	RET

/*
 * `single-element' cache operations.
 * in arm arch v7, they operate on all architected cache levels, so separate
 * l2 functions are usually unnecessary.
 */

TEXT cachedwbse(SB), $-4			/* D writeback SE */
	MOVW	R0, R2				/* start addr */

	MOVW	CPSR, R3
	CPSID					/* splhi */

	BARRIERS			/* force outstanding stores to cache */
	MOVW	R2, R0
	MOVW	4(FP), R1			/* size */
	ADD	R0, R1				/* R1 is end address */
	BIC	$(CACHELINESZ-1), R0		/* cache line start */
_dwbse:
	MTCP	CpSC, 0, R0, C(CpCACHE), C(CpCACHEwb), CpCACHEse
	/* erratum 764369: cache maint by mva may fail on inner shareable mem */
	DSB
	MTCP	CpSC, 0, R0, C(CpCACHE), C(CpCACHEwb), CpCACHEse
	ADD	$CACHELINESZ, R0
	CMP.S	R0, R1
	BGT	_dwbse
	B	_splxr3

TEXT cachedwbinvse(SB), $-4			/* D writeback+invalidate SE */
	MOVW	R0, R2				/* start addr */

	MOVW	CPSR, R3
	/* splhi, includes barriers to force outstanding stores to cache */
	CPSID				
	MOVW	R2, R0
	MOVW	4(FP), R1			/* size */
	ADD	R0, R1				/* R1 is end address */
	BIC	$(CACHELINESZ-1), R0		/* cache line start */
_dwbinvse:
	MTCP	CpSC, 0, R0, C(CpCACHE), C(CpCACHEwbi), CpCACHEse
	/* erratum 764369: cache maint by mva may fail on inner shareable mem */
	DSB
	MTCP	CpSC, 0, R0, C(CpCACHE), C(CpCACHEwbi), CpCACHEse
	ADD	$CACHELINESZ, R0
	CMP.S	R0, R1
	BGT	_dwbinvse
_splxr3:
	MOVW	R3, CPSR
	B	_barrret

/*
 * start addr should be the start of a cache line and bytes should be a multiple
 * of cache line size.  Otherwise, the behaviour is unpredictable.
 */
TEXT cachedinvse(SB), $-4			/* D invalidate SE */
	MOVW	R0, R2				/* start addr */

	MOVW	CPSR, R3
	/* splhi, includes barriers to force outstanding stores to cache */
	CPSID				
	MOVW	R2, R0
	MOVW	4(FP), R1			/* size */
	ADD	R0, R1				/* R1 is end address */
	BIC	$(CACHELINESZ-1), R0		/* cache line start */
_dinvse:
	MTCP	CpSC, 0, R0, C(CpCACHE), C(CpCACHEinvd), CpCACHEse
	/* erratum 764369: cache maint by mva may fail on inner shareable mem */
	DSB
	MTCP	CpSC, 0, R0, C(CpCACHE), C(CpCACHEinvd), CpCACHEse
	ADD	$CACHELINESZ, R0
	CMP.S	R0, R1
	BGT	_dinvse
	B	_splxr3

/*
 *  enable mmu
 */
TEXT mmuenable(SB), 1, $-4
	MOVW	CPSR, R3
	CPSID					/* interrupts off */
	/* invalidate TLBs immediately before */
	MTCP	CpSC, 0, PC, C(CpTLB), C(CpTLBinvu), CpTLBinv
	BARRIERS
	MFCP	CpSC, 0, R0, C(CpCONTROL), C(0), CpMainctl
	ORR	$CpCmmu, R0
	MTCP	CpSC, 0, R0, C(CpCONTROL), C(0), CpMainctl
	BARRIERS
	B	_splxr3

TEXT mmudisable(SB), 1, $-4
	BARRIERS
	MFCP	CpSC, 0, R0, C(CpCONTROL), C(0), CpMainctl
	BIC	$CpCmmu, R0
	MTCP	CpSC, 0, R0, C(CpCONTROL), C(0), CpMainctl
	B	_barrret

/*
 * If one of these MCR (MTCP) instructions crashes or hangs the machine,
 * check your Level 1 page table (at TTBR0) closely.
 */
TEXT mmuinvalidate(SB), $-4			/* invalidate all tlbs */
	MOVW	CPSR, R3
	CPSID					/* interrupts off */
	MTCP	CpSC, 0, PC, C(CpTLB), C(CpTLBinvu), CpTLBinv
	B	_splxr3

TEXT mmuinvalidateaddr(SB), $-4			/* invalidate single tlb */
	BARRIERS
	BIC	$(BY2PG-1), R0, R1
	MTCP	CpSC, 0, R1, C(CpTLB), C(CpTLBinvu), CpTLBinvse
	B	_barrret

TEXT cpidget(SB), 1, $-4			/* main ID */
	MFCP	CpSC, 0, R0, C(CpID), C(CpIDidct), CpIDid
	RET

TEXT cpctget(SB), 1, $-4			/* cache type */
	MFCP	CpSC, 0, R0, C(CpID), C(CpIDidct), CpIDct
	RET

TEXT cpmpidget(SB), 1, $-4			/* mp ID */
	MFCP	CpSC, 0, R0, C(CpID), C(CpIDidct), CpIDmpid
	RET

TEXT controlget(SB), 1, $-4			/* system control (sctlr) */
	MFCP	CpSC, 0, R0, C(CpCONTROL), C(0), CpMainctl
	RET

TEXT ttbget(SB), 1, $-4				/* translation table base */
	MFCP	CpSC, 0, R0, C(CpTTB), C(0), CpTTB0
	AND	$~(L1SIZE-1), R0		/* don't return flag bits */
	RET

TEXT ttbput(SB), 1, $-4				/* translation table base */
	MOVW	CPSR, R3
	CPSID
	/* finish prior accesses before changing ttb */
	MOVW	$0, R1
	MTCP	CpSC, 0, R1, C(CpTTB), C(0), CpTTBctl
	MTCP	CpSC, 0, R0, C(CpTTB), C(0), CpTTB0
	MTCP	CpSC, 0, R0, C(CpTTB), C(0), CpTTB1	/* non-secure too */
	B	_splxr3

TEXT dacget(SB), 1, $-4				/* domain access control */
	MFCP	CpSC, 0, R0, C(CpDAC), C(0)
	RET

TEXT dacput(SB), 1, $-4				/* domain access control */
	MOVW	R0, R1
	BARRIERS
	MTCP	CpSC, 0, R1, C(CpDAC), C(0)
	ISB
	RET

TEXT fsrget(SB), 1, $-4				/* D fault status */
	MFCP	CpSC, 0, R0, C(CpFSR), C(0), CpDFSR
	RET

TEXT farget(SB), 1, $-4				/* D fault address */
	MFCP	CpSC, 0, R0, C(CpFAR), C(0), CpDFAR
	RET

TEXT fsriget(SB), 1, $-4			/* I fault status */
	MFCP	CpSC, 0, R0, C(CpFSR), C(0), CpIFSR
	RET

TEXT fariget(SB), 1, $-4			/* I fault address */
	MFCP	CpSC, 0, R0, C(CpFAR), C(0), CpIFAR
	RET

TEXT getpsr(SB), 1, $-4
	MOVW	CPSR, R0
	RET

TEXT putpsr(SB), 1, $-4
	MOVW	R0, CPSR
	B	_barrret

TEXT getscr(SB), 1, $-4				/* secure configuration */
	MFCP	CpSC, 0, R0, C(CpCONTROL), C(CpCONTROLscr), CpSCRscr
	RET

TEXT pidget(SB), 1, $-4				/* address translation pid */
	MFCP	CpSC, 0, R0, C(CpPID), C(0x0)
	RET

TEXT pidput(SB), 1, $-4				/* address translation pid */
	MTCP	CpSC, 0, R0, C(CpPID), C(0), 0	/* pid, v7a deprecated */
	MTCP	CpSC, 0, R0, C(CpPID), C(0), 1	/* context id, errata 754322 */
	ISB
	RET

/*
 * access to yet more coprocessor registers
 */

TEXT getauxctl(SB), 1, $-4		/* get cortex-a9 aux. ctl. */
	MFCP	CpSC, 0, R0, C(CpCONTROL), C(0), CpAuxctl
	RET

TEXT putauxctl(SB), 1, $-4		/* put cortex-a9 aux. ctl. */
	BARRIERS
	MTCP	CpSC, 0, R0, C(CpCONTROL), C(0), CpAuxctl
	B	_barrret

TEXT getclvlid(SB), 1, $-4
	MFCP	CpSC, CpIDcsize, R0, C(CpID), C(CpIDidct), CpIDclvlid
	RET

TEXT getcyc(SB), 1, $-4
	MFCP	CpSC, 0, R0, C(CpCLD), C(CpCLDcyc), 0
	RET

TEXT getdebug(SB), 1, $-4		/* get cortex-a9 debug enable reg. */
	MFCP	CpSC, 0, R0, C(CpCONTROL), C(CpCONTROLscr), CpSCRdebug
	RET

TEXT getmmfr0(SB), 1, $-4		/* get ID_MMFR0 */
	MFCP	CpSC, 0, R0, C(CpID), C(CpIDidmmfr), CpIDidmmfr0
	RET

TEXT getmmfr1(SB), 1, $-4		/* get ID_MMFR1 */
	MFCP	CpSC, 0, R0, C(CpID), C(CpIDidmmfr), CpIDidmmfr0+1
	RET

TEXT getmmfr2(SB), 1, $-4		/* get ID_MMFR2 */
	MFCP	CpSC, 0, R0, C(CpID), C(CpIDidmmfr), CpIDidmmfr0+2
	RET

TEXT getmmfr3(SB), 1, $-4		/* get ID_MMFR3 */
	MFCP	CpSC, 0, R0, C(CpID), C(CpIDidmmfr), CpIDidmmfr0+3
	RET

TEXT getpc(SB), 1, $-4
	MOVW	R14, R0
	RET

TEXT getsb(SB), 1, $-4
	MOVW	R12, R0
	RET

TEXT setsp(SB), 1, $-4
	MOVW	R0, SP
	RET


/*
 * NB: the spl* functions contain barriers and it is safe to rely upon that.
 */

TEXT splhi(SB), 1, $-4
	MOVW	CPSR, R0			/* return old CPSR */
	CPSID					/* turn off interrupts */
	CMP.S	$0, R(MACH)
	MOVW.NE	R14, 4(R(MACH))			/* save caller pc in m->splpc */
	RET

TEXT spllo(SB), 1, $-4			/* start marker for devkprof.c */
	MOVW	CPSR, R0			/* return old CPSR */
	MOVW	$0, R1
	CMP.S	R1, R(MACH)
	MOVW.NE	R1, 4(R(MACH))			/* clear m->splpc */
	CPSIE
	RET

TEXT splx(SB), 1, $-4
	MOVW	CPSR, R3			/* save old CPSR */
	CPSID

	CMP.S	$0, R(MACH)
	MOVW.NE	R14, 4(R(MACH))			/* save caller pc in m->splpc */
	ORR	$PsrDfiq, R0			/* disallow fiqs */
	MOVW	R0, CPSR			/* reset interrupt level */
	BARRIERS				/* assist erratum 782773 */
	MOVW	R3, R0				/* return old CPSR */
	RET

TEXT spldone(SB), 1, $0				/* end marker for devkprof.c */
	RET

TEXT islo(SB), 1, $-4
	MOVW	CPSR, R0
	AND	$(PsrDirq), R0
	EOR	$(PsrDirq), R0
	RET

TEXT clz(SB), $-4
	CLZ(0, 0)			/* 0 is R0 */
	RET

TEXT setlabel(SB), 1, $-4
	MOVW	SP, 0(R0)
	MOVW	R14, 4(R0)		/* pc */
	MOVW	$0, R0
	RET

TEXT gotolabel(SB), 1, $-4
	MOVW	0(R0), SP
	MOVW	4(R0), R14		/* pc */
	MOVW	$1, R0
	RET

TEXT getcallerpc(SB), 1, $-4
	MOVW	0(SP), R0
	RET

TEXT wfi(SB), $-4
	MOVW	CPSR, R3
	/*
	 * an interrupt will break us out of wfi.  masking interrupts
	 * slows interrupt response slightly but prevents recursion.
	 */
	CPSID
	WFI
	BARRIERS  /* caution; errata such as PJ4B_ERRATA_4742 require this */
	B	_splxr3

TEXT clrex(SB), $-4
	CLREX
	/* fall through */
TEXT coherence(SB), $-4
	B	_barrret

/*
 * data synch. barrier: dmb + applies to cache, tlb & btc ops + no following
 * instr.s execute until dsb finishes.
 */
TEXT dsb(SB), $-4
	DSB
	ISB			/* temporary; fixes hangs */
	RET

#include "cache.v7.s"

TEXT	tas(SB), $-4		/* tas(ulong *); avoid libc one */
	/* returns old (R0) after modifying (R0) */
	MOVW	R0,R5
tasagain:
	DSB
	MOVW	$1, R2		/* new value of (R0) */
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
	DSB			/* required by erratum 754327 */
	B	tas0
lockloop2:
	PUTC('?'); PUTC('l'); PUTC('t')
	B	tasagain
lockbusy:
	CLREX
tas0:
	MOVW	R7, R0		/* return old value */
	RET
