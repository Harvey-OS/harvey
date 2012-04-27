	TEXT	memmove(SB), $0
	JMP	move

	TEXT	memcpy(SB), $0
move:

/*
 * performance:
 * (tba)
 */

	MOVW	R7, s1+0(FP)
	MOVW	n+8(FP), R9		/* R9 is count */
	MOVW	R7, R10			/* R10 is to-pointer */
	SUBCC	R0,R9, R0
	BGE	ok
	MOVW	0(R0), R0

ok:
	MOVW	s2+4(FP), R11		/* R11 is from-pointer */
	ADD	R9,R11, R13		/* R13 is end from-pointer */
	ADD	R9,R10, R12		/* R12 is end to-pointer */

/*
 * easiest test is copy backwards if
 * destination string has higher mem address
 */
	SUBCC	R11,R10, R0
	BGU	back

/*
 * if not at least 8 chars,
 * dont even mess around.
 * 7 chars to guarantee any
 * rounding up to a word
 * boundary and 8 characters
 * to get at least maybe one
 * full word store.
 */
	SUBCC	$8,R9, R0
	BL	fout

/*
 * test if both pointers
 * are similarly word aligned
 */
	XOR	R10,R11, R7
	ANDCC	$7,R7, R0
	BNE	fout

/*
 * byte at a time to double align
 */
f1:
	ANDCC	$7,R10, R0
	BE	f2
	MOVB	0(R11), R16
	ADD	$1, R11
	MOVB	R16, 0(R10)
	ADD	$1, R10
	JMP	f1

/*
 * turn R9 into to-end pointer-15
 * copy 16 at a time while theres room.
 * R12 is smaller than R13 --
 * there are problems if R13 is 0.
 */
f2:
	SUB	$15,R12, R9
f3:
	SUBCC	R10,R9, R0
	BLEU	f4
	MOVD	0(R11), R16
	MOVD	R16, 0(R10)
	MOVD	8(R11), R16
	ADD	$16, R11
	MOVD	R16, 8(R10)
	ADD	$16, R10
	JMP	f3

/*
 * turn R9 into to-end pointer-3
 * copy 4 at a time while theres room
 */
f4:
	SUB	$3,R12, R9
f5:
	SUBCC	R10,R9, R0
	BLEU	fout
	MOVW	0(R11), R16
	ADD	$4, R11
	MOVW	R16, 0(R10)
	ADD	$4, R10
	JMP	f5

/*
 * last loop, copy byte at a time
 */
fout:
	SUBCC	R11,R13, R0
	BLEU	ret
	MOVB	0(R11), R16
	ADD	$1, R11
	MOVB	R16, 0(R10)
	ADD	$1, R10
	JMP	fout

/*
 * whole thing repeated for backwards
 */
back:
	SUBCC	$8,R9, R0 
	BL	bout

	XOR	R12,R13, R7
	ANDCC	$7,R7, R0
	BNE	bout
b1:
	ANDCC	$7,R13, R0
	BE	b2
	MOVB	-1(R13), R16
	SUB	$1, R13
	MOVB	R16, -1(R12)
	SUB	$1, R12
	JMP	b1
b2:
	ADD	$15,R11, R9
b3:
	SUBCC	R9,R13, R0
	BLEU	b4

	MOVD	-8(R13), R16
	MOVD	R16, -8(R12)
	MOVD	-16(R13), R16
	SUB	$16, R13
	MOVD	R16, -16(R12);
	SUB	$16, R12
	JMP	b3
b4:
	ADD	$3,R11, R9
b5:
	SUBCC	R9,R13, R0
	BLEU	bout
	MOVW	-4(R13), R16
	SUB	$4, R13
	MOVW	R16, -4(R12)
	SUB	$4, R12
	JMP	b5

bout:
	SUBCC	R11,R13, R0
	BLEU	ret
	MOVB	-1(R13), R16
	SUB	$1, R13
	MOVB	R16, -1(R12)
	SUB	$1, R12
	JMP	bout

ret:
	MOVW	s1+0(FP), R7
	RETURN
