TEXT	_main(SB), 1, $16

	MOVQ	$setSB(SB), R29
	MOVL	R0, _clock(SB)

	MOVL	inargc-8(FP), R0
	MOVL	$inargv-4(FP), R1
	MOVL	R0, 8(R30)
	MOVL	R1, 12(R30)
	JSR	main(SB)
loop:
	MOVL	$_exitstr<>(SB), R0
	MOVL	R0, 8(R30)
	JSR	exits(SB)
	MOVQ	$_divq(SB), R31		/* force loading of divq */
	MOVQ	$_divl(SB), R31			/* force loading of divl */
	JMP	loop

DATA	_exitstr<>+0(SB)/4, $"main"
GLOBL	_exitstr<>+0(SB), $5
