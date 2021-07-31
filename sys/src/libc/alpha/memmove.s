#define QUAD	8
#define ALIGN	64
#define BLOCK	64

TEXT memmove(SB), $0
_memmove:
	MOVL	from+4(FP), R7
	MOVL	n+8(FP), R10
	MOVQ	R0, R6

	CMPUGE	R7, R0, R5
	BNE	R5, _forward

	MOVQ	R6, R8			/* end to address */
	ADDL	R10, R6, R6		/* to+n */
	ADDL	R10, R7, R7		/* from+n */

	CMPUGE	$ALIGN, R10, R1		/* need at least ALIGN bytes */
	BNE	R1, _b1tail

_balign:
	AND	$(ALIGN-1), R6, R1
	BEQ	R1, _baligned

	MOVBU	-1(R7), R2
	ADDL	$-1, R6, R6
	MOVB	R2, (R6)
	ADDL	$-1, R7, R7
	JMP	_balign
	
_baligned:
	AND	$(QUAD-1), R7, R1	/* is the source quad-aligned */
	BNE	R1, _bunaligned

	ADDL	$(BLOCK-1), R8, R9
_bblock:
	CMPUGE	R9, R6, R1
	BNE	R1, _b8tail

	MOVQ	-64(R7), R22
	MOVQ	-56(R7), R23
	MOVQ	-48(R7), R24
	MOVQ	-40(R7), R25
	MOVQ	-32(R7), R2
	MOVQ	-24(R7), R3
	MOVQ	-16(R7), R4
	MOVQ	-8(R7), R5

	SUBL	$64, R6, R6
	SUBL	$64, R7, R7

	MOVQ	R22, (R6)
	MOVQ	R23, 8(R6)
	MOVQ	R24, 16(R6)
	MOVQ	R25, 24(R6)
	MOVQ	R2, 32(R6)
	MOVQ	R3, 40(R6)
	MOVQ	R4, 48(R6)
	MOVQ	R5, 56(R6)
	JMP	_bblock

_b8tail:
	ADDL	$(QUAD-1), R8, R9
_b8block:
	CMPUGE	R9, R6, R1
	BNE	R1, _b1tail

	MOVQ	-8(R7), R2
	SUBL	$8, R6
	MOVQ	R2, (R6)
	SUBL	$8, R7
	JMP	_b8block

_b1tail:
	CMPUGE	R8, R6, R1
	BNE	R1, _ret

	MOVBU	-1(R7), R2
	SUBL	$1, R6, R6
	MOVB	R2, (R6)
	SUBL	$1, R7, R7
	JMP	_b1tail
_ret:
	RET

_bunaligned:
	ADDL	$(16-1), R8, R9

_bu8block:
	CMPUGE	R9, R6, R1
	BNE	R1, _b1tail

	MOVQU	-16(R7), R4
	MOVQU	-8(R7), R3
	MOVQU	(R7), R2
	SUBL	$16, R6
	EXTQH	R7, R2, R2
	EXTQL	R7, R3, R5
	OR	R5, R2, R11
	EXTQH	R7, R3, R3
	EXTQL	R7, R4, R4
	OR	R3, R4, R13
	MOVQ	R11, 8(R6)
	MOVQ	R13, (R6)
	SUBL	$16, R7
	JMP	_bu8block

_forward:
	ADDL	R10, R6, R8		/* end to address */

	CMPUGE	$ALIGN, R10, R1		/* need at least ALIGN bytes */
	BNE	R1, _f1tail

_falign:
	AND	$(ALIGN-1), R6, R1
	BEQ	R1, _faligned

	MOVBU	(R7), R2
	ADDL	$1, R6, R6
	ADDL	$1, R7, R7
	MOVB	R2, -1(R6)
	JMP	_falign

_faligned:
	AND	$(QUAD-1), R7, R1	/* is the source quad-aligned */
	BNE	R1, _funaligned

	SUBL	$(BLOCK-1), R8, R9
_fblock:
	CMPUGT	R9, R6, R1
	BEQ	R1, _f8tail

	MOVQ	(R7), R2
	MOVQ	8(R7), R3
	MOVQ	16(R7), R4
	MOVQ	24(R7), R5
	MOVQ	32(R7), R22
	MOVQ	40(R7), R23
	MOVQ	48(R7), R24
	MOVQ	56(R7), R25

	ADDL	$64, R6, R6
	ADDL	$64, R7, R7

	MOVQ	R2, -64(R6)
	MOVQ	R3, -56(R6)
	MOVQ	R4, -48(R6)
	MOVQ	R5, -40(R6)
	MOVQ	R22, -32(R6)
	MOVQ	R23, -24(R6)
	MOVQ	R24, -16(R6)
	MOVQ	R25, -8(R6)
	JMP	_fblock

_f8tail:
	SUBL	$(QUAD-1), R8, R9
_f8block:
	CMPUGT	R9, R6, R1
	BEQ	R1, _f1tail

	MOVQ	(R7), R2
	ADDL	$8, R6
	ADDL	$8, R7
	MOVQ	R2, -8(R6)
	JMP	_f8block

_f1tail:
	CMPUGT	R8, R6, R1
	BEQ	R1, _fret
	MOVBU	(R7), R2
	ADDL	$1, R6, R6
	ADDL	$1, R7, R7
	MOVB	R2, -1(R6)
	JMP	_f1tail

_fret:
	RET

_funaligned:
	SUBL	$(16-1), R8, R9
_fu8block:
	CMPUGT	R9, R6, R1
	BEQ	R1, _f1tail

	MOVQU	(R7), R2
	MOVQU	8(R7), R3
	MOVQU	16(R7), R4
	EXTQL	R7, R2, R2
	EXTQH	R7, R3, R5
	OR	R5, R2, R11
	EXTQL	R7, R3, R3
	MOVQ	R11, (R6)
	EXTQH	R7, R4, R4
	OR	R3, R4, R11
	MOVQ	R11, 8(R6)
	ADDL	$16, R6
	ADDL	$16, R7
	JMP	_fu8block

TEXT	memcpy(SB), $0
	JMP	_memmove
