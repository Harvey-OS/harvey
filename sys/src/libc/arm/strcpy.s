TEXT strcpy(SB), $-4
	MOVW		R0, to+0(FP)	/* need to save for return value */
	MOVW		from+4(FP), R1

	MOVW		$0xFF, R2	/* mask */

_align:					/* align source on 4 */
	AND.S		$3, R1, R3
	BEQ		_aligned

	MOVBU.P		1(R1), R3	/* implicit write back */
	TST		R3, R2
	MOVBU.P		R3, 1(R0)	/* implicit write back */
	BEQ		_return
	B		_align

_aligned:				/* is destination now aligned? */
	AND.S		$3, R0, R3
	BNE		_unaligned

_aloop:
	MOVW.P		4(R1), R4	/* 4 at a time */
	TST		R4, R2		/* AND.S R3, R2, Rx */
	TST.NE		R4>>8, R2
	TST.NE		R4>>16, R2
	TST.NE		R4>>24, R2
	BEQ		_tail

	MOVW.P		R4, 4(R0)
	B		_aloop

_tail:
	AND.S		R4, R2, R3
	MOVBU.P		R3, 1(R0)	/* implicit write back */
	BEQ		_return
	AND.S		R4>>8, R2, R3
	MOVBU.P		R3, 1(R0)	/* implicit write back */
	BEQ		_return
	AND.S		R4>>16, R2, R3
	MOVBU.P		R3, 1(R0)	/* implicit write back */
	BEQ		_return
	AND.S		R4>>24, R2, R3
	MOVBU.P		R3, 1(R0)	/* implicit write back */

_return:
	MOVW		to+0(FP), R0
	RET

_unaligned:
	MOVW.P		4(R1), R4	/* 4 at a time */

	AND.S		R4, R2, R3
	MOVBU.P		R3, 1(R0)
	AND.NE.S	R4>>8, R2, R3
	MOVBU.NE.P	R3, 1(R0)
	AND.NE.S	R4>>16, R2, R3
	MOVBU.NE.P	R3, 1(R0)
	AND.NE.S	R4>>24, R2, R3
	MOVBU.NE.P	R3, 1(R0)
	BEQ		_return

	B		_unaligned
