	TEXT	strcpy(SB), $0

MOVW	R7, 0(FP)
	MOVW	s1+0(FP), R9		/* R9 is to pointer */
	MOVW	s2+4(FP), R10		/* R10 is from pointer */

/*
 * test if both pointers
 * are similarly word aligned
 */
	XOR	R9,R10, R7
	ANDCC	$3,R7, R0
	BNE	una

/*
 * make byte masks
 */
	MOVW	$0xff, R17
	SLL	$8,R17, R16
	SLL	$16,R17, R13
	SLL	$24,R17, R12

/*
 * byte at a time to word align
 */
al1:
	ANDCC	$3,R10, R0
	BE	al2
	MOVB	(R10), R11
	ADD	$1, R10
	MOVB	R11, (R9)
	ADD	$1, R9
	SUBCC	R0,R11, R0
	BNE	al1
	JMP	out

/*
 * word at a time
 */
al2:
	ADD	$4, R9
	MOVW	(R10), R11	/* fetch */
	ADD	$4, R10
	ANDCC	R12,R11, R0	/* is it byte 0 */
	BE	b0
	ANDCC	R13,R11, R0	/* is it byte 1 */
	BE	b1
	ANDCC	R16,R11, R0	/* is it byte 2 */
	BE	b2
	MOVW	R11, -4(R9)	/* store */
	ANDCC	R17,R11, R0	/* is it byte 3 */
	BNE	al2

	JMP	out

b0:
	MOVB	R0, -4(R9)
	JMP	out

b1:
	SRL	$24, R11
	MOVB	R11, -4(R9)
	MOVB	R0, -3(R9)
	JMP	out

b2:
	SRL	$24,R11, R7
	MOVB	R7, -4(R9)
	SRL	$16, R11
	MOVB	R11, -3(R9)
	MOVB	R0, -2(R9)
	JMP	out

una:
	MOVB	(R10), R11
	ADD	$1, R10
	MOVB	R11, (R9)
	ADD	$1, R9
	SUBCC	R0,R11, R0
	BNE	una

out:
	MOVW	s1+0(FP),R7
	RETURN
