#define NPRIVATES	16

TEXT	_main(SB), 1, $(2*8+NPRIVATES*8)
	MOVQ	AX, _tos(SB)
	LEAQ	16(SP), AX
	MOVQ	AX, _privates(SB)
	MOVL	$NPRIVATES, _nprivates(SB)
	MOVL	inargc-8(FP), RARG
	LEAQ	inargv+0(FP), AX
	MOVQ	AX, 8(SP)
	CALL	main(SB)

loop:
	MOVQ	$_exits<>(SB), RARG
	CALL	exits(SB)
	JMP	loop

DATA	_exits<>+0(SB)/4, $"main"
GLOBL	_exits<>+0(SB), $5
