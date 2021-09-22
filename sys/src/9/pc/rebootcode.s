#include "mem.h"

#define WBINVD	BYTE $0x0F; BYTE $0x09

/* This is copied to REBOOTADDR before executing it. */

/*
 * Turn off MMU, then memmove the new kernel to its correct location
 * in physical memory.  Then jump to the start of the kernel.
 */

TEXT	main(SB),$0
	MOVL	p1+0(FP), DI		/* destination */
	MOVL	DI, AX			/* also entry point */
	MOVL	p2+4(FP), SI		/* source */
	MOVL	n+8(FP), CX		/* byte count */

/*
 * disable paging
 */
	MOVL	CR0, DX
	ANDL	$~PG, DX
/**/	MOVL	DX, CR0
	MOVL	$0, DX
	MOVL	DX, CR3

/*
 * the source and destination may overlap.
 * determine whether to copy forward or backwards
 */
	CLD				/* assume forward copy */
	CMPL	SI, DI
	JGT	_copy			/* src > dest, copy forward */
	MOVL	SI, DX
	ADDL	CX, DX
	CMPL	DX, DI
	JLE	_copy			/* src+len <= dest, copy forward */
_back:
	ADDL	CX, DI
	ADDL	CX, SI
	SUBL	$1, DI
	SUBL	$1, SI
	STD
_copy:
	REP;	MOVSB

/*
 * JMP to kernel entry point.  Note the true kernel entry point is
 * the virtual address KZERO|AX, but this must wait until
 * the MMU is enabled by the kernel in l.s.
 */
	MOVL	AX, DX			/* entry address */
	MOVL	$MBOOTREGMAG, AX	/* multiboot magic */
	MOVL	$(MBOOTTAB-KZERO), BX	/* multiboot data pointer */
	WBINVD
	ORL	DX, DX			/* NOP: avoid link bug */
	JMP*	DX
