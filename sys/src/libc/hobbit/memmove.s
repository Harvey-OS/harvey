/*
 * void *memmove(void *to, void *from, ulong nbytes);
 *
 * Return value is 'to', so R8 is used as the working copy.
 */
TEXT	memcpy+0(SB), $20
	MOV	a+0(FP), R4
	MOV	b+4(FP), R8
	MOV	c+8(FP), R12
	CALL	memmove+0(SB)
	MOV	R4, .ret+0(FP)
	RETURN

TEXT memmove(SB), $16
	CMPGT	$0, count+8(FP)			/* check count is ok */
	JMPFY	_countok
	MOV	*$0, R0				/* fault */

_countok:
	CMPHI	to+0(FP), from+4(FP)
	MOV	to+0(FP), R8			/* make working copy */
	JMPFY	_forwards			/* 'to' <= 'from' */

_backwards:
	ADD	nbytes+8(FP), R8		/* starting address */
	ADD	nbytes+8(FP), from+4(FP)

	AND3	$~0x3, from+4(FP)		/* word-align 'from' */
_bfalign:
	CMPEQ	from+4(FP), R4
	JMPTY	_bfaligned
	CMPEQ	$0, nbytes+8(FP)
	JMPTN	_return
	SUB	$1, R8
	SUB	$1, from+4(FP)
	MOV	*from+4(FP).UB, *R8.B
	SUB	$1, nbytes+8(FP)
	JMP	_bfalign

_bfaligned:
	XOR3	from+4(FP), R8			/* 'to' and 'from' quad aligned? */
	AND	$0xF, R4
	CMPEQ	$0, R4
	JMPTY	_bqalign			/* yes */

	AND3	$0x3, R8			/* 'to' also word aligned? */
	CMPEQ	$0, R4
	JMPTY	_bwaligned			/* yes */

	AND3	$0x1, R8			/* 'to' also half aligned? */
	CMPEQ	$0, R4
	JMPTY	_bhaligned			/* yes */

_bbaligned:
	CMPGT	$4, nbytes+8(FP)		/* byte aligned, do byte/half/byte... */
	JMPTN	_bbyte				/* ...but less than 4 bytes to go */

	SUB3	nbytes+8(FP), R8
	ADD	$3, R4				/* R4 becomes the ending address */
	AND	$3, nbytes+8(FP)		/* final count must be < 4 */

_bbcopy:
	CMPHI	R8, R4
	JMPFN	_bbyte

	SUB	$4, from+4(FP)
	MOV	*from+4(FP), R12
	SUB	$1, R8
	MOV	R15.UB, *R8.B
	MOV	R14.UB, R15.B
	MOV	R13.UB, R14.B
	SUB	$2, R8
	MOV	R14.UH, *R8.H
	SUB	$1, R8
	MOV	R12.UB, *R8.B
	JMP	_bbcopy

_bhaligned:
	CMPGT	$4, nbytes+8(FP)		/* half aligned, do half/half... */
	JMPTN	_bbyte				/* ...but less than 4 bytes to go */

	SUB3	nbytes+8(FP), R8
	ADD	$3, R4				/* R4 becomes the ending address */
	AND	$3, nbytes+8(FP)		/* final count must be < 4 */

_bhcopy:
	CMPHI	R8, R4
	JMPFN	_bbyte

	ADD	$-4, from+4(FP)
	MOV	*from+4(FP), R12
	ADD	$-2, R8
	MOV	R14.UH, *R8.H
	ADD	$-2, R8
	MOV	R12.UH, *R8.H
	JMP	_bhcopy

_bwaligned:
	CMPGT	$4, nbytes+8(FP)		/* word aligned... */
	JMPTN	_bbyte				/* ...but less than 4 bytes to go */

	SUB3	nbytes+8(FP), R8
	ADD	$3, R4				/* R4 becomes the ending address */
	AND	$3, nbytes+8(FP)		/* final count must be < 4 */

_bwcopy:
	CMPHI	R8, R4
	JMPFN	_bbyte

	SUB	$4, R8
	SUB	$4, from+4(FP)
	MOV	*from+4(FP), *R8
	JMP	_bwcopy

_bqalign:
	AND3	$~0xF, from+4(FP)		/* quad align 'from' */
_bqalign0:
	CMPEQ	from+4(FP), R4
	JMPTY	_bqaligned
	CMPGT	$4, nbytes+8(FP)
	JMPTN	_bbyte
	SUB	$4, R8
	SUB	$4, from+4(FP)
	MOV	*from+4(FP), *R8
	SUB	$4, nbytes+8(FP)
	JMP	_bqalign0

_bqaligned:
	CMPGT	$16, nbytes+8(FP)		/* quad aligned... */
	JMPTN	_bwaligned			/* ...but less than 16 bytes to go */

	SUB3	nbytes+8(FP), R8
	ADD	$0xF, R4			/* R4 becomes the ending address */
	AND	$0xF, nbytes+8(FP)		/* final count must be < 16 */

_bqcopy:
	CMPHI	R8, R4
	JMPFN	_bwaligned
	SUB	$16, R8
	SUB	$16, from+4(FP)
	DQM	*from+4(FP), *R8
	JMP	_bqcopy

_bbyte:						/* remaining odd bytes */
	SUB3	nbytes+8(FP), R8		/* R4 becomes the ending address */
_bbyte0:
	CMPEQ	R4, R8
	JMPTN	_return
	SUB	$1, R8
	SUB	$1, from+4(FP)
	MOV	*from+4(FP).UB, *R8.B
	JMP	_bbyte0

_forwards:
	ADD3	$4, from+4(FP)			/* word-align 'from' */
	AND3	$~0x3, R4
_ffalign:
	CMPEQ	from+4(FP), R4
	JMPTY	_ffaligned
	CMPEQ	$0, nbytes+8(FP)
	JMPTN	_return
	MOV	*from+4(FP).UB, *R8.B
	ADD	$1, R8
	ADD	$1, from+4(FP)
	SUB	$1, nbytes+8(FP)
	JMP	_ffalign

_ffaligned:
	XOR3	from+4(FP), R8			/* 'to' and 'from' quad aligned? */
	AND	$0xF, R4
	CMPEQ	$0, R4
	JMPTY	_fqalign			/* yes */

	AND3	$0x3, R8			/* 'to' also word aligned? */
	CMPEQ	$0, R4
	JMPTY	_fwaligned			/* yes */

	AND3	$0x1, R8			/* 'to' also half aligned? */
	CMPEQ	$0, R4
	JMPTN	_fhaligned			/* yes */

_fbaligned:
	CMPGT	$4, nbytes+8(FP)		/* byte aligned, do byte/half/byte... */
	JMPTN	_fbyte				/* ...but less than 4 bytes to go */

	ADD3	nbytes+8(FP), R8
	SUB	$3, R4				/* R4 becomes the ending address */
	AND	$3, nbytes+8(FP)		/* final count must be < 4 */

_fbcopy:
	CMPHI	R4, R8
	JMPFN	_fbyte

	MOV	*from+4(FP), R12
	MOV	R12.UB, *R8.B
	MOV	R13.UB, R12.B
	MOV	R14.UB, R13.B
	ADD	$1, R8
	MOV	R12.UH, *R8.H
	ADD	$2, R8
	MOV	R15.UB, *R8.B
	ADD	$1, R8
	ADD	$4, from+4(FP)
	JMP	_fbcopy

_fhaligned:
	CMPGT	$4, nbytes+8(FP)		/* half aligned, do half/half... */
	JMPTN	_fbyte				/* ...but less than 4 bytes to go */

	ADD3	nbytes+8(FP), R8
	SUB	$3, R4				/* R4 becomes the ending address */
	AND	$3, nbytes+8(FP)		/* final count must be < 4 */

_fhcopy:
	CMPHI	R4, R8
	JMPFN	_fbyte

	MOV	*from+4(FP), R12
	MOV	R12.UH, *R8.H
	ADD	$2, R8
	MOV	R14.UH, *R8.H
	ADD	$2, R8
	ADD	$4, from+4(FP)
	JMP	_fhcopy

_fwaligned:
	CMPGT	$4, nbytes+8(FP)		/* word aligned... */
	JMPTN	_fbyte				/* ...but less than 4 bytes to go */

	ADD3	nbytes+8(FP), R8
	SUB	$3, R4				/* R4 becomes the ending address */
	AND	$3, nbytes+8(FP)		/* final count must be < 4 */

_fwcopy:
	CMPHI	R4, R8
	JMPFN	_fbyte

	MOV	*from+4(FP), *R8
	ADD	$4, R8
	ADD	$4, from+4(FP)
	JMP	_fwcopy

_fqalign:
	ADD3	$16, from+4(FP)			/* quad align 'from' */
	AND	$0xF, R4
_fqalign0:
	CMPEQ	from+4(FP), R4
	JMPTY	_fqaligned
	CMPGT	$4, nbytes+8(FP)
	JMPTN	_fbyte
	MOV	*from+4(FP), *R8
	ADD	$4, R8
	ADD	$4, from+4(FP)
	SUB	$4, nbytes+8(FP)
	JMP	_fqalign0

_fqaligned:
	CMPGT	$16, nbytes+8(FP)		/* quad aligned... */
	JMPTN	_fwaligned			/* ...but less than 16 bytes to go */

	ADD3	nbytes+8(FP), R8
	SUB	$0xF, R4			/* R4 becomes the ending address */
	AND	$0xF, nbytes+8(FP)		/* final count must be < 16 */

_fqcopy:
	CMPHI	R4, R8
	JMPFN	_fwaligned
	DQM	*from+4(FP), *R8
	ADD	$16, R8
	ADD	$16, from+4(FP)
	JMP	_fqcopy

_fbyte:						/* remaining odd bytes */
	ADD3	nbytes+8(FP), R8		/* R4 becomes the ending address */
_fbyte0:
	CMPEQ	R4, R8
	JMPTN	_return
	MOV	*from+4(FP).UB, *R8.B
	ADD	$1, R8
	ADD	$1, from+4(FP)
	JMP	_fbyte0

_return:
	RETURN	R16
