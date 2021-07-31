TEXT	_main(SB), 1, $16

	MOVQ	$setSB(SB), R29
	JSR	_envsetup(SB)
	MOVL	inargc-8(FP), R0
	MOVL	$inargv-4(FP), R1
	MOVL	R0, 8(R30)
	MOVL	R1, 12(R30)
	JSR	main(SB)
loop:
	MOVL	R0, 8(R30)
	JSR	exit(SB)
	MOVQ	$_divq(SB), R31		/* force loading of divq */
	MOVQ	$_divl(SB), R31			/* force loading of divl */
	JMP	loop
