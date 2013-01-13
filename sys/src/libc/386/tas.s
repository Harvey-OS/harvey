/*
 * The kernel and the libc use the same constant for TAS
 */
TEXT	_tas(SB),$0

	MOVL	$0xdeaddead,AX
	MOVL	l+0(FP),BX
	XCHGL	AX,(BX)
	RET
