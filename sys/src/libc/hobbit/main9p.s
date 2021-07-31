TEXT _mainp(SB), 1, $12
	MOV	clock-4(FP), _clock(SB)

	CALL	_profmain(SB)
	MOV	__prof+4(SB), __prof+0(SB)

	MOV	argc+0(FP), R4
	MOV	argv+4(FP), R8
	CALL	main(SB)

_loop:
	MOV	$_exits<>+0(SB), R4
	CALL	exits(SB)
	JMP	_loop

TEXT _savearg(SB), 1, $0
	MOVA	R4, R4
	ADD	$32, R4		/* kludge, we know profin/profout have $32 frames */
	MOV	*R4, R4
	RETURN

TEXT _callpc(SB), 1, $0
	MOV	arg+0(FP), R4
	ADD	$4, R4
	MOV	*R4, R4
	RETURN

DATA	_exits<>+0(SB), $"main"
GLOBL	_exits<>+0(SB), $8
