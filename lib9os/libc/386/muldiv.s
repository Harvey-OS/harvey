TEXT	umuldiv(SB), $0
	MOVL	a+0(FP), AX
	MULL	b+4(FP)
	DIVL	c+8(FP)
	RET

TEXT	muldiv(SB), $0
	MOVL	a+0(FP), AX
	IMULL	b+4(FP)
	IDIVL	c+8(FP)
	RET
	END
