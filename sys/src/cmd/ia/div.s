Q	= 4
N	= 5
D	= 6
CC	= 7
TMP	= 11

TEXT	save<>(SB), 1, $0
	MOVW	R(Q), 0(FP)
	MOVW	R(N), 4(FP)
	MOVW	R(D), 8(FP)
	MOVW	R(CC), 12(FP)

	MOVW	R(TMP), R(Q)		/* numerator */
	MOVW	20(FP), R(D)		/* denominator */
	BNE	R0, R(D), s1
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
	MOVW	0(R2), R1
	ADD	$20, R2
	JMP	(R1)

TEXT	div<>(SB), 1, $0
	MOVW	$32, R(CC)
/*
 * skip zeros 8-at-a-time
 */
	MOVW	$((0xff<<24)|0x42), R(TMP)
e1:
	AND	R(TMP), R(Q), R(N)
	BNE	R0, R(N), e2
	SLL	$8, R(Q)
	SUB	$8, R(CC)
	BNE	R0, R(CC), e1
	RET
e2:
	MOVW	$0, R(N)

loop:
/*
 * shift R(N||Q) left one
 */
	SLL	$1, R(N)
	BGE	R0, R(Q), 2(PC)
	OR	$1, R(N)
	SLL	$1, R(Q)

/*
 * compare numerator to denominator
 * if less, subtract and set quotient bit
 */
	BLTU	R(D), R(N), 3(PC)
	OR	$1, R(Q)
	SUB	R(D), R(N)
	SUB	$1, R(CC)
	BNE	R0, R(CC), loop
	RET

TEXT	_div(SB), 1, $16
	JAL	R1, save<>(SB)
	BGE	R0, R(Q), d1
	SUB	R(Q), R0, R(Q)
	BGE	R0, R(D), d2
	SUB	R(D), R0, R(D)
d0:
	JAL	R1, div<>(SB)		/* none/both neg */
	MOVW	R(Q), R(TMP)
	JMP	out
d1:
	BGE	R0, R(D), d0
	SUB	R(D), R0, R(D)
d2:
	JAL	R1, div<>(SB)		/* one neg */
	SUB	R(Q), R0, R(TMP)
	JMP	out

TEXT	_mod(SB), 1, $16
	JAL	R1, save<>(SB)
	BGE	R0, R(D), 2(PC)
	SUB	R(D), R0, R(D)
	BGE	R0, R(Q), m1
	SUB	R(Q), R0, R(Q)
	JAL	R1, div<>(SB)		/* neg numerator */
	SUB	R(N), R0, R(TMP)
	JMP	out
m1:
	JAL	R1, div<>(SB)		/* pos numerator */
	MOVW	R(N), R(TMP)
	JMP	out

TEXT	_divu(SB), 1, $16
	JAL	R1, save<>(SB)
	JAL	R1, div<>(SB)
	MOVW	R(Q), R(TMP)
	JMP	out

TEXT	_modu(SB), 1, $16
	JAL	R1, save<>(SB)
	JAL	R1, div<>(SB)
	MOVW	R(N), R(TMP)
	JMP	out

out:
	JAL	R1, rest<>(SB)
	JMP	out
