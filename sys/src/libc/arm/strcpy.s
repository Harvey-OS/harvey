TEXT strcpy(SB), $0
	MOVW		R0, to+0(FP)	/* need to save for return value */
	MOVW		from+4(FP), R1
	MOVW		$0xFF, R2	/* mask */

salign:					/* align source on 4 */
	AND.S		$3, R1, R3
	BEQ		dalign
	MOVBU.P		1(R1), R3	/* implicit write back */
	TST		R3, R2
	MOVBU.P		R3, 1(R0)	/* implicit write back */
	BNE		salign
	B		return

dalign:				/* is destination now aligned? */
	AND.S		$3, R0, R3
	BNE		uloop

aloop:
	MOVW.P		4(R1), R4	/* read 4, write 4 */
	TST		R4, R2		/* AND.S R3, R2, Rx */
	TST.NE		R4>>8, R2
	TST.NE		R4>>16, R2
	TST.NE		R4>>24, R2
	BEQ		tail
	MOVW.P		R4, 4(R0)
	B		aloop

uloop:
	MOVW.P		4(R1), R4	/* read 4, write 1,1,1,1 */

tail:
	AND.S		R4, R2, R3
	MOVBU.NE.P	R3, 1(R0)
	AND.NE.S	R4>>8, R2, R3
	MOVBU.NE.P	R3, 1(R0)
	AND.NE.S	R4>>16, R2, R3
	MOVBU.NE.P	R3, 1(R0)
	AND.NE.S	R4>>24, R2, R3
	MOVBU.P		R3, 1(R0)
	BNE		uloop
	B		return

return:
	MOVW		to+0(FP), R0
	RET
