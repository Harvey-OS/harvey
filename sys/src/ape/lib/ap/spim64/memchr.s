	TEXT	memchr(SB), $0
MOVV R1, 0(FP)

	MOVW	n+16(FP), R1
	MOVV	s1+0(FP), R2
	MOVBU	c+8(FP), R3
	ADDVU	R1, R2, R6

	AND	$(~1), R1, R5
	ADDVU	R2, R5
	BEQ	R2, R5, lt2

l1:
	MOVBU	0(R2), R4
	MOVBU	1(R2), R7
	BEQ	R3, R4, eq0
	ADDVU	$2, R2
	BEQ	R3, R7, eq
	BNE	R2, R5, l1

lt2:
	BEQ	R2, R6, zret

l2:
	MOVBU	(R2), R4
	ADDVU	$1, R2
	BEQ	R3, R4, eq
	BNE	R2, R6, l2
zret:
	MOVV	R0, R1
	RET

eq0:
	MOVV	R2, R1
	RET

eq:
	SUBVU	$1,R2, R1
	RET
