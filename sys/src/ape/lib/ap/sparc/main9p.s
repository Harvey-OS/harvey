#define NPRIVATES	16

GLOBL	_tos(SB), $4
GLOBL	_privates(SB), $4
GLOBL	_nprivates(SB), $4

TEXT	_mainp(SB), 1, $(3*4+NPRIVATES*4)
	MOVW	$setSB(SB), R2

	/* _tos = arg */
	MOVW	R7, _tos(SB)
/*
	MOVW	_fpsr+0(SB), FSR
	FMOVD	$0.5, F26
	FSUBD	F26, F26, F24
	FADDD	F26, F26, F28
	FADDD	F28, F28, F30
*/
	MOVW	$8(SP), R1
	MOVW	R1, _privates(SB)
	MOVW	$NPRIVATES, R1
	MOVW	R1, _nprivates(SB)

	/* _profmain(); */
	JMPL	_profmain(SB)

	/* _tos->prof.pp = _tos->prof.next; */
	MOVW	_tos+0(SB),R7
	MOVW	4(R7),R8
	MOVW	R8,(R7)

	JMPL	_envsetup(SB)

	/* main(argc, argv, environ); */
	MOVW	inargc-4(FP), R7
	MOVW	$inargv+0(FP), R8
	MOVW	environ(SB), R9
	MOVW	R8, 8(R1)
	MOVW	R9, 12(R1)
	JMPL	main(SB)

loop:
	JMPL	exit(SB)
	MOVW	$_mul(SB), R0		/* force loading of muldiv */
	MOVW	$_profin(SB), R0	/* force loading of profile */
	JMP	loop

TEXT	_savearg(SB), 1, $0
	RETURN

TEXT	_callpc(SB), 1, $0
	MOVW	argp-4(FP), R7
	RETURN
