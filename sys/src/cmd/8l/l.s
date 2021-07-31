#include "mem.h"

TEXT	start(SB),$0

	JMP	start16

/*
 *  gdt to get us to 32-bit/segmented/unpaged mode
 */
TEXT	tgdt(SB),$0

	/* null descriptor */
	LONG	$0
	LONG	$0

	/* data segment descriptor for 4 gigabytes (PL 0) */
	LONG	$(SEGG|SEGB|(0xF<<16)|SEGP|SEGPL(0)|SEGDATA|SEGW)
	LONG	$0xFFFF

	/* exec segment descriptor for 4 gigabytes (PL 0) */
	LONG	$(SEGG|SEGD|(0xF<<16)|SEGP|SEGPL(0)|SEGEXEC|SEGR)
	LONG	$(0xFFFF)

/*
 *  pointer to initial gdt
 */
TEXT	tgdtptr(SB),$0

	WORD	$(3*8)
	LONG	$tgdt(SB)

/*
 *  come here in (16bit) real-address mode
 */
start16:
	CLI			/* disable interrupts */

	/* point data segment at low memory */
	XORL	AX,AX
	MOVW	AX,DS

	/* point CPU at the interrupt descriptor table */
	MOVL	tgdt(SB),IDTR

	/* point CPU at the global descriptor table */
	MOVL	gdtptr(SB),GDTR

	/* switch to protected mode */
	MOVL	CR0,AX
	ORL	$1,AX
	MOVL	AX,CR0

	/* clear prefetch buffer */
	JMP	flush

/*
 *  come here in USE16 protected mode
 */
flush:
	/* map data and stack address to physical memory */
	MOVW	$SELECTOR(1, SELGDT, 0),AX
	MOVW	AX,DS
	MOVW	AX,ES
	MOVW	AX,SS

	/* switch to 32 bit code
	JMPFAR	SELECTOR(2, SELGDT, 0):start32 */

/*
 *  come here in USE32 protected mode
 */
TEXT	start32(SB),$0

	/* clear bss (should use REP) */
	MOVL	$edata(SB),SI
	MOVL	$edata+4(SB),DI
	MOVL	$end(SB),CX
	SUBL	DI,CX
	SHRL	$2,CX
	XORL	AX,AX
	MOVL	AX,edata(SB)
	CLD
	REP;	MOVSL


	CALL	main(SB)
	/* never returns */

/*
 *  first 16 ``standard'' traps
 */
TEXT	trap0(SB),$0

	PUSHL	$0	/* put on an error code */
	PUSHL	$0
	JMP	alltrap

TEXT	trap1(SB),$0

	PUSHL	$0	/* put on an error code */
	PUSHL	$1
	JMP	alltrap

TEXT	trap2(SB),$0

	PUSHL	$0	/* put on an error code */
	PUSHL	$2
	JMP	alltrap

TEXT	trap3(SB),$0

	PUSHL	$0	/* put on an error code */
	PUSHL	$3
	JMP	alltrap

TEXT	trap4(SB),$0

	PUSHL	$0	/* put on an error code */
	PUSHL	$4
	JMP	alltrap

TEXT	trap5(SB),$0

	PUSHL	$0	/* put on an error code */
	PUSHL	$5
	JMP	alltrap

TEXT	trap6(SB),$0

	PUSHL	$0	/* put on an error code */
	PUSHL	$6
	JMP	alltrap

TEXT	trap7(SB),$0

	PUSHL	$0	/* put on an error code */
	PUSHL	$7
	JMP	alltrap

TEXT	trap8(SB),$0

	PUSHL	$8
	JMP	alltrap

TEXT	trap9(SB),$0

	PUSHL	$0	/* put on an error code */
	PUSHL	$9
	JMP	alltrap

TEXT	trap10(SB),$0

	PUSHL	$0	/* put on an error code */
	PUSHL	$10
	JMP	alltrap

TEXT	trap11(SB),$0

	PUSHL	$11
	JMP	alltrap

TEXT	trap12(SB),$0

	PUSHL	$12
	JMP	alltrap

TEXT	trap13(SB),$0

	PUSHL	$13
	JMP	alltrap

TEXT	trap14(SB),$0

	PUSHL	$14
	JMP	alltrap

TEXT	trap15(SB),$0

	PUSHL	$0	/* put on an error code */
	PUSHL	$15
	JMP	alltrap

/*
 *  invalid trap
 */
TEXT	invtrap(SB),$0

	PUSHL	$0	/* put on an error code */
	PUSHL	$16
	JMP	alltrap

/*
 *  common trap code
 */
alltrap:

	PUSHL	DS
	PUSHAL
	MOVL	$KDSEL, AX
	MOVW	AX, DS
	CALL	trap(SB)
	POPAL
	POPL	DS
	ADDL	$8,SP		/* pop the trap and error codes */
	IRETL
