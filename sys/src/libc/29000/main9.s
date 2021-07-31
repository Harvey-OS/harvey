#define NPRIVATES	16

arg=69
sp=65

TEXT	_main(SB), 1, $(16 + NPRIVATES*4)

	MOVL	$setR67(SB), R67
	MOVL	R(arg), _clock(SB)

	MOVL	$p-64(SP),R(arg)
	MOVL	R(arg),_privates+0(SB)
	MOVL	$16,R(arg)
	MOVL	R(arg),_nprivates+0(SB)

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
