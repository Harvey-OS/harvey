#define NPRIVATES	16

TEXT	_main(SB), 1, $(16 + NPRIVATES*4)

	MOVW	$setSB(SB), R2
	MOVW	R7, _tos(SB)

	MOVW	$p-64(SP),R7
	MOVW	R7,_privates+0(SB)
	MOVW	$16,R7
	MOVW	R7,_nprivates+0(SB)
/*
	MOVW	_fpsr+0(SB), FSR
	FMOVD	$0.5, F26
	FSUBD	F26, F26, F24
	FADDD	F26, F26, F28
	FADDD	F28, F28, F30
*/
	MOVW	inargc-4(FP), R7
	MOVW	$inargv+0(FP), R8
	MOVW	R8, 8(R1)
	JMPL	main(SB)

loop:
	MOVW	$_exits<>(SB), R7
	JMPL	exits(SB)
	MOVW	$_mul(SB), R8	/* force loading of muldiv */
	JMP	loop

DATA	_exits<>+0(SB)/5, $"main"
GLOBL	_exits<>+0(SB), $5
