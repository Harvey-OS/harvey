TEXT strrchr(SB), $16
	MOV	$0, R4
	MOV	R24, R24.B
_strrchr0:
	CMPEQ	*R20.B, R24.B
	JMPFY	_strrchr1
	MOV	R20, R4
_strrchr1:
	CMPEQ	$0, *R20.B
	ADD	$1, R20
	JMPFY	_strrchr0
	MOV	R4, R20
	RETURN
