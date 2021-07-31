TEXT strchr(SB), $-4
	MOVBU	c+4(FP), R1
	CMP	$0, R1
	BEQ	_null

_strchr:				/* not looking for a null, byte at a time */
	MOVBU.P	1(R0), R2
	CMP	R1, R2
	BEQ	_sub1

	CMP	$0, R2
	BNE	_strchr

_return0:				/* character not found in string, return 0 */
	MOVW	$0, R0
	RET

_null:					/* looking for null, align */
	AND.S	$3, R0, R2
	BEQ	_aligned

	MOVBU.P	1(R0), R4
	CMP	$0, R4
	BEQ	_sub1
	B	_null

_aligned:
	MOVW	$0xFF, R3		/* mask */

_loop:
	MOVW.P	4(R0), R4		/* 4 at a time */
	TST	R4, R3			/* AND.S R2, R3, Rx */
	BEQ	_sub4
	TST	R4>>8, R3
	BEQ	_sub3
	TST	R4>>16, R3
	BEQ	_sub2
	TST	R4>>24, R3
	BNE	_loop

_sub1:					/* compensate for pointer increment */
	SUB	$1, R0
	RET
_sub2:
	SUB	$2, R0
	RET
_sub3:
	SUB	$3, R0
	RET
_sub4:
	SUB	$4, R0
	RET
