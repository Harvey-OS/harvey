TEXT _main(SB), 1, $12
	CALL	_envsetup(SB)
	MOV	argc+0(FP), R4
	MOV	argv+4(FP), R8
	CALL	main(SB)
_loop:
	MOV	$0, R4
	CALL	exit(SB)
	JMP	_loop

GLOBL	errno+0(SB), $4
