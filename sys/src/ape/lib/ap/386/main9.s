	TEXT	_main(SB), 1, $12

	CALL	_envsetup(SB)
	MOVL	inargc-4(FP), AX
	MOVL	AX, 0(SP)
	LEAL	inargv+0(FP), AX
	MOVL	AX, 4(SP)
	MOVL	environ(SB), AX
	MOVL	AX, 8(SP)
	CALL	main(SB)
	MOVL	AX, 0(SP)
	CALL	exit(SB)
	RET
