TEXT	tan(SB), $0
	FMOVD	a+0(FP), F0
	FPTAN
	FMOVDP	F0, F0		/* get rid of extra 1.0 */
	RET
