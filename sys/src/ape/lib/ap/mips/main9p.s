TEXT	_mainp(SB), 1, $16
	MOVW	$setR30(SB), R30
	MOVW	R1, _clock(SB)
/*
	MOVW	$0,FCR31
	NOR	R0,R0
	MOVD	$0.5, F26
	SUBD	F26, F26, F24
	ADDD	F26, F26, F28
	ADDD	F28, F28, F30
*/
	JAL	_profmain(SB)
	MOVW	__prof+4(SB), R1
	MOVW	R1, __prof+0(SB)
	JAL	_envsetup(SB)
	MOVW	inargc-4(FP), R1
	MOVW	$inargv+0(FP), R2
	MOVW	R1, 4(R29)
	MOVW	R2, 8(R29)
	JAL	main(SB)
loop:
	MOVW	R1, 4(R29)
	JAL	exit(SB)
	MOVW	$_profin(SB), R0	/* force loading of profile */
	JMP	loop

TEXT	_savearg(SB), 1, $0
	RET

TEXT	_callpc(SB), 1, $0
	MOVW	argp-4(FP), R1
	RET
