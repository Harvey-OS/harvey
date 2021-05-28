	TEXT	strlen(SB),$0

	MOVL	$0, AX
	MOVQ	$-1, CX
	CLD
/*
 * look for end of string
 */

	MOVQ	RARG, DI
	REPN;	SCASB

	MOVQ	DI, AX
	SUBQ	RARG, AX
	SUBQ	$1, AX
	RET
