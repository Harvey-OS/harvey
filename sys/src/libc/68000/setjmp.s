TEXT	setjmp(SB), 1, $0
	MOVL	b+0(FP), A0
	MOVL	A7, (A0)+
	MOVL	(A7), (A0)
	CLRL	R0
	RTS

TEXT	longjmp(SB), 1, $0
	MOVL	b+0(FP), A0
	MOVL	r+4(FP), R0
	BNE	ok		/* ansi: "longjmp(0) => longjmp(1)" */
	MOVL	$1, R0		/* bless their pointed heads */
ok:	MOVL	(A0)+, A7
	MOVL	(A0), (A7)
	RTS
