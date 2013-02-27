TEXT	_tas(SB), 1, $-4
	MOVW	R3, R4
	MOVW	$0xdeaddead,R5
tas1:
/*	DCBF	(R4)			 	fix for 603x bug */
	SYNC
	LWAR	(R4), R3
	CMP	R3, $0
	BNE	tas0
	DCBT	(R4)				/* fix 405 errata cpu_210 */
	STWCCC	R5, (R4)
	BNE	tas1
tas0:
	RETURN
