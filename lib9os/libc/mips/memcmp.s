	TEXT	memcmp(SB), $0
MOVW R1, 0(FP)

/*
 * performance:
 *	alligned about 1.0us/call and 17.4mb/sec
 *	unalligned is about 3.1mb/sec
 */

	MOVW	n+8(FP), R3		/* R3 is count */
	MOVW	s1+0(FP), R4		/* R4 is pointer1 */
	MOVW	s2+4(FP), R5		/* R5 is pointer2 */
	ADDU	R3,R4, R6		/* R6 is end pointer1 */

/*
 * if not at least 4 chars,
 * dont even mess around.
 * 3 chars to guarantee any
 * rounding up to a word
 * boundary and 4 characters
 * to get at least maybe one
 * full word cmp.
 */
	SGT	$4,R3, R1
	BNE	R1, out

/*
 * test if both pointers
 * are similarly word alligned
 */
	XOR	R4,R5, R1
	AND	$3, R1
	BNE	R1, out

/*
 * byte at a time to word allign
 */
l1:
	AND	$3,R4, R1
	BEQ	R1, l2
	MOVBU	0(R4), R8
	MOVBU	0(R5), R9
	ADDU	$1, R4
	BNE	R8,R9, ne
	ADDU	$1, R5
	JMP	l1

/*
 * turn R3 into end pointer1-15
 * cmp 16 at a time while theres room
 */
l2:
	ADDU	$-15,R6, R3
l3:
	SGTU	R3,R4, R1
	BEQ	R1, l4
	MOVW	0(R4), R8
	MOVW	0(R5), R9
	MOVW	4(R4), R10
	BNE	R8,R9, ne
	MOVW	4(R5), R11
	MOVW	8(R4), R8
	BNE	R10,R11, ne1
	MOVW	8(R5), R9
	MOVW	12(R4), R10
	BNE	R8,R9, ne
	MOVW	12(R5), R11
	ADDU	$16, R4
	BNE	R10,R11, ne1
	BNE	R8,R9, ne
	ADDU	$16, R5
	JMP	l3

/*
 * turn R3 into end pointer1-3
 * cmp 4 at a time while theres room
 */
l4:
	ADDU	$-3,R6, R3
l5:
	SGTU	R3,R4, R1
	BEQ	R1, out
	MOVW	0(R4), R8
	MOVW	0(R5), R9
	ADDU	$4, R4
	BNE	R8,R9, ne	/* only works because big endian */
	ADDU	$4, R5
	JMP	l5

/*
 * last loop, cmp byte at a time
 */
out:
	SGTU	R6,R4, R1
	BEQ	R1, ret
	MOVBU	0(R4), R8
	MOVBU	0(R5), R9
	ADDU	$1, R4
	BNE	R8,R9, ne
	ADDU	$1, R5
	JMP	out

ne1:
	SGTU	R10,R11, R1
	BNE	R1, ret
	MOVW	$-1,R1
	RET
ne:
	SGTU	R8,R9, R1
	BNE	R1, ret
	MOVW	$-1,R1
ret:
	RET
	END
