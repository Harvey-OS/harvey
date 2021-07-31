TEXT	_mulv(SB), $0
	MOVL	r+0(FP), A0
	MOVL	a+8(FP), R0

	WORD	$0x4c2f
	WORD	$0x0401
	WORD	$0x0014
/*
 *	MULUL	b+16(FP), R0:R1
 *	philw made me do it!
 */

	MOVL	a+4(FP), R2
	MULUL	b+16(FP), R2
	ADDL	R2, R1

	MOVL	a+8(FP), R2
	MULUL	b+12(FP), R2
	ADDL	R2, R1

	MOVL	R1, (A0)+
	MOVL	R0, (A0)
	RTS
