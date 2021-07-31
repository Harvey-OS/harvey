	TEXT	strchr(SB), $0

	MOVL	s+0(FP), A0
	MOVB	c+7(FP), R2
	BEQ	null

l:
	MOVB	(A0)+, R1
	BEQ	out
	CMPB	R1, R2
	BNE	l

	MOVL	A0, R0
	ADDL	$-1, R0
	RTS

out:
	CLRL	R0
	RTS

null:
	TSTB	(A0)+
	BNE	null

	MOVL	A0, R0
	ADDL	$-1, R0
	RTS
