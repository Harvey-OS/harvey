TEXT	_main(SB), 1, $8
	MOVL	AX, _clock(SB)
	MOVL	inargc-4(FP), AX
	MOVL	AX, 0(SP)
	LEAL	inargv+0(FP), AX
	MOVL	AX, 4(SP)
	CALL	ALEF_init(SB)

loop:
	JMP	loop

GLOBL	_clock(SB), $4
