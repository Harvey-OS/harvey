TEXT	asin(SB), $0
	FMOVD	a+0(FP), F0	/* a */
	FMOVD	F0, F0		/* a,a */
	FMULD	F0, F0		/* a*a,a */
	FLD1			/* 1,a*a,a */
	FSUBRDP	F0, F1		/* 1-a*a,a */

	FTST
	WAIT
	FSTSW	AX
	SAHF
	JLO	bad

	FSQRT			/* sqrt(1-a*a),a */
	FPATAN			/* atan2(sqrt(1-a*a),a) */
	RET

TEXT	acos(SB), $0
	FMOVD	a+0(FP), F0
	FMOVD	F0, F0
	FMULD	F0, F0
	FLD1
	FSUBRDP	F0, F1

	FTST
	WAIT
	FSTSW	AX
	SAHF
	JLO	bad

	FSQRT
	FXCHD	F0, F1		/* identical except this */
	FPATAN
	RET

bad:
	FMOVDP	F0, F0
	FMOVDP	F0, F0
	CALL	NaN(SB)
	RET
