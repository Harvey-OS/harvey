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

	/* KZERO -> 0, IBAT and DBAT, 256 MB */
	MOVW	$(KZERO|(0x7ff<<2)|2), R3
	MOVW	$(PTEVALID|PTEWRITE), R4	/* PTEVALID => Cache coherency on */
	MOVW	R3, SPR(IBATU(0))
	MOVW	R4, SPR(IBATL(0))
	MOVW	R3, SPR(DBATU(0))
	MOVW	R4, SPR(DBATL(0))

	/* KZERO+256M -> 256M, IBAT and DBAT, 256 MB */
	ADD		$(1<<28), R3
	ADD		$(1<<28), R4
	MOVW	R3, SPR(IBATU(1))
	MOVW	R4, SPR(IBATL(1))
	MOVW	R3, SPR(DBATU(1))
	MOVW	R4, SPR(DBATL(1))

	/* FPGABASE -> FPGABASE, DBAT, 16 MB */
	MOVW	$(FPGABASE|(0x7f<<2)|2), R3
	MOVW	$(FPGABASE|PTEWRITE|PTEUNCACHED), R4	/* FPGA memory, don't cache */
	MOVW	R3, SPR(DBATU(2))
	MOVW	R4, SPR(DBATL(2))

	/* IBAT 2 unused */
	MOVW	R0, SPR(IBATU(2))
	MOVW	R0, SPR(IBATL(2))

	/* direct map last block, uncached, (not guarded, doesn't work for BAT), DBAT only */
	MOVW	$(INTMEM|(0x7ff<<2)|2), R3
	MOVW	$(INTMEM|PTEWRITE|PTEUNCACHED), R4	/* Don't set PTEVALID here */
	MOVW	R3, SPR(DBATU(3))
	MOVW	R4, SPR(DBATL(3))

	/* IBAT 3 unused */
	MOVW	R0, SPR(IBATU(3))
	MOVW	R0, SPR(IBATL(3))

	/* enable MMU */
	MOVW	LR, R3
	OR		$KZERO, R3
	MOVW	R3, SPR(SRR0)	/* Stored PC for RFI instruction */
	MOVW	MSR, R4
	OR		$(MSR_IR|MSR_DR|MSR_RI|MSR_FP), R4
	MOVW	R4, SPR(SRR1)
	RFI		/* resume in kernel mode in caller */

	RETURN
