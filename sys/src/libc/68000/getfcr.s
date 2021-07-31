TEXT	getfsr(SB), $0
	MOVL	$0, R0
	MOVL	FPSR, R0
	RTS

TEXT	setfsr(SB), $0
	MOVL	new+0(FP), R1
	MOVL	R1, FPSR
	RTS

TEXT	getfcr(SB), $0
	MOVL	$0, R0
	MOVL	FPCR, R0
	RTS

TEXT	setfcr(SB), $0
	MOVL	new+0(FP), R1
	MOVL	R1, FPCR
	RTS
