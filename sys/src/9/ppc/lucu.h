/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * on return from this function we will be running in virtual mode.
 * We set up the Block Address Translation (BAT) registers thus:
 * 1) first 3 BATs are 256M blocks, starting from KZERO->0
 * 2) remaining BAT maps last 256M directly
 */
TEXT	mmuinit0(SB), $0
	/* reset all the tlbs */
	MOVW	$64, R3
	MOVW	R3, CTR
	MOVW	$0, R4

tlbloop:
	TLBIE	R4
	SYNC
	ADD		$BIT(19), R4
	BDNZ	tlbloop
	TLBSYNC

	/* BATs 0 and 1 cover memory from 0x00000000 to 0x20000000 */

	/* KZERO -> 0, IBAT2 and DBAT2, 256 MB */
	MOVW	$(KZERO|(0x7ff<<2)|2), R3
	MOVW	$(PTEVALID|PTEWRITE), R4	/* PTEVALID => Cache coherency on */
	MOVW	R3, SPR(DBATU(2))
	MOVW	R4, SPR(DBATL(2))
	MOVW	R3, SPR(IBATU(2))
	MOVW	R4, SPR(IBATL(2))

	/* enable MMU */
	MOVW	LR, R3
	OR		$KZERO, R3
	MOVW	R3, SPR(SRR0)	/* Stored PC for RFI instruction */
	MOVW	MSR, R4
	OR		$(MSR_IR|MSR_DR|MSR_RI|MSR_FP), R4
	MOVW	R4, SPR(SRR1)
	RFI		/* resume in kernel mode in caller */

	RETURN
