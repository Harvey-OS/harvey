#define NPRIVATES	16

GLOBL	_tos(SB), $8
GLOBL	_privates(SB), $8
GLOBL	_nprivates(SB), $8

TEXT	_mainp(SB), 1, $(3*8+NPRIVATES*8)

	/* _tos = arg */
	MOVQ	AX, _tos(SB)
	LEAQ	8(SP), AX
	MOVQ	AX, _privates(SB)
	MOVQ	$NPRIVATES, _nprivates(SB)

	/* _profmain(); */
	CALL	_profmain(SB)

	/* _tos->prof.pp = _tos->prof.next; */
	MOVQ	_tos+0(SB),DX
	MOVQ	4(DX),CX
	MOVQ	CX,(DX)

	CALL	_envsetup(SB)

	/* main(argc, argv, environ); */
	MOVL	inargc-8(FP), RARG
	LEAQ	inargv+0(FP), AX
	MOVQ	AX, 8(SP)
	MOVQ	environ(SB), AX
	MOVQ	AX, 16(SP)
	CALL	main(SB)

loop:
	MOVL	AX, RARG
	CALL	exit(SB)
	MOVQ	$_profin(SB), AX	/* force loading of profile */
	MOVL	$0, AX
	JMP	loop

TEXT	_savearg(SB), 1, $0
	RET

TEXT	_callpc(SB), 1, $0
	MOVQ	8(RARG), AX
	RET
