/*
 *	Link to a new stack, freeing the parent
 */

	TEXT	ALEF_linksp(SB), $-4
	MOVW	0(FP), R1		/* ptr to spin */
	MOVW	4(FP), R2		/* new stack */
	MOVW	8(FP), R3		/* func to exec */

	MOVW	R0, (R1)		/* free parent */
	ADDU	$-4, R2
	MOVW	R2, R29			/* new stack */
	JAL	(R3)

	MOVW	R0,R2
	MOVW	R2,4(R29)
	JAL	exits(SB)
	RET
/*
 *	Return R1 which at func entry contains 
 *	a pointer to the return complex
 */
	TEXT	ALEF_getrp(SB), $-4
	RET
/*
 *	task switch
 */
	TEXT	ALEF_switch(SB),$-4
	MOVW	0(FP), R2		/* from task */
	MOVW	4(FP), R3		/* to task */
	MOVW	8(FP), R1
	BEQ	R2, R3, done		/* same task in/out */
	BEQ	R0, R2, nosave
	MOVW	R31, 4(R2)		/* pc */
	MOVW	R29, 0(R2)		/* sp */
nosave:	MOVW	4(R3), R31
	MOVW	0(R3), R29
done:	RET
/*
 *	link to a coroutine
 */
	TEXT	ALEF_linktask(SB),$-4
	MOVW	0(SP),R1
	JAL	(R1)
	MOVW	R0,4(R29)
	JAL	exits(SB)
	RET

	TEXT	abort(SB),$-4
	WORD	$0x2de1ec		/* reserved instruction */
	RET
