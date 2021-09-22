	TEXT	memcmp(SB), $0
	MOVW 	R8, s1+0(FP)
	MOVW	n+(2*XLEN)(FP), R15	/* R15 is count */
	MOVW	s1+0(FP), R9		/* R9 is pointer1 */
	MOVW	s2+XLEN(FP), R10	/* R10 is pointer2 */
	ADD	R15,R9, R11		/* R11 is end pointer1 */

/*
 * if not at least 4 chars,
 * dont even mess around.
 * 3 chars to guarantee any
 * rounding up to a word
 * boundary and 4 characters
 * to get at least maybe one
 * full word cmp.
 */
	SLT	$4,R15, R8
	BNE	R8, out

/*
 * test if both pointers
 * are similarly word aligned
 */
	XOR	R9,R10, R8
	AND	$3, R8
	BNE	R8, out

/*
 * byte at a time to word align
 */
l1:
	AND	$3,R9, R8
	BEQ	R8, l2
	MOVBU	0(R9), R14
	MOVBU	0(R10), R15
	ADD	$1, R9
	BNE	R14,R15, ne
	ADD	$1, R10
	JMP	l1

/*
 * turn R15 into end pointer1-15
 * cmp 16 at a time while theres room
 */
l2:
	ADD	$-15,R11, R15
l3:
	SLTU	R15,R9, R8
	BEQ	R8, l4
	MOVW	0(R9), R12
	MOVW	0(R10), R13
	BNE	R12,R13, ne1
	MOVW	4(R9), R12
	MOVW	4(R10), R13
	BNE	R12,R13, ne1
	MOVW	8(R9), R12
	MOVW	8(R10), R13
	BNE	R12,R13, ne1
	MOVW	12(R9), R12
	MOVW	12(R10), R13
	BNE	R12,R13, ne1
	ADD	$16, R9
	ADD	$16, R10
	JMP	l3

/*
 * turn R15 into end pointer1-3
 * cmp 4 at a time while theres room
 */
l4:
	ADD	$-3,R11, R15
l5:
	SLTU	R15,R9, R8
	BEQ	R8, out
	MOVW	0(R9), R12
	MOVW	0(R10), R13
	ADD	$4, R9
	BNE	R12,R13, ne1
	ADD	$4, R10
	JMP	l5

/*
 * last loop, cmp byte at a time
 */
out:
	SLTU	R11,R9, R8
	BEQ	R8, ret
	MOVBU	0(R9), R14
	MOVBU	0(R10), R15
	ADD	$1, R9
	BNE	R14,R15, ne
	ADD	$1, R10
	JMP	out

/*
 * compare bytes in R12 and R13, lsb first
 */
ne1:
	MOVW	$0xff, R8
ne1x:
	AND	R8, R12, R14
	AND	R8, R13, R15
	BNE	R14, R15, ne
	SLL	$8, R8
	BNE	R8, ne1x
	JMP	ret

ne:
	SLTU	R14,R15, R8
	BNE	R8, ret
	MOVW	$-1,R8
ret:
	RET
	END
