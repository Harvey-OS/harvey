	TEXT	strchr(SB), $0
	MOVBU	c+XLEN(FP), R11
	MOVW	R8, R10

	BEQ	R11, l2

/*
 * char is not null
 */
l1:
	MOVBU	(R10), R8
	ADD	$1, R10
	BEQ	R8, ret
	BNE	R8,R11, l1
	JMP	rm1

/*
 * char is null
 * align to word
 */
l2:
	AND	$3,R10, R8
	BEQ	R8, l3
	MOVBU	(R10), R8
	ADD	$1, R10
	BNE	R8, l2
	JMP	rm1

l3:
	MOVW	$0xff000000, R13
	MOVW	$0x00ff0000, R14
	MOVW	$0x0000ff00, R15

l4:
	MOVW	(R10), R12
	ADD	$4, R10
	AND	$0xff, R12, R8
	BEQ	R8, b0
	AND	R15, R12, R8
	BEQ	R8, b1
	AND	R14, R12, R8
	BEQ	R8, b2
	AND	R13, R12, R8
	BNE	R8, l4

rm1:
	ADD	$-1,R10, R8
	JMP	ret

b2:
	ADD	$-2,R10, R8
	JMP	ret

b1:
	ADD	$-3,R10, R8
	JMP	ret

b0:
	ADD	$-4,R10, R8
	JMP	ret

ret:
	RET
