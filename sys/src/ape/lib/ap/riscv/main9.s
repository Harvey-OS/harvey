#define NPRIVATES	16
#define FCSR		3

GLOBL	_tos(SB), $XLEN
GLOBL	_privates(SB), $XLEN
GLOBL	_nprivates(SB), $4

TEXT	_main(SB), 1, $(4*XLEN + NPRIVATES*XLEN)
	MOV	$setSB(SB), R3

	/* _tos = arg */
	MOV	R8, _tos(SB)
	MOV	$p-(NPRIVATES*XLEN)(SP), R9
	MOV	R9, _privates(SB)
	MOV	$NPRIVATES, R9
	MOVW	R9, _nprivates(SB)

	JAL	R1, _envsetup(SB)

	/* main(argc, argv, environ); */
	MOVW	inargc-XLEN(FP), R8
	MOV	$inargv+0(FP), R9
	MOV	R8, 0(SP)
	MOV	R9, XLEN(SP)
	MOV	environ(SB), R9
	MOV	R9, (2*XLEN)(SP)
	JAL	R1, main(SB)

	MOV	R8, 0(SP)
	JAL	R1, exit(SB)
	RET
