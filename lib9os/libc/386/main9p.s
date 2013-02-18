#define NPRIVATES	16

TEXT	_mainp(SB), 1, $(8+NPRIVATES*4)
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
	/* main(argc, argv); */
	MOVL	inargc-4(FP), AX
	MOVL	AX, 0(SP)
	LEAL	inargv+0(FP), AX
	MOVL	AX, 4(SP)
	CALL	main(SB)

loop:
	MOVL	$_exits<>(SB), AX
	MOVL	AX, 0(SP)
	CALL	exits(SB)
	MOVL	$_profin(SB), AX	/* force loading of profile */
	JMP	loop

TEXT	_savearg(SB), 1, $0
	RET

TEXT	_callpc(SB), 1, $0
	MOVL	argp+0(FP), AX
	MOVL	4(AX), AX
	RET

DATA	_exits<>+0(SB)/4, $"main"
GLOBL	_exits<>+0(SB), $5
