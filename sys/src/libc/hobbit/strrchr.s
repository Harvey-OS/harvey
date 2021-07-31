TEXT strrchr(SB), $16
	MOV	$0, R4
	MOV	c+4(FP), c+4(FP).B
_strrchr0:
	CMPEQ	*s+0(FP).B, c+4(FP).B
	JMPFY	_strrchr1
	MOV	s+0(FP), R4
_strrchr1:
	CMPEQ	$0, *s+0(FP).B
	ADD	$1, s+0(FP)
	JMPFY	_strrchr0
	MOV	R4, s+0(FP)
	RETURN
