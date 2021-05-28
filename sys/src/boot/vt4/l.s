/* virtex4 ppc405 machine assist */
#include "ppc405.h"
#include "define.h"

/* special instruction definitions */
#define	BDNZ	BC	16,0,
#define	BDNE	BC	0,2,

#define	TBRL	268	/* read time base lower in MFTB */
#define	TBRU	269	/* read time base upper in MFTB */
#define	MFTB(tbr,d)	WORD	$((31<<26)|((d)<<21)|((tbr&0x1f)<<16)|(((tbr>>5)&0x1f)<<11)|(371<<1))

#define	TLBIA		WORD	$((31<<26)|(370<<1))
#define	TLBSYNC		WORD	$((31<<26)|(566<<1))
	
/* 400 models; perhaps others */
#define	ICCCI(a,b)	WORD	$((31<<26)|((a)<<16)|((b)<<11)|(966<<1))
#define	DCCCI(a,b)	WORD	$((31<<26)|((a)<<16)|((b)<<11)|(454<<1))
/* these follow the source -> dest ordering */
#define	DCREAD(s,t)	WORD	$((31<<26)|((t)<<21)|((s)<<11)|(486<<1))
#define	TLBRELO(a,t)	WORD	$((31<<26)|((t)<<21)|((a)<<16)|(1<<11)|(946<<1))
#define	TLBREHI(a,t)	WORD	$((31<<26)|((t)<<21)|((a)<<16)|(0<<11)|(946<<1))
#define	TLBWELO(s,a)	WORD	$((31<<26)|((s)<<21)|((a)<<16)|(1<<11)|(978<<1))
#define	TLBWEHI(s,a)	WORD	$((31<<26)|((s)<<21)|((a)<<16)|(0<<11)|(978<<1))
#define	TLBSXF(a,b,t)	WORD	$((31<<26)|((t)<<21)|((a)<<16)|((b)<<11)|(914<<1))
#define	TLBSXCC(a,b,t)	WORD	$((31<<26)|((t)<<21)|((a)<<16)|((b)<<11)|(914<<1)|1)
#define	WRTMSR_EE(s)	WORD	$((31<<26)|((s)<<21)|(131<<1))
#define	WRTMSR_EEI(e)	WORD	$((31<<26)|((e)<<15)|(163<<1))

/* on some models mtmsr doesn't synchronise enough (eg, 603e) */
#define	MSRSYNC	SYNC; ISYNC
#define MSYNC	MSRSYNC

/*
 * on the 400 series, the prefetcher madly fetches across RFI, sys call,
 * and others; use BR 0(PC) to stop it.
 */
#define	RFI	WORD	$((19<<26)|(50<<1)); BR 0(PC)
#define	RFCI	WORD	$((19<<26)|(51<<1)); BR 0(PC)

#define MFCCR0(r) WORD $((31<<26) | ((r)<<21) | (0x1d<<11) | (0x13<<16) | (339<<1))
#define MTCCR0(r) WORD $((31<<26) | ((r)<<21) | (0x1d<<11) | (0x13<<16) | (467<<1))

/* print progress character.  steps on R7 and R8, needs SB set. */
#define PROG(c)	MOVW $(Uartlite+4), R7; MOVW $(c), R8; MOVW R8, 0(R7); SYNC	

	NOSCHED

TEXT start<>(SB), 1, $-4
	/* virtex4 CR 203746 patch for ppc405 errata cpu_213 */
	MFCCR0(3)
	OR	$0x50000000, R3
	MTCCR0(3)

	XORCC	R0, R0				/* from now on R0 == 0 */
	MOVW	R0, CR

	MOVW	R0, SPR(SPR_ESR)
	/*
	 * setup MSR
	 * turn off interrupts & mmu
	 * use 0x000 as exception prefix
	 * enable machine check
	 */
	MOVW	$(MSR_ME), R1
	ISYNC
	MOVW	R1, MSR
	MSYNC
	ISYNC

	/* setup SB for pre mmu */
	MOVW	$setSB(SB), R2		/* SB until mmu on */

PROG('\r')
PROG('\n')

	/*
	 * Invalidate the caches.
	 */
//	ICCCI(0, 0)
	MOVW	R0, SPR(SPR_ICCR)
	ICCCI(0, 2)  /* errata cpu_121 reveals that EA is used; we'll use SB */
	ISYNC
	DCCCI(0, 0)
	MSYNC

	MOVW	$((DCACHEWAYSIZE/DCACHELINESZ)-1), R3
	MOVW	R3, CTR
	MOVW	R0, R3
dcinv:
	DCCCI(0,3)
	ADD	$32, R3
	BDNZ	dcinv

	/*
	 * cache is write-through; no user-defined 0; big endian throughout.
	 * start with caches off until we have zeroed all of memory once.
	 */
	MOVW	$~0, R3
	MOVW	R3, SPR(SPR_DCWR)	/* write-through everywhere*/
	/* starting from the high bit, each bit represents 128MB */
	MOVW	R0, R3			/* region bits */
	MOVW	R3, SPR(SPR_DCCR)
	MOVW	R3, SPR(SPR_ICCR)
	ISYNC
	MOVW	R0, SPR(SPR_SU0R)
	MOVW	R0, SPR(SPR_SLER)
	ISYNC

	NOR	R3, R3		/* no speculative access in uncached mem */
	MOVW	R3, SPR(SPR_SGR)
	ISYNC

	/*
	 * set other system configuration values
	 */
	MOVW	R0, SPR(SPR_PIT)
	MOVW	$~0, R3
	MOVW	R3, SPR(SPR_TSR)


	/* run the boot loader with the mmu off */

	/*
	 * invalidate the caches again to flush any addresses
	 * below KZERO
	 */
	ICCCI(0, 0)
	ISYNC

	/*
	 * Set up SB, vector space (16KiB, 64KiB aligned),
	 * extern registers (m->, up->) and stack.
	 * Mach (and stack) will be cleared along with the
	 * rest of BSS below if this is CPU#0.
	 * Memstart is the first free memory location
	 * after the kernel.
	 */
	MOVW	$setSB(SB), R2			/* (SB) */

PROG('P')
PROG('l')

	MOVW	$PHYSSRAM, R6			/* vectors at bottom of sram */
	MOVW	R6, SPR(SPR_EVPR)

	/* only one cpu, # zero */
	/* sizeof(Mach) is currently 19*4 = 76 bytes */
	MOVW	R6, R(MACH)			/* m-> before 1st vector */

	MOVW	R0, R(USER)			/* up-> */
	MOVW	$0xfffffffc, R1		/* put stack in sram temporarily */

_CPU0:						/* boot processor */
	MOVW	$edata-4(SB), R3
	MOVW	R0, R4
	SUB	$8, R4				/* sram end, below JMP */
_clrbss:					/* clear BSS */
	MOVWU	R0, 4(R3)
	CMP	R3, R4
	BNE	_clrbss

	MOVW	R0, memstart(SB)	/* start of unused memory: dram */
	MOVW	R6, vectorbase(SB) /* 64KiB aligned vector base, for trapinit */

PROG('a')
PROG('n')
PROG(' ')
	BL	main(SB)
	BR	0(PC)
	RETURN

TEXT	cacheson(SB), 1, $-4
	/* cache is write-through; no user-defined 0; big endian throughout */
	MOVW	$~0, R3
	MOVW	R3, SPR(SPR_DCWR)	/* write-through everywhere*/
	/*
	 * cache bottom 128MB (dram) & top 128MB (sram), but not I/O reg.s.
	 * starting from the high bit, each bit represents another 128MB.
	 */
	MOVW	$(1<<31 | 1<<0), R3
	MOVW	R3, SPR(SPR_DCCR)
	MOVW	R3, SPR(SPR_ICCR)
	ISYNC
	MOVW	R0, SPR(SPR_SU0R)
	MOVW	R0, SPR(SPR_SLER)
	ISYNC

	MOVW	R3, R4
	NOR	R3, R3		/* no speculative access in uncached mem */
	MOVW	R3, SPR(SPR_SGR)
	ISYNC
	MOVW	R4, R3		/* return value: true iff caches on */
	RETURN

TEXT	splhi(SB), 1, $-4
	MOVW	MSR, R3
	WRTMSR_EEI(0)
//	MOVW	LR, R31
//	MOVW	R31, 4(R(MACH))	/* save PC in m->splpc */
	RETURN

TEXT	splx(SB), 1, $-4
//	MOVW	LR, R31
//	MOVW	R31, 4(R(MACH))	/* save PC in m->splpc */
	/* fall though */

TEXT	splxpc(SB), 1, $-4
	WRTMSR_EE(3)
	RETURN

TEXT	spllo(SB), 1, $-4
	MOVW	MSR, R3
	WRTMSR_EEI(1)
	RETURN

TEXT	spldone(SB), 1, $-4
	RETURN

TEXT	islo(SB), 1, $-4
	MOVW	MSR, R3
	RLWNM	$0, R3, $MSR_EE, R3
	RETURN

TEXT dcbi(SB), 1, $-4				/* dcbi(addr) */
	DCBI	(R3)
	RETURN

TEXT	icflush(SB), 1, $-4	/* icflush(virtaddr, count) */
	MOVW	n+4(FP), R4
	RLWNM	$0, R3, $~(ICACHELINESZ-1), R5
	SUB	R5, R3
	ADD	R3, R4
	ADD	$(ICACHELINESZ-1), R4
	SRAW	$ICACHELINELOG, R4
	MOVW	R4, CTR
icf0:	ICBI	(R5)
	ADD	$ICACHELINESZ, R5
	BDNZ	icf0
	ISYNC
	RETURN

TEXT sync(SB), 1, $-4				/* sync() */
	SYNC
	RETURN

TEXT dcflush(SB), 1, $-4			/* dcflush(virtaddr, count) */
	MOVW	n+4(FP), R4
	RLWNM	$0, R3, $~(DCACHELINESZ-1), R5
	CMP	R4, $0
	BLE	dcf1
	SUB	R5, R3
	ADD	R3, R4
	ADD	$(DCACHELINESZ-1), R4
	SRAW	$DCACHELINELOG, R4
	MOVW	R4, CTR
dcf0:
	DCBF	(R5)
	ADD	$DCACHELINESZ, R5
	BDNZ	dcf0
dcf1:
	SYNC
	RETURN

/* copied from ../vt5/l.s; hope it's right */
TEXT	cachesinvalidate(SB), 1, $-4
	ICCCI(0, 2) /* errata cpu_121 reveals that EA is used; we'll use SB */
	DCCCI(0, 2) /* dcache must not be in use (or just needs to be clean?) */
	MSYNC
	RETURN

TEXT	getpit(SB), 1, $0
	MOVW	SPR(SPR_PIT), R3
	RETURN

TEXT	putpit(SB), 1, $0
	MOVW	R3, SPR(SPR_PIT)
	RETURN

TEXT	putpid(SB), 1, $0
	MOVW	R3, SPR(SPR_PID)
	RETURN

TEXT	getpid(SB), 1, $0
	MOVW	SPR(SPR_PID), R3
	RETURN

/* 405s have no PIR, so use low bits of PVR, which rae can set. */
TEXT	getpir(SB), 1, $-4
	MOVW	SPR(SPR_PVR), R3
	ANDCC	$017, R3
	RETURN

TEXT	gettbl(SB), 1, $0
	MFTB(TBRL, 3)
	RETURN

TEXT	gettbu(SB), 1, $0
	MFTB(TBRU, 3)
	RETURN

TEXT	gettsr(SB), 1, $0
	MOVW	SPR(SPR_TSR), R3
	RETURN

TEXT	puttsr(SB), 1, $0
	MOVW	R3, SPR(SPR_TSR)
	RETURN

TEXT	gettcr(SB), 1, $0
	MOVW	SPR(SPR_TCR), R3
	RETURN

TEXT	puttcr(SB), 1, $0
	MOVW	R3, SPR(SPR_TCR)
	RETURN

TEXT	getpvr(SB), 1, $0
	MOVW	SPR(SPR_PVR), R3
	RETURN

TEXT	getmsr(SB), 1, $0
	MOVW	MSR, R3
	RETURN

TEXT	putmsr(SB), 1, $0
	SYNC
	MOVW	R3, MSR
	MSRSYNC
	RETURN

TEXT	getesr(SB), 1, $0
	MOVW	SPR(SPR_ESR), R3
	RETURN

TEXT	putesr(SB), 1, $0
	MOVW	R3, SPR(SPR_ESR)
	RETURN

TEXT	putevpr(SB), 1, $0
	MOVW	R3, SPR(SPR_EVPR)
	RETURN

TEXT	setsp(SB), 1, $0
	MOVW	R3, R1
	RETURN

TEXT	getdear(SB), 1, $0
	MOVW	SPR(SPR_DEAR), R3
	RETURN

TEXT	tas32(SB), 1, $0
	SYNC
	MOVW	R3, R4
	MOVW	$0xdead,R5
tas1:
	MSYNC
	LWAR	(R4), R3
	CMP	R3, $0
	BNE	tas0
	DCBT	(R4)				/* fix 405 errata cpu_210 */
	STWCCC	R5, (R4)
	BNE	tas1
tas0:
	SYNC
	ISYNC
	RETURN

TEXT	eieio(SB), 1, $0
	EIEIO
	RETURN

TEXT	syncall(SB), 1, $0
	SYNC
	ISYNC
	RETURN

TEXT _xinc(SB), 1, $0			/* void _xinc(long *); */
	MOVW	R3, R4
xincloop:
	LWAR	(R4), R3
	ADD	$1, R3
	DCBT	(R4)				/* fix 405 errata cpu_210 */
	STWCCC	R3, (R4)
	BNE	xincloop
	RETURN

TEXT _xdec(SB), 1, $0			/* long _xdec(long *); */
	MOVW	R3, R4
xdecloop:
	LWAR	(R4), R3
	ADD	$-1, R3
	DCBT	(R4)				/* fix 405 errata cpu_210 */
	STWCCC	R3, (R4)
	BNE	xdecloop
	RETURN


#define SPR_CSRR0	0x03a		/* Critical Save/Restore Register 0 */
#define SPR_CSRR1	0x03b		/* Critical Save/Restore Register 1 */
//#define SPR_DEAR	0x03d		/* Data Error Address Register */

#define SPR_SPRG4R	0x104		/* SPR general 4; user/supervisor R */
#define SPR_SPRG5R	0x105		/* SPR general 5; user/supervisor R */
#define SPR_SPRG6R	0x106		/* SPR general 6; user/supervisor R */
#define SPR_SPRG7R	0x107		/* SPR general 7; user/supervisor R */
#define SPR_SPRG4W	0x114		/* SPR General 4; supervisor W */
#define SPR_SPRG5W	0x115		/* SPR General 5; supervisor W  */
#define SPR_SPRG6W	0x116		/* SPR General 6; supervisor W  */
#define SPR_SPRG7W	0x117		/* SPR General 7; supervisor W */

#define SPR_MCSRR0	0x23a
#define SPR_MCSRR1	0x23b

#define	SAVER0		SPR_SPRG0	/* shorthand use in save/restore */
#define	SAVER1		SPR_SPRG1
#define	SAVELR		SPR_SPRG2
#define	SAVEXX		SPR_SPRG3

#define	UREGSPACE	(UREGSIZE+8)

#define RTBL		28		/* time stamp tracing */

/*
 * the 405 does not follow Book E: traps turn the mmu off.
 * the following code has been executed at the exception
 * vector location already:
 *	MOVW	R0, SPR(SAVER0)
 *	(critical interrupts disabled in MSR, using R0)
 *	MOVW	LR, R0
 *	MOVW	R0, SPR(SAVELR) 
 *	BL	trapvec(SB)
 */
TEXT	trapvec(SB), 1, $-4
	MOVW	LR, R0
	MOVW	R0, SPR(SAVEXX)			/* save interrupt vector offset */
trapcommon:					/* entry point for machine checks */
	MOVW	R1, SPR(SAVER1)			/* save stack pointer */

	/* did we come from user space? */
	MOVW	SPR(SPR_SRR1), R0
	MOVW	CR, R1
	MOVW	R0, CR
	BC	4,17,ktrap			/* if MSR[PR]=0, we are in kernel space */

	/* was user mode, switch to kernel stack and context */
	MOVW	R1, CR
	MOVW	SPR(SPR_SPRG7R), R1		/* up->kstack+KSTACK-UREGSPACE, set in touser and sysrforkret */
	MFTB(TBRL, RTBL)
	BL	saveureg(SB)

//	MOVW	$mach0(SB), R(MACH)		/* FIX FIX FIX */
//	MOVW	8(R(MACH)), R(USER)		/* FIX FIX FIX */
//try this:
/* 405s have no PIR; could use PVR */
//	MOVW	SPR(SPR_PIR), R4		/* PIN */
//	SLW	$2, R4				/* offset into pointer array */
	MOVW	$0, R4				/* assume cpu 0 */
	MOVW	$machptr(SB), R(MACH)		/* pointer array */
	ADD	R4, R(MACH)			/* pointer to array element */
	MOVW	(R(MACH)), R(MACH)		/* m-> */
	MOVW	8(R(MACH)), R(USER)		/* up-> */

	BL	trap(SB)
	BR	restoreureg

ktrap:
	/* was kernel mode, R(MACH) and R(USER) already set */
	MOVW	R1, CR
	MOVW	SPR(SAVER1), R1
	SUB	$UREGSPACE, R1		/* push onto current kernel stack */
	BL	saveureg(SB)
	BL	trap(SB)

restoreureg:
	MOVMW	48(R1), R2		/* r2:r31 */
	/* defer R1, R0 */
	MOVW	36(R1), R0
	MOVW	R0, CTR
	MOVW	32(R1), R0
	MOVW	R0, XER
	MOVW	28(R1), R0
	MOVW	R0, CR	/* CR */
	MOVW	24(R1), R0
	MOVW	R0, LR
	MOVW	20(R1), R0
	MOVW	R0, SPR(SPR_SPRG7W)	/* kstack for traps from user space */
	MOVW	16(R1), R0
	MOVW	R0, SPR(SPR_SRR0)	/* old PC */
	MOVW	12(R1), R0
	RLWNM	$0, R0, $~MSR_WE, R0	/* remove wait state */
	MOVW	R0, SPR(SPR_SRR1)	/* old MSR */
	/* cause, skip */
	MOVW	40(R1), R0
	MOVW	44(R1), R1		/* old SP */
	SYNC				/* fix 405 errata cpu_210 */
	RFI

/*
 * machine check.
 * make it look like the others.
 * it's safe to destroy SPR_SRR0/1 because they can only be in
 * use if a critical interrupt has interrupted a non-critical interrupt
 * before it has had a chance to block critical interrupts,
 * but no recoverable machine checks can occur during a critical interrupt,
 * so the lost state doesn't matter.
 */
TEXT	trapmvec(SB), 1, $-4
	MOVW	LR, R0
	MOVW	R0, SPR(SAVEXX)
	MOVW	SPR(SPR_MCSRR0), R0		/* PC or excepting insn */
	MOVW	R0, SPR(SPR_SRR0)
	MOVW	SPR(SPR_MCSRR1), R0		/* old MSR */
	MOVW	R0, SPR(SPR_SRR1)
	BR	trapcommon

/*
 * external interrupts (non-critical)
 */
TEXT	intrvec(SB), 1, $-4
	MOVW	LR, R0
	MOVW	R0, SPR(SAVEXX)			/* save interrupt vector offset */
	MOVW	R1, SPR(SAVER1)			/* save stack pointer */

	/* did we come from user space? */
	MOVW	SPR(SPR_SRR1), R0
	MOVW	CR, R1
	MOVW	R0, CR
	BC	4,17,intr1			/* if MSR[PR]=0, we are in kernel space */

	/* was user mode, switch to kernel stack and context */
	MOVW	R1, CR
	MOVW	SPR(SPR_SPRG7R), R1		/* up->kstack+KSTACK-UREGSPACE, set in touser and sysrforkret */
	BL	saveureg(SB)

//	MOVW	$mach0(SB), R(MACH)		/* FIX FIX FIX */
//	MOVW	8(R(MACH)), R(USER)
//try this:
/* 405s have no PIR */
//	MOVW	SPR(SPR_PIR), R4		/* PIN */
//	SLW	$2, R4				/* offset into pointer array */
	MOVW	$0, R4				/* assume cpu 0 */
	MOVW	$machptr(SB), R(MACH)		/* pointer array */
	ADD	R4, R(MACH)			/* pointer to array element */
	MOVW	(R(MACH)), R(MACH)		/* m-> */
	MOVW	8(R(MACH)), R(USER)		/* up-> */

	BL	intr(SB)
	BR	restoreureg

intr1:
	/* was kernel mode, R(MACH) and R(USER) already set */
	MOVW	R1, CR
	MOVW	SPR(SAVER1), R1
	SUB	$UREGSPACE, R1		/* push onto current kernel stack */
	BL	saveureg(SB)
	BL	intr(SB)
	BR	restoreureg

/*
 * critical interrupt
 */
TEXT	critintrvec(SB), 1, $-4
	MOVW	LR, R0
	MOVW	R0, SPR(SAVEXX)
	MOVW	R1, SPR(SAVER1)		/* save stack pointer */

	/* did we come from user space? */
	MOVW	SPR(SPR_CSRR1), R0
	MOVW	CR, R1
	MOVW	R0, CR
	BC	4,16,kintrintr		/* if MSR[EE]=0, kernel was interrupted at start of intrvec */
	BC	4,17,kcintr1		/* if MSR[PR]=0, we are in kernel space */

ucintr:
	/* was user mode or intrvec interrupted: switch to kernel stack and context */
	MOVW	R1, CR
	MOVW	SPR(SPR_SPRG7R), R1		/* up->kstack+KSTACK-UREGSPACE, set in touser and sysrforkret */
	BL	saveureg(SB)

//	MOVW	$mach0(SB), R(MACH)		/* FIX FIX FIX */
//	MOVW	8(R(MACH)), R(USER)
//try this:
/* 405s have no PIR */
//	MOVW	SPR(SPR_PIR), R4		/* PIN */
//	SLW	$2, R4				/* offset into pointer array */
	MOVW	$0, R4				/* assume cpu 0 */
	MOVW	$machptr(SB), R(MACH)		/* pointer array */
	ADD	R4, R(MACH)			/* pointer to array element */
	MOVW	(R(MACH)), R(MACH)		/* m-> */
	MOVW	8(R(MACH)), R(USER)		/* up-> */

	BR	cintrcomm

kintrintr:
	/* kernel mode, and EE off, so kernel intrvec interrupted, but was previous mode kernel or user? */
	MOVW	SPR(SPR_SRR1), R0
	MOVW	R0, CR
	BC	(4+8),17,ucintr	/* MSR[PR]=1, we were in user space, need set up */

kcintr1:
	/*  was kernel mode and external interrupts enabled, R(MACH) and R(USER) already set */
	MOVW	R1, CR
	MOVW	SPR(SAVER1), R1
	SUB	$UREGSPACE, R1	/* push onto current kernel stack */
	BL	saveureg(SB)

cintrcomm:
	/* special part of Ureg for critical interrupts only (using Ureg.dcmp, Ureg.icmp, Ureg.dmiss) */
	MOVW	SPR(SPR_SPRG6R), R4	/* critical interrupt saves volatile R0 in SPRG6 */
	MOVW	R4, (160+8)(R1)
	MOVW	SPR(SPR_CSRR0), R4	/* store critical interrupt pc */
	MOVW	R4, (164+8)(R1)
	MOVW	SPR(SPR_CSRR1), R4	/* critical interrupt msr */
	MOVW	R4, (168+8)(R1)

	BL	intr(SB)

	/* first restore usual part of Ureg */
	MOVMW	48(R1), R2	/* r2:r31 */
	/* defer R1, R0 */
	MOVW	40(R1), R0
	MOVW	R0, SPR(SAVER0)		/* restore normal r0 save */
	MOVW	36(R1), R0
	MOVW	R0, CTR
	MOVW	32(R1), R0
	MOVW	R0, XER
	MOVW	28(R1), R0
	MOVW	R0, CR	/* CR */
	MOVW	24(R1), R0
	MOVW	R0, LR
	MOVW	20(R1), R0
	MOVW	R0, SPR(SPR_SPRG7W)	/* kstack for traps from user space */
	MOVW	16(R1), R0
	MOVW	R0, SPR(SPR_SRR0)	/* saved normal PC */
	MOVW	12(R1), R0
	MOVW	R0, SPR(SPR_SRR1)	/* saved normal MSR */

	/* restore special bits for critical interrupts */
	MOVW	(164+8)(R1), R0		/* critical interrupt's saved pc */
	MOVW	R0, SPR(SPR_CSRR0)
	MOVW	(168+8)(R1), R0
	RLWNM	$0, R0, $~MSR_WE, R0	/* remove wait state */
	MOVW	R0, SPR(SPR_CSRR1)	

	/* cause, skip */
	MOVW	(160+8)(R1), R0		/* critical interrupt's saved R0 */
	MOVW	44(R1), R1		/* old SP */
	RFCI
	
/*
 * enter with stack set and mapped.
 * on return, SB (R2) has been set, and R3 has the Ureg*,
 * the MMU has been re-enabled, kernel text and PC are in KSEG,
 * Stack (R1), R(MACH) and R(USER) are set by caller, if required.
 */
TEXT	saveureg(SB), 1, $-4
	MOVMW	R2, 48(R1)			/* save gprs r2 to r31 */
	MOVW	$setSB(SB), R2
	MOVW	SPR(SAVER1), R4
	MOVW	R4, 44(R1)
	MOVW	SPR(SAVER0), R5
	MOVW	R5, 40(R1)
	MOVW	CTR, R6
	MOVW	R6, 36(R1)
	MOVW	XER, R4
	MOVW	R4, 32(R1)
	MOVW	CR, R5
	MOVW	R5, 28(R1)
	MOVW	SPR(SAVELR), R6			/* LR */
	MOVW	R6, 24(R1)
	MOVW	SPR(SPR_SPRG7R), R6		/* up->kstack+KSTACK-UREGSPACE */
	MOVW	R6, 20(R1)
	MOVW	SPR(SPR_SRR0), R0
	MOVW	R0, 16(R1)			/* PC of excepting insn (or next insn) */
	MOVW	SPR(SPR_SRR1), R0
	MOVW	R0, 12(R1)			/* old MSR */
	MOVW	SPR(SAVEXX), R0
	MOVW	R0, 8(R1)			/* cause/vector */
	ADD	$8, R1, R3			/* Ureg* */
	DCBT	(R1)				/* fix 405 errata cpu_210 */
	STWCCC	R3, (R1)			/* break any pending reservations */
	MOVW	$0, R0				/* compiler/linker expect R0 to be zero */
	RETURN
