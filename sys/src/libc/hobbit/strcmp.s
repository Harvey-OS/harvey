/*
 * int	strcmp(char *s1, char *s2);
 */
TEXT strcmp(SB), $16
_s2align:
	AND3	$0x3, s2+4(FP)			/* word align 's2' */
	CMPEQ	$0, R4
	JMPTN	_s2aligned

	CMPEQ	*s2+4(FP).UB, *s1+0(FP).UB
	JMPFN	_unequal

	CMPEQ	$0, *s1+0(FP).UB
	JMPTN	_equal

	ADD	$1, s1+0(FP)
	ADD	$1, s2+4(FP)
	JMP	_s2align

_s2aligned:
	AND3	$0x3, s1+0(FP)			/* 's1' also word aligned? */
	CMPEQ	$0, R4
	JMPTY	_waligned			/* yes */

	AND3	$0x1, s1+0(FP)			/* 's1' also half aligned? */
	CMPEQ	$0, R4
	JMPTN	_haligned			/* yes */

_bcmp:
	MOV	*s2+4(FP), R12			/* byte aligned, do byte/half/byte */
	CMPEQ	R12.UB, *s1+0(FP).UB
	JMPFN	_unequal

	CMPEQ	$0, R12.UB
	ADD	$1, s1+0(FP)
	JMPTN	_equal

	MOV	R13.UB, R12.B
	MOV	R14.UB, R13.B
	CMPEQ	R12.UH, *s1+0(FP).UH
	ADD	$1, s2+4(FP)
	JMPFN	_byte

	CMPEQ	$0, R12.UB
	JMPTN	_equal

	CMPEQ	$0, R13.UB
	ADD	$2, s1+0(FP)
	JMPTN	_equal

	CMPEQ	R15.UB, *s1+0(FP).UB
	ADD	$2, s2+4(FP)
	JMPFN	_byte

	CMPEQ	$0, R15.UB
	ADD	$1, s1+0(FP)
	ADD	$1, s2+4(FP)
	JMPTN	_equal

	JMP	_bcmp

_haligned:
	MOV	*s2+4(FP), R12			/* half aligned, do half/half */
	CMPEQ	R12.UH, *s1+0(FP).UH
	JMPFN	_byte

	CMPEQ	$0, R12.UB
	JMPTN	_equal

	CMPEQ	$0, R13.UB
	ADD	$2, s1+0(FP)
	JMPTN	_equal

	CMPEQ	R14.UH, *s1+0(FP).UH
	ADD	$2, s2+4(FP)
	JMPFN	_byte

	CMPEQ	$0, R14.UB
	ADD	$2, s1+0(FP)
	JMPTN	_equal

	CMPEQ	$0, R15.UB
	ADD	$2, s2+4(FP)
	JMPTN	_equal

	JMP	_haligned

_waligned:
	MOV	*s2+4(FP), R12			/* word aligned */
	CMPEQ	R12, *s1+0(FP)
	JMPFN	_byte

	CMPEQ	$0, R12.UB
	JMPTN	_equal

	CMPEQ	$0, R13.UB
	JMPTN	_equal

	CMPEQ	$0, R14.UB
	ADD	$4, s1+0(FP)
	JMPTN	_equal

	CMPEQ	$0, R15.UB
	ADD	$4, s2+4(FP)
	JMPTN	_equal

	JMP	_waligned

_byte:
	CMPEQ	*s2+4(FP).UB, *s1+0(FP).UB
	JMPFN	_unequal

	CMPEQ	$0, *s1+0(FP).UB
	ADD	$1, s1+0(FP)
	ADD	$1, s2+4(FP)
	JMPFY	_byte

_equal:
	MOV	$0, s1+0(FP)
	RETURN	R16

_unequal:
	MOV	*s1+0(FP).UB, s1+0(FP)
	SUB	*s2+4(FP).UB, s1+0(FP)
	RETURN	R16
