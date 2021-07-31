/*
 * R4000-compatible memset uses V instructions.
 * The kernel uses the 3000 compilers, but we can
 * use this procedure for speed.
 */
	TEXT	memset(SB),$16
	MOVW	R1, 0(FP)

	MOVW	n+8(FP), R3		/* R3 is count */
	MOVW	p+0(FP), R4		/* R4 is pointer */
	MOVW	c+4(FP), R5		/* R5 is char */
	ADDU	R3,R4, R6		/* R6 is end pointer */

/*
 * if not at least 8 chars,
 * dont even mess around.
 * 7 chars to guarantee any
 * rounding up to a word
 * boundary and 8 characters
 * to get at least maybe one
 * full word store.
 */
	SGT	$8,R3, R1
	BNE	R1, out

/*
 * turn R5 into a doubleword of characters
 */
	AND	$0xff, R5
	SLLV	$8,R5, R1
	OR	R1, R5
	SLLV	$16,R5, R1
	OR	R1, R5
	SLLV	$32,R5, R1
	OR	R1, R5

/*
 * store one byte at a time until pointer
 * is alligned on a doubleword boundary
 */
l1:
	AND	$7,R4, R1
	BEQ	R1, l2
	MOVB	R5, 0(R4)
	ADDU	$1, R4
	JMP	l1

/*
 * turn R3 into end pointer-31
 * store 32 at a time while theres room
 */
l2:
	ADDU	$-31,R6, R3
l3:
	SGTU	R3,R4, R1
	BEQ	R1, l4
	MOVV	R5, 0(R4)
	MOVV	R5, 8(R4)
	ADDU	$32, R4
	MOVV	R5, -16(R4)
	MOVV	R5, -8(R4)
	JMP	l3

/*
 * turn R3 into end pointer-3
 * store 4 at a time while theres room
 */
l4:
	ADDU	$-3,R6, R3
l5:
	SGTU	R3,R4, R1
	BEQ	R1, out
	MOVW	R5, 0(R4)
	ADDU	$4, R4
	JMP	l5

/*
 * last loop, store byte at a time
 */
out:
	SGTU	R6,R4 ,R1
	BEQ	R1, ret
	MOVB	R5, 0(R4)
	ADDU	$1, R4
	JMP	out

ret:
	MOVW	s1+0(FP), R1
	RET
	END
