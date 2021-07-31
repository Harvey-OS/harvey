TEXT	_mainp(SB), 1, $16

	MOVQ	$setSB(SB), R29
	MOVL	R0, _clock(SB)
	JSR	_profmain(SB)
	MOVL	__prof+4(SB), R0
	MOVL	R0, __prof+0(SB)
	JSR	_envsetup(SB)
	MOVL	inargc-4(FP), R0
	MOVL	$inargv+0(FP), R1
	MOVL	R0, 8(R30)
	MOVL	R1, 12(R30)
	JSR	main(SB)
loop:
	MOVL	R0, 8(R30)
	JSR	exit(SB)
	MOVQ	$_divq(SB), R31		/* force loading of divq */
	MOVQ	$_divl(SB), R31		/* force loading of divl */
	MOVQ	$_profin(SB), R31	/* force loading of profile */
	JMP	loop

TEXT	_savearg(SB), 1, $0
	RET

TEXT	_callpc(SB), 1, $0
	MOVL	argp-8(FP), R0
	RET
