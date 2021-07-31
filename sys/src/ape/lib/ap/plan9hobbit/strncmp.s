TEXT strncmp(SB), $0
	ADD	s1+0(FP), n+8(FP)
_strncmp0:
	CMPEQ	s1+0(FP), n+8(FP)
	JMPTN	_strncmp2

	CMPEQ	*s2+4(FP).B, *s1+0(FP).B
	JMPTY	_strncmp1

	SUB3	*s2+4(FP).B, *s1+0(FP).B
	RETURN
_strncmp1:
	CMPEQ	$0, *s1+0(FP).B
	ADD	$1, s1+0(FP)
	ADD	$1, s2+4(FP)
	JMPFY	_strncmp0
_strncmp2:
	MOV	$0, s1+0(FP)
	RETURN
