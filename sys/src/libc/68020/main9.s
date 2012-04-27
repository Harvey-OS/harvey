#define NPRIVATES	16

TEXT	_main(SB), 1, $(16 + NPRIVATES*4)
	MOVL	$a6base(SB), A6
	MOVL	R0, _tos(SB)
	LEA	p-64(SP),A0
	MOVL	A0,_privates+0(SB)
	MOVL	$16,R0
	MOVL	R0,_nprivates+0(SB)
	PEA	inargv+0(FP)
	MOVL	inargc-4(FP), TOS
	BSR	main(SB)
	MOVL	$_exits<>+0(SB), R0
	MOVL	R0,TOS
	BSR	exits(SB)
	RTS

DATA	_exits<>+0(SB)/4, $"main"
GLOBL	_exits<>+0(SB), $5
