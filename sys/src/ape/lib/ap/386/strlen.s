	TEXT	strlen(SB),$0

	MOVL	$0, AX
	MOVL	$-1, CX
	CLD
/*
 * look for end of string
 */

	MOVL	p+0(FP), DI
	REPN;	SCASB

	MOVL	DI, AX
	SUBL	p+0(FP), AX
	SUBL	$1, AX
	RET
