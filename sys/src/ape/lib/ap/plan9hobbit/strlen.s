TEXT strlen(SB), $16
	ADD3	$1, R20
_strlen0:
	CMPEQ	$0, *R20.B
	ADD	$1, R20
	JMPFY	_strlen0
	SUB	R4, R20
	RETURN
