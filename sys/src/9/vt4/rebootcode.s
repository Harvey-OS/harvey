/* virtex4 ppc405 reboot code */
#include "mem.h"

#define SPR_SRR0	26		/* Save/Restore Register 0 */
#define SPR_SRR1	27		/* Save/Restore Register 1 */
#define SPR_DCWR	954		/* Data Cache Write-through Register */
#define SPR_DCCR	1018		/* Data Cache Cachability Register */
#define SPR_ICCR	1019		/* Instruction Cache Cachability Register */

/* special instruction definitions */
#define	BDNZ	BC	16,0,
#define	BDNE	BC	0,2,

#define	ICCCI(a,b)	WORD	$((31<<26)|((a)<<16)|((b)<<11)|(966<<1))
#define	DCCCI(a,b)	WORD	$((31<<26)|((a)<<16)|((b)<<11)|(454<<1))

/* on the 400 series, the prefetcher madly fetches across RFI, sys call, and others; use BR 0(PC) to stop */
#define	RFI	WORD $((19<<26)|(50<<1)); BR 0(PC)
#define	RFCI	WORD $((19<<26)|(51<<1)); BR 0(PC)

/* print progress character.  steps on R7 and R8, needs SB set. */
#define PROG(c)	MOVW $(Uartlite+4), R7; MOVW $(c), R8; MOVW	R8, 0(R7); SYNC	

/*
 * Turn off MMU, then copy the new kernel to its correct location
 * in physical memory.  Then jump to the start of the kernel.
 */

	NOSCHED

TEXT	main(SB),1,$-4
	MOVW	$setSB+KZERO(SB), R2

PROG('*')
	MOVW	R3, p1+0(FP)		/* destination, passed in R3 */
	MOVW	R3, R6			/* entry point */
	MOVW	p2+4(FP), R4		/* source */
	MOVW	n+8(FP), R5		/* byte count */

	BL	mmuoff(SB)
	MOVW	$setSB(SB), R2

//	BL	resetcaches(SB)

PROG('*')
	MOVW	R3, R1			/* tiny trampoline stack */
	SUB	$0x20, R1		/* bypass a.out header */

	MOVWU	R0, -48(R1)		/* allocate stack frame */
	MOVW	R3, 44(R1)		/* save dest */
	MOVW	R5, 40(R1)		/* save count */
	MOVW	R3, 0(R1)
	MOVW	R3, 4(R1)		/* push dest */
	MOVW	R4, 8(R1)		/* push src */
	MOVW	R5, 12(R1)		/* push size */
	BL	memmove(SB)
	SYNC
	MOVW	44(R1), R6		/* restore R6 (dest) */
	MOVW	40(R1), R5		/* restore R5 (count) */
PROG('-')
	/*
	 * flush data cache
	 */
	MOVW	R6, R3			/* virtaddr */
	MOVW	R5, R4			/* count */
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

	/* be sure icache is cleared */
	BL	resetcaches(SB)

/*
 * jump to kernel entry point.  Note the true kernel entry point is
 * the virtual address KZERO|R6, but this must wait until
 * the MMU is enabled by the kernel in l.s
 */
	OR	R6, R6		/* NOP: avoid link bug */
	MOVW	R6, LR
PROG('>')
PROG('\r');
PROG('\n');
	BL	(LR)
	BR	0(PC)

TEXT	mmuoff(SB), 1, $-4
	MOVW	LR, R7
	RLWNM	$0, R7, $~KZERO, R7	/* new PC */
	MOVW	R7, SPR(SPR_SRR0)

	RLWNM	$0, R1, $~KZERO, R1	/* make stack pointer physical */

	/*
	 * turn off interrupts & MMU
	 * disable machine check
	 */
	MOVW	MSR, R7
	RLWNM	$0, R7, $~(MSR_EE), R7
	RLWNM	$0, R7, $~(MSR_ME), R7
	RLWNM	$0, R7, $~(MSR_IR|MSR_DR), R7
	MOVW	R7, SPR(SPR_SRR1)
/*	ISYNC		/* no ISYNC here as we have no TLB entry for the PC */
	SYNC		/* fix 405 errata cpu_210 */
	RFI		/* resume in kernel mode in caller */

/* clobbers R7 */
TEXT	resetcaches(SB),1,$-4
	/*
	 * reset the caches and disable them
	 */
	MOVW	R0, SPR(SPR_ICCR)
	ICCCI(0, 2)  /* errata cpu_121 reveals that EA is used; we'll use SB */
	ISYNC
	DCCCI(0, 0)
	SYNC; ISYNC

	MOVW	$((DCACHEWAYSIZE/DCACHELINESZ)-1), R7
	MOVW	R7, CTR
	MOVW	R0, R7
dcinv:
	DCCCI(0,7)
	ADD	$32, R7
	BDNZ	dcinv

	/* cache is copy-back, disabled */
	MOVW	R0, SPR(SPR_DCWR)
	MOVW	R0, SPR(SPR_DCCR)
	MOVW	R0, SPR(SPR_ICCR)
	ISYNC
	SYNC
	BR	(LR)
