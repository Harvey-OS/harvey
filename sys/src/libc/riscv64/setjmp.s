#define LINK	R1
#define SP	R2
#define RARG	R8

TEXT	setjmp(SB), 1, $-4
	MOV	SP, (RARG)	/* store sp in jmp_buf */
	MOV	LINK, XLEN(RARG) /* store return pc */
	MOV	R0, RARG	/* return 0 */
	RET

TEXT	longjmp(SB), 1, $-4
	MOV	r+XLEN(FP), R13
	BNE	R13, ok		/* ansi: "longjmp(0) => longjmp(1)" */
	MOV	$1, R13		/* bless their pointed heads */
ok:	MOV	(RARG), SP	/* restore sp */
	MOV	XLEN(RARG), LINK /* restore return pc */
	MOV	R13, RARG
	RET			/* jump to saved pc */
