/* _mainp - profiling _main */

#define NPRIVATES	16
#define FCSR		3

GLOBL	_tos(SB), $XLEN
GLOBL	_privates(SB), $XLEN
GLOBL	_nprivates(SB), $4

TEXT	_mainp(SB), 1, $(4*XLEN + NPRIVATES*XLEN)
	MOV	$setSB(SB), R3

	/* _tos = arg */
	MOV	R8, _tos(SB)
	MOV	$p-(NPRIVATES*XLEN)(SP), R9
	MOV	R9, _privates(SB)
	MOV	$NPRIVATES, R9
	MOVW	R9, _nprivates(SB)

	/* _profmain(); */
	JAL	R1, _profmain(SB)

	/* _tos->prof.pp = _tos->prof.next; */
	MOV	_tos+0(SB), R10
	MOV	XLEN(R10), R9
	MOV	R9, (R10)

	JAL	R1, _envsetup(SB)

	/* main(argc, argv, environ); */
	MOVW	inargc-XLEN(FP), R8
	MOV	$inargv+0(FP), R9
	MOV	R8, 0(SP)
	MOV	R9, XLEN(SP)
	MOV	environ(SB), R9
	MOV	R9, (2*XLEN)(SP)
	JAL	R1, main(SB)
loop:
	/* exit(main(argc, argv)) */
	MOV	R8, 0(SP)
	JAL	R1, exit(SB)
	MOV	$_profin(SB), R0	/* force loading of profile */
	MOV	R0, R8
	JMP	loop

TEXT	_savearg(SB), 1, $0
TEXT	_saveret(SB), 1, $0
	RET

TEXT	_callpc(SB), 1, $0
	MOV	argp+0(FP), R8
	MOV	XLEN(R8), R8
	RET
