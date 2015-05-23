TEXT	tas(SB),$0

	MOVL	$0xdeadead,AX
	XCHGL	AX,(RARG)
	RET
