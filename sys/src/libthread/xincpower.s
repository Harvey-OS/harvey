TEXT	_xinc(SB),$0	/* void _xinc(long *); */

	MOVW	R3, R4
xincloop:
	DCBF	(R4)	/* fix for 603x bug */
	LWAR	(R4), R3
	ADD		$1, R3
	STWCCC	R3, (R4)
	BNE		xincloop
	RETURN

TEXT	_xdec(SB),$0	/* long _xdec(long *); */

	MOVW	R3, R4
xdecloop:
	DCBF	(R4)	/* fix for 603x bug */
	LWAR	(R4), R3
	ADD		$-1, R3
	STWCCC	R3, (R4)
	BNE		xdecloop
	RETURN
