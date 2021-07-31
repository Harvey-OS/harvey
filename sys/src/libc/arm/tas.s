TEXT	_tas(SB), $-4
	MOVW	R0,R1
	MOVW	$1,R0
	SWPW	R0,(R1)		/* fix: deprecated in armv7 */
	RET
