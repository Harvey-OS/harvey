	TEXT	strchr(SB), $0

MOVW	R7, 0(FP)
	MOVB	c+7(FP), R10
	MOVW	s+0(FP), R9

	SUBCC	R0,R10, R0
	BE	l2

/*
 * char is not null
 */
l1:
	MOVB	(R9), R7
	ADD	$1, R9
	SUBCC	R0,R7, R0
	BE	ret
	SUBCC	R7,R10, R0
	BNE	l1
	JMP	rm1

/*
 * char is null
 * align to word
 */
l2:
	ANDCC	$3,R9, R0
	BE	l3
	MOVB	(R9), R7
	ADD	$1, R9
	SUBCC	R0,R7, R0
	BNE	l2
	JMP	rm1

/*
 * develop byte masks
 */
l3:
	MOVW	$0xff, R17
	SLL	$8,R17, R16
	SLL	$16,R17, R13
	SLL	$24,R17, R12

l4:
	MOVW	(R9), R11
	ADD	$4, R9
	ANDCC	R12,R11, R0
	BE	b0
	ANDCC	R13,R11, R0
	BE	b1
	ANDCC	R16,R11, R0
	BE	b2
	ANDCC	R17,R11, R0
	BNE	l4

rm1:
	SUB	$1,R9, R7
	JMP	ret

b2:
	SUB	$2,R9, R7
	JMP	ret

b1:
	SUB	$3,R9, R7
	JMP	ret

b0:
	SUB	$4,R9, R7
	JMP	ret

ret:
	RETURN
