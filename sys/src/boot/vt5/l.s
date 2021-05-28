/* virtex5 ppc440x5 machine assist */
#include	"mem.h"

#define MICROBOOT	1	/* if defined, see microboot.s for startup */

/*
 * Special Purpose Registers of interest here (440 versions)
 */
#define SPR_CCR0	0x3b3		/* Core Configuration Register 0 */
#define SPR_CCR1	0x378	/* core configuration register 1 */
#define SPR_DAC1	0x13c		/* Data Address Compare 1 */
#define SPR_DAC2	0x13d		/* Data Address Compare 2 */
#define SPR_DBCR0	0x134		/* Debug Control Register 0 */
#define SPR_DBCR1	0x135		/* Debug Control Register 1 */
#define SPR_DBCR2	0x136		/* Debug Control Register 2 */
#define SPR_DBSR	0x130		/* Debug Status Register */
#define SPR_DVC1	0x13e		/* Data Value Compare 1 */
#define SPR_DVC2	0x13f		/* Data Value Compare 2 */
#define SPR_DEAR	0x3D		/* Data Error Address Register */
#define SPR_ESR	0x3E		/* Exception Syndrome Register */
#define SPR_IAC1	0x138		/* Instruction Address Compare 1 */
#define SPR_IAC2	0x139		/* Instruction Address Compare 2 */
#define SPR_IAC3	0x13a		/* Instruction Address Compare 3 */
#define SPR_IAC4	0x13b		/* Instruction Address Compare 4 */
#define SPR_PID		0x30		/* Process ID (not the same as 405) */
#define SPR_PVR		0x11f		/* Processor Version Register */

#define SPR_SPRG0	0x110		/* SPR General 0 */
#define SPR_SPRG1	0x111		/* SPR General 1 */
#define SPR_SPRG2	0x112		/* SPR General 2 */
#define SPR_SPRG3	0x113		/* SPR General 3 */

/* beware that these registers differ in R/W ability on 440 compared to 405 */
#define SPR_SPRG4R		0x104	/* SPR general 4; user/supervisor R */
#define SPR_SPRG5R		0x105	/* SPR general 5; user/supervisor R */
#define SPR_SPRG6R		0x106	/* SPR general 6; user/supervisor R */
#define SPR_SPRG7R		0x107	/* SPR general 7; user/supervisor R */
#define SPR_SPRG4W	0x114		/* SPR General 4; supervisor W */
#define SPR_SPRG5W	0x115		/* SPR General 5; supervisor W  */
#define SPR_SPRG6W	0x116		/* SPR General 6; supervisor W  */
#define SPR_SPRG7W	0x117		/* SPR General 7; supervisor W */

#define SPR_SRR0	0x01a		/* Save/Restore Register 0 */
#define SPR_SRR1	0x01b		/* Save/Restore Register 1 */
#define SPR_CSRR0	0x03a		/* Critical Save/Restore Register 0 */
#define SPR_CSRR1	0x03b		/* Critical Save/Restore Register 1 */
#define SPR_TBL		0x11c		/* Time Base Lower */
#define SPR_TBU		0x11d		/* Time Base Upper */
#define	SPR_PIR		0x11e		/* Processor Identity Register */

#define SPR_TCR	0x154	/* timer control */
#define SPR_TSR	0x150	/* timer status */
#define SPR_MMUCR	0x3B2	/* mmu control */
#define SPR_DNV0	0x390	/* data cache normal victim 0-3 */
#define SPR_DNV1	0x391
#define SPR_DNV2	0x392
#define SPR_DNV3	0x393
#define SPR_DTV0	0x394	/* data cache transient victim 0-3 */
#define SPR_DTV1	0x395
#define SPR_DTV2	0x396
#define SPR_DTV3	0x397
#define SPR_DVLIM	0x398	/* data cache victim limit */
#define SPR_INV0	0x370	/* instruction cache normal victim 0-3 */
#define SPR_INV1	0x371
#define SPR_INV2	0x372
#define SPR_INV3	0x374
#define SPR_ITV0	0x374	/* instruction cache transient victim 0-3 */
#define SPR_ITV1	0x375
#define SPR_ITV2	0x376
#define SPR_ITV3	0x377
#define SPR_IVOR(n)	(0x190+(n))	/* interrupt vector offset registers 0-15 */
#define SPR_IVPR	0x03F	/* instruction vector prefix register */
#define SPR_IVLIM	0x399	/* instruction cache victim limit */

#define SPR_MCSRR0	0x23A	/* 440GX only */
#define SPR_MCSRR1	0x23B
#define SPR_MCSR	0x23C

#define SPR_DEC		0x16	/* on 440 they've gone back to using DEC instead of PIT  ... */
#define SPR_DECAR	0x36	/* ... with the auto-reload register now visible */

/* 440 */

/* use of SPRG registers in save/restore */
#define	SAVER0	SPR_SPRG0
#define	SAVER1	SPR_SPRG1
#define	SAVELR	SPR_SPRG2
#define	SAVEXX	SPR_SPRG3

/* special instruction definitions */
#define	BDNZ	BC	16,0,
#define	BDNE	BC	0,2,

#define	TBRL	268	/* read time base lower in MFTB */
#define	TBRU	269	/* read time base upper in MFTB */
#define	MFTB(tbr,d)	WORD	$((31<<26)|((d)<<21)|((tbr&0x1f)<<16)|(((tbr>>5)&0x1f)<<11)|(371<<1))

//#define TLBIA		WORD	$((31<<26)|(370<<1))	// not in 440
#define	TLBSYNC		WORD	$((31<<26)|(566<<1))

/* 400 models; perhaps others */
#define	ICCCI(a,b)	WORD	$((31<<26)|((a)<<16)|((b)<<11)|(966<<1))
#define	DCCCI(a,b)	WORD	$((31<<26)|((a)<<16)|((b)<<11)|(454<<1))
/* these follow the source -> dest ordering */
#define	DCREAD(s,t)	WORD	$((31<<26)|((t)<<21)|((s)<<11)|(486<<1))
#define	DCRF(n)	((((n)>>5)&0x1F)|(((n)&0x1F)<<5))
#define	MTDCR(s,n)	WORD	$((31<<26)|((s)<<21)|(DCRF(n)<<11)|(451<<1))
#define	MFDCR(n,t)	WORD	$((31<<26)|((t)<<21)|(DCRF(n)<<11)|(323<<1))
#define	MSYNC		WORD	$((31<<26)|(598<<1))
#define	TLBRELO(a,t)	WORD	$((31<<26)|((t)<<21)|((a)<<16)|(2<<11)|(946<<1))
#define	TLBREMD(a,t)	WORD	$((31<<26)|((t)<<21)|((a)<<16)|(1<<11)|(946<<1))
#define	TLBREHI(a,t)	WORD	$((31<<26)|((t)<<21)|((a)<<16)|(0<<11)|(946<<1))
#define	TLBWELO(s,a)	WORD	$((31<<26)|((s)<<21)|((a)<<16)|(2<<11)|(978<<1))
#define	TLBWEMD(s,a)	WORD	$((31<<26)|((s)<<21)|((a)<<16)|(1<<11)|(978<<1))
#define	TLBWEHI(s,a)	WORD	$((31<<26)|((s)<<21)|((a)<<16)|(0<<11)|(978<<1))
#define	TLBSXF(a,b,t)	WORD	$((31<<26)|((t)<<21)|((a)<<16)|((b)<<11)|(914<<1))
#define	TLBSXCC(a,b,t)	WORD	$((31<<26)|((t)<<21)|((a)<<16)|((b)<<11)|(914<<1)|1)
/* these are useless because there aren't CE/CEI equivalents for critical interrupts */
//#define	WRTMSR_EE(s)	WORD	$((31<<26)|((s)<<21)|(131<<1))
//#define	WRTMSR_EEI(e)	WORD	$((31<<26)|((e)<<15)|(163<<1))

/* on some models mtmsr doesn't synchronise enough (eg, 603e) */
#define	MSRSYNC	MSYNC; ISYNC

/*
 * on the 4xx series, the prefetcher madly fetches across RFI, sys call,
 * and others; use BR 0(PC) to stop.
 */
#define	RFI		WORD $((19<<26)|(50<<1)); BR 0(PC)
// #define	RFCI	WORD	$((19<<26)|(51<<1)); BR 0(PC)

/*
 * print progress character iff on cpu0.
 * steps on R7 and R8, needs SB set and TLB entry for i/o registers set.
 */
#define STEP(c, zero, notzero) \
	MOVW	SPR(SPR_PIR), R7; \
	CMP	R7, R0; \
	BEQ	zero; \
	CMP	R7, $017; \
	BNE	notzero; \
zero:	MOVW	$(Uartlite+4), R7; \
	MOVW	$(c), R8; \
	MOVW	R8, 0(R7); \
notzero: SYNC

//#define STEP(c, z, nz)
#define PROG(n)	MOVW $(n), R18		/* pre-uart progress indicator */

#define	UREGSPACE	(UREGSIZE+8)

	NOSCHED

TEXT start<>(SB), 1, $-4
PROG(1)
	/* except during trap handling, R0 is zero from now on */
	MOVW	$0, R0

	/*
	 * setup MSR
	 * turn off interrupts (clear ME, CE)
	 * use 0x000 as exception prefix
	 * enable kernel vm (clear IS, DS)
	 * clear, then enable machine checks
	 */
	MOVW	$~0, R3
	MOVW	R3, SPR(SPR_MCSR)	/* clear all bits */
	MSRSYNC
	MOVW	R0, SPR(SPR_ESR)
	MSRSYNC
PROG(2)
	MOVW	$MSR_ME, R3
	MOVW	R3, MSR
	MSRSYNC
	MOVW	R0, CR

	/* setup SB for pre mmu */
	MOVW	$setSB(SB), R2

	MOVW	$19, R19
	MOVW	$20, R20
	MOVW	$21, R21
	MOVW	$22, R22
	MOVW	$23, R23
	MOVW	$24, R24
PROG(3)
#ifndef MICROBOOT
	/*
	 * Invalidate the caches.
	 */
	ICCCI(0, 2)  /* errata cpu_121 reveals that EA is used; we'll use SB */
	DCCCI(0, 2)		/* dcache must not be in use */
	MSYNC
#endif
	MOVW	R0, SPR(SPR_DBCR0)
	MOVW	R0, SPR(SPR_DBCR1)
	MOVW	R0, SPR(SPR_DBCR2)
	ISYNC
	MOVW	$~0, R3
	MOVW	R3, SPR(SPR_DBSR)

	/*
	 * CCR0:
	 *	recover from data parity = 1
	 *	disable gathering = 0
	 *	disable trace broadcast = 1
	 *	force load/store alignment = 0 (was 1)
	 *	0: fill 1+0 speculative lines on icache miss
	 * CCR1:
	 *	normal parity, normal cache operation
	 *	cpu timer advances with tick of CPU input clock (not timer clock)   TO DO?
	 */
#ifndef MICROBOOT
	MOVW	$((1<<30)|(0<<21)|(1<<15)|(0<<8)|(0<<2)), R3
	MOVW	R3, SPR(SPR_CCR0)
	MOVW	$(0<<7), R3	/* TCS=0 */
	MOVW	R3, SPR(SPR_CCR1)
#endif
	/* clear i/d cache regions */
	MOVW	R0, SPR(SPR_INV0)
	MOVW	R0, SPR(SPR_INV1)
	MOVW	R0, SPR(SPR_INV2)
	MOVW	R0, SPR(SPR_INV3)
	MOVW	R0, SPR(SPR_DNV0)
	MOVW	R0, SPR(SPR_DNV1)
	MOVW	R0, SPR(SPR_DNV2)
	MOVW	R0, SPR(SPR_DNV3)

	/* set i/d cache limits (all normal) */
	MOVW	$((0<<22)|(63<<11)|(0<<0)), R3	/* TFLOOR=0, TCEILING=63 ways, NFLOOR = 0 */
	MOVW	R3, SPR(SPR_DVLIM)
	MOVW	R3, SPR(SPR_IVLIM)

PROG(4)
	/*
	 * set other system configuration values
	 */
	MOVW	R0, SPR(SPR_TBL)
	MOVW	R0, SPR(SPR_TBU)
	MOVW	R0, SPR(SPR_DEC)
	MOVW	$~0, R3
	MOVW	R3, SPR(SPR_TSR)
	MOVW	R0, SPR(SPR_TCR)
	ISYNC

	/*
	 * on the 440, the mmu is always on; kernelmmu just adds tlb entries.
	 * ppc440x5 cpu core manual §3.1 has the story about shadow
	 * tlbs and the like at reset.
	 */
PROG(5)
	BL	kernelmmu(SB)
	/* now running with MMU initialised & in kernel address space */

	TLBWEHI(0,0)
	TLBWEMD(0,0)
	TLBWELO(0,0)

	MOVW	$setSB(SB), R2
	/* it's now safe to print */
STEP('\r', zerobootcr, notbootcr)
STEP('\n', zerobootnl, notbootnl)
STEP('P',  zerobootP,  notbootP)

	/* make the vectors match the old values */
	MOVW	$PHYSSRAM, R3
	MOVW	R3, SPR(SPR_IVPR)	/* vector prefix at PHYSSRAM */
	MOVW	$INT_CI, R3
	MOVW	R3, SPR(SPR_IVOR(0))
	MOVW	$INT_MCHECK, R3
	MOVW	R3, SPR(SPR_IVOR(1))
	MOVW	$INT_DSI, R3
	MOVW	R3, SPR(SPR_IVOR(2))
	MOVW	$INT_ISI, R3
	MOVW	R3, SPR(SPR_IVOR(3))
	MOVW	$INT_EI, R3
	MOVW	R3, SPR(SPR_IVOR(4))
	MOVW	$INT_ALIGN, R3
	MOVW	R3, SPR(SPR_IVOR(5))
	MOVW	$INT_PROG, R3
	MOVW	R3, SPR(SPR_IVOR(6))
	MOVW	$INT_FPU, R3
	MOVW	R3, SPR(SPR_IVOR(7))	/* reserved (FPU?) */
	MOVW	$INT_SYSCALL, R3
	MOVW	R3, SPR(SPR_IVOR(8))	/* system call */
	MOVW	$INT_TRACE, R3
	MOVW	R3, SPR(SPR_IVOR(9))	/* reserved (trace?) */
	MOVW	$INT_PIT, R3
	MOVW	R3, SPR(SPR_IVOR(10))	/* decrementer */
	MOVW	$INT_FIT, R3
	MOVW	R3, SPR(SPR_IVOR(11))	/* fixed interval  */
	MOVW	$INT_WDT, R3
	MOVW	R3, SPR(SPR_IVOR(12))	/* watchdog */
	MOVW	$INT_DMISS,	R3
	MOVW	R3, SPR(SPR_IVOR(13))	/* data TLB */
	MOVW	$INT_IMISS,	R3
	MOVW	R3, SPR(SPR_IVOR(14))	/* instruction TLB */
	MOVW	$INT_DEBUG,  R3
	MOVW	R3, SPR(SPR_IVOR(15))	/* debug */
STEP('l', zerobootl, notbootl)

	/*
	 * Set up SB, vector space (16KiB, 64KiB aligned),
	 * extern registers (m->, up->) and stack.
	 * Mach (and stack) will be cleared along with the
	 * rest of BSS below if this is CPU#0.
	 * Memstart is the first free memory location
	 * after the kernel.
	 */
	/* set R2 to correct value */
	MOVW	$setSB(SB), R2

	/* invalidate the caches again to flush any addresses below KZERO */
	ICCCI(0, 2)  /* errata cpu_121 reveals that EA is used; we'll use SB */
	ISYNC

	MOVW	SPR(SPR_PIR), R7
	CMP	R7, $017
	BNE	notcpu0b
	MOVW	R0, R7
notcpu0b:
	CMP	R0, R7
	BNE	notcpu0

	/* set up Mach */
	/* for cpu0 */
	MOVW	$mach0(SB), R(MACH)
	/* put stack in sram below microboot temporarily */
	MOVW	$0xfffffe00, R1
//	ADD	$(MACHSIZE-8), R(MACH), R1	/* set stack */

//	SUB	$4, R(MACH), R3
//	ADD	$4, R1, R4
//clrmach:
//	MOVWU	R0, 4(R3)
//	CMP	R3, R4
//	BNE	clrmach

STEP('a', zeroboota, notboota)

	MOVW	$PHYSSRAM, R6			/* vectors at bottom of sram */
	MOVW	R0, R(USER)			/* up-> */
	MOVW	R0, 0(R(MACH))

_CPU0:						/* boot processor */
	MOVW	$edata-4(SB), R3
	MOVW	R1, R4				/* stop before microboot */
_clrbss:					/* clear BSS */
	MOVWU	R0, 4(R3)
	CMP	R3, R4
	BNE	_clrbss

STEP('n', zerobootn, notbootn)
//	MOVW	R4, memstart(SB)	/* start of unused memory */
	MOVW	R0, memstart(SB)	/* start of unused memory: dram */
	MOVW	R6, vectorbase(SB) /* 64KiB aligned vector base, for trapinit */

STEP(' ', zerobootsp, notbootsp)
	BL	main(SB)
	BR	0(PC)   		/* paranoia -- not reached */

notcpu0:
	MOVW	$mach0(SB), R(MACH)
	ADD	$(MACHSIZE-8), R(MACH)	/* cpu1 */
	/* put stack in sram below microboot & cpu0's stack temporarily */
	MOVW	$0xfffffb00, R1

	/* give cpu0 time to set things up */
	MOVW	$(10*1024*1024), R3
spin:
	SUB	$1, R3
	CMP	R0, R3
	BNE	spin

	BL	main(SB)
	BR	0(PC)   		/* paranoia -- not reached */


GLOBL	mach0(SB), $(MAXMACH*BY2PG)

TEXT	kernelmmu(SB), 1, $-4
PROG(6)
#ifndef MICROBOOT
	/* make following TLB entries shared, TID=PID=0 */
	MOVW	R0, SPR(SPR_PID)

	/*
	 * allocate cache on store miss, disable U1 as transient,
	 * disable U2 as SWOA, no dcbf or icbi exception, tlbsx search 0.
	 */
	MOVW	R0, SPR(SPR_MMUCR)
	ISYNC
#endif

	/* map various things 1:1 */
	MOVW	$tlbtab(SB), R4
	MOVW	$tlbtabe(SB), R5
	MOVW	$(3*4), R6	/* sizeof a tlb entry */
//	ADD	R6, R4		/* skip first 2 TLB entries in tlbtab */
//	ADD	R6, R4		/* skip first 2 TLB entries in tlbtab */
	SUB	R4, R5
	DIVW	R6, R5		/* R5 gets # of tlb entries in table */
	SUB	$4, R4		/* back up before MOVWU */
	MOVW	R5, CTR
	MOVW	$63, R3
//	SUB	$2, R3		/* skip first 2 TLB entries in TLB (#62-63) */
	SYNC
	ISYNC
PROG(7)
ltlb:
	MOVWU	4(R4), R5	/* TLBHI */
	TLBWEHI(5,3)
	MOVWU	4(R4), R5	/* TLBMD */
	TLBWEMD(5,3)
	MOVWU	4(R4), R5	/* TLBLO */
	TLBWELO(5,3)
	SUB	$1, R3
	BDNZ	ltlb
PROG(8)
	/* clear all remaining entries to remove any aliasing from boot */
#ifndef MICROBOOT
	CMP	R3, R0
	BEQ	cltlbe
	MOVW	R0, R5
cltlb:
	/* can't use 0 (R0) as first operand */
	TLBWEHI(5,3)
	TLBWEMD(5,3)
	TLBWELO(5,3)
	SUB	$1, R3
	CMP	R3, $0
	BGE	cltlb
cltlbe:
#endif
	SYNC
	ISYNC			/* switch to new TLBs */
PROG(9)
#ifdef MICROBOOT
	RETURN
#else
	/*
	 * this is no longer true; the microboot has done this:
	 * we're currently relying on the shadow I/D TLBs.  to switch to
	 * the new TLBs, we need a synchronising instruction.
	 */
	MOVW	LR, R4
	MOVW	R4, SPR(SPR_SRR0)
	MOVW	MSR, R4
	MOVW	R4, SPR(SPR_SRR1)
	/*
	 * resume in kernel mode in caller; R3 has the index of the first
	 * unneeded TLB entry
	 */
	RFI
#endif

TEXT	tlbinval(SB), 1, $-4
	TLBWEHI(0, 3)
	TLBWEMD(0, 3)
	TLBWELO(0, 3)
	ISYNC
	RETURN

TEXT	splhi(SB), 1, $-4
	MOVW	MSR, R3
	RLWNM	$0, R3, $~MSR_EE, R4
	RLWNM	$0, R4, $~MSR_CE, R4
	MOVW	R4, MSR
	MSRSYNC
	MOVW	LR, R31
	MOVW	R31, 4(R(MACH))	/* save PC in m->splpc */
	RETURN

TEXT	splx(SB), 1, $-4
	MOVW	LR, R31
	MOVW	R31, 4(R(MACH))	/* save PC in m->splpc */
	/* fall though */

TEXT	splxpc(SB), 1, $-4
	MOVW	MSR, R4
	RLWMI	$0, R3, $MSR_EE, R4
	RLWMI	$0, R3, $MSR_CE, R4
	MOVW	R4, MSR
	MSRSYNC
	RETURN

TEXT	spllo(SB), 1, $-4
	MOVW	MSR, R3
	OR	$MSR_EE, R3, R4
	OR	$MSR_CE, R4
	MOVW	R4, MSR
	MSRSYNC
	RETURN

TEXT	spldone(SB), 1, $-4
	RETURN

TEXT	islo(SB), 1, $-4
	MOVW	MSR, R3
	MOVW	$(MSR_EE|MSR_CE), R4
	AND	R4, R3
	RETURN

/* invalidate region of i-cache */
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

TEXT	sync(SB), 1, $0
	SYNC
	RETURN

TEXT	isync(SB), 1, $0
	ISYNC
	RETURN

/* write-back then invalidate region of d-cache */
TEXT	dcflush(SB), 1, $-4	/* dcflush(virtaddr, count) */
	MOVW	n+4(FP), R4
	RLWNM	$0, R3, $~(DCACHELINESZ-1), R5
	CMP	R4, $0
	BLE	dcf1
	SYNC
	SUB	R5, R3
	ADD	R3, R4
	ADD	$(DCACHELINESZ-1), R4
	SRAW	$DCACHELINELOG, R4
	MOVW	R4, CTR
dcf0:	DCBF	(R5)
	ADD	$DCACHELINESZ, R5
	BDNZ	dcf0
dcf1:
	SYNC
	RETURN

TEXT	cacheson(SB), 1, $-4
	MOVW	$1, R3			/* return value: true iff caches on */
	RETURN

TEXT	cachesinvalidate(SB), 1, $-4
	ICCCI(0, 2) /* errata cpu_121 reveals that EA is used; we'll use SB */
	DCCCI(0, 2) /* dcache must not be in use (or just needs to be clean?) */
	MSYNC
	RETURN

TEXT	getpit(SB), 1, $0
	MOVW	SPR(SPR_DEC), R3	/* they've moved it back to DEC */
	RETURN

TEXT	putpit(SB), 1, $0
	MOVW	R3, SPR(SPR_DEC)
	MOVW	R3, SPR(SPR_DECAR)
	RETURN

TEXT	putpid(SB), 1, $0
	MOVW	R3, SPR(SPR_PID)
	MOVW	SPR(SPR_MMUCR), R4
	RLWMI	$0, R3, $0xFF, R4
	MOVW	R4, SPR(SPR_MMUCR)
	RETURN

TEXT	getpid(SB), 1, $0
	MOVW	SPR(SPR_PID), R3
	RETURN

TEXT	getpir(SB), 1, $-4
	MOVW	SPR(SPR_PIR), R3
	CMP	R3, $017
	BNE	normal
	MOVW	R0, R3
normal:
	RETURN

TEXT	putstid(SB), 1, $0
	MOVW	SPR(SPR_MMUCR), R4
	RLWMI	$0, R3, $0xFF, R4
	MOVW	R4, SPR(SPR_MMUCR)
	RETURN

TEXT	getstid(SB), 1, $0
	MOVW	SPR(SPR_MMUCR), R3
	RLWNM	$0, R3, $0xFF, R3
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

TEXT	getmcsr(SB), 1, $0
	MOVW	SPR(SPR_MCSR), R3
	RETURN

TEXT	putmcsr(SB), 1, $-4
	MOVW	R3, SPR(SPR_MCSR)
	RETURN

TEXT	getesr(SB), 1, $0
	MOVW	SPR(SPR_ESR), R3
	RETURN

TEXT	putesr(SB), 1, $0
	MOVW	R3, SPR(SPR_ESR)
	RETURN

TEXT	putevpr(SB), 1, $0
	MOVW	R3, SPR(SPR_IVPR)
	RETURN

TEXT getccr0(SB), 1, $-4
	MOVW	SPR(SPR_CCR0), R3
	RETURN

TEXT	setsp(SB), 1, $0
	MOVW	R3, R1
	RETURN

TEXT	getsp(SB), 1, $0
	MOVW	R1, R3
	RETURN

TEXT	getdear(SB), 1, $0
	MOVW	SPR(SPR_DEAR), R3
	RETURN

TEXT	tas32(SB), 1, $0
	SYNC
	MOVW	R3, R4
	MOVW	$0xdead,R5
tas1:
	DCBF	(R4)	/* fix for 603x bug */
	LWAR	(R4), R3
	CMP	R3, $0
	BNE	tas0
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

TEXT	_xinc(SB), 1, $0	/* void _xinc(long *); */
	MOVW	R3, R4
xincloop:
	DCBF	(R4)		/* fix for 603x bug */
	LWAR	(R4), R3
	ADD	$1, R3
	STWCCC	R3, (R4)
	BNE	xincloop
	RETURN

TEXT	_xdec(SB), 1, $0	/* long _xdec(long *); */
	MOVW	R3, R4
xdecloop:
	DCBF	(R4)		/* fix for 603x bug */
	LWAR	(R4), R3
	ADD	$-1, R3
	STWCCC	R3, (R4)
	BNE	xdecloop
	RETURN

TEXT cas32(SB), 1, $0			/* int cas32(void*, u32int, u32int) */
	MOVW	R3, R4			/* addr */
	MOVW	old+4(FP), R5
	MOVW	new+8(FP), R6
	DCBF	(R4)			/* fix for 603x bug? */
	LWAR	(R4), R3
	CMP	R3, R5
	BNE	 fail
	STWCCC	R6, (R4)
	BNE	 fail
	MOVW	 $1, R3
	RETURN
fail:
	MOVW	 $0, R3
	RETURN

TEXT	tlbwrx(SB), 1, $-4
	MOVW	hi+4(FP), R5
	MOVW	mid+8(FP), R6
	MOVW	lo+12(FP), R7
	MSYNC
	TLBWEHI(5, 3)
	TLBWEMD(6, 3)
	TLBWELO(7, 3)
	ISYNC
	RETURN

TEXT	tlbrehi(SB), 1, $-4
	TLBREHI(3, 3)
	RETURN

TEXT	tlbremd(SB), 1, $-4
	TLBREMD(3, 3)
	RETURN

TEXT	tlbrelo(SB), 1, $-4
	TLBRELO(3, 3)
	RETURN

TEXT	tlbsxcc(SB), 1, $-4
	TLBSXCC(0, 3, 3)
	BEQ	tlbsxcc0
	MOVW	$-1, R3	/* not found */
tlbsxcc0:
	RETURN

TEXT	gotopc(SB), 1, $0
	MOVW	R3, CTR
	MOVW	LR, R31	/* for trace back */
	BR	(CTR)


/*
 * following Book E, traps thankfully leave the mmu on.
 * the following code has been executed at the exception
 * vector location already:
 *	MOVW R0, SPR(SAVER0)
 *	MOVW LR, R0
 *	MOVW R0, SPR(SAVELR)
 *	BL	trapvec(SB)
 */
TEXT	trapvec(SB), 1, $-4
	MOVW	LR, R0
	MOVW	R0, SPR(SAVEXX)	/* save ivoff--interrupt vector offset */
/* entry point for critical interrupts, machine checks, and faults */
trapcommon:
	MOVW	R1, SPR(SAVER1)		/* save stack pointer */
	/* did we come from user space? */
	MOVW	SPR(SPR_SRR1), R0
	MOVW	CR, R1
	MOVW	R0, CR
	BC	4,17,ktrap		/* if MSR[PR]=0, we are in kernel space */

	/* switch to kernel stack */
	MOVW	R1, CR

	MOVW	SPR(SPR_SPRG7R), R1 /* up->kstack+KSTACK-UREGSPACE, set in touser and sysrforkret */

	BL	saveureg(SB)
	MOVW	$mach0(SB), R(MACH)
	MOVW	8(R(MACH)), R(USER)
	BL	trap(SB)
	BR	restoreureg

ktrap:
	MOVW	R1, CR
	MOVW	SPR(SAVER1), R1
	SUB	$UREGSPACE, R1	/* push onto current kernel stack */
	BL	saveureg(SB)
	BL	trap(SB)

restoreureg:
	MOVMW	48(R1), R2	/* r2:r31 */
	/* defer R1 */
	MOVW	40(R1), R0
	MOVW	R0, SPR(SAVER0)
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
	MOVW	44(R1), R1	/* old SP */
	MOVW	SPR(SAVER0), R0
	RFI

/*
 * critical trap/interrupt.
 * the only one we can take is machine check, synchronously, and
 * outside any other trap handler.
 * [MSR_ME not cleared => handler may be interrupted by machine check]
 */
TEXT	trapcritvec(SB), 1, $-4
	MOVW	LR, R0
	MOVW	R0, SPR(SAVEXX)
	MOVW	SPR(SPR_CSRR0), R0		/* PC or excepting insn */
	MOVW	R0, SPR(SPR_SRR0)
	MOVW	SPR(SPR_CSRR1), R0		/* old MSR */
	MOVW	R0, SPR(SPR_SRR1)
	BR	trapcommon

/*
 * machine check.
 * make it look like the others.
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
 * enter with stack set and mapped.
 * on return, SB (R2) has been set, and R3 has the Ureg*,
 * the MMU has been re-enabled, kernel text and PC are in KSEG,
 * Stack (R1), R(MACH) and R(USER) are set by caller, if required.
 */
TEXT	saveureg(SB), 1, $-4
/*
 * save state
 */
	MOVMW	R2, 48(R1)		/* save gprs r2 to r31 */
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
	MOVW	SPR(SAVELR), R6	/* LR */
	MOVW	R6, 24(R1)
	MOVW	SPR(SPR_SPRG7R), R6	/* up->kstack+KSTACK-UREGSPACE */
	MOVW	R6, 20(R1)
	MOVW	SPR(SPR_SRR0), R0
	MOVW	R0, 16(R1)		/* PC of excepting insn (or next insn) */
	MOVW	SPR(SPR_SRR1), R0
	MOVW	R0, 12(R1)		/* old MSR */
	MOVW	SPR(SAVEXX), R0
	MOVW	R0, 8(R1)		/* cause/vector */
	ADD	$8, R1, R3		/* Ureg* */
	STWCCC	R3, (R1)		/* break any pending reservations */
	MOVW	$0, R0		/* compiler/linker expect R0 to be zero */
	RETURN

/*
 * restore state from Ureg and return from trap/interrupt
 */
TEXT sysrforkret(SB), 1, $-4
	MOVW	R1, 20(R1)	/* up->kstack+KSTACK-UREGSPACE set in ureg */
	BR	restoreureg

/*
 * 4xx specific
 */
TEXT	firmware(SB), 1, $0
	MOVW	$(3<<28), R3
	MOVW	R3, SPR(SPR_DBCR0)	/* system reset */
	BR	0(PC)
