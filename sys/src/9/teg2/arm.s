/*
 * nvidia tegra 2 machine assist, definitions
 * dual-core cortex-a9 processor
 *
 * R9 and R10 are used for `extern register' variables.
 * R11 is used by the loader as a temporary, so avoid it.
 */

#include "mem.h"
#include "arm.h"

#undef B					/* B is for 'botch' */

#define KADDR(pa)	(KZERO    | ((pa) & ~KSEGM))
#define PADDR(va)	(PHYSDRAM | ((va) & ~KSEGM))

#define L1X(va)		(((((va))>>20) & 0x0fff)<<2)

#define MACHADDR	(L1-MACHSIZE)		/* only room for cpu0's */

/* L1 pte values */
#define PTEDRAM	(Dom0|L1AP(Krw)|Section|L1ptedramattrs)
#define PTEIO	(Dom0|L1AP(Krw)|Section)

#define DOUBLEMAPMBS	 512	/* megabytes of low dram to double-map */

/* steps on R0 */
#define DELAY(label, mloops) \
	MOVW	$((mloops)*1000000), R0; \
label: \
	SUB.S	$1, R0; \
	BNE	label

/* print a byte on the serial console; clobbers R0 & R6; needs R12 (SB) set */
#define PUTC(c) \
	BARRIERS; \
	MOVW	$(c), R0; \
	MOVW	$PHYSCONS, R6; \
	MOVW	R0, (R6); \
	BARRIERS

/*
 * new instructions
 */

#define SMC	WORD	$0xe1600070	/* low 4-bits are call # (trustzone) */
/* flush branch-target cache */
#define FLBTC  MTCP CpSC, 0, PC, C(CpCACHE), C(CpCACHEinvi), CpCACHEflushbtc
/* flush one entry of the branch-target cache, va in R0 (cortex) */
#define FLBTSE MTCP CpSC, 0, R0, C(CpCACHE), C(CpCACHEinvi), CpCACHEflushbtse

/* arm v7 arch defines these */
#define DSB	WORD	$0xf57ff04f	/* data synch. barrier; last f = SY */
#define DMB	WORD	$0xf57ff05f	/* data mem. barrier; last f = SY */
#define ISB	WORD	$0xf57ff06f	/* instr. sync. barrier; last f = SY */

#define WFI	WORD	$0xe320f003	/* wait for interrupt */
#define NOOP	WORD	$0xe320f000

#define CLZ(s, d) WORD	$(0xe16f0f10 | (d) << 12 | (s))	/* count leading 0s */

#define SETEND(o) WORD	$(0xf1010000 | (o) << 9)  /* o==0, little-endian */

#define CPSIE	WORD	$0xf1080080	/* intr enable: zeroes I bit */
#define CPSID	WORD	$0xf10c00c0	/* intr disable: sets I,F bits */
#define CPSAE	WORD	$0xf1080100	/* async abt enable: zeroes A bit */
#define CPSMODE(m) WORD $(0xf1020000 | (m)) /* switch to mode m (PsrM*) */

#define	CLREX	WORD	$0xf57ff01f

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

/*
 * set kernel SB for zero segment (instead of usual KZERO segment).
 * NB: the next line puts rubbish in R12:
 *	MOVW	$setR12-KZERO(SB), R12
 */
#define SETZSB \
	MOVW	$setR12(SB), R12;		/* load kernel's SB */ \
	SUB	$KZERO, R12; \
	ADD	$PHYSDRAM, R12

/*
 * note that 5a's RFE is not the v6/7 arch. instruction (0xf8900a00),
 * which loads CPSR from the word after the PC at (R13), but rather
 * the pre-v6 simulation `MOVM.IA.S.W (R13), [R15]' (0xe8fd8000 since
 * MOVM is LDM in this case), which loads CPSR not from memory but
 * from SPSR due to `.S'.
 */
#define RFEV7(r)    WORD $(0xf8900a00 | (r) << 16)
#define RFEV7W(r)   WORD $(0xf8900a00 | (r) << 16 | 0x00200000)	/* RFE.W */
#define RFEV7DB(r)  WORD $(0xf9100a00 | (r) << 16)		/* RFE.DB */
#define RFEV7DBW(r) WORD $(0xf9100a00 | (r) << 16 | 0x00200000)	/* RFE.DB.W */

#define CKPSR(psr, tmp, bad)
#define CKCPSR(psrtmp, tmp, bad)

/* return with cpu id in r and condition codes set from "r == 0" */
#define CPUID(r) \
	MFCP	CpSC, 0, r, C(CpID), C(CpIDidct), CpIDmpid; \
	AND.S	$(MAXMACH-1), r			/* mask out non-cpu-id bits */
