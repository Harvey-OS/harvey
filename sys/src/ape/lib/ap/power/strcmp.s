TEXT	strcmp(SB), $0

	MOVW	s2+4(FP), R4

	SUB	$1, R3
	SUB	$1, R4
l1:
	MOVBZU	1(R3), R5
	MOVBZU	1(R4), R6
	CMP	R5, R6
	BNE	ne
	CMP	R5, $0
	BNE	l1
	MOVW	$0, R3
	RETURN
ne:
	MOVW	$1, R3
	BGT	ret
	MOVW	$-1, R3
ret:
	RETURN
