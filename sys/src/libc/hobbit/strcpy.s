/*
 * char *strcpy(char *to, char *from);
 */
TEXT strcpy(SB), $16
	MOV	to+0(FP), R12

_falign:
	AND3	$0x3, from+4(FP)		/* word-align 'from' */
	CMPEQ	$0, R4
	JMPTY	_faligned
	MOV	*from+4(FP).UB, *R12.B
	CMPEQ	$0, *R12.B			/* uses bypass */
	JMPTN	_return
	ADD	$1, R12
	ADD	$1, from+4(FP)
	JMP	_falign

_faligned:
	AND3	$0x3, R12			/* 'to' also word aligned? */
	CMPEQ	$0, R4
	JMPTY	_waligned			/* yes */

	AND3	$0x1, R12			/* 'to' also half aligned? */
	CMPEQ	$0, R4
	JMPTN	_haligned			/* yes */

_bcopy:
	MOV	*from+4(FP), R8			/* byte aligned, do byte/half/byte */
	MOV	R8.UB, *R12.B
	CMPEQ	$0, R8.UB
	JMPTN	_return

	CMPEQ	$0, R9.UB
	ADD	$1, R12
	MOV	R9.UB, R8.B
	JMPTN	_return1

	CMPEQ	$0, R10.UB
	MOV	R10.UB, R9.B
	MOV	R8.UH, *R12.H
	JMPTN	_return

	CMPEQ	$0, R11.UB
	ADD	$2, R12
	MOV	R11.UB, *R12.B
	JMPTN	_return

	ADD	$1, R12
	ADD	$4, from+4(FP)
	JMP	_bcopy

_haligned:
	MOV	*from+4(FP), R8			/* half aligned, do word/half+half */
	CMPEQ	$0, R8.UB
	JMPTN	_return1

	CMPEQ	$0, R9.UB
	MOV	R8.UH, *R12.H
	JMPTN	_return

	CMPEQ	$0, R10.UB
	ADD	$2, R12
	JMPTN	_return3

	CMPEQ	$0, R11.UB
	MOV	R10.UH, *R12.H
	JMPTN	_return

	ADD	$2, R12
	ADD	$4, from+4(FP)
	JMP	_haligned

_waligned:
	MOV	*from+4(FP), R8			/* word aligned */
	CMPEQ	$0, R8.UB
	JMPTN	_return1

	CMPEQ	$0, R9.UB
	JMPTN	_return2

	CMPEQ	$0, R10.UB
	JMPTN	_return4

	CMPEQ	$0, R11.UB
	MOV	R8, *R12
	JMPTN	_return

	ADD	$4, R12
	ADD	$4, from+4(FP)
	JMP	_waligned

_return1:
	MOV	R8.UB, *R12.B
	RETURN	R16

_return2:
	MOV	R8.UH, *R12.H
	RETURN	R16

_return4:
	MOV	R8.UH, *R12.H
	ADD	$2, R12
_return3:
	MOV	R10.UB, *R12.B
_return:
	RETURN	R16
