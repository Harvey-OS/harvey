TEXT	_main(SB), 1, $16
	MOVL	$a6base(SB), A6
	MOVL	R0, _clock(SB)
	PEA	inargv+0(FP)
	MOVL	inargc-4(FP), TOS
	BSR	main(SB)
	MOVL	$_exits<>+0(SB), R0
	MOVL	R0,TOS
	BSR	exits(SB)
	RTS

DATA	_exits<>+0(SB)/4, $"main"
GLOBL	_exits<>+0(SB), $5
