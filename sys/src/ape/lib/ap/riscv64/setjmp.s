TEXT	setjmp(SB), 1, $-4
	MOV 	R2, 0(R8)
	MOV 	R1, XLEN(R8)
	MOV 	$0, R8
	RET

TEXT	sigsetjmp(SB), 1, $-4
	MOVW 	savemask+XLEN(FP), R10
	MOV 	R10, 0(R8)
	MOVW 	$_psigblocked(SB), R10
	MOV 	R10, XLEN(R8)
	MOV 	R2, (2*XLEN)(R8)
	MOV 	R1, (3*XLEN)(R8)
	MOV 	$0, R8
	RET

TEXT	longjmp(SB), 1, $-4
	MOVW	r+XLEN(FP), R10
	BNE	R10, ok		/* ansi: "longjmp(0) => longjmp(1)" */
	MOV	$1, R10		/* bless their pointed heads */
ok:	MOV	0(R8), R2
	MOV	XLEN(R8), R1
	MOV	R10, R8
	RET
