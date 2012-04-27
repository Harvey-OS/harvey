TEXT strcmp(SB), $0
	MOVW	R0, R1
	MOVW	s2+4(FP), R2

	MOVW	$0xFF, R3		/* mask */

_align:					/* align s1 on 4 */
	TST	$3, R1
	BEQ	_aligned

	MOVBU.P	1(R1), R4		/* implicit write back */
	MOVBU.P	1(R2), R8		/* implicit write back */
	SUB.S	R8, R4, R0
	BNE	_return
	CMP	$0, R4
	BEQ	_return
	B	_align

_aligned:				/* is s2 now aligned? */
	TST	$3, R2
	BNE	_unaligned

_aloop:
	MOVW.P	4(R1), R5		/* 4 at a time */
	MOVW.P	4(R2), R7

	AND	R5, R3, R4
	AND	R7, R3, R8
	SUB.S	R8, R4, R0
	BNE	_return
	CMP	$0, R4
	BEQ	_return

	AND	R5>>8, R3, R4
	AND	R7>>8, R3, R8
	SUB.S	R8, R4, R0
	BNE	_return
	CMP	$0, R4
	BEQ	_return

	AND	R5>>16, R3, R4
	AND	R7>>16, R3, R8
	SUB.S	R8, R4, R0
	BNE	_return
	CMP	$0, R4
	BEQ	_return

	AND	R5>>24, R3, R4
	AND	R7>>24, R3, R8
	SUB.S	R8, R4, R0
	BNE	_return
	CMP	$0, R4
	BEQ	_return

	B	_aloop

_return:
	RET

_unaligned:
	MOVBU.P	1(R1), R4		/* implicit write back */
	MOVBU.P	1(R2), R8		/* implicit write back */
	SUB.S	R8, R4, R0
	BNE	_return
	CMP	$0, R4
	BEQ	_return
	B	_unaligned
