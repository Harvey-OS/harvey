#define	Bxx	BE

	TEXT	memcmp(SB), $0

/*
 * performance:
 *	(tba)
 */

MOVW	R7, 0(FP)
	MOVW	n+8(FP), R9		/* R9 is count */
	MOVW	s1+0(FP), R10		/* R10 is pointer1 */
	MOVW	s2+4(FP), R11		/* R11 is pointer2 */
	ADD	R9,R10, R12		/* R12 is end pointer1 */

/*
 * if not at least 4 chars,
 * dont even mess around.
 * 3 chars to guarantee any
 * rounding up to a word
 * boundary and 4 characters
 * to get at least maybe one
 * full word cmp.
 */
	SUBCC	$4,R9, R0
	BL	out

/*
 * test if both pointers
 * are similarly word alligned
 */
	XOR	R10,R11, R7
	ANDCC	$3,R7, R0
	BNE	out

/*
 * byte at a time to word allign
 */
l1:
	ANDCC	$3,R10, R0
	BE	l2
	MOVB	0(R10), R16
	MOVB	0(R11), R17
	ADD	$1, R10
	SUBCC	R16,R17, R0
	BNE	ne
	ADD	$1, R11
	JMP	l1

/*
 * turn R9 into end pointer1-15
 * cmp 16 at a time while theres room
 */
l2:
	SUB	$15,R12, R9
l3:
	SUBCC	R10,R9, R0
	BLEU	l4
	MOVW	0(R10), R16
	MOVW	0(R11), R17
	MOVW	4(R10), R18
	SUBCC	R16,R17, R0
	BNE	ne
	MOVW	4(R11), R19
	MOVW	8(R10), R16
	SUBCC	R18,R19, R0
	BNE	ne
	MOVW	8(R11), R17
	MOVW	12(R10), R18
	SUBCC	R16,R17, R0
	BNE	ne
	MOVW	12(R11), R19
	ADD	$16, R10
	SUBCC	R18,R19, R0
	BNE	ne
	SUBCC	R16,R17, R0
	BNE	ne
	ADD	$16, R11
	JMP	l3

/*
 * turn R9 into end pointer1-3
 * cmp 4 at a time while theres room
 */
l4:
	SUB	$3,R12, R9
l5:
	SUBCC	R10,R9, R0
	BLEU	out
	MOVW	0(R10), R16
	MOVW	0(R11), R17
	ADD	$4, R10
	SUBCC	R16,R17, R0		/* only works because big endian */
	BNE	ne
	ADD	$4, R11
	JMP	l5

/*
 * last loop, cmp byte at a time
 */
out:
	SUBCC	R10,R12, R0
	BE	zero
	MOVB	0(R10), R16
	MOVB	0(R11), R17
	ADD	$1, R10
	SUBCC	R16,R17, R0
	BNE	ne
	ADD	$1, R11
	JMP	out

ne:
	BG	plus
	MOVW	$1, R7
	RETURN
plus:
	MOVW	$-1, R7
	RETURN

zero:
	MOVW	R0, R7
	RETURN
