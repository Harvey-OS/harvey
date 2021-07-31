#define NPRIVATES	16

GLOBL	_tos(SB), $4
GLOBL	_privates(SB), $4
GLOBL	_nprivates(SB), $4

TEXT	_mainp(SB), 1, $(3*4+NPRIVATES*4)
	MOVL	$a6base(SB), A6

	/* _tos = arg */
	MOVL	R0, _tos(SB)		/* return value of sys exec!! */
	LEA	private+8(SP), _privates(SB)
	MOVL	$NPRIVATES, _nprivates(SB)

	/* _profmain(); */
	BSR	_profmain(SB)

	/* _tos->prof.pp = _tos->prof.next; */
	MOVL	_tos+0(SB),A1
	MOVL	4(A1),(A1)

	BSR	_envsetup(SB)

	/* main(argc, argv, environ); */
	MOVL	environ(SB), TOS
	PEA	inargv+0(FP)
	MOVL	inargc-4(FP), TOS
	BSR	main(SB)

loop:
	MOVL	R0,TOS
	BSR	exit(SB)
	LEA	_profin(SB), A0		/* force loading of profile */
	BRA	loop


TEXT	_savearg(SB), 1, $0
	RTS

TEXT	_callpc(SB), 1, $0
	MOVL	argp+0(FP), A0
	MOVL	4(A0), R0
	RTS
