TEXT _mainp(SB), $0
	MOV	R0, _clock(SB)

	CALL	_profmain(SB)
	MOV	__prof+4(SB), __prof+0(SB)
	CALL	main(SB)
_loop:
	MOV	$_exits<>+0(SB), R4
	CALL	exits(SB)
	JMP	_loop

TEXT _savearg(SB), $0
	RETURN

TEXT _callpc(SB), $0
	ADD	$4, R4
	MOV	*R4, R4
	RETURN

DATA	_exits<>+0(SB), $"main"
GLOBL	_exits<>+0(SB), $8
