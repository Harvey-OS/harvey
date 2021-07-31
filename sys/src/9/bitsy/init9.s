TEXT	_main(SB),$8
	MOVW	$setR12(SB), R12	/* load the SB */
	MOVW	$boot(SB), R0

	ADD	$12, R13, R1	/* get a pointer to 0(FP) */

	MOVW	R0, 4(R13)
	MOVW	R1, 8(R13)

	BL	startboot(SB)


