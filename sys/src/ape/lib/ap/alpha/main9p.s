#define NPRIVATES	16

GLOBL	_tos(SB), $4
GLOBL	_privates(SB), $4
GLOBL	_nprivates(SB), $4

TEXT	_mainp(SB), 1, $(3*4+NPRIVATES*4)
	MOVQ	$setSB(SB), R29

	/* _tos = arg */
	MOVL	R0, _tos(SB)
	MOVQ	$8(SP), R1
	MOVL	R1, _privates(SB)
	MOVQ	$NPRIVATES, R1
	MOVL	R1, _nprivates(SB)

	/* _profmain(); */
	JSR	_profmain(SB)

	/* _tos->prof.pp = _tos->prof.next; */
	MOVL	_tos+0(SB), R1
	MOVL	4(R1), R2
	MOVL	R2, 0(R1)

	JSR	_envsetup(SB)

	/* main(argc, argv, environ); */
	MOVL	inargc-4(FP), R0
	MOVL	$inargv+0(FP), R1
	MOVL	environ(SB), R2
	MOVL	R0, 8(R30)
	MOVL	R1, 12(R30)
	MOVL	R2, 16(R30)
	JSR	main(SB)
loop:
	MOVL	R0, 8(R30)
	JSR	exit(SB)
	MOVQ	$_divq(SB), R31		/* force loading of divq */
	MOVQ	$_divl(SB), R31		/* force loading of divl */
	MOVQ	$_profin(SB), R31	/* force loading of profile */
	JMP	loop

TEXT	_savearg(SB), 1, $0
	RET

TEXT	_callpc(SB), 1, $0
	MOVL	argp-8(FP), R0
	RET
