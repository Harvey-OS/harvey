TEXT strlen(SB), $16
	ADD3	$1, s+0(FP)
_strlen0:
	CMPEQ	$0, *s+0(FP).B
	ADD	$1, s+0(FP)
	JMPFY	_strlen0
	SUB	R4, s+0(FP)
	RETURN
