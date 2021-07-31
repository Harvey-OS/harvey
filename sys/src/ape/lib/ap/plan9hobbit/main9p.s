	TEXT	_main(SB), $0
	CALL	_envsetup+0(SB)
	CALL	main+0(SB)
	CALL	exit+0(SB)
	RETURN

GLOBL	errno+0(SB), $4
