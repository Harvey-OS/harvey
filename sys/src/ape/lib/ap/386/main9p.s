	TEXT	_mainp(SB), 1, $8

	MOVL	AX, _clock(SB)
	CALL	_profmain(SB)
	MOVL	__prof+4(SB), AX
	MOVL	AX, __prof+0(SB)
	CALL	_envsetup(SB)
	MOVL	inargc-4(FP), AX
	MOVL	AX, 0(SP)
	LEAL	inargv+0(FP), AX
	MOVL	AX, 4(SP)
	CALL	main(SB)

loop:
	MOVL	AX, 0(SP)
	CALL	exit(SB)
	MOVL	$_profin(SB), AX	/* force loading of profile */
	MOVL	$0, AX
	JMP	loop

TEXT	_savearg(SB), 1, $0
	RET

TEXT	_callpc(SB), 1, $0
	MOVL	argp+0(FP), AX
	MOVL	4(AX), AX
	RET
