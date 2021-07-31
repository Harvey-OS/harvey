	TEXT	_main(SB), 1, $16

	MOVW	$setSB(SB), R2
	MOVW	R7, _clock(SB)

	MOVW	inargc-4(FP), R7
	MOVW	$inargv+0(FP), R8
	MOVW	R7, 4(R1)
	MOVW	R8, 8(R1)
	JMPL	ALEF_init(SB)
loop:
	MOVW	$_mul(SB), R7	/* force loading of muldiv */
	JMP	loop

GLOBL	_clock(SB), $4
