TEXT	strcmp(SB), $0

	MOVW	s2+4(FP), R2

l1:
	MOVB	(R2), R3
	MOVB	(R1), R4
	ADDU	$1, R1
	BEQ	R3, end
	ADDU	$1, R2
	BEQ	R3, R4, l1

	SGTU	R4, R3, R1
	BNE	R1, ret
	MOVW	$-1, R1
	RET

end:
	SGTU	R4, R3, R1
ret:
	RET
