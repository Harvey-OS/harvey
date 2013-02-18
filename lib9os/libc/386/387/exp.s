TEXT	exp(SB), $0
	FLDL2E
	FMULD	a+0(FP), F0	/* now we want 2^ this number */

	FMOVD	F0, F0		/* x, x */
	FRNDINT			/* ix, x -- this is best in round mode */
	FSUBD	F0, F1		/* ix, fx */
	FXCHD	F0, F1		/* fx, ix */
	F2XM1			/* 2^fx-1, ix */
	FADDD	$1.0, F0	/* 2^fx, ix */
	FSCALE			/* 2^(fx+ix), ix */
	FMOVDP	F0, F1		/* 2^(fx+ix) == 2^x */
	RET
