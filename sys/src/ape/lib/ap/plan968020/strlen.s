	TEXT	strlen(SB), $0
	MOVL	s+0(FP), A1

	TSTB	(A1)+
	BEQ	null
	MOVL	A1, A2

l1:
	TSTB	(A1)+
	BNE	l1

	SUBL	A2, A1
	MOVL	A1, R0
	RTS

null:
	MOVL	$0, R0
	RTS
