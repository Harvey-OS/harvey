t = 97
u = 98
v = 99
Q = 131
link = 64

TEXT	_mull(SB), $-4
	DELAY			/* need an instruction for SETIP to take effect */
	MTSR	R0, R(Q)
	MSTEPL	$0, R0, R(t)
	MSTEPL	R(t), R0, R(t)
	MSTEPL	R(t), R0, R(t)
	MSTEPL	R(t), R0, R(t)
	MSTEPL	R(t), R0, R(t)
	MSTEPL	R(t), R0, R(t)
	MSTEPL	R(t), R0, R(t)
	MSTEPL	R(t), R0, R(t)
	MSTEPL	R(t), R0, R(t)
	MSTEPL	R(t), R0, R(t)
	MSTEPL	R(t), R0, R(t)
	MSTEPL	R(t), R0, R(t)
	MSTEPL	R(t), R0, R(t)
	MSTEPL	R(t), R0, R(t)
	MSTEPL	R(t), R0, R(t)
	MSTEPL	R(t), R0, R(t)
	MSTEPL	R(t), R0, R(t)
	MSTEPL	R(t), R0, R(t)
	MSTEPL	R(t), R0, R(t)
	MSTEPL	R(t), R0, R(t)
	MSTEPL	R(t), R0, R(t)
	MSTEPL	R(t), R0, R(t)
	MSTEPL	R(t), R0, R(t)
	MSTEPL	R(t), R0, R(t)
	MSTEPL	R(t), R0, R(t)
	MSTEPL	R(t), R0, R(t)
	MSTEPL	R(t), R0, R(t)
	MSTEPL	R(t), R0, R(t)
	MSTEPL	R(t), R0, R(t)
	MSTEPL	R(t), R0, R(t)
	MSTEPL	R(t), R0, R(t)
	MSTEPLL	R(t), R0, R(t)
	MFSR	R(Q), R0
	JMP	(R(link))

TEXT	_mulml(SB), $-4
	MOVL	R(link), R(v)
	CALL	_mull(SB)
	ORL	$0, R(t), R0
	JMP	(R(v))

TEXT	_mulul(SB), $-4
	DELAY			/* need an instruction for SETIP to take effect */
	MTSR	R0, R(Q)
	MSTEPUL	$0, R0, R(t)
	MSTEPUL	R(t), R0, R(t)
	MSTEPUL	R(t), R0, R(t)
	MSTEPUL	R(t), R0, R(t)
	MSTEPUL	R(t), R0, R(t)
	MSTEPUL	R(t), R0, R(t)
	MSTEPUL	R(t), R0, R(t)
	MSTEPUL	R(t), R0, R(t)
	MSTEPUL	R(t), R0, R(t)
	MSTEPUL	R(t), R0, R(t)
	MSTEPUL	R(t), R0, R(t)
	MSTEPUL	R(t), R0, R(t)
	MSTEPUL	R(t), R0, R(t)
	MSTEPUL	R(t), R0, R(t)
	MSTEPUL	R(t), R0, R(t)
	MSTEPUL	R(t), R0, R(t)
	MSTEPUL	R(t), R0, R(t)
	MSTEPUL	R(t), R0, R(t)
	MSTEPUL	R(t), R0, R(t)
	MSTEPUL	R(t), R0, R(t)
	MSTEPUL	R(t), R0, R(t)
	MSTEPUL	R(t), R0, R(t)
	MSTEPUL	R(t), R0, R(t)
	MSTEPUL	R(t), R0, R(t)
	MSTEPUL	R(t), R0, R(t)
	MSTEPUL	R(t), R0, R(t)
	MSTEPUL	R(t), R0, R(t)
	MSTEPUL	R(t), R0, R(t)
	MSTEPUL	R(t), R0, R(t)
	MSTEPUL	R(t), R0, R(t)
	MSTEPUL	R(t), R0, R(t)
	MSTEPUL	R(t), R0, R(t)
	MFSR	R(Q), R0
	JMP	(R(link))

TEXT	_mulmul(SB), $-4
	MOVL	R(link), R(v)
	CALL	_mulul(SB)
	ORL	$0, R(t), R0
	JMP	(R(v))
