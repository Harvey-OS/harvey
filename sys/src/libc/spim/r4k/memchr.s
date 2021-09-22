	TEXT	memchr(SB), $0

	MOVW	n+8(FP), R3		/* R3 is count */
	MOVW	R1, R4			/* R4 is pointer */
	MOVBU	c+4(FP), R5		/* R5 is char */
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
 * turn R7 into a doubleword of characters
 */
	SLLV	$8,R5, R7
	OR	R5, R7
	SLLV	$16,R7, R1
	OR	R1, R7
	SLLV	$32,R7, R1
	OR	R1, R7

/*
 * build sequential masks in
 * successive registers R10 ...
 */
	MOVV	$0xff, R10
	SLLV	$8,R10, R11
	SLLV	$16,R10, R12
	SLLV	$24,R10, R13
	SLLV	$32,R10, R14
	SLLV	$40,R10, R15
	SLLV	$48,R10, R16
	SLLV	$56,R10, R17

/*
 * compare one byte at a time until pointer
 * is alligned on a doubleword boundary
 */
l1:
	AND	$7,R4, R1
	BEQ	R1, l2
	MOVBU	0(R4), R1
	ADDU	$1, R4
	BEQ	R1,R5, eq7
	JMP	l1

/*
 * turn R3 into end pointer-7
 * compare longword at a time
 */
l2:
	ADDU	$-7,R6, R3
l3:
	SGTU	R3,R4, R1
	BEQ	R1, out
	MOVV	0(R4), R1
	ADDU	$8, R4
	XOR	R7,R1, R2		/* now looking for null char */
	AND	R10,R2, R8
	AND	R11,R2, R9
	BEQ	R8, eq0
	AND	R12,R2, R8
	BEQ	R9, eq1
	AND	R13,R2, R9
	BEQ	R8, eq2
	AND	R14,R2, R8
	BEQ	R9, eq3
	AND	R15,R2, R9
	BEQ	R8, eq4
	AND	R16,R2, R8
	BEQ	R9, eq5
	AND	R17,R2, R9
	BEQ	R8, eq6
	BEQ	R9, eq7
	JMP	l3

/*
 * last loop, store byte at a time
 */
out:
	SGTU	R6,R4, R1
	BEQ	R1, ret
	MOVBU	0(R4), R1
	ADDU	$1, R4
	BEQ	R1,R5, eq7
	JMP	out

eq0:	ADDU	$-8,R4, R1
	JMP	ret
eq1:	ADDU	$-7,R4, R1
	JMP	ret
eq2:	ADDU	$-6,R4, R1
	JMP	ret
eq3:	ADDU	$-5,R4, R1
	JMP	ret
eq4:	ADDU	$-4,R4, R1
	JMP	ret
eq5:	ADDU	$-3,R4, R1
	JMP	ret
eq6:	ADDU	$-2,R4, R1
	JMP	ret
eq7:	ADDU	$-1,R4, R1

ret:
	RET
	END
