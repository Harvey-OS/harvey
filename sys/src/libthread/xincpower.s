TEXT	_xinc(SB),$0	/* void _xinc(long *); */

	MOVW	R3, R4
xincloop:
	LWAR	(R4), R3
	ADD		$1, R3
	DCBT	(R4)				/* fix 405 errata cpu_210 */
	STWCCC	R3, (R4)
	BNE		xincloop
	RETURN

TEXT	_xdec(SB),$0	/* long _xdec(long *); */

	MOVW	R3, R4
xdecloop:
	LWAR	(R4), R3
	ADD		$-1, R3
	DCBT	(R4)				/* fix 405 errata cpu_210 */
	STWCCC	R3, (R4)
	BNE		xdecloop
	RETURN
