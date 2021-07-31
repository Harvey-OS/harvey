	TEXT	strcpy(SB), $0

	MOVL	s1+0(FP), A2
	MOVL	s2+4(FP), A1

l1:	MOVB	(A1)+, (A2)+
	BNE	l1

	MOVL	s1+0(FP), R0
	RTS
