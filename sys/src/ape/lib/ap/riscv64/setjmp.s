arg=8
link=1
sp=2

TEXT	setjmp(SB), 1, $-4
	MOV	R(sp), (R(arg))
	MOV	R(link), XLEN(R(arg))
	MOV	R0, R(arg)
	RET

TEXT	sigsetjmp(SB), 1, $-4		/* sigsetjmp(sigjmp_buf, int) */
	MOVW	savemask+8(FP), R(arg+2)
	MOVW	R(arg+2), 0(R(arg))
	MOV	$_psigblocked(SB), R(arg+2)
	MOV	R(arg+2), XLEN(R(arg))		/* save &_psigblocked */
	MOV	R(sp), (2*XLEN)(R(arg))		/* save sp */
	MOV	R(link), (3*XLEN)(R(arg))	/* save return pc */
	MOV	R0, R(arg)
	RET

TEXT	longjmp(SB), 1, $-4
	MOVW	r+XLEN(FP), R(arg+2)
	BNE	R(arg+2), ok		/* ansi: "longjmp(0) => longjmp(1)" */
	MOV	$1, R(arg+2)		/* bless their pointed heads */
ok:	MOV	(R(arg)), R(sp)
	MOV	XLEN(R(arg)), R(link)
	MOV	R(arg+2), R(arg)
	RET
