TEXT strcmp(SB), $0
_strcmp0:
	CMPEQ	*R8.B, *R4.B
	JMPFN	_strcmp2
_strcmp1:
	CMPEQ	$0, *R4.B
	ADD	$1, R4
	ADD	$1, R8
	JMPFY	_strcmp0

	MOV	$0, R4
	RETURN
_strcmp2:
	SUB3	*R8.B, *R4.B
	RETURN
