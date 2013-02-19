#define NPRIVATES	16

TEXT	_main(SB), 1, $(8+NPRIVATES*4)
	MOVL	AX, _tos(SB)
	LEAL	8(SP), AX
	MOVL	AX, _privates(SB)
	MOVL	$NPRIVATES, _nprivates(SB)
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
