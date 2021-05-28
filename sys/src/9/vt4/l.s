/* virtex4 ppc405 machine assist */
#include	"mem.h"

/*
 * 405 Special Purpose Registers of interest here
 */
#define SPR_CCR0	947		/* Core Configuration Register 0 */
#define SPR_DAC1	1014		/* Data Address Compare 1 */
#define SPR_DAC2	1015		/* Data Address Compare 2 */
#define SPR_DBCR0	1010		/* Debug Control Register 0 */
#define SPR_DBCR1	957		/* Debug Control Register 1 */
#define SPR_DBSR	1008		/* Debug Status Register */
#define SPR_DCCR	1018		/* Data Cache Cachability Register */
#define SPR_DCWR	954		/* Data Cache Write-through Register */
#define SPR_DVC1	950		/* Data Value Compare 1 */
#define SPR_DVC2	951		/* Data Value Compare 2 */
#define SPR_DEAR	981		/* Data Error Address Register */
#define SPR_ESR		980		/* Exception Syndrome Register */
#define SPR_EVPR	982		/* Exception Vector Prefix Register */
#define SPR_IAC1	1012		/* Instruction Address Compare 1 */
#define SPR_IAC2	1013		/* Instruction Address Compare 2 */
#define SPR_IAC3	948		/* Instruction Address Compare 3 */
#define SPR_IAC4	949		/* Instruction Address Compare 4 */
#define SPR_ICCR	1019		/* Instruction Cache Cachability Register */
#define SPR_ICDBDR	979		/* Instruction Cache Debug Data Register */
#define SPR_PID		945		/* Process ID */
#define SPR_PIT		987		/* Programmable Interval Timer */
#define SPR_PVR		287		/* Processor Version Register */
#define SPR_SGR		953		/* Store Guarded Register */
#define SPR_SLER	955		/* Storage Little Endian Register */
#define SPR_SPRG0	272		/* SPR General 0 */
#define SPR_SPRG1	273		/* SPR General 1 */
#define SPR_SPRG2	274		/* SPR General 2 */
#define SPR_SPRG3	275		/* SPR General 3 */

#define SPR_USPRG0	256		/* user SPR G0 */

/* beware that these registers differ in R/W ability on 440 compared to 405 */
#define SPR_SPRG4R		SPR_SPRG4W	/* SPR general 4 supervisor R*/
#define SPR_SPRG5R		SPR_SPRG5W	/* SPR general 5; supervisor R */
#define SPR_SPRG6R		SPR_SPRG6W	/* SPR general 6; supervisor R */
#define SPR_SPRG7R		SPR_SPRG7W	/* SPR general 7; supervisor R */
#define SPR_SPRG4W	0x114		/* SPR General 4; supervisor R/W */
#define SPR_SPRG5W	0x115		/* SPR General 5; supervisor R/W  */
#define SPR_SPRG6W	0x116		/* SPR General 6; supervisor R/W  */
#define SPR_SPRG7W	0x117		/* SPR General 7; supervisor R/W */

#define SPR_SRR0	26		/* Save/Restore Register 0 */
#define SPR_SRR1	27		/* Save/Restore Register 1 */
#define SPR_SRR2	990		/* Save/Restore Register 2 */
#define SPR_SRR3	991		/* Save/Restore Register 3 */
#define SPR_SU0R	956		/* Storage User-defined 0 Register */
#define SPR_TBL		284		/* Time Base Lower */
#define SPR_TBU		85		/* Time Base Upper */
#define SPR_TCR		986		/* Time Control Register */
#define SPR_TSR		984		/* Time Status Register */
#define SPR_ZPR		944		/* Zone Protection Register */

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
/* #define	MSYNC		WORD	$((31<<26)|(598<<1))   /* in the 405? */
#define	TLBRELO(a,t)	WORD	$((31<<26)|((t)<<21)|((a)<<16)|(1<<11)|(946<<1))
#define	TLBREHI(a,t)	WORD	$((31<<26)|((t)<<21)|((a)<<16)|(0<<11)|(946<<1))
#define	TLBWELO(s,a)	WORD	$((31<<26)|((s)<<21)|((a)<<16)|(1<<11)|(978<<1))
#define	TLBWEHI(s,a)	WORD	$((31<<26)|((s)<<21)|((a)<<16)|(0<<11)|(978<<1))
#define	TLBSXF(a,b,t)	WORD	$((31<<26)|((t)<<21)|((a)<<16)|((b)<<11)|(914<<1))
#define	TLBSXCC(a,b,t)	WORD	$((31<<26)|((t)<<21)|((a)<<16)|((b)<<11)|(914<<1)|1)
#define	WRTMSR_EE(s)	WORD	$((31<<26)|((s)<<21)|(131<<1)); MSRSYNC
#define	WRTMSR_EEI(e)	WORD	$((31<<26)|((e)<<15)|(163<<1)); MSRSYNC

/*
 * there are three flavours of barrier: MBAR, MSYNC and ISYNC.
 * ISYNC is a context sync, a strong instruction barrier.
 * MSYNC is an execution sync (weak instruction barrier) + data storage barrier.
 * MBAR is a memory (data storage) barrier.
 */
#define MBAR	EIEIO
/* on some models mtmsr doesn't synchronise enough (eg, 603e) */
#define	MSRSYNC	SYNC; ISYNC

/*
 * on the 400 series, the prefetcher madly fetches across RFI, sys call,
 * and others; use BR 0(PC) to stop it.
 */
#define	RFI	WORD $((19<<26)|(50<<1)); BR 0(PC)
#define	RFCI	WORD $((19<<26)|(51<<1)); BR 0(PC)

#define ORI(imm, reg)	WORD $((24<<26) | (reg)<<21 | (reg)<<16 | (imm))
#define ORIS(imm, reg)	WORD $((25<<26) | (reg)<<21 | (reg)<<16 | (imm))

/* print progress character.  steps on R7 and R8, needs SB set. */
#define PROG(c)	MOVW $(Uartlite+4), R7; MOVW $(c), R8; MOVW R8, 0(R7)

#define	UREGSPACE	(UREGSIZE+8)

	NOSCHED

	TEXT start(SB), 1, $-4

	/*
	 * our bootstrap may have already turned on the MMU.
	 * setup MSR
	 * turn off interrupts, FPU & MMU
	 * use 0x000 as exception prefix
	 * don't enable machine check until the vector is set up
	 */
	MOVW	MSR, R3
	RLWNM	$0, R3, $~MSR_EE, R3
	RLWNM	$0, R3, $~MSR_FP, R3
	RLWNM	$0, R3, $~(MSR_IR|MSR_DR), R3
	RLWNM	$0, R3, $~MSR_ME, R3
	ISYNC
	MOVW	R3, MSR
	MSRSYNC

	/* except during trap handling, R0 is zero from now on */
	MOVW	$0, R0
	MOVW	R0, CR

	/* setup SB for pre mmu */
	MOVW	$setSB-KZERO(SB), R2	/* SB until mmu on */

PROG('\r')
PROG('\n')
PROG('P')

	MOVW	$18, R18
	MOVW	$19, R19
	MOVW	$20, R20
	MOVW	$21, R21
	MOVW	$22, R22
	MOVW	$23, R23
	MOVW	$24, R24

	/*
	 * reset the caches and disable them until mmu on
	 */
	MOVW	R0, SPR(SPR_ICCR)
	ICCCI(0, 2)  /* errata cpu_121 reveals that EA is used; we'll use SB */
	ISYNC
	DCCCI(0, 0)
	MSRSYNC

	MOVW	$((DCACHEWAYSIZE/DCACHELINESZ)-1), R3
	MOVW	R3, CTR
	MOVW	R0, R3
dcinv:
	DCCCI(0,3)
	ADD	$32, R3
	BDNZ	dcinv

	/*
	 * cache is write-back; no user-defined 0; big endian throughout.
	 * write-through cache would require putting ~0 into R3.
	 */
	MOVW	R0, SPR(SPR_DCWR)	/* write-through nowhere: write-back */

	/* starting from the high bit, each bit represents 128MB */
	MOVW	R0, R3			/* region bits */
	MOVW	R3, SPR(SPR_DCCR)	/* caches off briefly */
	MOVW	R3, SPR(SPR_ICCR)
	ISYNC
	MOVW	R0, SPR(SPR_SU0R)
	MOVW	R0, SPR(SPR_SLER)
	ISYNC

	/*
	 * CCR0:
	 *	1<<25 LWL load word as line
	 *	1<<11 PFC prefetching for cacheable regions
	 */
	MOVW	SPR(SPR_CCR0), R4
	OR	$((1<<25)|(1<<11)), R4
	MOVW	R4, SPR(SPR_CCR0)

	/* R3 still has region bits */
	NOR	R3, R3		/* no speculative access in uncached mem */
	MOVW	R3, SPR(SPR_SGR)
	ISYNC

	/*
	 * set other system configuration values
	 */
	MOVW	R0, SPR(SPR_PIT)
	MOVW	$~0, R3
	MOVW	R3, SPR(SPR_TSR)

PROG('l')
	BL	kernelmmu(SB)
	/* now running with MMU on */

	/* set R2 to correct value */
	MOVW	$setSB(SB), R2

	/*
	 * now running with MMU in kernel address space
	 * invalidate the caches again to flush any addresses
	 * below KZERO
	 */
	ICCCI(0, 2)  /* errata cpu_121 reveals that EA is used; we'll use SB */
	ISYNC

PROG('a')

	/*
	 * config caches for kernel in real mode; data is write-through.
	 * cache bottom 128MB (dram) & top 128MB (sram), but not I/O reg.s.
	 */
	MOVW	$((1<<31) | (1<<0)), R3
	MOVW	R3, SPR(SPR_DCCR)
	MOVW	R3, SPR(SPR_ICCR)
	ISYNC
	/* R3 still has region bits */
	NOR	R3, R3		/* no speculative access in uncached mem */
	MOVW	R3, SPR(SPR_SGR)
	ISYNC

	/* no kfpinit on 4xx */

	MOVW	dverify(SB), R3
	MOVW	$0x01020304, R4
	CMP	R3, R4
	BEQ	dataok

PROG('?')
	/* seriously bad news, punt to vector 0x1500 (unused) */
	MOVW	$(PHYSSRAM + 0x1500), R3
	BL	0(R3)

dataok:
	/* set up Mach */
	MOVW	$mach0(SB), R(MACH)
	ADD	$(MACHSIZE-8), R(MACH), R1	/* set stack */
/*
 * mach0 is in bss, so this loop is redundant
	SUB	$4, R(MACH), R3
	ADD	$4, R1, R4
clrmach:
	MOVWU	R0, 4(R3)
	CMP	R3, R4
	BNE	clrmach
 */

PROG('n')
	MOVW	$edata-4(SB), R3
	MOVW	$end(SB), R4
clrbss:
	MOVWU	R0, 4(R3)
	CMP	R3, R4
	BNE	clrbss

	MOVW	R0, R(USER)
	MOVW	R0, 0(R(MACH))

PROG(' ')
PROG('9')
PROG('\r')
PROG('\n')
	BL	main(SB)
	BR	0(PC)   /* paranoia -- not reached */

GLOBL	mach0(SB), $(MAXMACH*BY2PG)

TEXT	kernelmmu(SB), 1, $-4
	TLBIA
	ISYNC
	SYNC

	/* make following TLB entries shared, TID=PID=0 */
	MOVW	R0, SPR(SPR_PID)
	ISYNC

	/* zone 0 is superviser-only; zone 1 is user and supervisor; all access controlled by TLB */
	MOVW	$((0<<30)|(1<<28)), R5
	MOVW	R5, SPR(SPR_ZPR)

	/* map various things 1:1 */
	MOVW	$tlbtab-KZERO(SB), R4
	MOVW	$tlbtabe-KZERO(SB), R5
	SUB	R4, R5
	/* size in bytes is now in R5 */
	MOVW	$(2*4), R6
	DIVW	R6, R5
	/* Number of TLBs is now in R5 */
	SUB	$4, R4
	MOVW	R5, CTR
	/* at start of this loop, # TLBs in CTR, R3 is tlb index 0, R4 is pointing at
	  * tlbstart[-1] (for pre-increment below)
	  */
	/* last thing to do: use 63 as index as we put kernel TLBs at top */
	MOVW	$63, R3
ltlb:
	MOVWU	4(R4), R5	/* TLBHI */
	TLBWEHI(5,3)
	MOVWU	4(R4), R5	/* TLBLO */
	TLBWELO(5,3)
	SUB	$1, R3
	BDNZ	ltlb

	/* enable MMU */
	MOVW	LR, R3
	OR	$KZERO, R3
	MOVW	R3, SPR(SPR_SRR0)
	MOVW	MSR, R4
	OR	$(MSR_IR|MSR_DR), R4
	MOVW	R4, SPR(SPR_SRR1)
/*	ISYNC	/* no ISYNC here as we have no TLB entry for the PC (without KZERO) */
	SYNC		/* fix 405 errata cpu_210 */
	RFI		/* resume in kernel mode in caller */

TEXT	splhi(SB), 1, $-4
	MOVW	MSR, R3
	WRTMSR_EEI(0)
	MOVW	LR, R31
	MOVW	R31, 4(R(MACH))	/* save PC in m->splpc */
	RETURN

/*
 * everything from here up to, but excluding, spldone
 * will be billed by devkprof to the pc saved when we went splhi.
 */
TEXT	spllo(SB), 1, $-4
	MOVW	MSR, R3
	WRTMSR_EEI(1)
	RETURN

TEXT	splx(SB), 1, $-4
	MOVW	LR, R31
	MOVW	R31, 4(R(MACH))	/* save PC in m->splpc */
	/* fall though */

TEXT	splxpc(SB), 1, $-4
	WRTMSR_EE(3)
	RETURN

/***/
TEXT	spldone(SB), 1, $-4
	RETURN

TEXT	islo(SB), 1, $-4
	MOVW	MSR, R3
	RLWNM	$0, R3, $MSR_EE, R3
	RETURN

TEXT	setlabel(SB), 1, $-4
	MOVW	LR, R31
	MOVW	R1, 0(R3)
	MOVW	R31, 4(R3)
	MOVW	$0, R3
	RETURN

TEXT	gotolabel(SB), 1, $-4
	MOVW	4(R3), R31
	MOVW	R31, LR
	MOVW	0(R3), R1
	MOVW	$1, R3
	RETURN

TEXT	touser(SB), 1, $-4
	WRTMSR_EEI(0)
	MOVW	R(USER), R4			/* up */
	MOVW	8(R4), R4			/* up->kstack */
	RLWNM	$0, R4, $~KZERO, R4		/* PADDR(up->kstack) */
	ADD	$(KSTACK-UREGSPACE), R4
	MOVW	R4, SPR(SPR_SPRG7W)	/* save for use in traps/interrupts */
	MOVW	$(UTZERO+32), R5	/* header appears in text */
	MOVW	$UMSR, R4
	MOVW	R4, SPR(SPR_SRR1)
	MOVW	R3, R1
	MOVW	R5, SPR(SPR_SRR0)
	ISYNC
	SYNC				/* fix 405 errata cpu_210 */
	RFI

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

TEXT	dcflush(SB), 1, $-4	/* dcflush(virtaddr, count) */
	MOVW	n+4(FP), R4
	RLWNM	$0, R3, $~(DCACHELINESZ-1), R5
	CMP	R4, $0
	BLE	dcf1
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

TEXT	tas32(SB), 1, $0
	SYNC
	MOVW	R3, R4
	MOVW	$0xdead,R5
tas1:
	DCBF	(R4)	/* fix for 603x bug */
	LWAR	(R4), R3
	CMP	R3, $0
	BNE	tas0
	DCBT	(R4)			/* fix 405 errata cpu_210 */
	STWCCC	R5, (R4)
	BNE	tas1
tas0:
	SYNC
	ISYNC
	RETURN

TEXT _xinc(SB), 1, $-4			/* long _xinc(long*); */
TEXT ainc(SB), 1, $-4			/* long ainc(long*); */
	MOVW	R3, R4
_ainc:
	DCBF	(R4)			/* fix for 603x bug */
	LWAR	(R4), R3
	ADD	$1, R3
	DCBT	(R4)			/* fix 405 errata cpu_210 */
	STWCCC	R3, (R4)
	BNE	_ainc

	CMP	R3, $0			/* overflow if -ve or 0 */
	BGT	_return
_trap:
	MOVW	$0, R0
	MOVW	(R0), R0		/* over under sideways down */
_return:
	RETURN

TEXT _xdec(SB), 1, $-4			/* long _xdec(long*); */
TEXT adec(SB), 1, $-4			/* long adec(long*); */
	MOVW	R3, R4
_adec:
	DCBF	(R4)			/* fix for 603x bug */
	LWAR	(R4), R3
	ADD	$-1, R3
	DCBT	(R4)			/* fix 405 errata cpu_210 */
	STWCCC	R3, (R4)
	BNE	_adec

	CMP	R3, $0			/* underflow if -ve */
	BLT	_trap

	RETURN

TEXT cas32(SB), 1, $0			/* int cas32(void*, u32int, u32int) */
	MOVW	R3, R4			/* addr */
	MOVW	old+4(FP), R5
	MOVW	new+8(FP), R6
	DCBF	(R4)			/* fix for 603x bug? */
	LWAR	(R4), R3
	CMP	R3, R5
	BNE	 fail
	DCBT	(R4)			/* fix 405 errata cpu_210 */
	STWCCC	R6, (R4)
	BNE	 fail
	MOVW	 $1, R3
	RETURN
fail:
	MOVW	 $0, R3
	RETURN

TEXT	getpit(SB), 1, $0
	MOVW	SPR(SPR_PIT), R3
	RETURN

TEXT	putpit(SB), 1, $0
	MOVW	R3, SPR(SPR_PIT)
	RETURN

TEXT	putpid(SB), 1, $0
	ISYNC
	MOVW	R3, SPR(SPR_PID)
	ISYNC
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

TEXT	putsr(SB), 1, $0
	MOVW	4(FP), R4
	MOVW	R4, SEG(R3)
	RETURN

TEXT	getesr(SB), 1, $0
	MOVW	SPR(SPR_ESR), R3
	RETURN

TEXT	putesr(SB), 1, $0
	MOVW	R3, SPR(SPR_ESR)
	ISYNC
	RETURN

TEXT	putevpr(SB), 1, $0
	MOVW	R3, SPR(SPR_EVPR)
	ISYNC
	RETURN

TEXT	getccr0(SB), 1, $-4
	MOVW	SPR(SPR_CCR0), R3
	RETURN

TEXT	getdear(SB), 1, $0
	MOVW	SPR(SPR_DEAR), R3
	RETURN

TEXT	mbar(SB), 1, $-4
TEXT	eieio(SB), 1, $-4
	MBAR
	RETURN

TEXT	barriers(SB), 1, $-4
TEXT	sync(SB), 1, $-4
	SYNC
	RETURN

TEXT	isync(SB), 1, $-4
	ISYNC
	RETURN

TEXT	tlbwrx(SB), 1, $-4
	MOVW	hi+4(FP), R5
	MOVW	lo+8(FP), R6
	SYNC
	TLBWEHI(5, 3)
	TLBWELO(6, 3)
	ISYNC
	SYNC			/* paranoia; inferno cerf405 port does this */
	RETURN

TEXT	tlbrehi(SB), 1, $-4
	TLBREHI(3, 3)
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

TEXT	dtlbmiss(SB), 1, $-4
	MOVW	R3, SPR(SPR_SPRG1)
	MOVW	$INT_DMISS, R3
	MOVW	R3, SPR(SAVEXX)	/* value for cause if entry not in soft tlb */
	MOVW	SPR(SPR_DEAR), R3
	BR	tlbmiss

TEXT	itlbmiss(SB), 1, $-4
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
	MOVW	$setSB-KZERO(SB), R2
	MOVW	$mach0-KZERO(SB), R2
	MOVW	(6*4)(R2), R4	/* m->tlbfault++ */
	ADD	$1, R4
	MOVW	R4, (6*4)(R2)
	MOVW	SPR(SPR_PID), R4
	SRW	$12, R3, R6
	RLWMI	$2, R4, $(0xFF<<2), R3	/* shift and insert PID for match */
	XOR	R3, R6		/* hash=(va>>12)^(pid<<2); (assumes STLBSIZE is 10 to 12 bits) */
	MOVW	(3*4)(R2), R5		/* m->pstlb == PADDR(m->stlb) */
	RLWNM	$3, R6, $((STLBSIZE-1)<<3), R6	/* shift to index 8-byte entries, and mask */
	MOVWU	(R6+R5), R4	/* fetch Softtlb.hi for comparison; updated address goes to R5 */
	CMP	R4, R3
	BNE	tlbtrap
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
	MOVW	4(R5), R4	/* tlb lo; R3 is high */
	TLBWELO(4, 6)
	ISYNC
	MOVW	SPR(SPR_SPRG5R), R6
	MOVW	R6, CR
	MOVW	SPR(SPR_SPRG4R), R6
	MOVW	SPR(SPR_SPRG6R), R5
	MOVW	SPR(SPR_SPRG2), R4
	MOVW	SPR(SPR_SPRG1), R3
	MOVW	SPR(SPR_SPRG0), R2

	ISYNC
	SYNC			/* fixes 405 errata cpu_210 */
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
 * traps force memory mapping off.
 * the following code has been executed at the exception
 * vector location already:
 *	MOVW R0, SPR(SAVER0)
 *	MOVW LR, R0
 *	MOVW R0, SPR(SAVELR)
 *	bl	trapvec(SB)
 */
TEXT	trapvec(SB), 1, $-4
	MOVW	LR, R0
	RLWNM	$0, R0, $~0x1F, R0		/* mask LR increment to get ivoff */
	MOVW	R0, SPR(SAVEXX)			/* save ivoff--interrupt vector offset */
trapcommon:					/* entry point for critical interrupts */

	MOVW	R1, SPR(SAVER1)			/* save stack pointer */
	/* did we come from user space */
	MOVW	SPR(SPR_SRR1), R0
	MOVW	CR, R1
	MOVW	R0, CR
	BC	4,17,ktrap			/* if MSR[PR]=0, we are in kernel space */

	/* switch to kernel stack */
	MOVW	R1, CR
	MOVW	SPR(SPR_SPRG7R), R1		/* PADDR(up->kstack+KSTACK-UREGSPACE), set in touser and sysrforkret */
	BL	saveureg(SB)
	MOVW	$mach0(SB), R(MACH)
	MOVW	8(R(MACH)), R(USER)
	BL	trap(SB)
	BR	restoreureg

ktrap:
	MOVW	R1, CR
	MOVW	SPR(SAVER1), R1
	RLWNM	$0, R1, $~KZERO, R1		/* PADDR(R1) */
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
	MOVW	R0, SPR(SPR_SPRG7W)	/* PADDR(up->kstack etc.) for traps from user space */
	MOVW	16(R1), R0
	MOVW	R0, SPR(SPR_SRR0)	/* old PC */
	MOVW	12(R1), R0
	RLWNM	$0, R0, $~MSR_WE, R0	/* remove wait state */
	MOVW	R0, SPR(SPR_SRR1)	/* old MSR */
	/* cause, skip */
	MOVW	44(R1), R1	/* old SP */
	MOVW	SPR(SAVER0), R0
	ISYNC
	SYNC				/* fixes 405 errata cpu_210 */
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
	MOVW	SPR(SPR_SRR2), R0		/* PC or excepting insn */
	MOVW	R0, SPR(SPR_SRR0)
	MOVW	SPR(SPR_SRR3), R0		/* old MSR */
	MOVW	R0, SPR(SPR_SRR1)
	BR	trapcommon

/*
 * enter with stack set and mapped.
 * on return, R0 is zero, SB (R2) has been set, and R3 has the Ureg*,
 * the MMU has been re-enabled, kernel text and PC are in KSEG,
 * Stack (R1), R(MACH) and R(USER) are set by caller, if required.
 */
TEXT	saveureg(SB), 1, $-4
/*
 * save state
 */
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
	MOVW	SPR(SAVELR), R6	/* LR */
	MOVW	R6, 24(R1)
	MOVW	SPR(SPR_SPRG7R), R6		/* PADDR(up->kstack+KSTACK-UREGSPACE) */
	MOVW	R6, 20(R1)
	MOVW	SPR(SPR_SRR0), R0
	MOVW	R0, 16(R1)			/* PC of excepting insn (or next insn) */
	MOVW	SPR(SPR_SRR1), R0
	MOVW	R0, 12(R1)			/* old MSR */
	MOVW	SPR(SAVEXX), R0
	MOVW	R0, 8(R1)	/* cause/vector */
	ADD	$8, R1, R3	/* Ureg* */
	OR	$KZERO, R3	/* fix ureg */
	DCBT	(R1)		/* fix 405 errata cpu_210 */
	STWCCC	R3, (R1)	/* break any pending reservations */
	MOVW	$0, R0	/* compiler/linker expect R0 to be zero */

	MOVW	MSR, R5
	OR	$(MSR_IR|MSR_DR), R5	/* enable MMU */
	MOVW	R5, SPR(SPR_SRR1)
	MOVW	LR, R31
	OR	$KZERO, R31	/* return PC in KSEG0 */
	MOVW	R31, SPR(SPR_SRR0)
	OR	$KZERO, R1	/* fix stack pointer */
/*	ISYNC			/* no ISYNC here either */
	SYNC			/* fix 405 errata cpu_210 */
	RFI	/* returns to trap handler */

/*
 * restore state from Ureg and return from trap/interrupt
 */
TEXT forkret(SB), 1, $-4
	RLWNM	$0, R1, $~KZERO, R2	/* PADDR(up->kstack+KSTACK-UREGSPACE) */
	MOVW	R2, 20(R1)	/* set in ureg */
	BR	restoreureg

/*
 * 4xx specific
 */
TEXT	firmware(SB), 1, $0
	MOVW	$(3<<28), R3
	MOVW	R3, SPR(SPR_DBCR0)	/* system reset */
	BR	0(PC)
