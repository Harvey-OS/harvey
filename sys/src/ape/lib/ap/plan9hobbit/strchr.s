TEXT strchr(SB), $0
_strchr0:
	CMPEQ	*R4.B, R11.B
	JMPTN	_strchr1
	CMPEQ	$0, *R4.B
	ADD	$1, R4
	JMPFY	_strchr0
	MOV	$0, R4
_strchr1:
	RETURN
