/*
 *	magnum user level lock code
 */
	TEXT	ALEF_tas(SB),$0
btas:
	MOVW	sema+0(FP), R1
	MOVB	R0, 1(R1)
	WORD	$0x27
	WORD	$(023<<26)
	BLTZ	R1, btas
	RET
