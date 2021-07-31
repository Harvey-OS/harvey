TEXT	_xdec(SB), $-8
	MOVQ	R0, R1		/* p */
dec1:
	MOVLL	(R1), R0		/* *p */
	SUBL		$1, R0
	MOVQ	R0, R2
	MOVLC	R2, (R1)		/* --(*p) */
	BEQ		R2, dec1		/* write failed, retry */
	RET

TEXT	_xinc(SB), $-8
	MOVQ	R0, R1		/* p */
inc1:
	MOVLL	(R1), R0		/* *p */
	ADDL	$1, R0
	MOVLC	R0, (R1)		/* (*p)++ */
	BEQ		R0, inc1		/* write failed, retry */
	RET

