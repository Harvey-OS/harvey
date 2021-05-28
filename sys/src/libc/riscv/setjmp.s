#define LINK	R1
#define SP	R2
#define RARG	R8

TEXT	setjmp(SB), 1, $-4
	MOVW	SP, (RARG)	/* store sp */
	MOVW	LINK, 4(RARG)	/* store return pc */
	MOVW	R0, RARG	/* return 0 */
	RET

TEXT	longjmp(SB), 1, $-4
	MOVW	r+4(FP), R13
	BNE	R13, ok		/* ansi: "longjmp(0) => longjmp(1)" */
	MOVW	$1, R13		/* bless their pointed heads */
ok:	MOVW	(RARG), SP	/* restore sp */
	MOVW	4(RARG), LINK	/* restore return pc */
	MOVW	R13, RARG
	RET
