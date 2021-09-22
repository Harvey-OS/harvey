	TEXT	memcpy(SB), $-4
	TEXT	memmove(SB), $-4
	MOVW	R8, s1+0(FP)

	MOVW	n+(2*XLEN)(FP), R9	/* count */
	BEQ	R9, return
	BGT	R9, ok
	MOVW	R0, R9
ok:
	MOVW	s1+0(FP), R11		/* dest pointer */
	MOVW	s2+XLEN	(FP), R10	/* source pointer */

	BLTU	R11, R10, back

/*
 * byte-at-a-time forward copy to
 * get source (R10) aligned.
 */
f1:
	AND	$3, R10, R8
	BEQ	R8, f2
	SUB	$1, R9
	BLT	R9, return
	MOVB	(R10), R8
	MOVB	R8, (R11)
	ADD	$1, R10
	ADD	$1, R11
	JMP	f1

/*
 * check that dest is aligned
 * if not, just go byte-at-a-time
 */
f2:
	AND	$3, R11, R8
	BEQ	R8, f3
	SUB	$1, R9
	BLT	R9, return
	JMP	f5
/*
 * quad-long-at-a-time forward copy
 */
f3:
	SUB	$16, R9
	BLT	R9, f4
	MOVW	0(R10), R12
	MOVW	4(R10), R13
	MOVW	8(R10), R14
	MOVW	12(R10), R15
	MOVW	R12, 0(R11)
	MOVW	R13, 4(R11)
	MOVW	R14, 8(R11)
	MOVW	R15, 12(R11)
	ADD	$16, R10
	ADD	$16, R11
	JMP	f3

/*
 * cleanup byte-at-a-time
 */
f4:
	ADD	$15, R9
	BLT	R9, return
f5:
	MOVB	(R10), R8
	MOVB	R8, (R11)
	ADD	$1, R10
	ADD	$1, R11
	SUB	$1, R9
	BGE	R9, f5
	JMP	return

return:
	MOVW	s1+0(FP),R8
	RET

/*
 * everything the same, but
 * copy backwards
 */
back:
	ADD	R9, R10
	ADD	R9, R11

/*
 * byte-at-a-time backward copy to
 * get source (R10) aligned.
 */
b1:
	AND	$3, R10, R8
	BEQ	R8, b2
	SUB	$1, R9
	BLT	R9, return
	SUB	$1, R10
	SUB	$1, R11
	MOVB	(R10), R8
	MOVB	R8, (R11)
	JMP	b1

/*
 * check that dest is aligned
 * if not, just go byte-at-a-time
 */
b2:
	AND	$3, R11, R8
	BEQ	R8, b3
	SUB	$1, R9
	BLT	R9, return
	JMP	b5
/*
 * quad-long-at-a-time backward copy
 */
b3:
	SUB	$16, R9
	BLT	R9, b4
	SUB	$16, R10
	SUB	$16, R11
	MOVW	0(R10), R12
	MOVW	4(R10), R13
	MOVW	8(R10), R14
	MOVW	12(R10), R15
	MOVW	R12, 0(R11)
	MOVW	R13, 4(R11)
	MOVW	R14, 8(R11)
	MOVW	R15, 12(R11)
	JMP	b3

/*
 * cleanup byte-at-a-time backward
 */
b4:
	ADD	$15, R9
	BLT	R9, return
b5:
	SUB	$1, R10
	SUB	$1, R11
	MOVB	(R10), R8
	MOVB	R8, (R11)
	SUB	$1, R9
	BGE	R9, b5
	JMP	return
