	TEXT	strcmp(SB), $0

	MOVW	s1+0(FP), R3
	MOVW	s2+4(FP), R2

l1:	MOVB	(R2), R1
	MOVB	(R3), R4
	BEQ	R1, end
	ADDU	$1, R2
	ADDU	$1, R3
	BEQ	R1, R4, l1

	SGTU	R4, R1
	BNE	R1, ret
	MOVW	$-1, R1
	RET

end:
	SGTU	R4, R1
ret:
	RET
