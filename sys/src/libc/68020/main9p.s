TEXT	_mainp(SB), 1, $16
	MOVL	$a6base(SB), A6
	MOVL	R0, _clock(SB)		/* return value of sys exec!! */
	BSR	_profmain(SB)
	MOVL	__prof+4(SB), __prof+0(SB)
	PEA	inargv+0(FP)
	MOVL	inargc-4(FP), TOS
	BSR	main(SB)

loop:
	MOVL	$_exits<>+0(SB), R0
	MOVL	R0,TOS
	BSR	exits(SB)
	LEA	_profin(SB), A0		/* force loading of profile */
	BRA	loop

TEXT	_savearg(SB), 1, $0
	RTS

TEXT	_callpc(SB), 1, $0
	MOVL	argp+0(FP), A0
	MOVL	4(A0), R0
	RTS

DATA	_exits<>+0(SB)/4, $"main"
GLOBL	_exits<>+0(SB), $5
