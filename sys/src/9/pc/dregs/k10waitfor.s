/*
 * int k10waitfor(int* address, int val, int mwaitext);
 *
 * Combined, thought to be usual, case of monitor+mwait with mwait extensions
 * but no hints, and return on interrupt even if val didn't change.
 * Returns new value of *address.
 * This function and prototype may change.
 */
TEXT k10waitfor(SB), 1, $-4
	MOVL	val+4(FP), BX

	MOVL	address+0(FP), AX		/* linear address to monitor */
	CMPL	(AX), BX			/* already changed? */
	JNE	_wwdone
	XORL	CX, CX				/* no optional extensions yet */
	XORL	DX, DX				/* no optional hints yet */
	MONITOR

	MOVL	address+0(FP), AX		/* linear address to monitor */
	CMPL	(AX), BX			/* changed yet? */
	JNE	_wwdone
	XORL	AX, AX				/* no optional hints yet */
	MOVL	mwaitext+8(FP), CX		/* extensions */
	MWAIT

_wwdone:
	MOVL	address+0(FP), AX		/* linear address to monitor */
	MOVL	(AX), AX
	RET
