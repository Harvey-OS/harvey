link=64
sp=65
arg=69

TEXT	setjmp(SB), 1, $-4
	MOVL	R(sp), (R(arg+0))
	MOVL	R(link), 4(R(arg+0))
	MOVL	$0, R1
	RET

TEXT	longjmp(SB), 1, $-4
	MOVL	r+4(FP), R(arg+2)
	CPNEQL	$0, R(arg+2)
	JMPT	R(arg+2), ok		/* ansi: "longjmp(0) => longjmp(1)" */
	MOVL	$1, R(arg+2)		/* bless their pointed heads */
ok:	MOVL	(R(arg+0)), R(sp)
	MOVL	4(R(arg+0)), R(link)
	MOVL	R(arg+2), R(arg+0)
	RET
