	TEXT	memccpy(SB), $0
MOVW R1, 0(FP)
	MOVW	n+12(FP), R1
	BEQ	R1, ret
	MOVW	s1+0(FP), R3
	MOVW	s2+4(FP), R2
	MOVBU	c+11(FP), R4
	ADDU	R1, R2, R5

l1:	MOVBU	(R2), R6
	ADDU	$1, R2
	MOVBU	R6, (R3)
	ADDU	$1, R3
	BEQ	R4, R6, eq
	BNE	R2, R5, l1
	MOVW	$0, R1
	RET

eq:	MOVW	R3, R1
ret:	RET
