TO = 1
TOE = 2
N = 3
TMP = 3					/* N and TMP don't overlap */

TEXT memset(SB), $0
	MOVW	R0, R(TO)
	MOVW	data+4(FP), R(4)
	MOVW	n+8(FP), R(N)

	ADD	R(N), R(TO), R(TOE)	/* to end pointer */

	CMP	$4, R(N)		/* need at least 4 bytes to copy */
	BLT	_1tail

	AND	$0xFF, R(4)		/* it's a byte */
	SLL	$8, R(4), R(TMP)	/* replicate to a word */
	ORR	R(TMP), R(4)
	SLL	$16, R(4), R(TMP)
	ORR	R(TMP), R(4)

_4align:				/* align on 4 */
	AND.S	$3, R(TO), R(TMP)
	BEQ	_4aligned

	MOVBU.P	R(4), 1(R(TO))		/* implicit write back */
	B	_4align

_4aligned:
	SUB	$31, R(TOE), R(TMP)	/* do 32-byte chunks if possible */
	CMP	R(TMP), R(TO)
	BHS	_4tail

	MOVW	R4, R5			/* replicate */
	MOVW	R4, R6
	MOVW	R4, R7
	MOVW	R4, R8
	MOVW	R4, R9
	MOVW	R4, R10
	MOVW	R4, R11

_f32loop:
	CMP	R(TMP), R(TO)
	BHS	_4tail

	MOVM.IA.W [R4-R11], (R(TO))
	B	_f32loop

_4tail:
	SUB	$3, R(TOE), R(TMP)	/* do remaining words if possible */
_4loop:
	CMP	R(TMP), R(TO)
	BHS	_1tail

	MOVW.P	R(4), 4(R(TO))		/* implicit write back */
	B	_4loop

_1tail:
	CMP	R(TO), R(TOE)
	BEQ	_return

	MOVBU.P	R(4), 1(R(TO))		/* implicit write back */
	B	_1tail

_return:
	RET
