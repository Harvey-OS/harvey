TEXT _main(SB), $0
	MOV	R0, _clock(SB)
	CALL	main(SB)
	MOV	$_exits<>+0(SB), R4
	CALL	exits(SB)
	RETURN

DATA	_exits<>+0(SB), $"main"
GLOBL	_exits<>+0(SB), $8
