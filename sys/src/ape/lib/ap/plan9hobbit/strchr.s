TEXT strchr(SB), $0
	MOV	c+4(FP), c+4(FP).B
_strchr0:
	CMPEQ	*s+0(FP).B, c+4(FP).B
	JMPTN	_strchr1
	CMPEQ	$0, *s+0(FP).B
	ADD	$1, s+0(FP)
	JMPFY	_strchr0
	MOV	$0, s+0(FP)
_strchr1:
	RETURN
