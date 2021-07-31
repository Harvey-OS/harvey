	TEXT	_main(SB), $16
	MOVW	$setSB(SB), R2
	JMPL	_envsetup(SB)
	MOVW	inargc-4(FP), R7
	MOVW	$inargv+0(FP), R8
	MOVW	R7, 4(R1)
	MOVW	R8, 8(R1)
	JMPL	main(SB)

loop:
	MOVW	R7, 4(R1)
	JMPL	exit(SB)
	MOVW	$_mul(SB),R7
	JMP	loop
