#define NPRIVATES	16

GLOBL	_tos(SB), $4
GLOBL	_privates(SB), $4
GLOBL	_nprivates(SB), $4

TEXT	_mainp(SB), 1, $(3*4+NPRIVATES*4)

	/* _tos = arg */
	MOVL	AX, _tos(SB)
	LEAL	8(SP), AX
	MOVL	AX, _privates(SB)
	MOVL	$NPRIVATES, _nprivates(SB)

	/* _profmain(); */
	CALL	_profmain(SB)

	/* _tos->prof.pp = _tos->prof.next; */
	MOVL	_tos+0(SB),DX
	MOVL	4(DX),CX
	MOVL	CX,(DX)

	CALL	_envsetup(SB)

	/* main(argc, argv, environ); */
	MOVL	inargc-4(FP), AX
	MOVL	AX, 0(SP)
	LEAL	inargv+0(FP), AX
	MOVL	AX, 4(SP)
	MOVL	environ(SB), AX
	MOVL	AX, 8(SP)
	CALL	main(SB)
loop:
	MOVL	AX, 0(SP)
	CALL	exit(SB)
	MOVL	$_profin(SB), AX	/* force loading of profile */
	MOVL	$0, AX
	JMP	loop

TEXT	_savearg(SB), 1, $0
	RET

TEXT	_callpc(SB), 1, $0
	MOVL	argp+0(FP), AX
	MOVL	4(AX), AX
	RET
