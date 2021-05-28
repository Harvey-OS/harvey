	TEXT	memccpy(SB), $0
MOVV R1, 0(FP)
	MOVW	n+24(FP), R1
	BEQ	R1, ret
	MOVV	s1+0(FP), R3
	MOVV	s2+8(FP), R2
	MOVBU	c+16(FP), R4	/* little endian */
	ADDVU	R1, R2, R5

l1:	MOVBU	(R2), R6
	ADDVU	$1, R2
	MOVBU	R6, (R3)
	ADDVU	$1, R3
	BEQ	R4, R6, eq
	BNE	R2, R5, l1
	MOVV	$0, R1
	RET

eq:	MOVV	R3, R1
ret:	RET
