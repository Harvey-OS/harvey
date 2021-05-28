	TEXT	_main(SB), 1, $(3*4)

	MOVW	$setSB(SB), R3
	JAL	R1, _envsetup(SB)

	MOVW	inargc-4(FP), R8
	MOVW	R8, 4(R2)
	MOVW	$inargv+0(FP), R9
	MOVW	R9, 8(R2)
	MOVW	environ(SB), R9
	MOVW	R9, 12(R2)
	JAL	R1, main(SB)

	MOVW	R8, 4(R2)
	JAL	R1, exit(SB)
	RET
