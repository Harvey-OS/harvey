TEXT	setjmp(SB), 1, $-4
	MOV	SP, R2
	MOV	R2, (RARG)
	MOV	LR, 8(RARG)
	MOV	$0, RARG
	RETURN

TEXT	sigsetjmp(SB), 1, $-4
	MOV	savemask+8(FP), R2
	MOV	R2, 0(RARG)
	MOV	$_psigblocked(SB), R2
	MOV	R2, 8(RARG)
	MOV	SP, R2
	MOV	R2, 16(RARG)
	MOV	LR, 24(RARG)
	MOV	$0, RARG
	RETURN

TEXT	longjmp(SB), 1, $-4
	MOV	r+8(FP), R1
	CBNZ	R1, ok
	MOV	$1, R1
ok:	MOV	(RARG), R2
	MOV	8(RARG), LR
	MOV	R2, SP
	MOV	R1, RARG
	RETURN
