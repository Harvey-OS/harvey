TEXT	_main(SB), 1, $0
	MOVL	AX, _clock(SB)
	LEAL	inargv+0(FP), AX
	PUSHL	AX
	MOVL	inargc-4(FP), AX
	PUSHL	AX
	CALL	main(SB)

loop:
	MOVL	$_exits<>(SB), AX
	PUSHL	AX
	CALL	exits(SB)
	JMP	loop

DATA	_exits<>+0(SB)/4, $"main"
GLOBL	_exits<>+0(SB), $5
