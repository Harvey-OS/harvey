TEXT	sigsetjmp(SB), 1, $-4
	MOVW	R29, (R1)
	MOVW	R31, 4(R1)
	MOVW	savemask+4(FP), R3
	MOVW	R3, 8(R1)
	MOVW	_psigblocked(SB), R3
	MOVW	R3, 12(R1)
	MOVW	$0, R1
	RET

TEXT	siglongjmp(SB), 1, $-4
	MOVW	r+4(FP), R3
	BNE	R3, ok
	MOVW	$1, R3
ok:	MOVW	(R1), R29
	MOVW	4(R1), R31
	MOVW	8(R1), R2
	BEQ	R3, ret
	MOVW	12(R1), R2
	MOVW	R2, _psigblocked(SB)
ret:
	MOVW	R3, R1
	RET
