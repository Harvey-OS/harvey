TEXT	strcmp(SB), $0

	MOVW	s2+XLEN(FP), R9

l1:
	MOVBU	(R9), R10
	MOVBU	(R8), R11
	ADD	$1, R8
	BEQ	R10, end
	ADD	$1, R9
	BEQ	R10, R11, l1

	SLTU	R11, R10, R8
	BNE	R8, ret
	MOVW	$-1, R8
	RET

end:
	SLTU	R11, R10, R8
ret:
	RET
