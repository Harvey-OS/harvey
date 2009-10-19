TEXT	ainc(SB),$-8	/* long ainc(long *); */
	MOVQ	R0, R1			/* p */
inc1:
	MOVLL	(R1), R2		/* *p */
	ADDL	$1, R2
	MOVQ	R2, R0			/* copy to return */
	MOVLC	R2, (R1)		/* (*p)++ */
	BEQ	R2, inc1		/* write failed, retry */
	RET

TEXT	adec(SB),$-8	/* long ainc(long *); */
	MOVQ	R0, R1			/* p */
dec1:
	MOVLL	(R1), R2		/* *p */
	SUBL	$1, R2
	MOVQ	R2, R0			/* copy to return */
	MOVLC	R2, (R1)		/* (*p)++ */
	BEQ	R2, dec1		/* write failed, retry */
	RET

TEXT	cas(SB), $-8
TEXT	cas32(SB), $-8
TEXT	casl(SB), $-8
TEXT	casp(SB), $-8
	MOVQ	R0, R1	/* p */
	MOVL	old+4(FP), R2
	MOVL	new+8(FP), R3
	MOVLL	(R1), R0
	CMPEQ	R0, R2, R4
	BEQ	R4, fail	/* if R0 != [sic] R2, goto fail */
	MOVQ	R3, R0
	MOVLC	R0, (R1)
	RET
fail:
	MOVL	$0, R0
	RET

TEXT	loadlink(SB), $-8
	MOVLL	(R0), R0
	RET

TEXT	storecond(SB), $-8
	MOVW	val+4(FP), R1
	MOVLC	R1, (R0)
	BEQ	R1, storecondfail	/* write failed */
	MOVW	$1, R0
	RET
storecondfail:
	MOVW	$0, R0
	RET
