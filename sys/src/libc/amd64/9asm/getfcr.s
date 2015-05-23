
TEXT	setfcr(SB), $4
	XORL	$(0x3F<<7),RARG	/* bits are cleared in csr to enable them */
	ANDL	$0xFFC0, RARG	/* just the fcr bits */
	WAIT	/* is this needed? */
	STMXCSR	0(SP)
	MOVL	0(SP), AX
	ANDL	$~0x3F, AX
	ORL	RARG, AX
	MOVL	AX, 0(SP)
	LDMXCSR	0(SP)
	RET

TEXT	getfcr(SB), $4
	WAIT
	STMXCSR	0(SP)
	MOVWLZX	0(SP), AX
	ANDL	$0xFFC0, AX
	XORL	$(0x3F<<7),AX
	RET

TEXT	getfsr(SB), $4
	WAIT
	STMXCSR	0(SP)
	MOVL	0(SP), AX
	ANDL	$0x3F, AX
	RET

TEXT	setfsr(SB), $4
	ANDL	$0x3F, RARG
	WAIT
	STMXCSR	0(SP)
	MOVL	0(SP), AX
	ANDL	$~0x3F, AX
	ORL	RARG, AX
	MOVL	AX, 0(SP)
	LDMXCSR	0(SP)
	RET
