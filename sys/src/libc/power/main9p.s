TEXT	_mainp(SB), 1, $16

	MOVW	$setSB(SB), R2
	MOVW	R3, _clock(SB)
	BL	_profmain(SB)
	MOVW	__prof+4(SB), R3
	MOVW	R3, __prof+0(SB)
	MOVW	inargc-4(FP), R3
	MOVW	$inargv+0(FP), R4
	MOVW	R3, 4(R1)
	MOVW	R4, 8(R1)
	BL	main(SB)
loop:
	MOVW	$exits<>(SB), R3
	MOVW	R3, 4(R1)
	BL	exits(SB)
	MOVW	$_profin(SB), R3	/* force loading of profile */
	BR	loop

TEXT	_savearg(SB), 1, $0
	RETURN

TEXT	_callpc(SB), 1, $0
	MOVW	argp-4(FP), R3
	RETURN

DATA	exits<>+0(SB)/4, $"main"
GLOBL	exits<>+0(SB), $5
