TEXT	_main(SB), 1, $16
	MOVL	$a6base(SB), A6
	PEA	inargv+0(FP)
	MOVL	inargc-4(FP), TOS
	BSR	_envsetup(SB)
	BSR	main(SB)
	MOVL	R0,TOS
	BSR	exit(SB)
	RTS
