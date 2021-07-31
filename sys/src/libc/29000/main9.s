arg=69
sp=65

TEXT	_main(SB), 1, $16

	MOVL	$setR67(SB), R67
	MOVL	R(arg), _clock(SB)

	MOVL	inargc-4(FP), R(arg)
	MOVL	$inargv+0(FP), R(arg+1)
	MOVL	R(arg), 4(R(sp))
	MOVL	R(arg+1), 8(R(sp))
	CALL	main(SB)
loop:
	MOVL	$_exitstr<>(SB), R(arg)
	MOVL	R(arg), 4(R(sp))
	CALL	exits(SB)
	JMP	loop

DATA	_exitstr<>+0(SB)/4, $"main"
GLOBL	_exitstr<>+0(SB), $5
