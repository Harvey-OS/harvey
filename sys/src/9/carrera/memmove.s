/*
 * R4000-compatible memmove uses V instructions.
 * The kernel uses the 3000 compilers, but we can
 * use this procedure for speed.
 */
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
	SGT	$15,R3, R2
	SGTU	R4,R5, R1
	BNE	R1, back

/*
 * if not at least 15 chars,
 * don't even mess around.
 * 7 chars to guarantee any
 * rounding up to a doubleword
 * boundary and 8 characters
 * to get at least maybe one
 * doubleword store.
 */
	BNE	R2, fout

/*
 * byte at a time to doubleword align destination
 */
f1:
	AND	$7,R4, R1
	BEQ	R1, f2
	MOVB	0(R5), R8
	ADDU	$1, R5
	MOVB	R8, 0(R4)
	ADDU	$1, R4
	JMP	f1

/*
 * test if source is now doubleword aligned
 */
f2:
	AND	$7, R5, R1
	BNE	R1, fun2

/*
 * turn R3 into to-end pointer-31
 * copy 32 at a time while theres room.
 * R6 is smaller than R7 --
 * there are problems if R7 is 0.
 */
	ADDU	$-31,R6, R3
f3:
	SGTU	R3,R4, R1
	BEQ	R1, f4
	MOVV	0(R5), R8
	MOVV	8(R5), R9
	MOVV	R8, 0(R4)
	MOVV	16(R5), R8
	MOVV	R9, 8(R4)
	MOVV	24(R5), R9
	ADDU	$32, R5
	MOVV	R8, 16(R4)
	MOVV	R9, 24(R4)
	ADDU	$32, R4
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
 * forward copy, unaligned
 * turn R3 into to-end pointer-31
 * copy 32 at a time while theres room.
 * R6 is smaller than R7 --
 * there are problems if R7 is 0.
 */
fun2:
	ADDU	$-31,R6, R3
fun3:
	SGTU	R3,R4, R1
	BEQ	R1, fun4
	MOVVL	0(R5), R8
	MOVVR	7(R5), R8
	MOVVL	8(R5), R9
	MOVVR	15(R5), R9
	MOVV	R8, 0(R4)
	MOVVL	16(R5), R8
	MOVVR	23(R5), R8
	MOVV	R9, 8(R4)
	MOVVL	24(R5), R9
	MOVVR	31(R5), R9
	ADDU	$32, R5
	MOVV	R8, 16(R4)
	MOVV	R9, 24(R4)
	ADDU	$32, R4
	JMP	fun3

/*
 * turn R3 into to-end pointer-3
 * copy 4 at a time while theres room
 */
fun4:
	ADDU	$-3,R6, R3
fun5:
	SGTU	R3,R4, R1
	BEQ	R1, fout
	MOVWL	0(R5), R8
	MOVWR	3(R5), R8
	ADDU	$4, R5
	MOVW	R8, 0(R4)
	ADDU	$4, R4
	JMP	fun5

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
b1:
	AND	$7,R6, R1
	BEQ	R1, b2
	MOVB	-1(R7), R8
	ADDU	$-1, R7
	MOVB	R8, -1(R6)
	ADDU	$-1, R6
	JMP	b1

b2:
	AND	$7, R7, R1
	BNE	R1, bun2

	ADDU	$31,R5, R3
b3:
	SGTU	R7,R3, R1
	BEQ	R1, b4
	MOVV	-8(R7), R8
	MOVV	-16(R7), R9
	MOVV	R8, -8(R6)
	MOVV	-24(R7), R8
	MOVV	R9, -16(R6)
	MOVV	-32(R7), R9
	ADDU	$-32, R7
	MOVV	R8, -24(R6)
	MOVV	R9, -32(R6)
	ADDU	$-32, R6
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

bun2:
	ADDU	$31,R5, R3
bun3:
	SGTU	R7,R3, R1
	BEQ	R1, bun4
	MOVVL	-8(R7), R8
	MOVVR	-1(R7), R8
	MOVVL	-16(R7), R9
	MOVVR	-9(R7), R9
	MOVV	R8, -8(R6)
	MOVVL	-24(R7), R8
	MOVVR	-17(R7), R8
	MOVV	R9, -16(R6)
	MOVVL	-32(R7), R9
	MOVVR	-25(R7), R9
	ADDU	$-32, R7
	MOVV	R8, -24(R6)
	MOVV	R9, -32(R6)
	ADDU	$-32, R6
	JMP	bun3

bun4:
	ADDU	$3,R5, R3
bun5:
	SGTU	R7,R3, R1
	BEQ	R1, bout
	MOVWL	-4(R7), R8
	MOVWR	-1(R7), R8
	ADDU	$-4, R7
	MOVW	R8, -4(R6)
	ADDU	$-4, R6
	JMP	bun5

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
