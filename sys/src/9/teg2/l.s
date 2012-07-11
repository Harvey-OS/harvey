/*
 * tegra 2 SoC machine assist
 * dual arm cortex-a9 processors
 *
 * ARM v7 arch. ref. man. §B1.3.3 says that we don't need barriers
 * around writes to CPSR.
 *
 * LDREX/STREX use an exclusive monitor, which is part of the data cache unit
 * for the L1 cache, so they won't work right if the L1 cache is disabled.
 */

#include "arm.s"

#define	LDREX(fp,t)   WORD $(0xe<<28|0x01900f9f | (fp)<<16 | (t)<<12)
/* `The order of operands is from left to right in dataflow order' - asm man */
#define	STREX(f,tp,r) WORD $(0xe<<28|0x01800f90 | (tp)<<16 | (r)<<12 | (f)<<0)

#define MAXMB	(KiB-1)			/* last MB has vectors */
#define TMPSTACK (DRAMSIZE - 64*MiB)	/* used only during cpu startup */
/* tas/cas strex debugging limits; started at 10000 */
#define MAXSC 100000

GLOBL	testmem(SB), $4

/*
 * Entered here from Das U-Boot or another Plan 9 kernel with MMU disabled.
 * Until the MMU is enabled it is OK to call functions provided
 * they are within ±32MiB relative and do not require any
 * local variables or more than one argument (i.e. there is
 * no stack).
 */
TEXT _start(SB), 1, $-4
	CPSMODE(PsrMsvc)
	CPSID					/* interrupts off */
	CPSAE
	SETEND(0)				/* little-endian */
	BARRIERS
	CLREX
	SETZSB

	MOVW	CPSR, R0
	ORR	$PsrDfiq, R0
	MOVW	R0, CPSR

	/* invalidate i-cache and branch-target cache */
	MTCP	CpSC, 0, PC, C(CpCACHE), C(CpCACHEinvi), CpCACHEall
	BARRIERS

	/* put cpus other than 0 to sleep until cpu 0 is ready */
	CPUID(R1)
	BEQ	cpuinit

	/* not cpu 0 */
PUTC('Z')
PUTC('Z')
	BARRIERS
dowfi:
	WFI
	MOVW	cpus_proceed(SB), R1
	CMP	$0, R1
	BEQ	dowfi
	BL	cpureset(SB)
	B	dowfi

cpuinit:
	DELAY(printloopret, 1)
PUTC('\r')
	DELAY(printloopnl, 1)
PUTC('\n')

	DELAY(printloops, 1)
PUTC('P')
	/* disable the PL310 L2 cache on cpu0 */
	MOVW	$(PHYSL2BAG+0x100), R1
	MOVW	$0, R2
	MOVW	R2, (R1)
	BARRIERS
	/* invalidate it */
	MOVW	$((1<<16)-1), R2
	MOVW	R2, 0x77c(R1)
	BARRIERS

	/*
	 * disable my MMU & caches
	 */
	MFCP	CpSC, 0, R1, C(CpCONTROL), C(0), CpMainctl
	ORR	$CpCsbo, R1
	BIC	$(CpCsbz|CpCmmu|CpCdcache|CpCicache|CpCpredict), R1
	MTCP	CpSC, 0, R1, C(CpCONTROL), C(0), CpMainctl
	BARRIERS

	/* cortex-a9 model-specific initial configuration */
	MOVW	$0, R1
	MTCP	CpSC, 0, R1, C(CpCONTROL), C(0), CpAuxctl
	BARRIERS

PUTC('l')
	DELAY(printloop3, 1)

	MOVW	$testmem-KZERO(SB), R0
	BL	memdiag(SB)

PUTC('a')
	/* clear Mach for cpu 0 */
	MOVW	$PADDR(MACHADDR), R4		/* address of Mach for cpu 0 */
	MOVW	$0, R0
_machZ:
	MOVW	R0, (R4)
	ADD	$4, R4
	CMP.S	$PADDR(L1+L1X(0)), R4	/* end at top-level page table */
	BNE	_machZ

	/*
	 * set up the MMU page table for cpu 0
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
	 * trimslice has 1 bank of 1GB at PHYSDRAM.
	 * Map the maximum.
	 */
PUTC('9')
	MOVW	$PTEDRAM, R2			/* PTE bits */
	MOVW	$PHYSDRAM, R3
	MOVW	$PADDR(L1+L1X(KZERO)), R4	/* start with PTE for KZERO */
	MOVW	$MAXMB, R5			/* inner loop count (MBs) */
_ptekrw:					/* set PTEs */
	FILLPTE()
	SUB.S	$1, R5				/* decrement inner loop count */
	BNE	_ptekrw

	/*
	 * back up and fill in PTEs for MMIO
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

	MOVW	$(PHYSDRAM | TMPSTACK), SP

	/*
	 * learn l1 cache characteristics (on cpu 0 only).
	 */

	MOVW	$(1-1), R0			/* l1 */
	SLL	$1, R0				/* R0 = (cache - 1) << 1 */
	MTCP	CpSC, CpIDcssel, R0, C(CpID), C(CpIDid), 0 /* select l1 cache */
	BARRIERS
	MFCP	CpSC, CpIDcsize, R0, C(CpID), C(CpIDid), 0 /* get sets & ways */
	MOVW	$CACHECONF, R8

	/* get log2linelen into l1setsh */
	MOVW	R0, R1
	AND	$3, R1
	ADD	$4, R1
	/* l1 & l2 must have same cache line size, thus same set shift */
	MOVW	R1, 4(R8)		/*  +4 = l1setsh */
	MOVW	R1, 12(R8)		/* +12 = l2setsh */

	/* get nways in R1 */
	SRA	$3, R0, R1
	AND	$((1<<10)-1), R1
	ADD	$1, R1

	/* get log2(nways) in R2 (assume nways is 2^n) */
	MOVW	$(BI2BY*BY2WD - 1), R2
	CLZ(1, 1)
	SUB.S	R1, R2			/* R2 = 31 - clz(nways) */
	ADD.EQ	$1, R2
//	MOVW	R2, R3			/* print log2(nways): 2 */

	MOVW	$32, R1
	SUB	R2, R1			/* R1 = 32 - log2(nways) */
	MOVW	R1, 0(R8)		/* +0 = l1waysh */

	BARRIERS

	MOVW	$testmem-KZERO(SB), R0
	BL	memdiag(SB)

	/*
	 * the mpcore manual says invalidate d-cache, scu, pl310 in that order,
	 * but says nothing about when to disable them.
	 *
	 * invalidate my caches before enabling
	 */
	BL	cachedinv(SB)
	MTCP	CpSC, 0, PC, C(CpCACHE), C(CpCACHEinvi), CpCACHEall
	BARRIERS

PUTC('f')
	/*
	 * the mpcore manual says enable scu, d-cache, pl310, smp mode
	 * in that order.  we have to reverse the last two; see main().
	 */
	BL	scuon(SB)

	/*
	 * turn my L1 cache on; need it for tas below.
	 */
	MFCP	CpSC, 0, R1, C(CpCONTROL), C(0), CpMainctl
	ORR	$(CpCdcache|CpCicache|CpCalign|CpCpredict), R1
	MTCP	CpSC, 0, R1, C(CpCONTROL), C(0), CpMainctl
	BARRIERS

	/* cortex-a9 model-specific configuration */
	MOVW	$CpACl1pref, R1
	MTCP	CpSC, 0, R1, C(CpCONTROL), C(0), CpAuxctl
	BARRIERS

	/* we're supposed to wait until l1 & l2 are on before calling smpon */

PUTC('r')
	/* set the domain access control */
	MOVW	$Client, R0
	BL	dacput(SB)

	DELAY(printloop5, 2)
PUTC('o')
	BL	mmuinvalidate(SB)

	MOVW	$0, R0
	BL	pidput(SB)

	/* set the translation table base */
	MOVW	$PADDR(L1), R0
	BL	ttbput(SB)

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
	 * cpu 0 is now running at KZERO+something!
	 */

	BARRIERS
	MOVW	$setR12(SB), R12		/* reload kernel SB */
	MOVW	$(KZERO | TMPSTACK), SP

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

	BL	cachedwb(SB)
	BL	mmuinvalidate(SB)

	/*
	 * call main in C
	 * pass Mach to main and set up the stack in it
	 */
	MOVW	$MACHADDR, R0			/* cpu 0 Mach */
	MOVW	R0, R(MACH)			/* m = MACHADDR */
	ADD	$(MACHSIZE-4), R0, SP		/* leave space for link register */
PUTC('e')
	BL	main(SB)			/* main(m) */
limbo:
	BL	idlehands(SB)
	B	limbo

	BL	_div(SB)			/* hack to load _div, etc. */


/*
 * called on cpu(s) other than 0, to start them, from _vrst
 * (reset vector) in lexception.s, with interrupts disabled
 * and in SVC mode, running in the zero segment (pc is in lower 256MB).
 * SB is set for the zero segment.
 */
TEXT cpureset(SB), 1, $-4
	CLREX
	MOVW	CPSR, R0
	ORR	$PsrDfiq, R0
	MOVW	R0, CPSR

	MOVW	$(PHYSDRAM | TMPSTACK), SP	/* stack for cache ops */

	/* paranoia: turn my mmu and caches off. */
	MFCP	CpSC, 0, R0, C(CpCONTROL), C(0), CpMainctl
	ORR	$CpCsbo, R0
	BIC	$(CpCsbz|CpCmmu|CpCdcache|CpCicache|CpCpredict), R0
	MTCP	CpSC, 0, R0, C(CpCONTROL), C(0), CpMainctl
	BARRIERS

	/* cortex-a9 model-specific initial configuration */
	MOVW	$0, R1
	MTCP	CpSC, 0, R1, C(CpCONTROL), C(0), CpAuxctl
	ISB

	/* invalidate my caches before enabling */
	BL	cachedinv(SB)
	MTCP	CpSC, 0, PC, C(CpCACHE), C(CpCACHEinvi), CpCACHEall
	BARRIERS

	/*
	 * turn my L1 cache on; need it (and mmu) for tas below.
	 * need branch prediction to make delay() timing right.
	 */
	MFCP	CpSC, 0, R0, C(CpCONTROL), C(0), CpMainctl
	ORR	$(CpCdcache|CpCicache|CpCalign|CpCpredict), R0
	MTCP	CpSC, 0, R0, C(CpCONTROL), C(0), CpMainctl
	BARRIERS

	/* enable l1 caches coherency, at minimum for ldrex/strex. */
	BL	smpon(SB)
	BARRIERS

	/*
	 * we used to write to PHYSEVP here; now we do it in C, which offers
	 * more assurance that we're up and won't go off the rails.
	 */

	/* set the domain access control */
	MOVW	$Client, R0
	BL	dacput(SB)

	BL	setmach(SB)

	/*
	 * redo double map of PHYSDRAM, KZERO in this cpu's ptes.
	 * mmuinit will undo this later.
	 */

	MOVW	$PHYSDRAM, R3
	CMP	$KZERO, R3
	BEQ	noun2map

	/* launchinit set m->mmul1 to a copy of cpu0's l1 page table */
	MOVW	12(R(MACH)), R0		/* m->mmul1 (virtual addr) */
	BL	k2paddr(SB)		/* R0 = PADDR(m->mmul1) */
	ADD	$L1X(PHYSDRAM), R0, R4	/* R4 = address of PHYSDRAM's PTE */

	MOVW	$PTEDRAM, R2		/* PTE bits */
	MOVW	$DOUBLEMAPMBS, R5
_ptrdbl:
	ORR	R3, R2, R1		/* first identity-map 0 to 0, etc. */
	MOVW	R1, (R4)
	ADD	$4, R4			/* bump PTE address */
	ADD	$MiB, R3		/* bump pa */
	SUB.S	$1, R5
	BNE	_ptrdbl
noun2map:

	MOVW	$0, R0
	BL	pidput(SB)

	/* set the translation table base to PADDR(m->mmul1) */
	MOVW	12(R(MACH)), R0		/* m->mmul1 */
	BL	k2paddr(SB)		/* R0 = PADDR(m->mmul1) */
	BL	ttbput(SB)

	/*
	 * the little dance to turn the MMU on
	 */
	BL	cacheuwbinv(SB)
	BL	mmuinvalidate(SB)
	BL	mmuenable(SB)

	/*
	 * mmu is now on, with l1 pt at m->mmul1.
	 */

	/* warp the PC into the virtual map */
	MOVW	$KZERO, R0
	BL	_r15warp(SB)

	/*
	 * now running at KZERO+something!
	 */

	BARRIERS
	MOVW	$setR12(SB), R12	/* reload kernel's SB */
	MOVW	$(KZERO | TMPSTACK), SP	/* stack for cache ops*/
	BL	setmach(SB)
	ADD	$(MACHSIZE-4), R(MACH), SP /* leave space for link register */
	BL	cpustart(SB)


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

/*
 * converts address in R0 to the current segment, as defined by the PC.
 * clobbers R1.
 */
TEXT addr2pcseg(SB), 1, $-4
	BIC	$KSEGM, R0
	MOVW	PC, R1
	AND	$KSEGM, R1		/* segment PC is in */
	ORR	R1, R0
	RET

/* sets R(MACH), preserves other registers */
TEXT setmach(SB), 1, $-4
	MOVM.DB.W [R14], (R13)
	MOVM.DB.W [R0-R2], (R13)

	CPUID(R2)
	SLL	$2, R2			/* convert to word index */

	MOVW	$machaddr(SB), R0
	BL	addr2pcseg(SB)
	ADD	R2, R0			/* R0 = &machaddr[cpuid] */
	MOVW	(R0), R0		/* R0 = machaddr[cpuid] */
	CMP	$0, R0
	MOVW.EQ	$MACHADDR, R0		/* paranoia: use MACHADDR if 0 */
	BL	addr2pcseg(SB)
	MOVW	R0, R(MACH)		/* m = machaddr[cpuid] */

	MOVM.IA.W (R13), [R0-R2]
	MOVM.IA.W (R13), [R14]
	RET


/*
 * memory diagnostic
 * tests word at (R0); modifies R7 and R8
 */
TEXT memdiag(SB), 1, $-4
	MOVW	$0xabcdef89, R7
	MOVW	R7, (R0)
	MOVW	(R0), R8
	CMP	R7, R8
	BNE	mbuggery		/* broken memory */

	BARRIERS
	MOVW	(R0), R8
	CMP	R7, R8
	BNE	mbuggery		/* broken memory */

	MOVW	$0, R7
	MOVW	R7, (R0)
	BARRIERS
	RET

/* modifies R0, R3—R6 */
TEXT printhex(SB), 1, $-4
	MOVW	R0, R3
	PUTC('0')
	PUTC('x')
	MOVW	$(32-4), R5	/* bits to shift right */
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

mbuggery:
	PUTC('?')
	PUTC('m')
mtopanic:
	MOVW	$membmsg(SB), R0
	MOVW	R14, R1		/* get R14's segment ... */
	AND	$KSEGM, R1
	BIC	$KSEGM,	R0	/* strip segment from address */
	ORR	R1, R0		/* combine them */
	BL	panic(SB)
mbugloop:
	WFI
	B	mbugloop

	DATA	membmsg+0(SB)/8,$"memory b"
	DATA	membmsg+8(SB)/6,$"roken\z"
	GLOBL	membmsg(SB), $14

TEXT _r15warp(SB), 1, $-4
	BIC	$KSEGM, R14			/* link reg, will become PC */
	ORR	R0, R14
	BIC	$KSEGM, SP
	ORR	R0, SP
	RET

/*
 * `single-element' cache operations.
 * in arm arch v7, they operate on all architected cache levels, so separate
 * l2 functions are usually unnecessary.
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
	MTCP	CpSC, 0, R0, C(CpCACHE), C(CpCACHEwb), CpCACHEse
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
	MTCP	CpSC, 0, R0, C(CpCACHE), C(CpCACHEwbi), CpCACHEse
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

	/*
	 * if start & end addresses are not on cache-line boundaries,
	 * flush first & last cache lines before invalidating.
	 */
	AND.S	$(CACHELINESZ-1), R0, R4
	BEQ	stok
	BIC	$(CACHELINESZ-1), R0, R4	/* cache line start */
	MTCP	CpSC, 0, R4, C(CpCACHE), C(CpCACHEwb), CpCACHEse
stok:
	AND.S	$(CACHELINESZ-1), R1, R4
	BEQ	endok
	BIC	$(CACHELINESZ-1), R1, R4	/* cache line start */
	MTCP	CpSC, 0, R4, C(CpCACHE), C(CpCACHEwb), CpCACHEse
endok:
	BIC	$(CACHELINESZ-1), R0		/* cache line start */
_dinvse:
	MTCP	CpSC, 0, R0, C(CpCACHE), C(CpCACHEinvd), CpCACHEse
	ADD	$CACHELINESZ, R0
	CMP.S	R0, R1
	BGT	_dinvse
	B	_wait

/*
 *  enable mmu and high vectors
 */
TEXT mmuenable(SB), 1, $-4
	MFCP	CpSC, 0, R0, C(CpCONTROL), C(0), CpMainctl
	ORR	$CpCmmu, R0
	MTCP	CpSC, 0, R0, C(CpCONTROL), C(0), CpMainctl
	BARRIERS
	RET

TEXT mmudisable(SB), 1, $-4
	MFCP	CpSC, 0, R0, C(CpCONTROL), C(0), CpMainctl
	BIC	$CpCmmu, R0
	MTCP	CpSC, 0, R0, C(CpCONTROL), C(0), CpMainctl
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
	MTCP	CpSC, 0, PC, C(CpTLB), C(CpTLBinvu), CpTLBinv
	BARRIERS
	MOVW	R2, CPSR			/* interrupts restored */
	RET

TEXT mmuinvalidateaddr(SB), $-4			/* invalidate single entry */
	MTCP	CpSC, 0, R0, C(CpTLB), C(CpTLBinvu), CpTLBinvse
	BARRIERS
	RET

TEXT cpidget(SB), 1, $-4			/* main ID */
	MFCP	CpSC, 0, R0, C(CpID), C(CpIDidct), CpIDid
	RET

TEXT cpctget(SB), 1, $-4			/* cache type */
	MFCP	CpSC, 0, R0, C(CpID), C(CpIDidct), CpIDct
	RET

TEXT controlget(SB), 1, $-4			/* system control (sctlr) */
	MFCP	CpSC, 0, R0, C(CpCONTROL), C(0), CpMainctl
	RET

TEXT ttbget(SB), 1, $-4				/* translation table base */
	MFCP	CpSC, 0, R0, C(CpTTB), C(0), CpTTB0
	RET

TEXT ttbput(SB), 1, $-4				/* translation table base */
	MOVW	CPSR, R2
	CPSID
	MOVW	R0, R1
	BARRIERS		/* finish prior accesses before changing ttb */
	MTCP	CpSC, 0, R1, C(CpTTB), C(0), CpTTB0
	MTCP	CpSC, 0, R1, C(CpTTB), C(0), CpTTB1	/* non-secure too */
	MOVW	$0, R0
	MTCP	CpSC, 0, R0, C(CpTTB), C(0), CpTTBctl
	BARRIERS
	MOVW	R2, CPSR
	RET

TEXT dacget(SB), 1, $-4				/* domain access control */
	MFCP	CpSC, 0, R0, C(CpDAC), C(0)
	RET

TEXT dacput(SB), 1, $-4				/* domain access control */
	MOVW	R0, R1
	BARRIERS
	MTCP	CpSC, 0, R1, C(CpDAC), C(0)
	ISB
	RET

TEXT fsrget(SB), 1, $-4				/* fault status */
	MFCP	CpSC, 0, R0, C(CpFSR), C(0), CpDFSR
	RET

TEXT farget(SB), 1, $-4				/* fault address */
	MFCP	CpSC, 0, R0, C(CpFAR), C(0), CpDFAR
	RET

TEXT getpsr(SB), 1, $-4
	MOVW	CPSR, R0
	RET

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
	BARRIERS
	RET

TEXT getclvlid(SB), 1, $-4
	MFCP	CpSC, CpIDcsize, R0, C(CpID), C(CpIDidct), CpIDclvlid
	RET

TEXT getcyc(SB), 1, $-4
	MFCP	CpSC, 0, R0, C(CpCLD), C(CpCLDcyc), 0
	RET

TEXT getdebug(SB), 1, $-4		/* get cortex-a9 debug enable register */
	MFCP	CpSC, 0, R0, C(1), C(1), 1
	RET

TEXT getpc(SB), 1, $-4
	MOVW	PC, R0
	RET

TEXT getsb(SB), 1, $-4
	MOVW	R12, R0
	RET

TEXT setsp(SB), 1, $-4
	MOVW	R0, SP
	RET


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
	MOVW	CPSR, R3			/* must return old CPSR */
	CPSID

	CMP.S	$0, R(MACH)
	MOVW.NE	R14, 4(R(MACH))			/* save caller pc in m->splpc */
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
	MOVW	CPSR, R1
	/*
	 * an interrupt should break us out of wfi.  masking interrupts
	 * slows interrupt response slightly but prevents recursion.
	 */
//	CPSIE
	CPSID

	BARRIERS
	WFI

	MOVW	R1, CPSR
	RET

TEXT coherence(SB), $-4
	BARRIERS
	RET

GLOBL cpus_proceed+0(SB), $4

#include "cache.v7.s"

TEXT	tas(SB), $-4			/* _tas(ulong *) */
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
	PUTC('?')
	PUTC('l')
	PUTC('t')
	BL	abort(SB)
lockbusy:
	CLREX
tas0:
	MOVW	R7, R0		/* return old value */
	RET
