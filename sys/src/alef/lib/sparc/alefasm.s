/*
 *	Link to a new stack, freeing the parent
 */

	TEXT	ALEF_linksp(SB), $-4
	MOVW	0(FP), R7		/* ptr to spin */
	MOVW	4(FP), R8		/* new stack */
	MOVW	8(FP), R9		/* func to exec */

	MOVW	R0, (R7)		/* free parent */
	ADD	$-4, R8
	MOVW	R8, R1			/* new stack */
	JMPL	(R9)

	MOVW	R0,R8
	MOVW	R8,4(R1)
	JMPL	exits(SB)
	RETURN
/*
 *	Return R7 which at func entry contains 
 *	a pointer to the return complex
 */
	TEXT	ALEF_getrp(SB), $-4
	RETURN
/*
 *	task switch
 */
	TEXT	ALEF_switch(SB),$0
	MOVW	0(FP), R8		/* from task */
	MOVW	4(FP), R9		/* to task */
	MOVW	8(FP), R7
	CMP	R8, R9
	BE	done			/* same task in/out */
	CMP	R0, R8
	BE	nosave
	MOVW	R15, 4(R8)		/* pc */
	MOVW	R1, 0(R8)		/* sp */
nosave:	MOVW	4(R9), R15
	MOVW	0(R9), R1
done:	RETURN
/*
 *	link to a coroutine
 */
	TEXT	ALEF_linktask(SB),$-4
	MOVW	0(R1),R7
	JMPL	(R7)
	MOVW	R0,4(R1)
	JMPL	exits(SB)
	RETURN

	TEXT	abort(SB),$-4
	MOVW	R7,0(R0)
	RETURN
/*
 *	tas uses LDSTUB
 */
	TEXT	tas(SB),$-4
	MOVW	0(FP),R7
	TAS	(R7),R7
	RETURN
