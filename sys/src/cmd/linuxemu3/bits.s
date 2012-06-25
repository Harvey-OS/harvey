TEXT	incref(SB),$0
	MOVL	l+0(FP),AX
	LOCK
	INCL	0(AX)
	RET

TEXT	decref(SB),$0
	MOVL	l+0(FP),AX
	LOCK
	DECL	0(AX)
	JZ	iszero
	MOVL	$1, AX
	RET
iszero:
	MOVL	$0, AX
	RET

TEXT jumpureg(SB), 1, $0
	MOVL ureg+0(FP), AX	/* ureg in AX */
	MOVL 68(AX), SP		/* restore SP */
	SUBL $12, SP
	MOVL 28(AX), BX		/* put AX on 4(SP) */
	MOVL BX, 4(SP)
	MOVL 56(AX), BX		/* put PC on 8(SP) */
	MOVL BX, 8(SP)
	MOVL 0(AX), DI		/* restore registers */
	MOVL 4(AX), SI
	MOVL 8(AX), BP
	MOVL 16(AX), BX
	MOVL 20(AX), DX
	MOVL 24(AX), CX
	MOVL 4(SP), AX		/* restore AX */
	ADDL $8, SP
	RET

TEXT linux_sigreturn(SB), 1, $0
	MOVL $119, AX		/* sys_sigreturn */
	INT $0x80
	RET

TEXT linux_rtsigreturn(SB), 1, $0
	MOVL $173, AX		/* sys_rt_sigreturn */
	INT $0x80
	RET

TEXT get_ds(SB), 1, $0
	PUSHL DS
	POPL AX
	RET
TEXT get_cs(SB), 1, $0
	PUSHL CS
	POPL AX
	RET
