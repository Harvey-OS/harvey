	TEXT	_main(SB), $16
	MOVW	$setR30(SB), R30
	JAL	_envsetup(SB)
	MOVW	inargc-4(FP), R1
	MOVW	$inargv+0(FP), R2
	MOVW	R1, 4(R29)
	MOVW	R2, 8(R29)
	JAL	main(SB)
loop:
	MOVW	R1, 4(R29)
	JAL	exit(SB)
	JMP	loop
