TEXT	_tas(SB),$0

	MOVL	$0xdeadead,AX
	MOVL	l+0(FP),BX
	XCHGL	AX,(BX)
	RET
