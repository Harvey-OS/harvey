	TEXT	memset(SB),$0

/*
 * performance:
 *	(tba)
 */

MOVW	R7, 0(FP)
	MOVW	n+8(FP), R9		/* R9 is count */
	MOVW	p+0(FP), R10		/* R10 is pointer */
	MOVW	c+4(FP), R11		/* R11 is char */
	ADD	R9,R10, R12		/* R12 is end pointer */

/*
 * if not at least 4 chars,
 * dont even mess around.
 * 3 chars to guarantee any
 * rounding up to a word
 * boundary and 4 characters
 * to get at least maybe one
 * full word store.
 */
	SUBCC	$4,R9, R0
	BL	out

/*
 * turn R11 into a word of characters
 */
	AND	$0xff, R11
	SLL	$8,R11, R7
	OR	R7, R11
	SLL	$16,R11, R7
	OR	R7, R11

/*
 * store one byte at a time until pointer
 * is alligned on a word boundary
 */
l1:
	ANDCC	$3,R10, R0
	BE	l2
	MOVB	R11, 0(R10)
	ADD	$1, R10
	JMP	l1

/*
 * turn R9 into end pointer-15
 * store 16 at a time while theres room
 */
l2:
	ADD	$-15,R12, R9
	SUBCC	R10,R9, R0
	BLEU	l4
l3:
	MOVW	R11, 0(R10)
	MOVW	R11, 4(R10)
	ADD	$16, R10
	SUBCC	R10,R9, R0
	MOVW	R11, -8(R10)
	MOVW	R11, -4(R10)
	BGU	l3

/*
 * turn R9 into end pointer-3
 * store 4 at a time while theres room
 */
l4:
	ADD	$-3,R12, R9
l5:
	SUBCC	R10,R9, R0
	BLEU	out
	MOVW	R11, 0(R10)
	ADD	$4, R10
	JMP	l5

/*
 * last loop, store byte at a time
 */
out:
	SUBCC	R10,R12, R0
	BLEU	ret
	MOVB	R11, 0(R10)
	ADD	$1, R10
	JMP	out

ret:
	MOVW	s1+0(FP), R7
	RETURN
