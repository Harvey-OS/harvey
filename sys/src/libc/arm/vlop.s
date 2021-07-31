arg=0

TEXT	_mulv(SB), $0
	MOVW	8(FP), R(arg+1)
	MOVW	4(FP), R(arg+2)
	MOVW	16(FP), R(arg+3)
	MOVW	12(FP), R(arg+4)
/*
	MULU	R(arg+3), R(arg+1)
	MOVL	LO, R62
	MOVL	HI, R7
	MULU	R(arg+2), R(arg+3)
	MOVL	LO, R74
	ADDU	R74, R7
	MULU	R(arg+1), R(arg+4)
	MOVL	LO, R74
	ADDU	R74, R73
*/
	MOVW	R0, 4(R(arg+0))
	MOVW	R0, 0(R(arg+0))
	RET
