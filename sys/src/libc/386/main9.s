TEXT	_main(SB), 1, $8
	MOVL	AX, _clock(SB)
	MOVL	inargc-4(FP), AX
	MOVL	AX, 0(SP)
	LEAL	inargv+0(FP), AX
	MOVL	AX, 4(SP)
	CALL	main(SB)

loop:
	MOVL	$_exits<>(SB), AX
	MOVL	AX, 0(SP)
	CALL	exits(SB)
	JMP	loop

DATA	_exits<>+0(SB)/4, $"main"
GLOBL	_exits<>+0(SB), $5
