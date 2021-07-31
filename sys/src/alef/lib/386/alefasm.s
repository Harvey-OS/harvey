/*
 *	Link to a new stack, freeing the parent
 */

	TEXT	ALEF_linksp(SB), 1, $-4
	MOVL	spin+0(FP), AX		/* ptr to spin */
	MOVL	nsp+4(FP), BX		/* new stack */
	MOVL	fn+8(FP), CX		/* func to exec */

	MOVL	$0, (AX)		/* free parent */
	MOVL	BX, SP			/* new stack */
	CALL	*CX

	PUSHL	$0
	CALL	terminate(SB)
	POPL	AX
	RET

/*
 *	Return R1 which at func entry contains 
 *	a pointer to the return complex
 */
	TEXT	ALEF_getrp(SB), 1, $-4
	RET
/*
 *	task switch
 */
	TEXT	ALEF_switch(SB), 1, $-4
	MOVL	ft+0(FP), CX		/* from task */
	MOVL	tt+4(FP), BX		/* to task */
	MOVL	stf+8(FP), AX		/* stack to unallocate */
	CMPL	CX, BX			/* same task in/out */
	JEQ	done
	CMPL	CX, $0
	JEQ	nosave
	MOVL	SP, (CX)
	MOVL	(SP), DX
	MOVL	DX, 4(CX)
nosave:	MOVL	(BX), SP
	MOVL	4(BX), DX
	MOVL	DX, (SP)
done:	RET
/*
 *	link to a coroutine
 */
	TEXT	ALEF_linktask(SB), 1, $-4
	CMPL	AX, $0
	JEQ	nofree
	PUSHL	AX
	CALL	free(SB)
	POPL	AX
nofree:
	POPL	AX
	CALL	*AX
	PUSHL	$0
	CALL	terminate(SB)
	RET

	TEXT	abort(SB), 1, $0
	MOVL	$0, AX
	MOVL	AX, 0(AX)
	RET

	TEXT	tas(SB), 1, $0
	MOVL	$0xdeadead,AX
	MOVL	l+0(FP),BX
	XCHGL	AX,(BX)
	RET
