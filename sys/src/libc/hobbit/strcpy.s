TEXT strcpy(SB), $16
	MOV	R20, R4
_strcpy0:
	MOV	*R24.B, *R4.B
	CMPEQ	$0, *R4.B
	ADD	$1, R4
	ADD	$1, R24
	JMPFY	_strcpy0
	RETURN
