
TEXT	setfcr(SB), $4
	MOVW	R0, FPCR
	RET

TEXT	getfcr(SB), $4
	MOVW	FPCR, R0
	RET

TEXT	getfsr(SB), $0
	MOVW	FPSR, R0
	RET

TEXT	setfsr(SB), $0
	MOVW	R0, FPSR
	RET
