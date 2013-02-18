	TEXT	tramp(SB), 1, $0
	MOVW	fn+4(FP), R1		/* func to exec */
	MOVW	arg+8(FP), R2		/* argument */
	SUB	$8, R0			/* new stack */
	MOVW	R0, SP
	MOVW	R2, R0
	BL	(R1)

	MOVW	$0, R0
	BL	_exits(SB)
	RET

	TEXT	vstack(SB), 1, $0
	MOVW	ustack(SB), SP
	BL		exectramp(SB)
	RET

	TEXT	FPsave(SB), 1, $0
	RET

	TEXT	FPrestore(SB), 1, $0
	RET
