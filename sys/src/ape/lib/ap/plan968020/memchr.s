	TEXT	memchr(SB),$0
	MOVL	n+8(FP),R0
	BEQ	ret
	MOVL	s1+0(FP),A1
	MOVL	c+4(FP),R1

l1:	CMPB	R1,(A1)+
	BEQ	eq
	SUBL	$1,R0
	BNE	l1
	RTS

eq:	MOVL	A1,R0
	SUBL	$1,R0
ret:	RTS
