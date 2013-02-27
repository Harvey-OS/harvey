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

	AND	$0xFF, R(4)
	ORR	R(4)<<8, R(4)
	ORR	R(4)<<16, R(4)		/* replicate to word */

_4align:				/* align on 4 */
	AND.S	$3, R(TO), R(TMP)
	BEQ	_4aligned

	MOVBU.P	R(4), 1(R(TO))		/* implicit write back */
	B	_4align

_4aligned:
	SUB	$15, R(TOE), R(TMP)	/* do 16-byte chunks if possible */
	CMP	R(TMP), R(TO)
	BHS	_4tail

	MOVW	R4, R5			/* replicate */
	MOVW	R4, R6
	MOVW	R4, R7

_f16loop:
	CMP	R(TMP), R(TO)
	BHS	_4tail

	MOVM.IA.W [R4-R7], (R(TO))
	B	_f16loop

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
