TEXT ptclbsum(SB), $0
	MOVL	addr+0(FP), SI
	MOVL	len+4(FP), CX

	XORL	AX, AX			/* sum */

	TESTL	$1, SI			/* byte aligned? */
	MOVL	SI, DI
	JEQ	_2align

	DECL	CX
	JLT	_return

	MOVB	0x00(SI), AH
	INCL	SI

_2align:
	TESTL	$2, SI			/* word aligned? */
	JEQ	_32loop

	CMPL	CX, $2			/* less than 2 bytes? */
	JLT	_1dreg
	SUBL	$2, CX

	XORL	BX, BX
	MOVW	0x00(SI), BX
	ADDL	BX, AX
	ADCL	$0, AX
	LEAL	2(SI), SI

_32loop:
	CMPL	CX, $0x20
	JLT	_8loop

	MOVL	CX, BP
	SHRL	$5, BP
	ANDL	$0x1F, CX

_32loopx:
	MOVL	0x00(SI), BX
	MOVL	0x1C(SI), DX
	ADCL	BX, AX
	MOVL	0x04(SI), BX
	ADCL	DX, AX
	MOVL	0x10(SI), DX
	ADCL	BX, AX
	MOVL	0x08(SI), BX
	ADCL	DX, AX
	MOVL	0x14(SI), DX
	ADCL	BX, AX
	MOVL	0x0C(SI), BX
	ADCL	DX, AX
	MOVL	0x18(SI), DX
	ADCL	BX, AX
	LEAL	0x20(SI), SI
	ADCL	DX, AX

	DECL	BP
	JNE	_32loopx

	ADCL	$0, AX

_8loop:
	CMPL	CX, $0x08
	JLT	_2loop

	MOVL	CX, BP
	SHRL	$3, BP
	ANDL	$0x07, CX

_8loopx:
	MOVL	0x00(SI), BX
	ADCL	BX, AX
	MOVL	0x04(SI), DX
	ADCL	DX, AX

	LEAL	0x08(SI), SI
	DECL	BP
	JNE	_8loopx

	ADCL	$0, AX

_2loop:
	CMPL	CX, $0x02
	JLT	_1dreg

	MOVL	CX, BP
	SHRL	$1, BP
	ANDL	$0x01, CX

_2loopx:
	MOVWLZX	0x00(SI), BX
	ADCL	BX, AX

	LEAL	0x02(SI), SI
	DECL	BP
	JNE	_2loopx

	ADCL	$0, AX

_1dreg:
	TESTL	$1, CX			/* 1 byte left? */
	JEQ	_fold

	XORL	BX, BX
	MOVB	0x00(SI), BX
	ADDL	BX, AX
	ADCL	$0, AX

_fold:
	MOVL	AX, BX
	SHRL	$16, BX
	JEQ	_swab

	ANDL	$0xFFFF, AX
	ADDL	BX, AX
	JMP	_fold

_swab:
	TESTL	$1, addr+0(FP)
	/*TESTL	$1, DI*/
	JNE	_return
	XCHGB	AH, AL

_return:
	RET
