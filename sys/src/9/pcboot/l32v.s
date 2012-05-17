#include "/sys/src/boot/pc/x16.h"
#include "mem.h"

#define KB		1024
#define MB		(1024*1024)

#define WRMSR		BYTE $0x0F; BYTE $0x30	/* WRMSR, argument in AX/DX (lo/hi) */
#define RDTSC 		BYTE $0x0F; BYTE $0x31	/* RDTSC, result in AX/DX (lo/hi) */
#define RDMSR		BYTE $0x0F; BYTE $0x32	/* RDMSR, result in AX/DX (lo/hi) */
#define CPUID		BYTE $0x0F; BYTE $0xA2	/* CPUID, argument in AX */

TEXT _start32v(SB),$0
	CLI

	MOVL	$edata(SB), DI
	XORL	AX, AX
	MOVL	$end(SB), CX
	SUBL	DI, CX			/* end-edata bytes */
	SHRL	$2, CX			/* end-edata doublewords */

	CLD
	REP; STOSL			/* clear BSS */

	MOVL	CR3, AX
	/* 1+LOWPTEPAGES zeroed pages at (AX): pdb, pte */
	ADDL	$KZERO, AX		/* VA of PDB */
	MOVL	AX, mach0pdb(SB)
	ADDL	$((1+LOWPTEPAGES)*BY2PG), AX /* skip pdb & n pte */
	MOVL	AX, memstart(SB)	/* start of available memory */

	/* 2 zeroed pages at CPU0MACH: mach, gdt */
	MOVL	$CPU0MACH, AX
	MOVL	AX, mach0m(SB)		/* ... VA of Mach */
	MOVL	AX, m(SB)		/* initialise global Mach pointer */
	MOVL	$0, 0(AX)		/* initialise m->machno */
	ADDL	$MACHSIZE, AX
	MOVL	AX, SP			/* switch to new stack in Mach */
	MOVL	$CPU0GDT, mach0gdt(SB)

	MOVL	$0x240000, AX		/* try to set Id|Ac in EFLAGS */
	PUSHL	AX
	POPFL

	PUSHFL				/* retrieve EFLAGS -> BX */
	POPL	BX

	MOVL	$0, AX			/* clear Id|Ac, EFLAGS initialised */
	PUSHL	AX
	POPFL

	PUSHFL				/* retrieve EFLAGS -> AX */

	XORL	BX, (SP)		/* togglable bits */
	CALL	main(SB)

/*
 * Park a processor. Should never fall through a return from main to here,
 * should only be called by application processors when shutting down.
 */
TEXT idle(SB), $0
_idle:
	STI
	HLT
	JMP	_idle

TEXT hlt(SB), $0
	STI
	HLT
	RET

#ifdef UNUSED
/*
 */
TEXT _warp9o(SB), $0
	MOVL	entry+0(FP), CX
	MOVL	multibootheader-KZERO(SB), BX	/* multiboot data pointer */
	MOVL	$0x2badb002, AX			/* multiboot magic */

	CLI
	JMP*	CX

	JMP	_idle

/*
 * Macro for calculating offset within the page directory base.
 * Note that this is assembler-specific hence the '<<2'.
 */
#define PDO(a)		(((((a))>>22) & 0x03FF)<<2)

/*
 */
TEXT _warp9(SB), $0
	CLI
	MOVL	entry+0(FP), BP

	MOVL	CR3, CX				/* load address of PDB */
	ADDL	$KZERO, CX
	MOVL	PDO(KZERO)(CX), DX		/* double-map KZERO at 0 */
	MOVL	DX, PDO(0)(CX)

	MOVL	CR3, CX
	MOVL	CX, CR3				/* load and flush the mmu */

	MOVL	$_start32id<>-KZERO(SB), AX
	JMP*	AX				/* jump to identity-map */

TEXT _start32id<>(SB), $0
	MOVL	CR0, DX				/* turn off paging */
	ANDL	$~PG, DX

	MOVL	$_stop32pg<>-KZERO(SB), AX
	MOVL	DX, CR0
	JMP*	AX				/* forward into the past */

TEXT _stop32pg<>(SB), $0
	MOVL	multibootheader-KZERO(SB), BX	/* multiboot data pointer */
	MOVL	$0x2badb002, AX			/* multiboot magic */

	JMP*	BP

	JMP	_idle
#endif

/*
 *  input a byte
 */
TEXT	inb(SB),$0

	MOVL	p+0(FP),DX
	XORL	AX,AX
	INB
	RET

/*
 * input a short from a port
 */
TEXT	ins(SB), $0

	MOVL	p+0(FP), DX
	XORL	AX, AX
	OPSIZE; INL
	RET

/*
 * input a long from a port
 */
TEXT	inl(SB), $0

	MOVL	p+0(FP), DX
	XORL	AX, AX
	INL
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
 * output a short to a port
 */
TEXT	outs(SB), $0
	MOVL	p+0(FP), DX
	MOVL	s+4(FP), AX
	OPSIZE; OUTL
	RET

/*
 * output a long to a port
 */
TEXT	outl(SB), $0
	MOVL	p+0(FP), DX
	MOVL	s+4(FP), AX
	OUTL
	RET

/*
 *  input a string of bytes from a port
 */
TEXT	insb(SB),$0

	MOVL	p+0(FP),DX
	MOVL	a+4(FP),DI
	MOVL	c+8(FP),CX
	CLD; REP; INSB
	RET

/*
 *  input a string of shorts from a port
 */
TEXT	inss(SB),$0
	MOVL	p+0(FP),DX
	MOVL	a+4(FP),DI
	MOVL	c+8(FP),CX
	CLD
	REP; OPSIZE; INSL
	RET

/*
 *  output a string of bytes to a port
 */
TEXT	outsb(SB),$0

	MOVL	p+0(FP),DX
	MOVL	a+4(FP),SI
	MOVL	c+8(FP),CX
	CLD; REP; OUTSB
	RET

/*
 *  output a string of shorts to a port
 */
TEXT	outss(SB),$0
	MOVL	p+0(FP),DX
	MOVL	a+4(FP),SI
	MOVL	c+8(FP),CX
	CLD
	REP; OPSIZE; OUTSL
	RET

/*
 *  input a string of longs from a port
 */
TEXT	insl(SB),$0

	MOVL	p+0(FP),DX
	MOVL	a+4(FP),DI
	MOVL	c+8(FP),CX
	CLD; REP; INSL
	RET

/*
 *  output a string of longs to a port
 */
TEXT	outsl(SB),$0

	MOVL	p+0(FP),DX
	MOVL	a+4(FP),SI
	MOVL	c+8(FP),CX
	CLD; REP; OUTSL
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

TEXT lgdt(SB), $0			/* GDTR - global descriptor table */
	MOVL	gdtptr+0(FP), AX
	MOVL	(AX), GDTR
	RET

TEXT lidt(SB), $0			/* IDTR - interrupt descriptor table */
	MOVL	idtptr+0(FP), AX
	MOVL	(AX), IDTR
	RET

TEXT	putcr3(SB),$0		/* top level page table pointer */
	MOVL	t+0(FP),AX
	MOVL	AX,CR3
	RET

TEXT	getcr0(SB),$0		/* coprocessor bits */
	MOVL	CR0,AX
	RET

TEXT	getcr2(SB),$0		/* fault address */
	MOVL	CR2,AX
	RET

TEXT	getcr3(SB),$0		/* page directory base */
	MOVL	CR3,AX
	RET

TEXT	getcr4(SB), $0		/* CR4 - extensions */
	MOVL	CR4, AX
	RET

TEXT putcr4(SB), $0
	MOVL	cr4+0(FP), AX
	MOVL	AX, CR4
	RET

TEXT _cycles(SB), $0				/* time stamp counter */
	RDTSC
	MOVL	vlong+0(FP), CX			/* &vlong */
	MOVL	AX, 0(CX)			/* lo */
	MOVL	DX, 4(CX)			/* hi */
	RET

TEXT rdmsr(SB), $0				/* model-specific register */
	MOVL	index+0(FP), CX
	RDMSR
	MOVL	vlong+4(FP), CX			/* &vlong */
	MOVL	AX, 0(CX)			/* lo */
	MOVL	DX, 4(CX)			/* hi */
	RET
	
TEXT wrmsr(SB), $0
	MOVL	index+0(FP), CX
	MOVL	lo+4(FP), AX
	MOVL	hi+8(FP), DX
	WRMSR
	RET

/*
 * memory barriers
 */
TEXT mb386(SB), $0
	POPL	AX				/* return PC */
	PUSHFL
	PUSHL	CS
	PUSHL	AX
	IRETL

TEXT mb586(SB), $0
	XORL	AX, AX
	CPUID
	RET

TEXT sfence(SB), $0
	BYTE $0x0f
	BYTE $0xae
	BYTE $0xf8
	RET

TEXT lfence(SB), $0
	BYTE $0x0f
	BYTE $0xae
	BYTE $0xe8
	RET

TEXT mfence(SB), $0
	BYTE $0x0f
	BYTE $0xae
	BYTE $0xf0
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
	JMP	intrcommon
TEXT	intr9(SB),$0
	PUSHL	$0
	PUSHL	$9
	JMP	intrcommon
TEXT	intr10(SB),$0
	PUSHL	$10
	JMP	intrcommon
TEXT	intr11(SB),$0
	PUSHL	$11
	JMP	intrcommon
TEXT	intr12(SB),$0
	PUSHL	$12
	JMP	intrcommon
TEXT	intr13(SB),$0
	PUSHL	$13
	JMP	intrcommon
TEXT	intr14(SB),$0
	PUSHL	$14
	JMP	intrcommon
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
	PUSHL	$32
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
	PUSHL	FS
	PUSHL	GS
	PUSHAL
	MOVL	$(KDSEL),AX
	MOVW	AX,DS
	MOVW	AX,ES
	LEAL	0(SP),AX
	PUSHL	AX
	CALL	trap(SB)
	POPL	AX
	POPAL
	POPL	GS
	POPL	FS
	POPL	ES
	POPL	DS
	ADDL	$8,SP	/* error code and trap type */
	IRETL


/*
 *  interrupt level is interrupts on or off.
 *  kprof knows that spllo to spldone is splx routines.
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

TEXT spldone(SB), $0
	RET

TEXT islo(SB), $0
	PUSHFL
	POPL	AX
	ANDL	$0x200, AX			/* interrupt enable flag */
	RET

/*
 *  basic timing loop to determine CPU frequency
 */
TEXT	aamloop(SB),$0

	MOVL	c+0(FP),CX
aaml1:
	AAM
	LOOP	aaml1
	RET
