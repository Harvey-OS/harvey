TEXT	_main(SB), 1, $16

	MOVW	$setSB(SB), R2

	BL		_envsetup(SB)
	MOVW	inargc-4(FP), R3
	MOVW	$inargv+0(FP), R4
	MOVW	R3, 4(R1)
	MOVW	R4, 8(R1)
	BL	main(SB)
loop:
	MOVW	R3, 4(R1)
	BL	exit(SB)
	BR	loop
