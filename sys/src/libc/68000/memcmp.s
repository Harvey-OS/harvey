	TEXT	memcmp(SB),$0
	MOVL	n+8(FP),R0
	BEQ	ret
	MOVL	s1+0(FP),A2
	MOVL	s2+4(FP),A1

l1:	CMPB	(A1)+,(A2)+
	BNE	neq
	SUBL	$1,R0
	BNE	l1
	RTS

neq:	BCS	gtr
	MOVL	$-1,R0
	RTS

gtr:	MOVL	$1,R0
ret:	RTS
