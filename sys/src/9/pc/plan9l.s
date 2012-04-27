#include "mem.h"

/*
 * This must match io.h.
 */
#define VectorSYSCALL	0x40

/*
 *  Used to get to the first process:
 * 	set up an interrupt return frame and IRET to user level.
 */
TEXT touser(SB), $0
	PUSHL	$(UDSEL)			/* old ss */
	MOVL	sp+0(FP), AX			/* old sp */
	PUSHL	AX
	MOVL	$0x200, AX			/* interrupt enable flag */
	PUSHL	AX				/* old flags */
	PUSHL	$(UESEL)			/* old cs */
	PUSHL	$(UTZERO+32)			/* old pc */
	MOVL	$(UDSEL), AX
	MOVW	AX, DS
	MOVW	AX, ES
	MOVW	AX, GS
	MOVW	AX, FS
	IRETL

/*
 * This is merely _strayintr from l.s optimised to vector
 * to syscall() without going through trap().
 */
TEXT _syscallintr(SB), $0
	PUSHL	$VectorSYSCALL			/* trap type */

	PUSHL	DS
	PUSHL	ES
	PUSHL	FS
	PUSHL	GS
	PUSHAL
	MOVL	$(KDSEL), AX
	MOVW	AX, DS
	MOVW	AX, ES
	PUSHL	SP
	CALL	syscall(SB)

	POPL	AX
	POPAL
	POPL	GS
	POPL	FS
	POPL	ES
	POPL	DS
	ADDL	$8, SP				/* pop error code and trap type */
	IRETL
