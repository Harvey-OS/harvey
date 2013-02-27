	TEXT	memchr(SB), $0
MOVW R1, 0(FP)

	MOVW	n+8(FP), R1
	MOVW	s1+0(FP), R2
	MOVBU	c+7(FP), R3
	ADDU	R1, R2, R6

	AND	$(~1), R1, R5
	ADDU	R2, R5
	BEQ	R2, R5, lt2

l1:
	MOVBU	0(R2), R4
	MOVBU	1(R2), R7
	BEQ	R3, R4, eq0
	ADDU	$2, R2
	BEQ	R3, R7, eq
	BNE	R2, R5, l1

lt2:
	BEQ	R2, R6, zret

l2:
	MOVBU	(R2), R4
	ADDU	$1, R2
	BEQ	R3, R4, eq
	BNE	R2, R6, l2
zret:
	MOVW	R0, R1
	RET

eq0:
	MOVW	R2, R1
	RET

eq:
	SUBU	$1,R2, R1
	RET
