TEXT	strcpy(SB), $0

	MOVW	s2+XLEN(FP),R9		/* R9 is from pointer */
	MOVW	R8, R10			/* R10 is to pointer */

/*
 * align 'from' pointer
 */
l1:
	AND	$3, R9, R12
	ADD	$1, R9
	BEQ	R12, l2
	MOVB	-1(R9), R12
	ADD	$1, R10
	MOVB	R12, -1(R10)
	BNE	R12, l1
	RET

/*
 * test if 'to' is also aligned
 */
l2:
	AND	$3,R10, R12
	BEQ	R12, l4

/*
 * copy 4 at a time, 'to' not aligned
 */
l3:
	MOVW	-1(R9), R11
	ADD	$4, R9
	ADD	$4, R10
	MOVB	R11, -4(R10)
	AND	$0xff, R11, R12
	BEQ	R12, out

	SRL	$8, R11
	MOVB	R11, -3(R10)
	AND	$0xff, R11, R12
	BEQ	R12, out

	SRL	$8, R11
	MOVB	R11, -2(R10)
	AND	$0xff, R11, R12
	BEQ	R12, out

	SRL	$8, R11
	MOVB	R11, -1(R10)
	BNE	R11, l3

out:
	RET

/*
 * word at a time both aligned
 */
l4:
	MOVW	$0xff000000, R14
	MOVW	$0x00ff0000, R15
	MOVW	$0x0000ff00, R13

l5:
	ADD	$4, R10
	MOVW	-1(R9), R11	/* fetch */
	ADD	$4, R9

	AND	$0xff, R11, R12	/* is it byte 0 */
	BEQ	R12, b0
	AND	R13, R11, R12	/* is it byte 1 */
	BEQ	R12, b1
	AND	R15, R11, R12	/* is it byte 2 */
	BEQ	R12, b2
	MOVW	R11, -4(R10)	/* store */
	AND	R14, R11, R12	/* is it byte 3 */
	BNE	R12, l5
	JMP	out

b0:
	MOVB	R0, -4(R10)
	JMP	out

b1:
	MOVB	R11, -4(R10)
	MOVB	R0, -3(R10)
	JMP	out

b2:
	MOVB	R11, -4(R10)
	SRL	$8, R11
	MOVB	R11, -3(R10)
	MOVB	R0, -2(R10)
	JMP	out
