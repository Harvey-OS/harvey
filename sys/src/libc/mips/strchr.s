	TEXT	strchr(SB), $0
MOVW R1, 0(FP)
	MOVB	c+7(FP), R4
	MOVW	s+0(FP), R3

	BEQ	R4, l2

/*
 * char is not null
 */
l1:
	MOVB	(R3), R1
	ADDU	$1, R3
	BEQ	R1, ret
	BNE	R1,R4, l1
	JMP	rm1

/*
 * char is null
 * align to word
 */
l2:
	AND	$3,R3, R1
	BEQ	R1, l3
	MOVB	(R3), R1
	ADDU	$1, R3
	BNE	R1, l2
	JMP	rm1

l3:
	MOVW	$0xff000000, R6
	MOVW	$0x00ff0000, R7

l4:
	MOVW	(R3), R5
	ADDU	$4, R3
	AND	R6,R5, R1
	AND	R7,R5, R2
	BEQ	R1, b0
	AND	$0xff00,R5, R1
	BEQ	R2, b1
	AND	$0xff,R5, R2
	BEQ	R1, b2
	BNE	R2, l4

rm1:
	ADDU	$-1,R3, R1
	JMP	ret

b2:
	ADDU	$-2,R3, R1
	JMP	ret

b1:
	ADDU	$-3,R3, R1
	JMP	ret

b0:
	ADDU	$-4,R3, R1
	JMP	ret

ret:
	RET
