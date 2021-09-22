TEXT insb(SB), 1, $-4
	MOVL	RARG, DX
	MOVQ	address+8(FP), DI
	MOVL	count+16(FP), CX
	CLD
	REP;	INSB
	RET

TEXT insl(SB), 1, $-4
	MOVL	RARG, DX
	MOVQ	address+8(FP), DI
	MOVL	count+16(FP), CX
	CLD
	REP; INSL
	RET

TEXT outsb(SB), 1, $-4
	MOVL	RARG, DX
	MOVQ	address+8(FP), SI
	MOVL	count+16(FP), CX
	CLD
	REP; OUTSB
	RET

TEXT outsl(SB), 1, $-4
	MOVL	RARG, DX
	MOVQ	address+8(FP), SI
	MOVL	count+16(FP), CX
	CLD
	REP; OUTSL
	RET
