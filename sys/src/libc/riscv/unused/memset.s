	TEXT	memset(SB),$12
MOVW R8, s1+0(FP)

	MOVW	n+(2*XLEN)(FP), R10	/* R10 is count */
	MOVW	p+0(FP), R11		/* R11 is pointer */
	MOVW	c+XLEN(FP), R12		/* R12 is char */
	ADD	R10,R11, R13		/* R13 is end pointer */

/*
 * if not at least 4 chars,
 * dont even mess around.
 * 3 chars to guarantee any
 * rounding up to a word
 * boundary and 4 characters
 * to get at least maybe one
 * full word store.
 */
	SLT	$4,R10, R8
	BNE	R8, out

/*
 * turn R12 into a word of characters
 */
	AND	$0xff, R12
	SLL	$8,R12, R8
	OR	R8, R12
	SLL	$16,R12, R8
	OR	R8, R12

/*
 * store one byte at a time until pointer
 * is aligned on a word boundary
 */
l1:
	AND	$3,R11, R8
	BEQ	R8, l2
	MOVB	R12, 0(R11)
	ADD	$1, R11
	JMP	l1

/*
 * turn R10 into end pointer-15
 * store 16 at a time while theres room
 */
l2:
	ADD	$-15,R13, R10
l3:
	SLTU	R10,R11, R8
	BEQ	R8, l4
	MOVW	R12, 0(R11)
	MOVW	R12, 4(R11)
	ADD	$16, R11
	MOVW	R12, -8(R11)
	MOVW	R12, -4(R11)
	JMP	l3

/*
 * turn R10 into end pointer-3
 * store 4 at a time while theres room
 */
l4:
	ADD	$-3,R13, R10
l5:
	SLTU	R10,R11, R8
	BEQ	R8, out
	MOVW	R12, 0(R11)
	ADD	$4, R11
	JMP	l5

/*
 * last loop, store byte at a time
 */
out:
	SLTU	R13,R11 ,R8
	BEQ	R8, ret
	MOVB	R12, 0(R11)
	ADD	$1, R11
	JMP	out

ret:
	MOVW	s1+0(FP), R8
	RET
	END
