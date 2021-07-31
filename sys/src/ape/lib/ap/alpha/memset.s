TEXT memset(SB), $0
	MOVL	R0, R6
	MOVBU	data+4(FP), R2
	MOVL	n+8(FP), R10

	ADDL	R10, R0, R8

	CMPUGE	$8, R10, R1		/* need at least 8 bytes */
	BNE	R1, _1loop

	SLLQ	$8, R2, R1		/* replicate the byte */
	OR	R1, R2
	SLLQ	$16, R2, R1
	OR	R1, R2
	SLLQ	$32, R2, R1
	OR	R1, R2

_align:
	AND	$(8-1), R6, R1
	BEQ	R1, _aligned

	MOVB	R2, (R6)
	ADDL	$1, R6, R6
	JMP	_align

_aligned:
	SUBL	$(64-1), R8, R9		/* end pointer minus slop */
_64loop:
	CMPUGT	R9, R6, R1
	BEQ	R1, _8tail

	MOVQ	R2, (R6)
	MOVQ	R2, 8(R6)
	MOVQ	R2, 16(R6)
	MOVQ	R2, 24(R6)
	MOVQ	R2, 32(R6)
	MOVQ	R2, 40(R6)
	MOVQ	R2, 48(R6)
	MOVQ	R2, 56(R6)
	ADDL	$64, R6, R6
	JMP	_64loop

_8tail:
	SUBL	$(8-1), R8, R9
_8loop:
	CMPUGT	R9, R6, R1
	BEQ	R1, _1loop

	MOVQ	R2, (R6)
	ADDL	$8, R6
	JMP	_8loop

_1loop:
	CMPUGT	R8, R6, R1
	BEQ	R1, _ret
	MOVB	R2, (R6)
	ADDL	$1, R6
	JMP	_1loop

_ret:
	RET
