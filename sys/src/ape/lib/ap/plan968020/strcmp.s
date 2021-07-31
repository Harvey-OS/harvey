	TEXT	strcmp(SB), $0
	MOVL	s1+0(FP), A2
	MOVL	s2+4(FP), A1

l1:	MOVB	(A1)+, R0
	BEQ	end
	CMPB	R0, (A2)+
	BEQ	l1

	BCS	gtr
	MOVL	$-1, R0
	RTS

gtr:	MOVL	$1, R0
	RTS

end:	TSTB	(A2)
	BNE	gtr
	CLRL	R0
	RTS
