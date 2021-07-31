TEXT	longjmp(SB),$0
	MOVL	l+0(FP),AX
	MOVL	0(AX),SP	/* restore sp */
	MOVL	4(AX),AX	/* put return pc on the stack */
	MOVL	AX,0(SP)
	MOVL	$1,AX		/* return 1 */
	RET

TEXT	setjmp(SB),$0
	MOVL	l+0(FP),AX
	MOVL	SP,0(AX)	/* store sp */
	MOVL	0(SP),BX	/* store return pc */
	MOVL	BX,4(AX)
	MOVL	$0,AX		/* return 0 */
	RET
