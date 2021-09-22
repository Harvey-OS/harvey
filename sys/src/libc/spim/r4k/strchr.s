	TEXT	strchr(SB), $0
	MOVBU	c+4(FP), R4		/* character */
	MOVW	R1, R3			/* pointer */

	BEQ	R4, l2

/*
 * char is not null
 * align to word
 */
n2:
	AND	$7,R3, R1
	BEQ	R1, n3
	MOVBU	(R3), R1
	ADDU	$1, R3
	BEQ	R1, ret
	BNE	R1,R4, n2
	JMP	rm1

n3:
	MOVV	(R3), R5
	ADDU	$8, R3

	AND	$0xff,R5, R1
	BEQ	R1, ret
	BEQ	R1,R4, b0

	SRLV	$8,R5, R1
	AND	$0xff, R1
	BEQ	R1, ret
	BEQ	R1,R4, b1

	SRLV	$16,R5, R1
	AND	$0xff, R1
	BEQ	R1, ret
	BEQ	R1,R4, b2

	SRLV	$24,R5, R1
	AND	$0xff, R1
	BEQ	R1, ret
	BEQ	R1,R4, b3

	SRLV	$32,R5, R1
	AND	$0xff, R1
	BEQ	R1, ret
	BEQ	R1,R4, b4

	SRLV	$40,R5, R1
	AND	$0xff, R1
	BEQ	R1, ret
	BEQ	R1,R4, b5

	SRLV	$48,R5, R1
	AND	$0xff, R1
	BEQ	R1, ret
	BEQ	R1,R4, b6

	SRLV	$56,R5, R1
	BEQ	R1, ret
	BNE	R1,R4, n3
	JMP	rm1

/*
 * char is null
 * align to word
 */
l2:
	AND	$7,R3, R1
	BEQ	R1, l3
	MOVBU	(R3), R1
	ADDU	$1, R3
	BNE	R1, l2
	JMP	rm1

l3:
	MOVV	$0xff, R10
	SLLV	$8,R10, R11
	SLLV	$16,R10, R12
	SLLV	$24,R10, R13
	SLLV	$32,R10, R14
	SLLV	$40,R10, R15
	SLLV	$48,R10, R16
	SLLV	$56,R10, R17

l4:
	MOVV	(R3), R5
	ADDU	$8, R3
	AND	R10,R5, R1

	AND	R11,R5, R2
	BEQ	R1, b0
	AND	R12,R5, R1
	BEQ	R2, b1

	AND	R13,R5, R2
	BEQ	R1, b2
	AND	R14,R5, R1
	BEQ	R2, b3

	AND	R15,R5, R2
	BEQ	R1, b4
	AND	R16,R5, R1
	BEQ	R2, b5

	AND	R17,R5, R2
	BEQ	R1, b6
	BNE	R2, l4

rm1:	ADDU	$-1,R3, R1
	JMP	ret
b0:	ADDU	$-8,R3, R1
	JMP	ret
b1:	ADDU	$-7,R3, R1
	JMP	ret
b2:	ADDU	$-6,R3, R1
	JMP	ret
b3:	ADDU	$-5,R3, R1
	JMP	ret
b4:	ADDU	$-4,R3, R1
	JMP	ret
b5:	ADDU	$-3,R3, R1
	JMP	ret
b6:	ADDU	$-2,R3, R1
	JMP	ret

ret:
	RET
