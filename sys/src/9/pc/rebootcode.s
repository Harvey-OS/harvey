#include "mem.h"

/*
 * Turn off MMU, then memmove the new kernel to its correct location
 * in physical memory.  Then jump to the start of the kernel.
 */

TEXT	main(SB),$0
	MOVL	p1+0(FP), DI		/* destination */
	MOVL	DI, AX			/* entry point */
	MOVL	p2+4(FP), SI		/* source */
	MOVL	n+8(FP), CX		/* byte count */

/*
 * disable paging
 */
	MOVL	CR0, DX
	ANDL	$~0x80000000, DX		/* ~(PG) */
	MOVL	DX, CR0
	MOVL	$0, DX
	MOVL	DX, CR3

/*
 * the source and destination may overlap.
 * determine whether to copy forward or backwards
 */
	CMPL	SI, DI
	JGT	_forward
	MOVL	SI, DX
	ADDL	CX, DX
	CMPL	DX, DI
	JGT	_back

_forward:
	CLD
	REP;	MOVSB
	JMP	_startkernel

_back:
	ADDL	CX, DI
	ADDL	CX, SI
	SUBL	$1, DI
	SUBL	$1, SI
	STD
	REP;	MOVSB
	JMP	_startkernel
/*
 * JMP to kernel entry point.  Note the true kernel entry point is
 * the virtual address KZERO|AX, but this must wait until
 * the MMU is enabled by the kernel in l.s
 */
_startkernel:
	ORL	AX, AX		/* NOP: avoid link bug */
	JMP*	AX
