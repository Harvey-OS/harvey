	TEXT	memset(SB), $0
	MOVL	n+8(FP), R0
	BLE	return
	MOVL	s1+0(FP), A1
	CLRL	R1
	MOVB	c+7(FP), R1
	BEQ	l1

/*
 * create 4 replicated copies
 * of the byte in R1
 */
	MOVL	R1, R2
	ASLL	$8, R2
	ORL	R2, R1
	MOVL	R1, R2
	SWAP	R2
	ORL	R2, R1

/*
 * byte-at-a-time until alligned
 */
l1:
	MOVL	A1, R1
	ANDL	$3, R1
	BEQ	l2
	SUBL	$1, R0
	BLT	return
	MOVB	R1, (A1)+
	BRA	l1

/*
 * quad-long-at-a-time set
 */
l2:
	SUBL	$16, R0
	BLT	l3
	MOVL	R1, (A1)+
	MOVL	R1, (A1)+
	MOVL	R1, (A1)+
	MOVL	R1, (A1)+
	BRA	l2

/*
 * cleanup byte-at-a-time
 */
l3:
	ADDL	$15, R0
	BLT	return
l4:
	MOVB	R1, (A1)+
	SUBL	$1, R0
	BGE	l4

return:
	MOVL	s1+0(FP),R0
	RTS
