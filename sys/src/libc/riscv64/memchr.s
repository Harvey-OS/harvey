/* void* memchr(void *s, int c, ulong n) */
	TEXT	memchr(SB), $0
	MOV	R8, 0(FP)
	MOVW	n+(XLEN+4)(FP), R8
	MOV	s+0(FP), R9
	MOVBU	c+XLEN(FP), R10
	ADD	R8, R9, R13

	AND	$(~1), R8, R12
	ADD	R9, R12
	BEQ	R9, R12, lt2

l1:
	MOVBU	0(R9), R11
	MOVBU	1(R9), R14
	BEQ	R10, R11, eq0
	ADD	$2, R9
	BEQ	R10, R14, eq
	BNE	R9, R12, l1

lt2:
	BEQ	R9, R13, zret

l2:
	MOVBU	(R9), R11
	ADD	$1, R9
	BEQ	R10, R11, eq
	BNE	R9, R13, l2
zret:
	MOV	R0, R8
	RET

eq0:
	MOV	R9, R8
	RET

eq:
	SUB	$1, R9, R8
	RET
