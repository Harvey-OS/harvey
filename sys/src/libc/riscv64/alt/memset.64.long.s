/* memset(void *p, int c, long n) - clear vlongs */
	TEXT	memset(SB),$12
	MOV	R8, p+0(FP)
	MOV	R8, R11			/* R11 is pointer */
	MOVWU	c+XLEN(FP), R12		/* R12 is char */
	MOVW	n+(XLEN+4)(FP), R10	/* R10 is count */
	ADD	R10,R11, R13		/* R13 is end pointer */

/*
 * if not at least 8 chars,
 * dont even mess around.
 * 7 chars to guarantee any
 * rounding up to a doubleword
 * boundary and 8 characters
 * to get at least maybe one
 * full doubleword store.
 */
	SLT	$8,R10, R8
	BNE	R8, out

/*
 * turn R12 into a doubleword of characters
 */
	AND	$0xff, R12
	SLL	$8,R12, R8
	OR	R8, R12
	SLL	$16,R12, R8
	OR	R8, R12
	SLL	$32,R12, R8
	OR	R8, R12

/*
 * store one byte at a time until pointer
 * is aligned on a doubleword boundary
 */
l1:
	AND	$7,R11, R8
	BEQ	R8, l2
	MOVB	R12, 0(R11)
	ADD	$1, R11
	JMP	l1

/*
 * turn R10 into end pointer-31
 * store 32 at a time while theres room
 */
l2:
	ADD	$-31,R13, R10
l3:
	SLTU	R10,R11, R8
	BEQ	R8, l4
	MOV	R12, 0(R11)
	MOV	R12, 8(R11)
	ADD	$32, R11
	MOV	R12, -16(R11)
	MOV	R12, -8(R11)
	JMP	l3

/*
 * turn R10 into end pointer-7
 * store 8 at a time while theres room
 */
l4:
	ADD	$-7,R13, R10
l5:
	SLTU	R10,R11, R8
	BEQ	R8, out
	MOV	R12, 0(R11)
	ADD	$8, R11
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
	MOV	s1+0(FP), R8
	RET
	END
