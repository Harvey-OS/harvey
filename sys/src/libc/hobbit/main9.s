TEXT _main(SB), 1, $12
	MOV	R0, _clock(SB)

	MOV	argc+0(FP), R4
	MOV	argv+4(FP), R8
	CALL	main(SB)

_loop:
	MOV	$_exits<>+0(SB), R4
	CALL	exits(SB)
	JMP	_loop

DATA	_exits<>+0(SB), $"main"
GLOBL	_exits<>+0(SB), $8
