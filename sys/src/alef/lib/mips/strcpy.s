	TEXT	strcpy(SB),$0

	MOVW	s1+0(FP),R3		/* R3 is to pointer */
	MOVW	s2+4(FP),R4		/* R4 is from pointer */

/*
 * test if both pointers
 * are similarly word aligned
 */
	XOR	R3,R4, R1
	AND	$3, R1
	BNE	R1, una

	MOVW	$0xff000000, R6
	MOVW	$0x00ff0000, R7

/*
 * byte at a time to word align
 */
al1:
	AND	$3,R4, R1
	BEQ	R1, al2
	MOVB	(R4), R5
	ADDU	$1, R4
	MOVB	R5, (R3)
	ADDU	$1, R3
	BNE	R5, al1
	JMP	out

/*
 * word at a time
 */
al2:
	ADDU	$4, R3
	MOVW	(R4), R5	/* fetch */

	ADDU	$4, R4
	AND	R6,R5, R1	/* is it byte 0 */
	AND	R7,R5, R2	/* is it byte 1 */
	BEQ	R1, b0


	AND	$0xff00,R5, R1	/* is it byte 2 */
	BEQ	R2, b1

	AND	$0xff,R5, R2	/* is it byte 3 */
	BEQ	R1, b2

	MOVW	R5, -4(R3)	/* store */
	BNE	R2, al2

	JMP	out

b0:
	MOVB	$0, -4(R3)
	JMP	out

b1:
	SRL	$24, R5
	MOVB	R5, -4(R3)
	MOVB	$0, -3(R3)
	JMP	out

b2:
	SRL	$24,R5, R1
	MOVB	R1, -4(R3)
	SRL	$16, R5
	MOVB	R5, -3(R3)
	MOVB	$0, -2(R3)
	JMP	out

una:
	MOVB	(R4), R5
	ADDU	$1, R4
	MOVB	R5, (R3)
	ADDU	$1, R3
	BNE	R5, una

out:
	MOVW	s1+0(FP),R1
	RET
