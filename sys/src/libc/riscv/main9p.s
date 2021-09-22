/* _mainp - profiling _main */

#define NPRIVATES	16

TEXT	_mainp(SB), 1, $(4*XLEN + NPRIVATES*XLEN)
	MOV	$setSB(SB), R3
	/* _tos = arg */
	MOV	R8, _tos(SB)

	MOV	$p-(NPRIVATES*XLEN)(SP), R9
	MOV	R9, _privates(SB)
	MOV	$NPRIVATES, R9
	MOV	R9, _nprivates(SB)

	/* _profmain(); */
	JAL	R1, _profmain(SB)
	/* _tos->prof.pp = _tos->prof.next; */
	MOV	_tos(SB), R9
	MOV	XLEN(R9), R10
	MOV	R10, 0(R9)

	/* main(argc, argv); */
	MOV	inargc-XLEN(FP), R8
	MOV	$inargv+0(FP), R10
	MOV	R8, XLEN(SP)
	MOV	R10, (2*XLEN)(SP)
	JAL	R1, main(SB)
loop:
	/* exits("main"); */
	MOV	$_exitstr<>(SB), R8
	MOV	R8, XLEN(SP)
	JAL	R1, exits(SB)
	MOV	$_profin(SB), R0	/* force loading of profile */
	JMP	loop

TEXT	_savearg(SB), 1, $0
TEXT	_saveret(SB), 1, $0
	RET

TEXT	_callpc(SB), 1, $0
	MOV	argp-XLEN(FP), R8
	RET

DATA	_exitstr<>+0(SB)/4, $"main"
GLOBL	_exitstr<>+0(SB), $5
