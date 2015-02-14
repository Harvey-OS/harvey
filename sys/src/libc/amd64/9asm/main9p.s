#define NPRIVATES	16

TEXT _mainp(SB), 1, $(2*8+NPRIVATES*8)
	MOVQ	AX, _tos(SB)		/* _tos = arg */
	LEAQ	16(SP), AX
	MOVQ	AX, _privates(SB)
	MOVL	$NPRIVATES, _nprivates(SB)

	CALL	_profmain(SB)		/* _profmain(); */

	MOVQ	_tos+0(SB), DX		/* _tos->prof.pp = _tos->prof.next; */
	MOVQ	8(DX), CX
	MOVQ	CX, (DX)

	MOVL	inargc-8(FP), RARG	/* main(argc, argv); */
	LEAQ	inargv+0(FP), AX
	MOVQ	AX, 8(SP)
	CALL	main(SB)

loop:
	MOVQ	$_exits<>(SB), RARG
	CALL	exits(SB)
	MOVQ	$_profin(SB), AX	/* force loading of profile */
	JMP	loop

TEXT	_savearg(SB), 1, $0
	MOVQ	RARG, AX
	RET

TEXT	_saveret(SB), 1, $0
	RET

TEXT	_restorearg(SB), 1, $0
	RET				/* we want RARG in RARG */

TEXT	_callpc(SB), 1, $0
	MOVQ	8(RARG), AX
	RET

DATA	_exits<>+0(SB)/4, $"main"
GLOBL	_exits<>+0(SB), $5
