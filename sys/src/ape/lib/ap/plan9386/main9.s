	TEXT	_main(SB), $0

	CALL	_envsetup(SB)
	LEAL	inargv+0(FP), AX
	PUSHL	AX
	MOVL	inargc-4(FP), AX
	PUSHL	AX
	CALL	main(SB)
	PUSHL	AX
	CALL	exit(SB)
	RET
