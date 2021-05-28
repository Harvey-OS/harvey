/* virtex5 ppc440x5 reboot code */
#include "mem.h"

#define SPR_SRR0	26		/* Save/Restore Register 0 */
#define SPR_SRR1	27		/* Save/Restore Register 1 */
#define SPR_DBCR0	0x134		/* Debug Control Register 0 */
#define SPR_DBCR1	0x135		/* Debug Control Register 1 */
#define SPR_DBCR2	0x136		/* Debug Control Register 1 */
#define SPR_DBSR	0x130		/* Debug Status Register */

/* special instruction definitions */
#define	BDNZ	BC	16,0,
#define	BDNE	BC	0,2,

#define	ICCCI(a,b)	WORD	$((31<<26)|((a)<<16)|((b)<<11)|(966<<1))
#define	DCCCI(a,b)	WORD	$((31<<26)|((a)<<16)|((b)<<11)|(454<<1))

/*
 * on the 400 series, the prefetcher madly fetches across RFI, sys call,
 * and others; use BR 0(PC) to stop.
 */
#define	RFI	WORD	$((19<<26)|(50<<1)); BR 0(PC)
#define	RFCI	WORD	$((19<<26)|(51<<1)); BR 0(PC)

/* print progress character.  steps on R7 and R8, needs SB set. */
#define PROG(c)	MOVW $(Uartlite+4), R7; MOVW $(c), R8; MOVW	R8, 0(R7); SYNC	

/*
 * copied to REBOOTADDR and executed there.
 * since it copies the new kernel over the old one, it is
 * self-contained and does not call any other kernel code.
 *
 * Turn off interrupts, then copy the new kernel to its correct location
 * in physical memory.  Then jump to the start of the kernel.
 * destination and source arguments are virtual addresses.
 */

	NOSCHED

TEXT	main(SB),1,$-4
	MOVW	$setSB(SB), R2

	MOVW	R3, p1+0(FP)		/* destination, passed in R3 */
	MOVW	R3, R6			/* entry point */
	MOVW	p2+4(FP), R4		/* source */
	MOVW	n+8(FP), R5		/* byte count */

	/* make it safe to copy over the old kernel */
PROG('R')
	BL	introff(SB)
//	MOVW	$setSB(SB), R2

PROG('e')
//	BL	resetcaches(SB)

	/* disable debug facilities */
	ISYNC
	MOVW	R0, SPR(SPR_DBCR0)
	MOVW	R0, SPR(SPR_DBCR1)
	MOVW	R0, SPR(SPR_DBCR2)
	ISYNC
	MOVW	$~0, R7
	MOVW	R7, SPR(SPR_DBSR)
	ISYNC

	MOVW	R3, R1			/* tiny trampoline stack before dest */
	SUB	$0x20, R1		/* bypass a.out header */

	/* copy new kernel over the old */
	MOVWU	R0, -48(R1)		/* allocate stack frame */
	MOVW	R3, 44(R1)		/* save dest */
	MOVW	R5, 40(R1)		/* save count */
	MOVW	R3, 0(R1)
	MOVW	R3, 4(R1)		/* push dest */
	MOVW	R4, 8(R1)		/* push src */
	MOVW	R5, 12(R1)		/* push size */
PROG('s')
	BL	memmove(SB)
	SYNC
	MOVW	44(R1), R6		/* restore dest */
	MOVW	p2+4(FP), R4		/* restore source */
	MOVW	40(R1), R5		/* restore count */

	/* verify that we copied it correctly */
PROG('t')
	MOVW	R4, R10
	MOVW	R6, R11
	MOVW	R5, R12
	SUB	$4, R10
	SUB	$4, R11
cmploop:
	MOVWU	4(R10), R13
	MOVWU	4(R11), R14
	CMP	R13, R14
	BNE	buggered
	SUB	$4, R12
	CMP	R12, R0
	BGT	cmploop

PROG('a')
	ANDCC	$~KZERO, R6, R3		/* physical entry addr */

	MOVW	R0, R4
	SUB	$(4*2), R4		/* r4 is now points to bootstart */
	MOVW	R3, 0(R4)		/* entry addr into bootstart */
	SYNC
	ISYNC

PROG('r')
	/*
	 * flush data cache
	 */
	MOVW	R6, R3			/* virtaddr */
	MOVW	R5, R4			/* count */
	RLWNM	$0, R3, $~(DCACHELINESZ-1), R5
	CMP	R4, R0
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
	ISYNC

	/* be sure icache is cleared */
	BL	resetcaches(SB)

/*
 * jump to kernel entry point.  Note the true kernel entry point is
 * the virtual address R6, and the MMU is always enabled.
 */
	OR	R6, R6		/* NOP: avoid link bug */
	MOVW	R6, LR
PROG('t')
PROG('\r')
PROG('\n')
	BL	(LR)	/* without reset. leaves qtm, dram, etc. intact */
	BR	0(PC)

//	BR	reset
buggered:
	PROG('C')
	PROG('m')
	PROG('p')
	PROG(' ')
	PROG('f')
	PROG('a')
	PROG('i')
	PROG('l')
reset:
	ISYNC
	MOVW	$(3<<28), R3
	MOVW	R3, SPR(SPR_DBCR0)	/* cause a system reset */
	ISYNC
	BR	0(PC)

TEXT	introff(SB), 1, $-4
	MOVW	LR, R7
	MOVW	R7, SPR(SPR_SRR0)

	/*
	 * turn off interrupts & MMU
	 * disable machine check
	 */
	MOVW	MSR, R7
	RLWNM	$0, R7, $~(MSR_EE), R7
	RLWNM	$0, R7, $~(MSR_CE), R7
	RLWNM	$0, R7, $~(MSR_DE), R7
	RLWNM	$0, R7, $~(MSR_ME), R7
	RLWNM	$0, R7, $~(MSR_IS|MSR_DS), R7
	MOVW	R7, SPR(SPR_SRR1)
	SYNC		/* fix 405 errata cpu_210 */
	RFI		/* resume in kernel mode in caller */

/*
 * reset the caches
 * clobbers R7
 */
TEXT	resetcaches(SB),1,$-4
	ICCCI(0, 2)  /* errata cpu_121 reveals that EA is used; we'll use SB */
	ISYNC

	MOVW	$(DCACHEWAYSIZE/DCACHELINESZ), R7
	MOVW	R7, CTR
	MOVW	$KZERO, R7
dcinv:
	DCBF	(R7)
	ADD	$DCACHELINESZ, R7
	BDNZ	dcinv

	SYNC; ISYNC

	BR	(LR)
	BR	0(PC)
