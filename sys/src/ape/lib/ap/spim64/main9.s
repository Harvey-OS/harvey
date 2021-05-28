	TEXT	_main(SB), $16
	MOVV	$setR30(SB), R30
	JAL	_envsetup(SB)
	MOVW	inargc-4(FP), R1
	MOVV	$inargv+0(FP), R2
	MOVW	R1, 12(R29)
	MOVV	R2, 16(R29)
	JAL	main(SB)
loop:
	MOVV	R1, 8(R29)
	JAL	exit(SB)
	JMP	loop
