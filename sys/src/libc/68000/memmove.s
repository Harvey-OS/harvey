	TEXT	memmove(SB), $0
	BRA	move

	TEXT	memcpy(SB), $0
move:

	MOVL	n+8(FP), R0	/* count */
	BEQ	return
	BGT	ok
	MOVL	0, R0
ok:
	MOVL	s1+0(FP), A2	/* dest pointer */
	MOVL	s2+4(FP), A1	/* source pointer */

	CMPL	A2,A1
	BHI	back

/*
 * byte-at-a-time foreward copy to
 * get source (A1) alligned.
 */
f1:
	MOVL	A1, R1
	ANDL	$3, R1
	BEQ	f2
	SUBL	$1, R0
	BLT	return
	MOVB	(A1)+, (A2)+
	BRA	f1

/*
 * check that dest is alligned
 * if not, just go byte-at-a-time
 */
f2:
	MOVL	A2, R1
	ANDL	$3, R1
	BEQ	f3
	SUBL	$1, R0
	BLT	return
	BRA	f5
/*
 * quad-long-at-a-time forward copy
 */
f3:
	SUBL	$16, R0
	BLT	f4
	MOVL	(A1)+, (A2)+
	MOVL	(A1)+, (A2)+
	MOVL	(A1)+, (A2)+
	MOVL	(A1)+, (A2)+
	BRA	f3

/*
 * cleanup byte-at-a-time
 */
f4:
	ADDL	$15, R0
	BLT	return
f5:
	MOVB	(A1)+, (A2)+
	SUBL	$1, R0
	BGE	f5
	BRA	return

return:
	MOVL	s1+0(FP),R0
	RTS

/*
 * everything the same, but
 * copy backwards
 */
back:
	ADDL	R0, A1
	ADDL	R0, A2

/*
 * byte-at-a-time backward copy to
 * get source (A1) alligned.
 */
b1:
	MOVL	A1, R1
	ANDL	$3, R1
	BEQ	b2
	SUBL	$1, R0
	BLT	return
	MOVB	-(A1), -(A2)
	BRA	b1

/*
 * check that dest is alligned
 * if not, just go byte-at-a-time
 */
b2:
	MOVL	A2, R1
	ANDL	$3, R1
	BEQ	b3
	SUBL	$1, R0
	BLT	return
	BRA	b5
/*
 * quad-long-at-a-time backward copy
 */
b3:
	SUBL	$16, R0
	BLT	b4
	MOVL	-(A1), -(A2)
	MOVL	-(A1), -(A2)
	MOVL	-(A1), -(A2)
	MOVL	-(A1), -(A2)
	BRA	b3

/*
 * cleanup byte-at-a-time backward
 */
b4:
	ADDL	$15, R0
	BLT	return
b5:
	MOVB	-(A1), -(A2)
	SUBL	$1, R0
	BGE	b5
	BRA	return
