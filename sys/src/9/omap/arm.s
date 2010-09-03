/*
 * omap3530 machine assist, definitions
 * cortex-a8 processor
 *
 * loader uses R11 as scratch.
 */

#include "mem.h"
#include "arm.h"

#undef B					/* B is for 'botch' */

#define KADDR(pa)	(KZERO    | ((pa) & ~KSEGM))
#define PADDR(va)	(PHYSDRAM | ((va) & ~KSEGM))

#define L1X(va)		(((((va))>>20) & 0x0fff)<<2)

#define MACHADDR	(L1-MACHSIZE)

#define PTEDRAM		(Dom0|L1AP(Krw)|Section|Cached|Buffered)
#define PTEIO		(Dom0|L1AP(Krw)|Section)

#define DOUBLEMAPMBS	256	/* megabytes of low dram to double-map */

/* steps on R0 */
#define DELAY(label, mloops) \
	MOVW	$((mloops)*1000000), R0; \
label: \
	SUB.S	$1, R0; \
	BNE	label

/* wave at the user; clobbers R0, R1 & R6; needs R12 (SB) set */
#define WAVE(c) \
	BARRIERS; \
	MOVW	$(c), R1; \
	MOVW	$PHYSCONS, R6; \
	MOVW	R1, (R6); \
	BARRIERS

/*
 * new instructions
 */

#define SMC	WORD	$0xe1600070	/* low 4-bits are call # (trustzone) */
/* flush branch-target cache; zeroes R0 (cortex) */
#define FLBTC	\
	MOVW	$0, R0; \
	MCR	CpSC, 0, R0, C(CpCACHE), C(CpCACHEinvi), CpCACHEflushbtc
/* flush one entry of the branch-target cache, va in R0 (cortex) */
#define FLBTSE	\
	MCR	CpSC, 0, R0, C(CpCACHE), C(CpCACHEinvi), CpCACHEflushbtse

/* arm v7 arch defines these */
#define WFI	WORD	$0xe320f003	/* wait for interrupt */
#define DMB	WORD	$0xf57ff05f	/* data mem. barrier; last f = SY */
#define DSB	WORD	$0xf57ff04f	/* data synch. barrier; last f = SY */
#define ISB	WORD	$0xf57ff06f	/* instr. sync. barrier; last f = SY */
#define NOOP	WORD	$0xe320f000
#define CLZ(s, d) WORD	$(0xe16f0f10 | (d) << 12 | (s))	/* count leading 0s */
#define CPSIE	WORD	$0xf1080080	/* intr enable: zeroes I bit */
#define CPSID	WORD	$0xf10c0080	/* intr disable: sets I bit */

/* floating point */
#define VMRS(fp, cpu) WORD $(0xeef00a10 | (fp)<<16 | (cpu)<<12) /* FP → arm */
#define VMSR(cpu, fp) WORD $(0xeee00a10 | (fp)<<16 | (cpu)<<12) /* arm → FP */

/*
 * a popular code sequence used to write a pte for va is:
 *
 *	MOVW	R(n), TTB[LnX(va)]
 *	// clean the cache line
 *	DSB
 *	// invalidate tlb entry for va
 *	FLBTC
 *	DSB
 * 	PFF (now ISB)
 */
/* zeroes R0 */
#define	BARRIERS	FLBTC; DSB; ISB

/*
 * invoked with PTE bits in R2, pa in R3, PTE pointed to by R4.
 * fill PTE pointed to by R4 and increment R4 past it.
 * increment R3 by a MB.  clobbers R1.
 */
#define FILLPTE() \
	ORR	R3, R2, R1;			/* pte bits in R2, pa in R3 */ \
	MOVW	R1, (R4); \
	ADD	$4, R4;				/* bump PTE address */ \
	ADD	$MiB, R3;			/* bump pa */ \

/* zero PTE pointed to by R4 and increment R4 past it. assumes R0 is 0. */
#define ZEROPTE() \
	MOVW	R0, (R4); \
	ADD	$4, R4;				/* bump PTE address */
