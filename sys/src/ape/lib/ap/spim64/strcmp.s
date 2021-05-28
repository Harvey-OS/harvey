TEXT	strcmp(SB), $0

	MOVV	s2+8(FP), R2

l1:
	MOVB	(R2), R3
	MOVB	(R1), R4
	ADDVU	$1, R1
	BEQ	R3, end
	ADDVU	$1, R2
	BEQ	R3, R4, l1

	SGTU	R4, R3, R1
	BNE	R1, ret
	MOVW	$-1, R1
	RET

end:
	SGTU	R4, R3, R1
ret:
	RET
