TEXT	_mainp(SB), 1, $16

	MOVW	$setSB(SB), R2
	MOVW	R7, _clock(SB)
/*
	MOVW	_fpsr+0(SB), FSR
	FMOVD	$0.5, F26
	FSUBD	F26, F26, F24
	FADDD	F26, F26, F28
	FADDD	F28, F28, F30
*/

	JMPL	_profmain(SB)
	MOVW	__prof+4(SB), R7
	MOVW	R7, __prof+0(SB)

	MOVW	inargc-4(FP), R7
	MOVW	$inargv+0(FP), R8
	MOVW	R8, 8(R1)
	JMPL	main(SB)

loop:
	MOVW	$_exits<>(SB), R7
	JMPL	exits(SB)
	MOVW	$_mul(SB), R8		/* force loading of muldiv */
	MOVW	$_profin(SB), R9	/* force loading of profile */
	JMP	loop

TEXT	_savearg(SB), 1, $0
	RETURN

TEXT	_callpc(SB), 1, $0
	MOVW	argp-4(FP), R7
	RETURN

DATA	_exits<>+0(SB)/4, $"main"
GLOBL	_exits<>+0(SB), $5
