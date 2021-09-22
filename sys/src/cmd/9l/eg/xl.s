#include "/sys/src/9k/ppc440/mem.h"

#define	KOFFSET	KZERO	/* value to subtract from setSB before mmu on */

/*
 * Special Purpose Registers of interest here (440 versions)
 */
#define SPR_CCR0	0x3b3		/* Core Configuration Register 0 */
#define SPR_CCR1	0x378	/* core configuration register 1 */
#define SPR_DAC1	0x13c		/* Data Address Compare 1 */
#define SPR_DAC2	0x13d		/* Data Address Compare 2 */
#define SPR_DBCR0	0x134		/* Debug Control Register 0 */
#define SPR_DBCR1	0x135		/* Debug Control Register 1 */
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

#define SPR_DEC	0x16		/* on 440 they've gone back to using DEC instead of PIT  ... */
#define SPR_DECAR	0x36		/* ... with the auto-reload register now visible */

/* 440 */

/* 440 L2 cache (DCR) */
#define L2C0_CFG	0x30	/* cache config */
#define L2C0_CMD	0x31	/* cache command */
#define L2C0_ADDR	0x32	/* cache address register */
#define L2C0_DATA	0x33	/* cache data register */
#define L2C0_SR	0x34	/* cache status */
#define L2C0_REVID	0x35	/* cache revision */
/* don't bother with snoop */

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

#define	TLBIA		WORD	$((31<<26)|(370<<1))
#define	TLBSYNC		WORD	$((31<<26)|(566<<1))
	
/* 400 models; perhaps others */
#define	ICCCI(a,b)	WORD	$((31<<26)|((a)<<16)|((b)<<11)|(966<<1))
#define	DCCCI(a,b)	WORD	$((31<<26)|((a)<<16)|((b)<<11)|(454<<1))
/* these follow the source -> dest ordering */
#define	DCREAD(s,t)	WORD	$((31<<26)|((t)<<21)|((s)<<11)|(486<<1))
#define	DCRF(n)	((((n)>>5)&0x1F)|(((n)&0x1F)<<5))
#define	MTDCR(s,n)	WORD	$((31<<26)|((s)<<21)|(DCRF(n)<<11)|(451<<1))
#define	MFDCR(n,t)	WORD	$((31<<26)|((t)<<21)|(DCRF(n)<<11)|(323<<1))
#define	MSYNC	WORD	$((31<<26)|(598<<1))
#define	TLBRELO(a,t)	WORD	$((31<<26)|((t)<<21)|((a)<<16)|(2<<11)|(946<<1))
#define	TLBREMD(a,t)	WORD	$((31<<26)|((t)<<21)|((a)<<16)|(1<<11)|(946<<1))
#define	TLBREHI(a,t)	WORD	$((31<<26)|((t)<<21)|((a)<<16)|(0<<11)|(946<<1))
#define	TLBWELO(s,a)	WORD	$((31<<26)|((s)<<21)|((a)<<16)|(2<<11)|(978<<1))
#define	TLBWEMD(s,a)	WORD	$((31<<26)|((s)<<21)|((a)<<16)|(1<<11)|(978<<1))
#define	TLBWEHI(s,a)	WORD	$((31<<26)|((s)<<21)|((a)<<16)|(0<<11)|(978<<1))
#define	TLBSXF(a,b,t)	WORD	$((31<<26)|((t)<<21)|((a)<<16)|((b)<<11)|(914<<1))
#define	TLBSXCC(a,b,t)	WORD	$((31<<26)|((t)<<21)|((a)<<16)|((b)<<11)|(914<<1)|1)
	
/* on some models mtmsr doesn't synchronise enough (eg, 603e) */
#define	MSRSYNC	MSYNC; ISYNC

#define	STEP(x)	MOVW $(x), R3; BL xuartputc(SB)
#undef STEP
#define	STEP(x)

#define	UREGSPACE	(UREGSIZE+8)

	TEXT start(SB), $-8
	/*
	 * setup MSR
	 * turn off interrupts
	 * use 0x000 as exception prefix
	 * enable machine check
	 */
	MOVD	MSR, R3
	RLWNM	$0, R3, $~MSR_EE, R3
	RLWNM	$0, R3, $~(MSR_IS|MSR_DS), R3
	OR	$(MSR_ME), R3
	ISYNC
	MOVD	R3, MSR
	MSRSYNC

	/* except during trap handling, R0 is zero from now on */
	MOVW	$0, R0
	MOVW	R0, CR

	/* setup SB for pre mmu */
	MOVD	$setSB-KOFFSET(SB), R2	/* SB until tlb established */
	MOVW	$0x300000, R1	/* temporary stack */

	MOVW	$18, R18
	MOVW	$19, R19
	MOVW	$20, R20
	MOVW	$21, R21
	MOVW	$22, R22
	MOVW	$23, R23
	MOVW	$24, R24

STEP('A')
	MOVW	R0, xuartlocn(SB)

/*
 * invalidate the caches
 */
//	ICCCI(0, 2)	/* this flushes the icache of a 440; the errata reveals that EA is used; we'll use SB */
//	DCCCI(0, 2)	/* this flash invalidates the dcache of a 440 (must not be in use) */
	MSYNC

#ifdef DBCRCONFIG	/* we'll let the bootstrap decide */
	MOVW	R0, SPR(SPR_DBCR0)
	MOVW	R0, SPR(SPR_DBCR1)
	MOVW	R0, SPR(SPR_DBCR2)
	MOVW	R0, SPR(SPR_DBSR)
#endif

STEP('B')

	/*
	 * CCR0:
	 *	recover from data parity = 1
	 *	disable gathering = 0
	 *	disable trace broadcast = 1
	 *	force load/store alignment = 1
	 *	fill one speculative line on icache miss
	 * CCR1:
	 *	normal parity, normal cache operation
	 *	cpu timer advances with tick of CPU input clock (not timer clock)   TO DO?
	 */
//	MOVW	$((1<<30)|(0<<21)|(1<<15)|(1<<8)|(1<<2)), R3
//	MOVW	$((1<<30)|(0<<21)|(1<<15)|(1<<8)|(0<<2)), R3
//	MOVW	R3, SPR(SPR_CCR0)
//	MOVW	$(0<<7), R3	/* TCS=0 */
//	MOVW	R3, SPR(SPR_CCR1)
//
//	/* clear i/d cache regions */
//	MOVW	R0, SPR(SPR_INV0)
//	MOVW	R0, SPR(SPR_INV1)
//	MOVW	R0, SPR(SPR_INV2)
//	MOVW	R0, SPR(SPR_INV3)
//	MOVW	R0, SPR(SPR_DNV0)
//	MOVW	R0, SPR(SPR_DNV1)
//	MOVW	R0, SPR(SPR_DNV2)
//	MOVW	R0, SPR(SPR_DNV3)

	/* set i/d cache limits (all normal) */
//	MOVW	$((0<<22)|(63<<11)|(0<<0)), R3	/* TFLOOR=0, TCEILING=63 ways, NFLOOR = 0 */
//	MOVW	R3, SPR(SPR_DVLIM)
//	MOVW	R3, SPR(SPR_IVLIM)

#ifdef L2CACHECONFIG
	/*
	 * clear L2 cache and set configuration
	 *	L2M = 1	set SRAM for L2 use
	 *	ICU=1	instruction cache enabled for L2
	 *	DCU=1	data cache enabled for L2
	 *	DCW=0	enable ways 0, 1, 2, 3
	 *	TPC=1 CPC=1	enable parity checking
	 */
	MOVW	$((1<<31)|(1<<30)|(1<<29)|(1<<24)|(1<<23)), R3
	MSYNC
	MOVW	R3, DCR(L2C0_CFG)
	MSYNC
	/* and much more TO DO */
	/* but we'll assume the bootstrap has enabled it */
#endif

//	MOVW	$(1<<31), R4	/* reset MAL */
//	MOVW	R4, DCR(0x180)

	/*
	 * set other system configuration values
	 */
	MOVW	R0, SPR(SPR_DEC)
//	MOVW	$~0, R3
//	MOVW	R3, SPR(SPR_TSR)
STEP('C')

//	BL	xtlb(SB)

STEP('!')
STEP('\n')

	BL	kernelmmu(SB)
	/* now running with MMU in kernel address space */

	MOVD	$setSB(SB), R2

MOVW R2, xuartlocn(SB); GLOBL xuartlocn(SB), $4

	/* make the vectors match the old values */
	MOVW	$KZERO, R3
	MOVW	R3, SPR(SPR_IVPR)	/* vector prefix at KZERO */
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
STEP('D')

STEP('E')

	/* set R2 to correct value */
	MOVD	$setSB(SB), R2

	/* invalidate the caches again to flush any addresses below KZERO */
	ICCCI(0, 2)	/* this flushes the icache of a 440; the errata reveals that EA is used; we'll use SB */
	ISYNC

/*	BL	kfpinit(SB)	*/	/* TO DO: bg has FPU */

	/* set up Mach */
	MOVW	$mach0(SB), R(MACH)
	ADD	$(MACHSIZE-8), R(MACH), R1	/* set stack */
	SUB	$4, R(MACH), R3
	ADD	$4, R1, R4
clrmach:
	MOVWZU	R0, 4(R3)
	CMP	R3, R4
	BNE	clrmach

	MOVW	R0, R(USER)
	MOVW	R0, 0(R(MACH))

	MOVW	$edata(SB), R3
	MOVW	$end(SB), R4
	ADD	$4, R4
	SUB	$4, R3
clrbss:
	MOVWZU	R0, 4(R3)
	CMP	R3, R4
	BNE	clrbss

STEP('F')

	BL	main(SB)
	BR	0(PC)   /* paranoia -- not reached */

GLOBL	mach0(SB), $(MAXMACH*BY2PG)

TEXT	kernelmmu(SB), $-4

	/* make following TLB entries shared, TID=PID=0 */
	MOVW	R0, SPR(SPR_PID)

	/* allocate cache on store miss, disable U1 as transient, disable U2 as SWOA, no dcbf or icbi exception, tlbsx search 0 */
	MOVW	R0, SPR(SPR_MMUCR)

	/* map various things 1:1 */
	MOVW	$tlbtab-KOFFSET(SB), R4
	MOVW	$tlbtabe-KOFFSET(SB), R5
	SUB		R4, R5
	MOVW	$(3*4), R6
	DIVW	R6, R5
	SUB		$4, R4
	MOVW	R5, CTR
	MOVW	$63, R3
ltlb:
	MOVWZU	4(R4), R5	/* TLBHI */
	TLBWEHI(5,3)
	MOVWZU	4(R4), R5	/* TLBMD */
	TLBWEMD(5,3)
	MOVWZU	4(R4), R5	/* TLBLO */
	TLBWELO(5,3)
	SUB		$1, R3
	BDNZ	ltlb

	/* clear all remaining entries to remove any aliasing from boot */
	CMP	R3, $0
	BEQ	cltlbe
cltlb:
	TLBWEHI(0,3)
	TLBWEMD(0,3)
	TLBWELO(0,3)
	SUB	$1, R3
	CMP	R3, $0
	BGE	cltlb
cltlbe:

	/*
 	* we're currently relying on the shadow I/D TLBs.  to switch to the new TLBs,
	* we need a synchronising instruction.  ISYNC won't work here
	* because the PC is still below KZERO, but there's no TLB entry to support that,
	* and once we ISYNC the shadow i-tlb entry vanishes, taking the PC's location with it.
	 */
	MOVW	LR, R4
	OR	$KZERO, R4
	MOVW	R4, SPR(SPR_SRR0)
	MOVD	MSR, R4
	MOVW	R4, SPR(SPR_SRR1)
	RFI	/* resume in kernel mode in caller; R3 has the index of the first unneeded TLB entry */

TEXT	tlbinval(SB), $0
	TLBWEHI(0, 3)
	TLBWEMD(0, 3)
	TLBWELO(0, 3)
	ISYNC
	RETURN

TEXT	splhi(SB), $-4
	MOVD	MSR, R3
	RLWNM	$0, R3, $~MSR_EE, R4
	MOVD	R4, MSR
	MSRSYNC
	MOVW	LR, R31
	MOVW	R31, 4(R(MACH))	/* save PC in m->splpc */
	RETURN

TEXT	splx(SB), $-4
	MOVW	LR, R31
	MOVW	R31, 4(R(MACH))	/* save PC in m->splpc */
	/* fall though */

TEXT	splxpc(SB), $-4
	MOVD	MSR, R4
	RLWMI	$0, R3, $MSR_EE, R4
	MOVD	R4, MSR
	MSRSYNC
	RETURN

TEXT	spllo(SB), $-4
	MOVD	MSR, R3
	OR	$MSR_EE, R3, R4
	MOVD	R4, MSR
	MSRSYNC
	RETURN

TEXT	spldone(SB), $-4
	RETURN

TEXT	islo(SB), $-4
	MOVD	MSR, R3
	MOVD	$MSR_EE, R4
	AND	R4, R3
	RETURN

TEXT	setlabel(SB), $-4
	MOVW	LR, R31
	MOVW	R1, 0(R3)
	MOVW	R31, 4(R3)
	MOVW	$0, R3
	RETURN

TEXT	gotolabel(SB), $-4
	MOVW	4(R3), R31
	MOVW	R31, LR
	MOVW	0(R3), R1
	MOVW	$1, R3
	RETURN

TEXT	touser(SB), $-4
	/* splhi */
	MOVD	MSR, R5
	RLWNM	$0, R5, $~MSR_EE, R5
	MOVD	R5, MSR
	MSRSYNC
	MOVW	R(USER), R4			/* up */
	MOVW	8(R4), R4				/* up->kstack */
	ADD		$(KSTACK-UREGSPACE), R4
	MOVW	R4, SPR(SPR_SPRG7W)		/* save for use in traps/interrupts */
	MOVW	$(UTZERO+32), R5	/* header appears in text */
	MOVD	$UMSR, R4
	MOVW	R4, SPR(SPR_SRR1)
	MOVW	R3, R1
	MOVW	R5, SPR(SPR_SRR0)
	RFI

TEXT	icflush(SB), $-4	/* icflush(virtaddr, count) */
	MOVW	n+4(FP), R4
	RLWNM	$0, R3, $~(ICACHELINESZ-1), R5
	SUB	R5, R3
	ADD	R3, R4
	ADD		$(ICACHELINESZ-1), R4
	SRAW	$ICACHELINELOG, R4
	MOVW	R4, CTR
icf0:	ICBI	(R5)
	ADD	$ICACHELINESZ, R5
	BDNZ	icf0
	ISYNC
	RETURN

TEXT	dcflush(SB), $-4	/* dcflush(virtaddr, count) */
	MOVW	n+4(FP), R4
	RLWNM	$0, R3, $~(DCACHELINESZ-1), R5
	CMP	R4, $0
	BLE	dcf1
	SUB	R5, R3
	ADD	R3, R4
	ADD		$(DCACHELINESZ-1), R4
	SRAW	$DCACHELINELOG, R4
	MOVW	R4, CTR
dcf0:	DCBF	(R5)
	ADD	$DCACHELINESZ, R5
	BDNZ	dcf0
dcf1:
	SYNC
	RETURN

TEXT	tas(SB), $0
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

TEXT	_xinc(SB),$0	/* void _xinc(long *); */
	MOVW	R3, R4
xincloop:
	DCBF	(R4)	/* fix for 603x bug */
	LWAR	(R4), R3
	ADD	$1, R3
	STWCCC	R3, (R4)
	BNE	xincloop
	RETURN

TEXT	_xdec(SB),$0	/* long _xdec(long *); */
	MOVW	R3, R4
xdecloop:
	DCBF	(R4)	/* fix for 603x bug */
	LWAR	(R4), R3
	ADD	$-1, R3
	STWCCC	R3, (R4)
	BNE	xdecloop
	RETURN

TEXT cas(SB), $0			/* int cmpswap(int*, int, int) */
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

TEXT	getpit(SB), $0
	MOVW	SPR(SPR_DEC), R3	/* they've moved it back to DEC */
	RETURN

TEXT	putpit(SB), $0
	MOVW	R3, SPR(SPR_DEC)
	MOVW	R3, SPR(SPR_DECAR)
	RETURN

TEXT	putpid(SB), $0
	MOVW	R3, SPR(SPR_PID)
	MOVW	SPR(SPR_MMUCR), R4
	RLWMI	$0, R3, $0xFF, R4
	MOVW	R4, SPR(SPR_MMUCR)
	RETURN

TEXT	getpid(SB), $0
	MOVW	SPR(SPR_PID), R3
	RETURN

TEXT	putstid(SB), $0
	MOVW	SPR(SPR_MMUCR), R4
	RLWMI	$0, R3, $0xFF, R4
	MOVW	R4, SPR(SPR_MMUCR)
	RETURN

TEXT	getstid(SB), $0
	MOVW	SPR(SPR_MMUCR), R3
	RLWNM	$0, R3, $0xFF, R3
	RETURN

TEXT	gettbl(SB), $0
	MFTB(TBRL, 3)
	RETURN

TEXT	gettbu(SB), $0
	MFTB(TBRU, 3)
	RETURN

TEXT	gettsr(SB), $0
	MOVW	SPR(SPR_TSR), R3
	RETURN

TEXT	puttsr(SB), $0
	MOVW	R3, SPR(SPR_TSR)
	RETURN

TEXT	puttcr(SB), $0
	MOVW	R3, SPR(SPR_TCR)
	RETURN

TEXT	getpvr(SB), $0
	MOVW	SPR(SPR_PVR), R3
	RETURN

TEXT	getmsr(SB), $0
	MOVD	MSR, R3
	RETURN

TEXT	putmsr(SB), $0
	SYNC
	MOVD	R3, MSR
	MSRSYNC
	RETURN

TEXT	getmcsr(SB), $0
	MOVW	SPR(SPR_MCSR), R3
	RETURN

TEXT	getesr(SB), $0
	MOVW	SPR(SPR_ESR), R3
	RETURN

TEXT	putesr(SB), $0
	MOVW	R3, SPR(SPR_ESR)
	RETURN

TEXT	putevpr(SB), $0
	MOVW	R3, SPR(SPR_IVPR)
	RETURN

TEXT getccr0(SB), $-4
	MOVW	SPR(SPR_CCR0), R3
	RETURN
		
TEXT	getdear(SB), $0
	MOVW	SPR(SPR_DEAR), R3
	RETURN

TEXT	getdcr(SB), $-4
	MOVW	$_getdcr(SB), R5
	SLW	$3, R3
	ADD	R3, R5
	MOVW	R5, CTR
	BR	(CTR)

TEXT	putdcr(SB), $-4
	MOVW	$_putdcr(SB), R5
	SLW	$3, R3
	ADD	R3, R5
	MOVW	R5, CTR
	MOVW	8(R1), R3
	BR	(CTR)
	
TEXT	eieio(SB), $0
	EIEIO
	RETURN

TEXT	sync(SB), $0
	SYNC
	RETURN

TEXT	tlbwrx(SB), $-4
	MOVW	hi+4(FP), R5
	MOVW	mid+8(FP), R6
	MOVW	lo+12(FP), R7
	MSYNC
	TLBWEHI(5, 3)
	TLBWEMD(6, 3)
	TLBWELO(7, 3)
	ISYNC
	RETURN

TEXT	tlbrehi(SB), $-4
	TLBREHI(3, 3)
	RETURN

TEXT	tlbremd(SB), $-4
	TLBREMD(3, 3)
	RETURN

TEXT	tlbrelo(SB), $-4
	TLBRELO(3, 3)
	RETURN

TEXT	tlbsxcc(SB), $-4
	TLBSXCC(0, 3, 3)
	BEQ	tlbsxcc0
	MOVW	$-1, R3	/* not found */
tlbsxcc0:
	RETURN

TEXT	gotopc(SB), $0
	MOVW	R3, CTR
	MOVW	LR, R31	/* for trace back */
	BR	(CTR)

TEXT	dtlbmiss(SB), $-4
	MOVW	R3, SPR(SPR_SPRG1)
	MOVW	$INT_DMISS, R3
	MOVW	R3, SPR(SAVEXX)	/* value for cause if entry not in soft tlb */
	MOVW	SPR(SPR_DEAR), R3
	BR	tlbmiss

TEXT	itlbmiss(SB), $-4
	MOVW	R3, SPR(SPR_SPRG1)
	MOVW	$INT_IMISS, R3
	MOVW	R3, SPR(SAVEXX)
	MOVW	SPR(SPR_SRR0), R3

tlbmiss:
	/* R3 contains missed address */
	RLWNM	$0, R3, $~(BY2PG-1), R3	/* just the page */
	MOVW	R2, SPR(SPR_SPRG0)
	MOVW	R4, SPR(SPR_SPRG2)
	MOVW	R5, SPR(SPR_SPRG6W)
	MOVW	R6, SPR(SPR_SPRG4W)
	MOVW	CR, R6
	MOVW	R6, SPR(SPR_SPRG5W)
	MOVD	$setSB(SB), R2
	MOVW	$mach0(SB), R2
	MOVW	(6*4)(R2), R4	/* m->tlbfault++ */
	ADD	$1, R4
	MOVW	R4, (6*4)(R2)
	MOVW	SPR(SPR_PID), R4
	SRW		$12, R3, R6
	RLWMI	$2, R4, $(0xFF<<2), R3	/* shift and insert PID for match */
	XOR		R3, R6		/* hash=(va>>12)^(pid<<2); (assumes STLBSIZE is 10 to 12 bits) */
	MOVW	(3*4)(R2), R5		/* m->stlb */
	RLWNM	$0, R6, $(STLBSIZE-1), R6	/* mask */
	SLW	$1, R6, R4
	ADD	R4, R6
	SLW	$2, R6	/* index 12-byte entries */
	MOVWZU	(R6+R5), R4	/* fetch Softtlb.hi for comparison; updated address goes to R5 */
	CMP		R4, R3
	BNE		tlbtrap
	MFTB(TBRL, 6)
	MOVW	(4*4)(R2), R4	/* m->utlbhi */
	RLWNM	$0, R6, $(NTLB-1), R6		/* pseudo-random tlb index */
	CMP	R6, R4
	BLE	tlbm1
	SUB	R4, R6
tlbm1:
	RLWNM	$0, R3, $~(BY2PG-1), R3
	OR	$(TLB4K | TLBVALID), R3	/* make valid tlb hi */
	TLBWEHI(3, 6)
	MOVW	4(R5), R4	/* tlb mid */
	TLBWEMD(4, 6)
	MOVW	8(R5), R4	/* tlb lo */
	TLBWELO(4, 6)
	ISYNC
	MOVW	SPR(SPR_SPRG5R), R6
	MOVW	R6, CR
	MOVW	SPR(SPR_SPRG4R), R6
	MOVW	SPR(SPR_SPRG6R), R5
	MOVW	SPR(SPR_SPRG2), R4
	MOVW	SPR(SPR_SPRG1), R3
	MOVW	SPR(SPR_SPRG0), R2
	RFI

tlbtrap:
	MOVW	SPR(SPR_SPRG5R), R6
	MOVW	R6, CR
	MOVW	SPR(SPR_SPRG4R), R6
	MOVW	SPR(SPR_SPRG6R), R5
	MOVW	SPR(SPR_SPRG2), R4
	MOVW	SPR(SPR_SPRG1), R3
	MOVW	SPR(SPR_SPRG0), R2
	MOVW	R0, SPR(SAVER0)
	MOVW	LR, R0
	MOVW	R0, SPR(SAVELR)
	BR	trapcommon

/*
 * following Book E, traps thankfully leave the mmu on.
 * the following code has been executed at the exception
 * vector location already:
 *	MOVW R0, SPR(SAVER0)
 *	MOVW LR, R0
 *	MOVW R0, SPR(SAVELR) 
 *	bl	trapvec(SB)
 */
TEXT	trapvec(SB), $-4
	MOVW	LR, R0
	MOVW	R0, SPR(SAVEXX)			/* save ivoff--interrupt vector offset */
trapcommon:					/* entry point for critical interrupts, machine checks, and faults */
	MOVW	R1, SPR(SAVER1)			/* save stack pointer */
	/* did we come from user space */
	MOVW	SPR(SPR_SRR1), R0
	MOVW	CR, R1
	MOVW	R0, CR
	BC	4,17,ktrap			/* if MSR[PR]=0, we are in kernel space */

	/* switch to kernel stack */
	MOVW	R1, CR

	MOVW	SPR(SPR_SPRG7R), R1		/* up->kstack+KSTACK-UREGSPACE, set in touser and _sysrforkret */

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
	MOVD	R0, SPR(SPR_SRR1)	/* old MSR */
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
TEXT	trapcritvec(SB), $-4
	MOVW	LR, R0
	MOVW	R0, SPR(SAVEXX)
	MOVW	SPR(SPR_CSRR0), R0		/* PC or excepting insn */
	MOVW	R0, SPR(SPR_SRR0)
	MOVD	SPR(SPR_CSRR1), R0		/* old MSR */
	MOVW	R0, SPR(SPR_SRR1)
	BR	trapcommon

/*
 * machine check.
 * make it look like the others.
 */
TEXT	trapmvec(SB), $-4
	MOVW	LR, R0
	MOVW	R0, SPR(SAVEXX)
	MOVW	SPR(SPR_MCSRR0), R0		/* PC or excepting insn */
	MOVW	R0, SPR(SPR_SRR0)
	MOVD	SPR(SPR_MCSRR1), R0		/* old MSR */
	MOVW	R0, SPR(SPR_SRR1)
	BR	trapcommon
	
/*
 * enter with stack set and mapped.
 * on return, SB (R2) has been set, and R3 has the Ureg*,
 * the MMU has been re-enabled, kernel text and PC are in KSEG,
 * Stack (R1), R(MACH) and R(USER) are set by caller, if required.
 */
TEXT	saveureg(SB), $-4
/*
 * save state
 */
	MOVMW	R2, 48(R1)			/* save gprs r2 to r31 */
	MOVD	$setSB(SB), R2
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
	MOVW	SPR(SPR_SPRG7R), R6		/* up->kstack+KSTACK-UREGSPACE */
	MOVW	R6, 20(R1)
	MOVW	SPR(SPR_SRR0), R0
	MOVW	R0, 16(R1)				/* PC of excepting insn (or next insn) */
	MOVW	SPR(SPR_SRR1), R0
	MOVD	R0, 12(R1)				/* old MSR */
	MOVW	SPR(SAVEXX), R0
	MOVW	R0, 8(R1)	/* cause/vector */
	ADD	$8, R1, R3	/* Ureg* */
	STWCCC	R3, (R1)	/* break any pending reservations */
	MOVW	$0, R0	/* compiler/linker expect R0 to be zero */
	RETURN

/*
 * restore state from Ureg and return from trap/interrupt
 */
TEXT _sysrforkret(SB), $-4
	MOVW	R1, 20(R1)	/* up->kstack+KSTACK-UREGSPACE set in ureg */
	BR	restoreureg

/*
 * 4xx specific
 */
TEXT	firmware(SB), $0
	MOVW	$(3<<28), R3
	MOVW	R3, SPR(SPR_DBCR0)	/* system reset */
	BR	0(PC)
