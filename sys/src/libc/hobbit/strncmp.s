TEXT strncmp(SB), $0
	ADD	R4, R12
_strncmp0:
	CMPEQ	R4, R12
	JMPTN	_strncmp2

	CMPEQ	*R8.B, *R4.B
	JMPTY	_strncmp1

	SUB3	*R8.B, *R4.B
	RETURN
_strncmp1:
	CMPEQ	$0, *R4.B
	ADD	$1, R4
	ADD	$1, R8
	JMPFY	_strncmp0
_strncmp2:
	MOV	$0, R4
	RETURN
