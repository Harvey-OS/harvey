	TEXT	strcat(SB), $0
	MOVL	s1+0(FP), A2
	MOVL	s2+4(FP), A1

l1:	TSTB	(A2)+
	BNE	l1

	MOVB	(A1)+, -1(A2)
	BEQ	done

l2:	MOVB	(A1)+, (A2)+
	BNE	l2

done:	MOVL	s1+0(FP), R0
	RTS
