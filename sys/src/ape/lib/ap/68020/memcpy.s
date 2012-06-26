	TEXT	memcpy(SB), $0

	MOVL	n+8(FP),R0
	BEQ	return
	BGT	ok
	MOVL	0, R0
ok:
	MOVL	s1+0(FP),A2
	MOVL	s2+4(FP),A1

	CMPL	A2,A1
	BHI	back

/*
 * speed depends on source allignment
 * destination allignment is secondary
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
 * quad-long-at-a-time forward copy
 */
f2:
	SUBL	$16, R0
	BLT	f3
	MOVL	(A1)+, (A2)+
	MOVL	(A1)+, (A2)+
	MOVL	(A1)+, (A2)+
	MOVL	(A1)+, (A2)+
	BRA	f2

/*
 * cleanup byte-at-a-time
 */
f3:
	ADDL	$15, R0
	BLT	return
f4:
	MOVB	(A1)+, (A2)+
	SUBL	$1, R0
	BGE	f4
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
 * quad-long-at-a-time backward copy
 */
b2:
	SUBL	$16, R0
	BLT	b3
	MOVL	-(A1), -(A2)
	MOVL	-(A1), -(A2)
	MOVL	-(A1), -(A2)
	MOVL	-(A1), -(A2)
	BRA	b2

/*
 * cleanup byte-at-a-time backward
 */
b3:
	ADDL	$15, R0
	BLT	return
b4:
	MOVB	-(A1), -(A2)
	SUBL	$1, R0
	BGE	b4
	BRA	return
