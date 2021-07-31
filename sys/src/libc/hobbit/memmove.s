TEXT memmove(SB), $16
	JMP	move

TEXT memcpy(SB), $16
move:
	CMPGT	$0, count+8(FP)
	JMPFY	ok
	MOV	*$0, R0
ok:
	CMPHI	to+0(FP), from+4(FP)	/* if to > from copy backwards */
	JMPTN	_backwards

_forwards:
	CMPHI	count+8(FP), $3		/* >= 4 ? */
	MOV	to+0(FP), R0		/* R0 == to */
	ADD3	count+8(FP), R0
	MOV	R4, R8			/* R8 = to end address */
	JMPFN	fout

	XOR3	from+4(FP), R0		/* both pointers similarly word aligned? */
	AND	$3, R4
	CMPEQ	$0, R4			/* word aligned? */
	JMPFN	fout

_dowordalignf:				/* byte at a time to word align */
	AND3	$3, R0
	CMPEQ	$0, R4
	JMPTY	_iswordalignf
	MOV	*from+4(FP).UB, *R0.UB
	ADD	$1, from+4(FP)
	ADD	$1, R0
	JMP	_dowordalignf

_iswordalignf:
	XOR3	from+4(FP), R0		/* both pointers similarly quad aligned? */
	AND	$15, R4
	CMPEQ	$0, R4			/* quad aligned? */
	JMPFN	_wordf

	SUB3	R0, R8			/* >= 16 ? */
	CMPHI	R4, $15
	JMPFN	_wordf

_doquadalignf:				/* word at a time to quad align */
	AND3	$15, R0
	CMPEQ	$0, R4
	JMPTN	_isquadalignf
	MOV	*from+4(FP), *R0
	ADD	$4, from+4(FP)
	ADD	$4, R0
	JMP	_doquadalignf

_isquadalignf:
	SUB3	$15, R8			/* R4 == to-end pointer -15 */
f3:
	CMPHI	R4, R0
	JMPFN	_wordf
	DQM	*from+4(FP), *R0
	ADD	$16, from+4(FP)
	ADD	$16, R0
	JMP	f3

_wordf:
	SUB3	$3, R8			/* R4 == to-end pointer -3 */
fword1:
	CMPHI	R4, R0
	JMPFN	fout
	MOV	*from+4(FP), *R0
	ADD	$4, from+4(FP)
	ADD	$4, R0
	JMP	fword1

fout:					/* last loop, byte at a time */
	CMPEQ	R0, R8
	JMPT	return
	MOV	*from+4(FP).UB, *R0.UB
	ADD	$1, from+4(FP)
	ADD	$1, R0
	JMP	fout

return:
	RETURN

_backwards:
	CMPHI	count+8(FP), $3
	ADD3	count+8(FP), to+0(FP)
	MOV	R4, R0			/* R0 == to end address*/
	ADD3	count+8(FP), from+4(FP)
	MOV	R4, R8			/* R8 = from end address */
	JMPFN	bout

	XOR3	R0, R8			/* both pointers similarly word aligned? */
	AND	$3, R4
	CMPEQ	$0, R4			/* word aligned? */
	JMPFN	bout

_dowordalignb:				/* byte at a time to word align */
	AND3	$3, R8
	CMPEQ	$0, R4
	JMPT	_iswordalignb
	SUB	$1, R8
	SUB	$1, R0
	MOV	*R8.UB, *R0.UB
	JMP	_dowordalignb

_iswordalignb:
	XOR3	R8, R0			/* both pointers similarly quad aligned? */
	AND	$15, R4
	CMPEQ	$0, R4			/* quad aligned? */
	JMPFN	_wordb

	SUB3	from+4(FP), R8		/* >= 16 ? */
	CMPHI	R4, $15
	JMPFN	_wordb

_doquadalignb:				/* word at a time to quad align */
	AND3	$15, R8
	CMPEQ	$0, R4
	JMPTN	_isquadalignb
	SUB	$4, R8
	SUB	$4, R0
	MOV	*R8, *R0
	JMP	_doquadalignb

_isquadalignb:
	ADD3	$15, to+0(FP)		/* R4 == to pointer +15 */
b2:
	CMPHI	R0, R4
	JMPFN	_wordb
	SUB	$16, R8
	SUB	$16, R0
	DQM	*R8, *R0
	JMP	b2

_wordb:
	ADD3	$3, to+0(FP)		/* R4 == to pointer +3 */
b3:
	CMPHI	R0, R4
	JMPFN	bout
	SUB	$4, R8
	SUB	$4, R0
	MOV	*R8, *R0
	JMP	b3

bout:
	CMPEQ	R0, to+0(FP)
	JMPTN	return
	SUB	$1, R8
	SUB	$1, R0
	MOV	*R8.UB, *R0.UB
	JMP	bout
