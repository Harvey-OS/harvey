TEXT	_mainp(SB), 1, $16
	MOVL	$a6base(SB), A6
	MOVL	R0, _clock(SB)		/* return value of sys exec!! */
	BSR	_profmain(SB)
	MOVL	__prof+4(SB), __prof+0(SB)
	PEA	inargv+0(FP)
	MOVL	inargc-4(FP), TOS
	BSR	_envsetup(SB)
	BSR	main(SB)

loop:
	MOVL	R0,TOS
	BSR	exit(SB)
	LEA	_profin(SB), A0		/* force loading of profile */
	BRA	loop


TEXT	_savearg(SB), 1, $0
	RTS

TEXT	_callpc(SB), 1, $0
	MOVL	argp+0(FP), A0
	MOVL	4(A0), R0
	RTS
