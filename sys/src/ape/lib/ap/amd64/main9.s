	TEXT	_main(SB), 1, $(3*8)

	CALL	_envsetup(SB)
	MOVL	inargc-8(FP), RARG
	LEAQ	inargv+0(FP), AX
	MOVQ	AX, 8(SP)
	MOVQ	environ(SB), AX
	MOVQ	AX, 16(SP)
	CALL	main(SB)
	MOVQ	AX, RARG
	CALL	exit(SB)
	RET
