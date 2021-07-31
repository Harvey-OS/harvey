#include "mem.h"

#define OP16	BYTE	$0x66

TEXT	origin(SB),$0


/*
 *	This part of l.s is used only in the boot kernel.
 *	It assumes that we are in real address mode, i.e.,
 *	that we look like an 8086.
 */

/*
 *	Ask MSDOS to turn on CGA mode
 */
	XORL	AX,AX
	MOVB	$3,AL
	INT	$0x10

/*
 *	about to walk all over ms/dos - turn off interrupts
 */
	CLI

/*
 *	relocate everything to a half meg and jump there
 *	- looks wierd because it is being assembled by a 32 bit
 *	  assembler for a 16 bit world
 */
	MOVL	$0,BX
	INCL	BX
	SHLL	$15,BX
	MOVL	BX,CX
	MOVW	BX,ES
	MOVL	$0,SI
	MOVL	SI,DI
	CLD
	REP
	MOVSL
/*	JMPFAR	0X8000:$lowcore(SB) /**/
	 BYTE	$0xEA
	 WORD	$lowcore(SB)
	 WORD	$0X8000

TEXT	lowcore(SB),$0

/*
 *	now that we're in low core, update the DS
 */

	MOVW	BX,DS

/*
 * 	goto protected mode
 */
/*	MOVL	tgdtptr(SB),GDTR /**/
	 BYTE	$0x0f
	 BYTE	$0x01
	 BYTE	$0x16
	 WORD	$tgdtptr(SB)
	MOVL	CR0,AX
	ORL	$1,AX
	MOVL	AX,CR0

/*
 *	clear prefetch queue (wierd code to avoid optimizations)
 */
	CLC
	JCC	flush
	MOVL	AX,AX
flush:

/*
 *	set all segs
 */
/*	MOVW	$SELECTOR(1, SELGDT, 0),AX	/**/
	 BYTE	$0xc7
	 BYTE	$0xc0
	 WORD	$SELECTOR(1, SELGDT, 0)
	MOVW	AX,DS
	MOVW	AX,SS
	MOVW	AX,ES

/*	JMPFAR	SELECTOR(2, SELGDT, 0):$mode32bit(SB) /**/
	 BYTE	$0x66
	 BYTE	$0xEA
	 LONG	$mode32bit-KZERO(SB)
	 WORD	$SELECTOR(2, SELGDT, 0)

TEXT	mode32bit(SB),$0


	/*
	 * Clear BSS
	 */
	LEAL	edata-KZERO(SB),SI
	MOVL	SI,DI
	ADDL	$4,DI
	MOVL	$0,AX
	MOVL	AX,(SI)
	LEAL	end-KZERO(SB),CX
	SUBL	DI,CX
	SHRL	$2,CX
	REP
	MOVSL

	/*
	 *  make a bottom level page table page that maps the first
	 *  16 meg of physical memory
	 */
	LEAL	tpt-KZERO(SB),AX	/* get phys addr of temporary page table */
	ADDL	$(BY2PG-1),AX		/* must be page alligned */
	ANDL	$(~(BY2PG-1)),AX	/* ... */
	MOVL	$(4*1024),CX		/* pte's per page */
	MOVL	$((((4*1024)-1)<<PGSHIFT)|PTEVALID|PTEKERNEL|PTEWRITE),BX
setpte:
	MOVL	BX,-4(AX)(CX*4)
	SUBL	$(1<<PGSHIFT),BX
	LOOP	setpte

	/*
	 *  make a top level page table page that maps the first
	 *  16 meg of memory to 0 thru 16meg and to KZERO thru KZERO+16meg
	 */
	MOVL	AX,BX
	ADDL	$(4*BY2PG),AX
	ADDL	$(PTEVALID|PTEKERNEL|PTEWRITE),BX
	MOVL	BX,0(AX)
	MOVL	BX,((((KZERO>>1)&0x7FFFFFFF)>>(2*PGSHIFT-1-4))+0)(AX)
	ADDL	$BY2PG,BX
	MOVL	BX,4(AX)
	MOVL	BX,((((KZERO>>1)&0x7FFFFFFF)>>(2*PGSHIFT-1-4))+4)(AX)
	ADDL	$BY2PG,BX
	MOVL	BX,8(AX)
	MOVL	BX,((((KZERO>>1)&0x7FFFFFFF)>>(2*PGSHIFT-1-4))+8)(AX)
	ADDL	$BY2PG,BX
	MOVL	BX,12(AX)
	MOVL	BX,((((KZERO>>1)&0x7FFFFFFF)>>(2*PGSHIFT-1-4))+12)(AX)

	/*
	 *  point processor to top level page & turn on paging
	 */
	MOVL	AX,CR3
	MOVL	CR0,AX
	ORL	$0X80000000,AX
	MOVL	AX,CR0

	/*
	 *  use a jump to an absolute location to get the PC into
	 *  KZERO.
	 */
	LEAL	tokzero(SB),AX
	JMP*	AX

TEXT	tokzero(SB),$0

	/*
	 *  stack and mach
	 */
	MOVL	$mach0(SB),SP
	MOVL	SP,m(SB)
	MOVL	$0,0(SP)
	ADDL	$(MACHSIZE-4),SP	/* start stack above machine struct */
	MOVL	$0, u(SB)

	CALL	main(SB)

loop:
	JMP	loop

GLOBL	mach0+0(SB), $MACHSIZE
GLOBL	u(SB), $4
GLOBL	m(SB), $4
GLOBL	tpt(SB), $(BY2PG*6)

/*
 *  gdt to get us to 32-bit/segmented/unpaged mode
 */
TEXT	tgdt(SB),$0

	/* null descriptor */
	LONG	$0
	LONG	$0

	/* data segment descriptor for 4 gigabytes (PL 0) */
	LONG	$(0xFFFF)
	LONG	$(SEGG|SEGB|(0xF<<16)|SEGP|SEGPL(0)|SEGDATA|SEGW)

	/* exec segment descriptor for 4 gigabytes (PL 0) */
	LONG	$(0xFFFF)
	LONG	$(SEGG|SEGD|(0xF<<16)|SEGP|SEGPL(0)|SEGEXEC|SEGR)

/*
 *  pointer to initial gdt
 */
TEXT	tgdtptr(SB),$0

	WORD	$(3*8)
	LONG	$tgdt-KZERO(SB)

/*
 *  input a byte
 */
TEXT	inb(SB),$0

	MOVL	p+0(FP),DX
	XORL	AX,AX
	INB
	RET

/*
 *  output a byte
 */
TEXT	outb(SB),$0

	MOVL	p+0(FP),DX
	MOVL	b+4(FP),AX
	OUTB
	RET

/*
 *  input a string of shorts from a port
 */
TEXT	inss(SB),$0
	MOVL	p+0(FP),DX
	MOVL	a+4(FP),DI
	MOVL	c+8(FP),CX
	CLD
	REP; OP16; INSL
	RET

/*
 *  output a string of shorts to a port
 */
TEXT	outss(SB),$0
	MOVL	p+0(FP),DX
	MOVL	a+4(FP),SI
	MOVL	c+8(FP),CX
	CLD
	REP; OP16; OUTSL
	RET

/*
 *  test and set
 */
TEXT	tas(SB),$0
	MOVL	$0xdeadead,AX
	MOVL	l+0(FP),BX
	XCHGL	AX,(BX)
	RET

/*
 *  routines to load/read various system registers
 */
GLOBL	idtptr(SB),$6
TEXT	putidt(SB),$0		/* interrupt descriptor table */
	MOVL	t+0(FP),AX
	MOVL	AX,idtptr+2(SB)
	MOVL	l+4(FP),AX
	MOVW	AX,idtptr(SB)
	MOVL	idtptr(SB),IDTR
	RET

GLOBL	gdtptr(SB),$6
TEXT	putgdt(SB),$0		/* global descriptor table */
	MOVL	t+0(FP),AX
	MOVL	AX,gdtptr+2(SB)
	MOVL	l+4(FP),AX
	MOVW	AX,gdtptr(SB)
	MOVL	gdtptr(SB),GDTR
	RET

TEXT	putcr3(SB),$0		/* top level page table pointer */
	MOVL	t+0(FP),AX
	MOVL	AX,CR3
	RET

TEXT	puttr(SB),$0		/* task register */
	MOVL	t+0(FP),AX
	MOVW	AX,TASK
	RET

TEXT	getcr2(SB),$0		/* fault address */
	MOVL	CR2,AX
	RET

/*
 *  special traps
 */
TEXT	intr0(SB),$0
	PUSHL	$0
	PUSHL	$0
	JMP	intrcommon
TEXT	intr1(SB),$0
	PUSHL	$0
	PUSHL	$1
	JMP	intrcommon
TEXT	intr2(SB),$0
	PUSHL	$0
	PUSHL	$2
	JMP	intrcommon
TEXT	intr3(SB),$0
	PUSHL	$0
	PUSHL	$3
	JMP	intrcommon
TEXT	intr4(SB),$0
	PUSHL	$0
	PUSHL	$4
	JMP	intrcommon
TEXT	intr5(SB),$0
	PUSHL	$0
	PUSHL	$5
	JMP	intrcommon
TEXT	intr6(SB),$0
	PUSHL	$0
	PUSHL	$6
	JMP	intrcommon
TEXT	intr7(SB),$0
	PUSHL	$0
	PUSHL	$7
	JMP	intrcommon
TEXT	intr8(SB),$0
	PUSHL	$8
	JMP	intrscommon
TEXT	intr9(SB),$0
	PUSHL	$0
	PUSHL	$9
	JMP	intrcommon
TEXT	intr10(SB),$0
	PUSHL	$10
	JMP	intrscommon
TEXT	intr11(SB),$0
	PUSHL	$11
	JMP	intrscommon
TEXT	intr12(SB),$0
	PUSHL	$12
	JMP	intrscommon
TEXT	intr13(SB),$0
	PUSHL	$13
	JMP	intrscommon
TEXT	intr14(SB),$0
	PUSHL	$14
	JMP	intrscommon
TEXT	intr15(SB),$0
	PUSHL	$0
	PUSHL	$15
	JMP	intrcommon
TEXT	intr16(SB),$0
	PUSHL	$0
	PUSHL	$16
	JMP	intrcommon
TEXT	intr24(SB),$0
	PUSHL	$0
	PUSHL	$24
	JMP	intrcommon
TEXT	intr25(SB),$0
	PUSHL	$0
	PUSHL	$25
	JMP	intrcommon
TEXT	intr26(SB),$0
	PUSHL	$0
	PUSHL	$26
	JMP	intrcommon
TEXT	intr27(SB),$0
	PUSHL	$0
	PUSHL	$27
	JMP	intrcommon
TEXT	intr28(SB),$0
	PUSHL	$0
	PUSHL	$28
	JMP	intrcommon
TEXT	intr29(SB),$0
	PUSHL	$0
	PUSHL	$29
	JMP	intrcommon
TEXT	intr30(SB),$0
	PUSHL	$0
	PUSHL	$30
	JMP	intrcommon
TEXT	intr31(SB),$0
	PUSHL	$0
	PUSHL	$31
	JMP	intrcommon
TEXT	intr32(SB),$0
	PUSHL	$0
	PUSHL	$16
	JMP	intrcommon
TEXT	intr33(SB),$0
	PUSHL	$0
	PUSHL	$33
	JMP	intrcommon
TEXT	intr34(SB),$0
	PUSHL	$0
	PUSHL	$34
	JMP	intrcommon
TEXT	intr35(SB),$0
	PUSHL	$0
	PUSHL	$35
	JMP	intrcommon
TEXT	intr36(SB),$0
	PUSHL	$0
	PUSHL	$36
	JMP	intrcommon
TEXT	intr37(SB),$0
	PUSHL	$0
	PUSHL	$37
	JMP	intrcommon
TEXT	intr38(SB),$0
	PUSHL	$0
	PUSHL	$38
	JMP	intrcommon
TEXT	intr39(SB),$0
	PUSHL	$0
	PUSHL	$39
	JMP	intrcommon
TEXT	intr64(SB),$0
	PUSHL	$0
	PUSHL	$64
	JMP	intrcommon
TEXT	intrbad(SB),$0
	PUSHL	$0
	PUSHL	$0x1ff
	JMP	intrcommon

intrcommon:
	PUSHL	DS
	PUSHL	ES
	PUSHAL
	MOVL	$(KDSEL),AX
	MOVW	AX,DS
	MOVW	AX,ES
	LEAL	0(SP),AX
	PUSHL	AX
	CALL	trap(SB)
	POPL	AX
	POPAL
	POPL	ES
	POPL	DS
	ADDL	$8,SP	/* error code and trap type */
	IRETL

intrscommon:
	PUSHL	DS
	PUSHL	ES
	PUSHAL
	MOVL	$(KDSEL),AX
	MOVW	AX,DS
	MOVW	AX,ES
	LEAL	0(SP),AX
	PUSHL	AX
	CALL	trap(SB)
	POPL	AX
	POPAL
	POPL	ES
	POPL	DS
	ADDL	$8,SP	/* error code and trap type */
	IRETL

/*
 *  interrupt level is interrupts on or off
 */
TEXT	spllo(SB),$0
	PUSHFL
	POPL	AX
	STI
	RET

TEXT	splhi(SB),$0
	PUSHFL
	POPL	AX
	CLI
	RET

TEXT	splx(SB),$0
	MOVL	s+0(FP),AX
	PUSHL	AX
	POPFL
	RET

/*
 *  do nothing whatsoever till interrupt happens
 */
TEXT	idle(SB),$0
	HLT
	RET

/*
 *  label consists of a stack pointer and a PC
 */
TEXT	gotolabel(SB),$0
	MOVL	l+0(FP),AX
	MOVL	0(AX),SP	/* restore sp */
	MOVL	4(AX),AX	/* put return pc on the stack */
	MOVL	AX,0(SP)
	MOVL	$1,AX		/* return 1 */
	RET

TEXT	setlabel(SB),$0
	MOVL	l+0(FP),AX
	MOVL	SP,0(AX)	/* store sp */
	MOVL	0(SP),BX	/* store return pc */
	MOVL	BX,4(AX)
	MOVL	$0,AX		/* return 0 */
	RET

/*
 *  Used to get to the first process.
 *  Set up an interrupt return frame and IRET to user level.
 */
TEXT	touser(SB),$0
	PUSHL	$(UDSEL)		/* old ss */
	PUSHL	$(USTKTOP)		/* old sp */
	PUSHFL				/* old flags */
	PUSHL	$(UESEL)		/* old cs */
	PUSHL	$(UTZERO+32)		/* old pc */
	MOVL	$(UDSEL),AX
	MOVW	AX,DS
	MOVW	AX,ES
	IRETL

/*
 *  save/restore floating point
 *	- to be filled in at some future time
 */
TEXT	fpsave(SB),$0
	RET

TEXT	fprestore(SB),$0
	RET


/*
 *  set configuration register
 */
TEXT	config(SB),$0
	MOVL	l+0(FP),AX
	MOVL	$0x3F3,DX
	OUTB
	OUTB
	RET
