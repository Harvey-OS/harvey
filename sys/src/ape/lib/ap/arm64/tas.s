TEXT	_tas(SB), 1, $-4
	MOV	RARG, R2
	MOV	$0xdead, R3
tas1:
	LDXR	(R2), R0
	CBNZ	R0, tas0
	STXR	R3, (R2), R4
	CBNZ	R4, tas1
tas0:
	CLREX
	RETURN
