TEXT	_tas(SB), $-8
	MOVQ	R0, R1			/* l */
tas1:
	MOVLL	(R1), R0		/* l->key */
	BNE	R0, tas2
	MOVQ	$1, R2
	MOVLC	R2, (R1)		/* l->key = 1 */
	BEQ	R2, tas1		/* write failed, try again? */
tas2:
	RET
