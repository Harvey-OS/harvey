TEXT	strcpy(SB), $0

	MOVW	f+4(FP),R2		/* R2 is from pointer */
	MOVW	R1, R3			/* R3 is to pointer */

/*
 * align 'from' pointer
 */
l1:
	AND	$7,R2, R5
	ADDU	$1, R2
	BEQ	R5, l2
	MOVBU	-1(R2), R5
	ADDU	$1, R3
	MOVB	R5, -1(R3)
	BNE	R5, l1
	RET

/*
 * test if 'to' is also alligned
 */
l2:
	AND	$7,R3, R5
	BEQ	R5, l4

/*
 * copy doubleword at a time
 * 'to' not aligned
 */
l3:
	MOVV	-1(R2), R4
	ADD	$8, R2
	ADD	$8, R3
	AND	$0xff,R4, R5
	MOVB	R5, -8(R3)
	BEQ	R5, out

	SRLV	$8,R4, R5
	AND	$0xff, R5
	MOVB	R5, -7(R3)
	BEQ	R5, out

	SRLV	$16,R4, R5
	AND	$0xff, R5
	MOVB	R5, -6(R3)
	BEQ	R5, out

	SRLV	$24,R4, R5
	AND	$0xff, R5
	MOVB	R5, -5(R3)
	BEQ	R5, out

	SRLV	$32,R4, R5
	AND	$0xff, R5
	MOVB	R5, -4(R3)
	BEQ	R5, out

	SRLV	$40,R4, R5
	AND	$0xff, R5
	MOVB	R5, -3(R3)
	BEQ	R5, out

	SRLV	$48,R4, R5
	AND	$0xff, R5
	MOVB	R5, -2(R3)
	BEQ	R5, out

	SRLV	$56,R4, R5
	MOVB	R5, -1(R3)
	BNE	R5, l3

out:
	RET

/*
 * word at a time both aligned
 */
l4:
	MOVV	$0xff, R10
	SLLV	$8,R10, R11
	SLLV	$16,R10, R12
	SLLV	$24,R10, R13
	SLLV	$32,R10, R14
	SLLV	$40,R10, R15
	SLLV	$48,R10, R16
	SLLV	$56,R10, R17

l5:
	ADDU	$8, R3
	MOVV	-1(R2), R4

	ADDU	$8, R2
	AND	R10,R4, R5
	AND	R11,R4, R6
	BEQ	R5, b1

	AND	R12,R4, R5
	BEQ	R6, b2

	AND	R13,R4, R6
	BEQ	R5, b3

	AND	R14,R4, R5
	BEQ	R6, b4

	AND	R15,R4, R6
	BEQ	R5, b5

	AND	R16,R4, R5
	BEQ	R6, b6

	AND	R17,R4, R6
	BEQ	R5, b7

	MOVV	R4, -8(R3)
	BNE	R6, l5
	JMP	out

b7:	/* store 7 bytes */
	SRLV	$48,R4, R5
	MOVB	R5, -2(R3)
b6:	/* store 6 bytes */
	SRLV	$40,R4, R5
	MOVB	R5, -3(R3)
b5:	/* store 5 bytes */
	SRLV	$32,R4, R5
	MOVB	R5, -4(R3)
b4:	/* store 4 bytes */
	SRLV	$24,R4, R5
	MOVB	R5, -5(R3)
b3:	/* store 3 bytes */
	SRLV	$16,R4, R5
	MOVB	R5, -6(R3)
b2:	/* store 2 bytes */
	SRLV	$8,R4, R5
	MOVB	R5, -7(R3)
b1:	/* store 1 bytes */
	AND	$0xff,R4, R5
	MOVB	R5, -8(R3)
	JMP	out
