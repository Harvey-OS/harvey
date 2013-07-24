TEXT	_main(SB), $8
	MOVW	$setR30(SB), R30
	MOVW	$boot(SB), R1
	ADDU	$12, R29, R2	/* get a pointer to 0(FP) */
	MOVW	R1, 4(R29)
	MOVW	R2, 8(R29)
	JAL	startboot(SB)

