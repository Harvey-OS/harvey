/*
 * R4000 double-word version.  Only works in kernel mode.
 * Requires low-core support to save R5 and R6 in a 64-bit hole.
 */

/*
 *  R4000 instructions
 */
#define	LD(offset, base, rt)		WORD	$((067<<26)|((base)<<21)|((rt)<<16)|((offset)&0xFFFF))
#define	STD(rt, offset, base)		WORD	$((077<<26)|((base)<<21)|((rt)<<16)|((offset)&0xFFFF))

	TEXT	memmove(SB), $0

	JMP	move

	TEXT	memcpy(SB), $0
move:
	MOVW	R1, s1+0(FP)

	MOVW	n+8(FP), R3		/* R3 is count */
	MOVW	R1, R4			/* R4 is to-pointer */
	SGT	R0, R3, R10
	BEQ	R10, ok
	MOVW	(R0), R0		/* abort if negative count */
ok:
	MOVW	s2+4(FP), R10		/* R10 is from-pointer */
	ADDU	R3,R10, R7		/* R7 is end from-pointer */
	ADDU	R3,R4, R11		/* R11 is end to-pointer */

/*
 * easiest test is copy backwards if
 * destination string has higher mem address
 */
	SGT	$8,R3, R2
	SGTU	R4,R10, R1
	BNE	R1, back

/*
 * if not at least 8 chars,
 * dont even mess around.
 * 7 chars to guarantee any
 * rounding up to a doubleword
 * boundary and 8 characters
 * to get at least maybe one
 * full doubleword store.
 */
	BNE	R2, fout

/*
 * test if both pointers
 * are similarly word aligned
 */
	XOR	R4,R10, R1
	AND	$3, R1
	BNE	R1, fout

/*
 * test if both pointers
 * are similarly double-word aligned
 */
	XOR	R4,R10, R1
	AND	$7, R1
	BNE	R1, f1

/*
 * double-word aligned; start by byte at a time until aligned
 */
fd1:
	AND	$7,R4, R1
	BEQ	R1, fd2
	MOVB	0(R10), R8
	ADDU	$1, R10
	MOVB	R8, 0(R4)
	ADDU	$1, R4
	JMP	fd1

/*
 * turn R3 into to-end pointer-31
 * copy 32 at a time while there's room.
 * R11 is smaller than R7 --
 * there are problems if R7 is 0.
 */
fd2:
	ADDU	$-31,R11, R3
fd3:
	SGTU	R3,R4, R1
	BEQ	R1, f4
	LD	(0,(10), 5)			/* MOVW	0(R10), R8 */
	LD	(8,(10), 6)			/* MOVW	4(R10), R9 */
	STD	(5, 0,(4))			/* MOVW	R8, 0(R4) */
	LD	(16,(10), 5)		/* MOVW	8(R10), R8 */
	STD	(6, 8,(4))			/* MOVW	R9, 4(R4) */
	LD	(24,(10), 6)		/* MOVW	12(R10), R9 */
	ADDU	$32, R10
	STD	(5, 16,(4))			/* MOVW	R8, 8(R4) */
	STD	(6, 24,(4))			/* MOVW	R9, 12(R4) */
	ADDU	$32, R4
	JMP	fd3

/*
 * byte at a time to word align
 */
f1:
	AND	$3,R4, R1
	BEQ	R1, f2
	MOVB	0(R10), R8
	ADDU	$1, R10
	MOVB	R8, 0(R4)
	ADDU	$1, R4
	JMP	f1

/*
 * turn R3 into to-end pointer-15
 * copy 16 at a time while there's room.
 * R11 is smaller than R7 --
 * there are problems if R7 is 0.
 */
f2:
	ADDU	$-15,R11, R3
f3:
	SGTU	R3,R4, R1
	BEQ	R1, f4
	MOVW	0(R10), R8
	MOVW	4(R10), R9
	MOVW	R8, 0(R4)
	MOVW	8(R10), R8
	MOVW	R9, 4(R4)
	MOVW	12(R10), R9
	ADDU	$16, R10
	MOVW	R8, 8(R4)
	MOVW	R9, 12(R4)
	ADDU	$16, R4
	JMP	f3

/*
 * turn R3 into to-end pointer-3
 * copy 4 at a time while theres room
 */
f4:
	ADDU	$-3,R11, R3
f5:
	SGTU	R3,R4, R1
	BEQ	R1, fout
	MOVW	0(R10), R8
	ADDU	$4, R10
	MOVW	R8, 0(R4)
	ADDU	$4, R4
	JMP	f5

/*
 * last loop, copy byte at a time
 */
fout:
	BEQ	R7,R10, ret
	MOVB	0(R10), R8
	ADDU	$1, R10
	MOVB	R8, 0(R4)
	ADDU	$1, R4
	JMP	fout

/*
 * whole thing repeated for backwards
 */
back:
	BNE	R2, bout
	XOR	R11,R7, R1
	AND	$3, R1
	BNE	R1, bout

	XOR	R11,R7, R1
	AND	$7, R1
	BNE	R1, b1

	/* double words */
bd1:
	AND	$7,R7, R1
	BEQ	R1, bd2
	MOVB	-1(R7), R8
	ADDU	$-1, R7
	MOVB	R8, -1(R11)
	ADDU	$-1, R11
	JMP	bd1
bd2:
	ADDU	$31,R10, R3
bd3:
	SGTU	R7,R3, R1
	BEQ	R1, b4
	LD	(-8,(7), 5)				/* MOVW	-4(R7), R8 */
	LD	(-16,(7), 6)			/* MOVW	-8(R7), R9 */
	STD	(5, -8,(11))			/* MOVW	R8, -4(R11) */
	LD	(-24,(7), 5)			/* MOVW	-12(R7), R8 */
	STD	(6, -16,(11))			/* MOVW	R9, -8(R11) */
	LD	(-32,(7), 6)			/* MOVW	-16(R7), R9 */
	ADDU	$-32, R7
	STD	(5, -24,(11))			/* MOVW	R8, -12(R11) */
	STD	(6, -32,(11))			/* MOVW	R9, -16(R11) */
	ADDU	$-32, R11
	JMP	bd3

	/* regular words */
b1:
	AND	$3,R7, R1
	BEQ	R1, b2
	MOVB	-1(R7), R8
	ADDU	$-1, R7
	MOVB	R8, -1(R11)
	ADDU	$-1, R11
	JMP	b1
b2:
	ADDU	$15,R10, R3
b3:
	SGTU	R7,R3, R1
	BEQ	R1, b4
	MOVW	-4(R7), R8
	MOVW	-8(R7), R9
	MOVW	R8, -4(R11)
	MOVW	-12(R7), R8
	MOVW	R9, -8(R11)
	MOVW	-16(R7), R9
	ADDU	$-16, R7
	MOVW	R8, -12(R11)
	MOVW	R9, -16(R11)
	ADDU	$-16, R11
	JMP	b3
b4:
	ADDU	$3,R10, R3
b5:
	SGTU	R7,R3, R1
	BEQ	R1, bout
	MOVW	-4(R7), R8
	ADDU	$-4, R7
	MOVW	R8, -4(R11)
	ADDU	$-4, R11
	JMP	b5

bout:
	BEQ	R7,R10, ret
	MOVB	-1(R7), R8
	ADDU	$-1, R7
	MOVB	R8, -1(R11)
	ADDU	$-1, R11
	JMP	bout

ret:
	MOVW	s1+0(FP), R1
	RET
	END
