	TEXT	memccpy(SB),$0
	MOVL	n+12(FP),R0
	BEQ	ret
	MOVL	s1+0(FP),A2
	MOVL	s2+4(FP),A1
	MOVL	c+8(FP),R1
	BEQ	l2

/*
 * general case
 */
l1:	MOVB	(A1)+,R2
	MOVB	R2,(A2)+
	CMPB	R2,R1
	BEQ	eq
	SUBL	$1,R0
	BNE	l1
	RTS

/*
 * special case for null character
 */
l2:	MOVB	(A1)+,(A2)+
	BEQ	eq
	SUBL	$1,R0
	BNE	l2
	RTS

eq:	MOVL	A2,R0
ret:	RTS
