Q	= 0
N	= 1
D	= 2
CC	= 3
TMP	= 11

TEXT	save<>(SB), 1, $0
	MOVW	R(Q), 0(FP)
	MOVW	R(N), 4(FP)
	MOVW	R(D), 8(FP)
	MOVW	R(CC), 12(FP)

	MOVW	R(TMP), R(Q)		/* numerator */
	MOVW	20(FP), R(D)		/* denominator */
	CMP	$0, R(D)
	BNE	s1
	MOVW	-1(R(D)), R(TMP)	/* divide by zero fault */
s1:	RET

TEXT	rest<>(SB), 1, $0
	MOVW	0(FP), R(Q)
	MOVW	4(FP), R(N)
	MOVW	8(FP), R(D)
	MOVW	12(FP), R(CC)
/*
 * return to caller
 * of rest<>
 */
	MOVW	0(R13), R14
	ADD	$20, R13
	B	(R14)

TEXT	div<>(SB), 1, $0
	MOVW	$32, R(CC)
/*
 * skip zeros 8-at-a-time
 */
e1:
	AND.S	$(0xff<<24),R(Q), R(N)
	BNE	e2
	SLL	$8, R(Q)
	SUB.S	$8, R(CC)
	BNE	e1
	RET
e2:
	MOVW	$0, R(N)

loop:
/*
 * shift R(N||Q) left one
 */
	SLL	$1, R(N)
	CMP	$0, R(Q)
	ORR.LT	$1, R(N)
	SLL	$1, R(Q)

/*
 * compare numerator to denominator
 * if less, subtract and set quotent bit
 */
	CMP	R(D), R(N)
	ORR.HS	$1, R(Q)
	SUB.HS	R(D), R(N)
	SUB.S	$1, R(CC)
	BNE	loop
	RET

TEXT	_div(SB), 1, $16
	BL	save<>(SB)
	CMP	$0, R(Q)
	BGE	d1
	RSB	$0, R(Q), R(Q)
	CMP	$0, R(D)
	BGE	d2
	RSB	$0, R(D), R(D)
d0:
	BL	div<>(SB)		/* none/both neg */
	MOVW	R(Q), R(TMP)
	B	out
d1:
	CMP	$0, R(D)
	BGE	d0
	RSB	$0, R(D), R(D)
d2:
	BL	div<>(SB)		/* one neg */
	RSB	$0, R(Q), R(TMP)
	B	out

TEXT	_mod(SB), 1, $16
	BL	save<>(SB)
	CMP	$0, R(D)
	RSB.LT	$0, R(D), R(D)
	CMP	$0, R(Q)
	BGE	m1
	RSB	$0, R(Q), R(Q)
	BL	div<>(SB)		/* neg numerator */
	RSB	$0, R(N), R(TMP)
	B	out
m1:
	BL	div<>(SB)		/* pos numerator */
	MOVW	R(N), R(TMP)
	B	out

TEXT	_divu(SB), 1, $16
	BL	save<>(SB)
	BL	div<>(SB)
	MOVW	R(Q), R(TMP)
	B	out

TEXT	_modu(SB), 1, $16
	BL	save<>(SB)
	BL	div<>(SB)
	MOVW	R(N), R(TMP)
	B	out

out:
	BL	rest<>(SB)
	B	out
