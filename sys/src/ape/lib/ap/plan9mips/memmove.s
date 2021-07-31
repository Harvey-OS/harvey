	TEXT	memmove(SB), $0

	JMP	move

	TEXT	memcpy(SB), $0
move:
	MOVW	R1, s1+0(FP)

	MOVW	n+8(FP), R3		/* R3 is count */
	MOVW	R1, R4			/* R4 is to-pointer */
	SGT	R0, R3, R5
	BEQ	R5, ok
	MOVW	(R0), R0		/* abort if negative count */
ok:
	MOVW	s2+4(FP), R5		/* R5 is from-pointer */
	ADDU	R3,R5, R7		/* R7 is end from-pointer */
	ADDU	R3,R4, R6		/* R6 is end to-pointer */

/*
 * easiest test is copy backwards if
 * destination string has higher mem address
 */
	SGT	$4,R3, R2
	SGTU	R4,R5, R1
	BNE	R1, back

/*
 * if not at least 4 chars,
 * dont even mess around.
 * 3 chars to guarantee any
 * rounding up to a word
 * boundary and 4 characters
 * to get at least maybe one
 * full word store.
 */
	BNE	R2, fout

/*
 * test if both pointers
 * are similarly word aligned
 */
	XOR	R4,R5, R1
	AND	$3, R1
	BNE	R1, fout

/*
 * byte at a time to word align
 */
f1:
	AND	$3,R4, R1
	BEQ	R1, f2
	MOVB	0(R5), R8
	ADDU	$1, R5
	MOVB	R8, 0(R4)
	ADDU	$1, R4
	JMP	f1

/*
 * turn R3 into to-end pointer-15
 * copy 16 at a time while theres room.
 * R6 is smaller than R7 --
 * there are problems if R7 is 0.
 */
f2:
	ADDU	$-15,R6, R3
f3:
	SGTU	R3,R4, R1
	BEQ	R1, f4
	MOVW	0(R5), R8
	MOVW	4(R5), R9
	MOVW	R8, 0(R4)
	MOVW	8(R5), R8
	MOVW	R9, 4(R4)
	MOVW	12(R5), R9
	ADDU	$16, R5
	MOVW	R8, 8(R4)
	MOVW	R9, 12(R4)
	ADDU	$16, R4
	JMP	f3

/*
 * turn R3 into to-end pointer-3
 * copy 4 at a time while theres room
 */
f4:
	ADDU	$-3,R6, R3
f5:
	SGTU	R3,R4, R1
	BEQ	R1, fout
	MOVW	0(R5), R8
	ADDU	$4, R5
	MOVW	R8, 0(R4)
	ADDU	$4, R4
	JMP	f5

/*
 * last loop, copy byte at a time
 */
fout:
	BEQ	R7,R5, ret
	MOVB	0(R5), R8
	ADDU	$1, R5
	MOVB	R8, 0(R4)
	ADDU	$1, R4
	JMP	fout

/*
 * whole thing repeated for backwards
 */
back:
	BNE	R2, bout
	XOR	R6,R7, R1
	AND	$3, R1
	BNE	R1, bout
b1:
	AND	$3,R7, R1
	BEQ	R1, b2
	MOVB	-1(R7), R8
	ADDU	$-1, R7
	MOVB	R8, -1(R6)
	ADDU	$-1, R6
	JMP	b1
b2:
	ADDU	$15,R5, R3
b3:
	SGTU	R7,R3, R1
	BEQ	R1, b4
	MOVW	-4(R7), R8
	MOVW	-8(R7), R9
	MOVW	R8, -4(R6)
	MOVW	-12(R7), R8
	MOVW	R9, -8(R6)
	MOVW	-16(R7), R9
	ADDU	$-16, R7
	MOVW	R8, -12(R6)
	MOVW	R9, -16(R6)
	ADDU	$-16, R6
	JMP	b3
b4:
	ADDU	$3,R5, R3
b5:
	SGTU	R7,R3, R1
	BEQ	R1, bout
	MOVW	-4(R7), R8
	ADDU	$-4, R7
	MOVW	R8, -4(R6)
	ADDU	$-4, R6
	JMP	b5

bout:
	BEQ	R7,R5, ret
	MOVB	-1(R7), R8
	ADDU	$-1, R7
	MOVB	R8, -1(R6)
	ADDU	$-1, R6
	JMP	bout

ret:
	MOVW	s1+0(FP), R1
	RET
	END
