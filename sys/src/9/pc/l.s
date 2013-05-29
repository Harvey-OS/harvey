#include "mem.h"
#include "/sys/src/boot/pc/x16.h"
#undef DELAY

#define PADDR(a)	((a) & ~KZERO)
#define KADDR(a)	(KZERO|(a))

/*
 * Some machine instructions not handled by 8[al].
 */
#define OP16		BYTE $0x66
#define DELAY		BYTE $0xEB; BYTE $0x00	/* JMP .+2 */
#define CPUID		BYTE $0x0F; BYTE $0xA2	/* CPUID, argument in AX */
#define WRMSR		BYTE $0x0F; BYTE $0x30	/* WRMSR, argument in AX/DX (lo/hi) */
#define RDTSC 		BYTE $0x0F; BYTE $0x31	/* RDTSC, result in AX/DX (lo/hi) */
#define RDMSR		BYTE $0x0F; BYTE $0x32	/* RDMSR, result in AX/DX (lo/hi) */
#define HLT		BYTE $0xF4
#define INVLPG	BYTE $0x0F; BYTE $0x01; BYTE $0x39	/* INVLPG (%ecx) */
#define WBINVD	BYTE $0x0F; BYTE $0x09
#define FXSAVE		BYTE $0x0f; BYTE $0xae; BYTE $0x00  /* SSE FP save */
#define FXRSTOR		BYTE $0x0f; BYTE $0xae; BYTE $0x08  /* SSE FP restore */

/*
 * Macros for calculating offsets within the page directory base
 * and page tables. Note that these are assembler-specific hence
 * the '<<2'.
 */
#define PDO(a)		(((((a))>>22) & 0x03FF)<<2)
#define PTO(a)		(((((a))>>12) & 0x03FF)<<2)

/*
 * For backwards compatiblity with 9load - should go away when 9load is changed
 * 9load currently sets up the mmu, however the first 16MB of memory is identity
 * mapped, so behave as if the mmu was not setup
 */
TEXT _startKADDR(SB), $0
	MOVL	$_startPADDR(SB), AX
	ANDL	$~KZERO, AX
	JMP*	AX

/*
 * Must be 4-byte aligned.
 */
TEXT _multibootheader(SB), $0
	LONG	$0x1BADB002			/* magic */
	LONG	$0x00010003			/* flags */
	LONG	$-(0x1BADB002 + 0x00010003)	/* checksum */
	LONG	$_multibootheader-KZERO(SB)	/* header_addr */
	LONG	$_startKADDR-KZERO(SB)		/* load_addr */
	LONG	$edata-KZERO(SB)		/* load_end_addr */
	LONG	$end-KZERO(SB)			/* bss_end_addr */
	LONG	$_startKADDR-KZERO(SB)		/* entry_addr */
	LONG	$0				/* mode_type */
	LONG	$0				/* width */
	LONG	$0				/* height */
	LONG	$0				/* depth */

/*
 * In protected mode with paging turned off and segment registers setup
 * to linear map all memory. Entered via a jump to PADDR(entry),
 * the physical address of the virtual kernel entry point of KADDR(entry).
 * Make the basic page tables for processor 0. Six pages are needed for
 * the basic set:
 *	a page directory;
 *	page tables for mapping the first 8MB of physical memory to KZERO;
 *	a page for the GDT;
 *	virtual and physical pages for mapping the Mach structure.
 * The remaining PTEs will be allocated later when memory is sized.
 * An identity mmu map is also needed for the switch to virtual mode.
 * This identity mapping is removed once the MMU is going and the JMP has
 * been made to virtual memory.
 */
TEXT _startPADDR(SB), $0
	CLI					/* make sure interrupts are off */

	/* set up the gdt so we have sane plan 9 style gdts. */
	MOVL	$tgdtptr(SB), AX
	ANDL	$~KZERO, AX
	MOVL	(AX), GDTR
	MOVW	$1, AX
	MOVW	AX, MSW

	/* clear prefetch queue (weird code to avoid optimizations) */
	DELAY

	/* set segs to something sane (avoid traps later) */
	MOVW	$(1<<3), AX
	MOVW	AX, DS
	MOVW	AX, SS
	MOVW	AX, ES
	MOVW	AX, FS
	MOVW	AX, GS

/*	JMP	$(2<<3):$mode32bit(SB) /**/
	 BYTE	$0xEA
	 LONG	$mode32bit-KZERO(SB)
	 WORD	$(2<<3)

/*
 *  gdt to get us to 32-bit/segmented/unpaged mode
 */
TEXT tgdt(SB), $0

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
 *  Note the -KZERO which puts the physical address in the gdtptr. 
 *  that's needed as we start executing in physical addresses. 
 */
TEXT tgdtptr(SB), $0
	WORD	$(3*8)
	LONG	$tgdt-KZERO(SB)

TEXT m0rgdtptr(SB), $0
	WORD	$(NGDT*8-1)
	LONG	$(CPU0GDT-KZERO)

TEXT m0gdtptr(SB), $0
	WORD	$(NGDT*8-1)
	LONG	$CPU0GDT

TEXT m0idtptr(SB), $0
	WORD $(256*8-1)
	LONG $IDTADDR

TEXT mode32bit(SB), $0
	/* At this point, the GDT setup is done. */

	MOVL	$PADDR(CPU0PDB), DI		/* clear 4 pages for the tables etc. */
	XORL	AX, AX
	MOVL	$(4*BY2PG), CX
	SHRL	$2, CX

	CLD
	REP;	STOSL

	MOVL	$PADDR(CPU0PDB), AX
	ADDL	$PDO(KZERO), AX			/* page directory offset for KZERO */
	MOVL	$PADDR(CPU0PTE), (AX)		/* PTE's for KZERO */
	MOVL	$(PTEWRITE|PTEVALID), BX	/* page permissions */
	ORL	BX, (AX)

	ADDL	$4, AX
	MOVL	$PADDR(CPU0PTE1), (AX)		/* PTE's for KZERO+4MB */
	MOVL	$(PTEWRITE|PTEVALID), BX	/* page permissions */
	ORL	BX, (AX)

	MOVL	$PADDR(CPU0PTE), AX		/* first page of page table */
	MOVL	$1024, CX			/* 1024 pages in 4MB */
_setpte:
	MOVL	BX, (AX)
	ADDL	$(1<<PGSHIFT), BX
	ADDL	$4, AX
	LOOP	_setpte

	MOVL	$PADDR(CPU0PTE1), AX		/* second page of page table */
	MOVL	$1024, CX			/* 1024 pages in 4MB */
_setpte1:
	MOVL	BX, (AX)
	ADDL	$(1<<PGSHIFT), BX
	ADDL	$4, AX
	LOOP	_setpte1

	MOVL	$PADDR(CPU0PTE), AX
	ADDL	$PTO(MACHADDR), AX		/* page table entry offset for MACHADDR */
	MOVL	$PADDR(CPU0MACH), (AX)		/* PTE for Mach */
	MOVL	$(PTEWRITE|PTEVALID), BX	/* page permissions */
	ORL	BX, (AX)

/*
 * Now ready to use the new map. Make sure the processor options are what is wanted.
 * It is necessary on some processors to immediately follow mode switching with a JMP instruction
 * to clear the prefetch queues.
 */
	MOVL	$PADDR(CPU0PDB), CX		/* load address of page directory */
	MOVL	(PDO(KZERO))(CX), DX		/* double-map KZERO at 0 */
	MOVL	DX, (PDO(0))(CX)
	MOVL	CX, CR3
	DELAY					/* JMP .+2 */

	MOVL	CR0, DX
	ORL	$0x80010000, DX			/* PG|WP */
	ANDL	$~0x6000000A, DX		/* ~(CD|NW|TS|MP) */

	MOVL	$_startpg(SB), AX		/* this is a virtual address */
	MOVL	DX, CR0				/* turn on paging */
	JMP*	AX				/* jump to the virtual nirvana */

/*
 * Basic machine environment set, can clear BSS and create a stack.
 * The stack starts at the top of the page containing the Mach structure.
 * The x86 architecture forces the use of the same virtual address for
 * each processor's Mach structure, so the global Mach pointer 'm' can
 * be initialised here.
 */
TEXT _startpg(SB), $0
	MOVL	$0, (PDO(0))(CX)		/* undo double-map of KZERO at 0 */
	MOVL	CX, CR3				/* load and flush the mmu */

_clearbss:
	MOVL	$edata(SB), DI
	XORL	AX, AX
	MOVL	$end(SB), CX
	SUBL	DI, CX				/* end-edata bytes */
	SHRL	$2, CX				/* end-edata doublewords */

	CLD
	REP;	STOSL				/* clear BSS */

	MOVL	$MACHADDR, SP
	MOVL	SP, m(SB)			/* initialise global Mach pointer */
	MOVL	$0, 0(SP)			/* initialise m->machno */


	ADDL	$(MACHSIZE-4), SP		/* initialise stack */

/*
 * Need to do one final thing to ensure a clean machine environment,
 * clear the EFLAGS register, which can only be done once there is a stack.
 */
	MOVL	$0, AX
	PUSHL	AX
	POPFL

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

/*
 * Save registers.
 */
TEXT saveregs(SB), $0
	/* appease 8l */
	SUBL $32, SP
	POPL AX
	POPL AX
	POPL AX
	POPL AX
	POPL AX
	POPL AX
	POPL AX
	POPL AX
	
	PUSHL	AX
	PUSHL	BX
	PUSHL	CX
	PUSHL	DX
	PUSHL	BP
	PUSHL	DI
	PUSHL	SI
	PUSHFL

	XCHGL	32(SP), AX	/* swap return PC and saved flags */
	XCHGL	0(SP), AX
	XCHGL	32(SP), AX
	RET

TEXT restoreregs(SB), $0
	/* appease 8l */
	PUSHL	AX
	PUSHL	AX
	PUSHL	AX
	PUSHL	AX
	PUSHL	AX
	PUSHL	AX
	PUSHL	AX
	PUSHL	AX
	ADDL	$32, SP
	
	XCHGL	32(SP), AX	/* swap return PC and saved flags */
	XCHGL	0(SP), AX
	XCHGL	32(SP), AX

	POPFL
	POPL	SI
	POPL	DI
	POPL	BP
	POPL	DX
	POPL	CX
	POPL	BX
	POPL	AX
	RET

/*
 * Assumed to be in protected mode at time of call.
 * Switch to real mode, execute an interrupt, and
 * then switch back to protected mode.  
 *
 * Assumes:
 *
 *	- no device interrupts are going to come in
 *	- 0-16MB is identity mapped in page tables
 *	- realmode() has copied us down from 0x100000 to 0x8000
 *	- can use code segment 0x0800 in real mode
 *		to get at l.s code
 *	- l.s code is less than 1 page
 */
#define RELOC	(RMCODE-KTZERO)

TEXT realmodeidtptr(SB), $0
	WORD	$(4*256-1)
	LONG	$0

TEXT realmode0(SB), $0
	CALL	saveregs(SB)

	/* switch to low code address */
	LEAL	physcode-KZERO(SB), AX
	JMP *AX

TEXT physcode(SB), $0

	/* switch to low stack */
	MOVL	SP, AX
	MOVL	$0x7C00, SP
	PUSHL	AX

	/* change gdt to physical pointer */
	MOVL	m0rgdtptr-KZERO(SB), GDTR

	/* load IDT with real-mode version*/
	MOVL	realmodeidtptr-KZERO(SB), IDTR

	/* edit INT $0x00 instruction below */
	MOVL	$(RMUADDR-KZERO+48), AX	/* &rmu.trap */
	MOVL	(AX), AX
	MOVB	AX, realmodeintrinst+(-KZERO+1+RELOC)(SB)

	/* disable paging */
	MOVL	CR0, AX
	ANDL	$0x7FFFFFFF, AX
	MOVL	AX, CR0
	/* JMP .+2 to clear prefetch queue*/
	BYTE $0xEB; BYTE $0x00

	/* jump to 16-bit code segment */
/*	JMPFAR	SELECTOR(KESEG16, SELGDT, 0):$again16bit(SB) /**/
	 BYTE	$0xEA
	 LONG	$again16bit-KZERO(SB)
	 WORD	$SELECTOR(KESEG16, SELGDT, 0)

TEXT again16bit(SB), $0
	/*
	 * Now in 16-bit compatibility mode.
	 * These are 32-bit instructions being interpreted
	 * as 16-bit instructions.  I'm being lazy and
	 * not using the macros because I know when
	 * the 16- and 32-bit instructions look the same
	 * or close enough.
	 */

	/* disable protected mode and jump to real mode cs */
	OPSIZE; MOVL CR0, AX
	OPSIZE; XORL BX, BX
	OPSIZE; INCL BX
	OPSIZE; XORL BX, AX
	OPSIZE; MOVL AX, CR0

	/* JMPFAR 0x0800:now16real */
	 BYTE $0xEA
	 WORD	$now16real-KZERO(SB)
	 WORD	$0x0800

TEXT now16real(SB), $0
	/* copy the registers for the bios call */
	LWI(0x0000, rAX)
	MOVW	AX,SS
	LWI(RMUADDR, rBP)
	
	/* offsets are in Ureg */
	LXW(44, xBP, rAX)
	MOVW	AX, DS
	LXW(40, xBP, rAX)
	MOVW	AX, ES

	OPSIZE; LXW(0, xBP, rDI)
	OPSIZE; LXW(4, xBP, rSI)
	OPSIZE; LXW(16, xBP, rBX)
	OPSIZE; LXW(20, xBP, rDX)
	OPSIZE; LXW(24, xBP, rCX)
	OPSIZE; LXW(28, xBP, rAX)

	CLC

TEXT realmodeintrinst(SB), $0
	INT $0x00
	CLI			/* who knows what evil the bios got up to */

	/* save the registers after the call */

	LWI(0x7bfc, rSP)
	OPSIZE; PUSHFL
	OPSIZE; PUSHL AX

	LWI(0, rAX)
	MOVW	AX,SS
	LWI(RMUADDR, rBP)
	
	OPSIZE; SXW(rDI, 0, xBP)
	OPSIZE; SXW(rSI, 4, xBP)
	OPSIZE; SXW(rBX, 16, xBP)
	OPSIZE; SXW(rDX, 20, xBP)
	OPSIZE; SXW(rCX, 24, xBP)
	OPSIZE; POPL AX
	OPSIZE; SXW(rAX, 28, xBP)

	MOVW	DS, AX
	OPSIZE; SXW(rAX, 44, xBP)
	MOVW	ES, AX
	OPSIZE; SXW(rAX, 40, xBP)

	OPSIZE; POPL AX
	OPSIZE; SXW(rAX, 64, xBP)	/* flags */

	/* re-enter protected mode and jump to 32-bit code */
	OPSIZE; MOVL $1, AX
	OPSIZE; MOVL AX, CR0
	
/*	JMPFAR	SELECTOR(KESEG, SELGDT, 0):$again32bit(SB) /**/
	 OPSIZE
	 BYTE $0xEA
	 LONG	$again32bit-KZERO(SB)
	 WORD	$SELECTOR(KESEG, SELGDT, 0)

TEXT again32bit(SB), $0
	MOVW	$SELECTOR(KDSEG, SELGDT, 0),AX
	MOVW	AX,DS
	MOVW	AX,SS
	MOVW	AX,ES
	MOVW	AX,FS
	MOVW	AX,GS

	/* enable paging and jump to kzero-address code */
	MOVL	CR0, AX
	ORL	$0x80010000, AX	/* PG|WP */
	MOVL	AX, CR0
	LEAL	again32kzero(SB), AX
	JMP*	AX

TEXT again32kzero(SB), $0
	/* breathe a sigh of relief - back in 32-bit protected mode */

	/* switch to old stack */	
	PUSHL	AX	/* match popl below for 8l */
	MOVL	$0x7BFC, SP
	POPL	SP

	/* restore idt */
	MOVL	m0idtptr(SB),IDTR

	/* restore gdt */
	MOVL	m0gdtptr(SB), GDTR

	CALL	restoreregs(SB)
	RET

/*
 * BIOS32.
 */
TEXT bios32call(SB), $0
	MOVL	ci+0(FP), BP
	MOVL	0(BP), AX
	MOVL	4(BP), BX
	MOVL	8(BP), CX
	MOVL	12(BP), DX
	MOVL	16(BP), SI
	MOVL	20(BP), DI
	PUSHL	BP

	MOVL	12(SP), BP			/* ptr */
	BYTE $0xFF; BYTE $0x5D; BYTE $0x00	/* CALL FAR 0(BP) */

	POPL	BP
	MOVL	DI, 20(BP)
	MOVL	SI, 16(BP)
	MOVL	DX, 12(BP)
	MOVL	CX, 8(BP)
	MOVL	BX, 4(BP)
	MOVL	AX, 0(BP)

	XORL	AX, AX
	JCC	_bios32xxret
	INCL	AX

_bios32xxret:
	RET

/*
 * Port I/O.
 *	in[bsl]		input a byte|short|long
 *	ins[bsl]	input a string of bytes|shorts|longs
 *	out[bsl]	output a byte|short|long
 *	outs[bsl]	output a string of bytes|shorts|longs
 */
TEXT inb(SB), $0
	MOVL	port+0(FP), DX
	XORL	AX, AX
	INB
	RET

TEXT insb(SB), $0
	MOVL	port+0(FP), DX
	MOVL	address+4(FP), DI
	MOVL	count+8(FP), CX
	CLD
	REP;	INSB
	RET

TEXT ins(SB), $0
	MOVL	port+0(FP), DX
	XORL	AX, AX
	OP16;	INL
	RET

TEXT inss(SB), $0
	MOVL	port+0(FP), DX
	MOVL	address+4(FP), DI
	MOVL	count+8(FP), CX
	CLD
	REP;	OP16; INSL
	RET

TEXT inl(SB), $0
	MOVL	port+0(FP), DX
	INL
	RET

TEXT insl(SB), $0
	MOVL	port+0(FP), DX
	MOVL	address+4(FP), DI
	MOVL	count+8(FP), CX
	CLD
	REP;	INSL
	RET

TEXT outb(SB), $0
	MOVL	port+0(FP), DX
	MOVL	byte+4(FP), AX
	OUTB
	RET

TEXT outsb(SB), $0
	MOVL	port+0(FP), DX
	MOVL	address+4(FP), SI
	MOVL	count+8(FP), CX
	CLD
	REP;	OUTSB
	RET

TEXT outs(SB), $0
	MOVL	port+0(FP), DX
	MOVL	short+4(FP), AX
	OP16;	OUTL
	RET

TEXT outss(SB), $0
	MOVL	port+0(FP), DX
	MOVL	address+4(FP), SI
	MOVL	count+8(FP), CX
	CLD
	REP;	OP16; OUTSL
	RET

TEXT outl(SB), $0
	MOVL	port+0(FP), DX
	MOVL	long+4(FP), AX
	OUTL
	RET

TEXT outsl(SB), $0
	MOVL	port+0(FP), DX
	MOVL	address+4(FP), SI
	MOVL	count+8(FP), CX
	CLD
	REP;	OUTSL
	RET

/*
 * Read/write various system registers.
 * CR4 and the 'model specific registers' should only be read/written
 * after it has been determined the processor supports them
 */
TEXT lgdt(SB), $0				/* GDTR - global descriptor table */
	MOVL	gdtptr+0(FP), AX
	MOVL	(AX), GDTR
	RET

TEXT lidt(SB), $0				/* IDTR - interrupt descriptor table */
	MOVL	idtptr+0(FP), AX
	MOVL	(AX), IDTR
	RET

TEXT ltr(SB), $0				/* TR - task register */
	MOVL	tptr+0(FP), AX
	MOVW	AX, TASK
	RET

TEXT getcr0(SB), $0				/* CR0 - processor control */
	MOVL	CR0, AX
	RET

TEXT getcr2(SB), $0				/* CR2 - page fault linear address */
	MOVL	CR2, AX
	RET

TEXT getcr3(SB), $0				/* CR3 - page directory base */
	MOVL	CR3, AX
	RET

TEXT putcr0(SB), $0
	MOVL	cr0+0(FP), AX
	MOVL	AX, CR0
	RET

TEXT putcr3(SB), $0
	MOVL	cr3+0(FP), AX
	MOVL	AX, CR3
	RET

TEXT getcr4(SB), $0				/* CR4 - extensions */
	MOVL	CR4, AX
	RET

TEXT putcr4(SB), $0
	MOVL	cr4+0(FP), AX
	MOVL	AX, CR4
	RET

TEXT invlpg(SB), $0
	/* 486+ only */
	MOVL	va+0(FP), CX
	INVLPG
	RET

TEXT wbinvd(SB), $0
	WBINVD
	RET

TEXT _cycles(SB), $0				/* time stamp counter */
	RDTSC
	MOVL	vlong+0(FP), CX			/* &vlong */
	MOVL	AX, 0(CX)			/* lo */
	MOVL	DX, 4(CX)			/* hi */
	RET

/*
 * stub for:
 * time stamp counter; low-order 32 bits of 64-bit cycle counter
 * Runs at fasthz/4 cycles per second (m->clkin>>3)
 */
TEXT lcycles(SB),1,$0
	RDTSC
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
 * Try to determine the CPU type which requires fiddling with EFLAGS.
 * If the Id bit can be toggled then the CPUID instruction can be used
 * to determine CPU identity and features. First have to check if it's
 * a 386 (Ac bit can't be set). If it's not a 386 and the Id bit can't be
 * toggled then it's an older 486 of some kind.
 *
 *	cpuid(fun, regs[4]);
 */
TEXT cpuid(SB), $0
	MOVL	$0x240000, AX
	PUSHL	AX
	POPFL					/* set Id|Ac */
	PUSHFL
	POPL	BX				/* retrieve value */
	MOVL	$0, AX
	PUSHL	AX
	POPFL					/* clear Id|Ac, EFLAGS initialised */
	PUSHFL
	POPL	AX				/* retrieve value */
	XORL	BX, AX
	TESTL	$0x040000, AX			/* Ac */
	JZ	_cpu386				/* can't set this bit on 386 */
	TESTL	$0x200000, AX			/* Id */
	JZ	_cpu486				/* can't toggle this bit on some 486 */
	/* load registers */
	MOVL	regs+4(FP), BP
	MOVL	fn+0(FP), AX			/* cpuid function */
	MOVL	4(BP), BX
	MOVL	8(BP), CX			/* typically an index */
	MOVL	12(BP), DX
	CPUID
	JMP	_cpuid
_cpu486:
	MOVL	$0x400, AX
	JMP	_maybezapax
_cpu386:
	MOVL	$0x300, AX
_maybezapax:
	CMPL	fn+0(FP), $1
	JE	_zaprest
	XORL	AX, AX
_zaprest:
	XORL	BX, BX
	XORL	CX, CX
	XORL	DX, DX
_cpuid:
	MOVL	regs+4(FP), BP
	MOVL	AX, 0(BP)
	MOVL	BX, 4(BP)
	MOVL	CX, 8(BP)
	MOVL	DX, 12(BP)
	RET

/*
 * Basic timing loop to determine CPU frequency.
 */
TEXT aamloop(SB), $0
	MOVL	count+0(FP), CX
_aamloop:
	AAM
	LOOP	_aamloop
	RET

/*
 * Floating point.
 * Note: the encodings for the FCLEX, FINIT, FSAVE, FSTCW, FSENV and FSTSW
 * instructions do NOT have the WAIT prefix byte (i.e. they act like their
 * FNxxx variations) so WAIT instructions must be explicitly placed in the
 * code as necessary.
 */
#define	FPOFF(l)						 ;\
	MOVL	CR0, AX 					 ;\
	ANDL	$0xC, AX			/* EM, TS */	 ;\
	CMPL	AX, $0x8					 ;\
	JEQ 	l						 ;\
	WAIT							 ;\
l:								 ;\
	MOVL	CR0, AX						 ;\
	ANDL	$~0x4, AX			/* EM=0 */	 ;\
	ORL	$0x28, AX			/* NE=1, TS=1 */ ;\
	MOVL	AX, CR0

#define	FPON							 ;\
	MOVL	CR0, AX						 ;\
	ANDL	$~0xC, AX			/* EM=0, TS=0 */ ;\
	MOVL	AX, CR0

TEXT fpon(SB), $0				/* enable */
	FPON
	RET

TEXT fpoff(SB), $0				/* disable */
	FPOFF(l1)
	RET

TEXT fpinit(SB), $0				/* enable and init */
	FPON
	FINIT
	WAIT
	/* setfcr(FPPDBL|FPRNR|FPINVAL|FPZDIV|FPOVFL) */
	/* note that low 6 bits are masks, not enables, on this chip */
	PUSHW	$0x0232
	FLDCW	0(SP)
	POPW	AX
	WAIT
	RET

TEXT fpx87save(SB), $0				/* save state and disable */
	MOVL	p+0(FP), AX
	FSAVE	0(AX)				/* no WAIT */
	FPOFF(l2)
	RET

TEXT fpx87restore(SB), $0			/* enable and restore state */
	FPON
	MOVL	p+0(FP), AX
	FRSTOR	0(AX)
	WAIT
	RET

TEXT fpstatus(SB), $0				/* get floating point status */
	FSTSW	AX
	RET

TEXT fpenv(SB), $0				/* save state without waiting */
	MOVL	p+0(FP), AX
	FSTENV	0(AX)				/* also masks FP exceptions */
	RET

TEXT fpclear(SB), $0				/* clear pending exceptions */
	FPON
	FCLEX					/* no WAIT */
	FPOFF(l3)
	RET

TEXT fpssesave0(SB), $0				/* save state and disable */
	MOVL	p+0(FP), AX
	FXSAVE					/* no WAIT */
	FPOFF(l4)
	RET

TEXT fpsserestore0(SB), $0			/* enable and restore state */
	FPON
	MOVL	p+0(FP), AX
	FXRSTOR
	WAIT
	RET

/*
 */
TEXT splhi(SB), $0
shi:
	PUSHFL
	POPL	AX
	TESTL	$0x200, AX
	JZ	alreadyhi
	MOVL	$(MACHADDR+0x04), CX 		/* save PC in m->splpc */
	MOVL	(SP), BX
	MOVL	BX, (CX)
alreadyhi:
	CLI
	RET

TEXT spllo(SB), $0
slo:
	PUSHFL
	POPL	AX
	TESTL	$0x200, AX
	JNZ	alreadylo
	MOVL	$(MACHADDR+0x04), CX		/* clear m->splpc */
	MOVL	$0, (CX)
alreadylo:
	STI
	RET

TEXT splx(SB), $0
	MOVL	s+0(FP), AX
	TESTL	$0x200, AX
	JNZ	slo
	JMP	shi

TEXT spldone(SB), $0
	RET

TEXT islo(SB), $0
	PUSHFL
	POPL	AX
	ANDL	$0x200, AX			/* interrupt enable flag */
	RET

/*
 * Test-And-Set
 */
TEXT tas(SB), $0
	MOVL	$0xDEADDEAD, AX
	MOVL	lock+0(FP), BX
	XCHGL	AX, (BX)			/* lock->key */
	RET

TEXT _xinc(SB), $0				/* void _xinc(long*); */
	MOVL	l+0(FP), AX
	LOCK;	INCL 0(AX)
	RET

TEXT _xdec(SB), $0				/* long _xdec(long*); */
	MOVL	l+0(FP), BX
	XORL	AX, AX
	LOCK;	DECL 0(BX)
	JLT	_xdeclt
	JGT	_xdecgt
	RET
_xdecgt:
	INCL	AX
	RET
_xdeclt:
	DECL	AX
	RET

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

TEXT xchgw(SB), $0
	MOVL	v+4(FP), AX
	MOVL	p+0(FP), BX
	XCHGW	AX, (BX)
	RET

TEXT cmpswap486(SB), $0
	MOVL	addr+0(FP), BX
	MOVL	old+4(FP), AX
	MOVL	new+8(FP), CX
	LOCK
	BYTE $0x0F; BYTE $0xB1; BYTE $0x0B	/* CMPXCHGL CX, (BX) */
	JNZ didnt
	MOVL	$1, AX
	RET
didnt:
	XORL	AX,AX
	RET

TEXT mul64fract(SB), $0
/*
 * Multiply two 64-bit number s and keep the middle 64 bits from the 128-bit result
 * See ../port/tod.c for motivation.
 */
	MOVL	r+0(FP), CX
	XORL	BX, BX				/* BX = 0 */

	MOVL	a+8(FP), AX
	MULL	b+16(FP)			/* a1*b1 */
	MOVL	AX, 4(CX)			/* r2 = lo(a1*b1) */

	MOVL	a+8(FP), AX
	MULL	b+12(FP)			/* a1*b0 */
	MOVL	AX, 0(CX)			/* r1 = lo(a1*b0) */
	ADDL	DX, 4(CX)			/* r2 += hi(a1*b0) */

	MOVL	a+4(FP), AX
	MULL	b+16(FP)			/* a0*b1 */
	ADDL	AX, 0(CX)			/* r1 += lo(a0*b1) */
	ADCL	DX, 4(CX)			/* r2 += hi(a0*b1) + carry */

	MOVL	a+4(FP), AX
	MULL	b+12(FP)			/* a0*b0 */
	ADDL	DX, 0(CX)			/* r1 += hi(a0*b0) */
	ADCL	BX, 4(CX)			/* r2 += carry */
	RET

/*
 *  label consists of a stack pointer and a PC
 */
TEXT gotolabel(SB), $0
	MOVL	label+0(FP), AX
	MOVL	0(AX), SP			/* restore sp */
	MOVL	4(AX), AX			/* put return pc on the stack */
	MOVL	AX, 0(SP)
	MOVL	$1, AX				/* return 1 */
	RET

TEXT setlabel(SB), $0
	MOVL	label+0(FP), AX
	MOVL	SP, 0(AX)			/* store sp */
	MOVL	0(SP), BX			/* store return pc */
	MOVL	BX, 4(AX)
	MOVL	$0, AX				/* return 0 */
	RET

/*
 * Attempt at power saving. -rsc
 */
TEXT halt(SB), $0
	CLI
	CMPL	nrdy(SB), $0
	JEQ	_nothingready
	STI
	RET

_nothingready:
	STI
	HLT
	RET

/*
 * Interrupt/exception handling.
 * Each entry in the vector table calls either _strayintr or _strayintrx depending
 * on whether an error code has been automatically pushed onto the stack
 * (_strayintrx) or not, in which case a dummy entry must be pushed before retrieving
 * the trap type from the vector table entry and placing it on the stack as part
 * of the Ureg structure.
 * The size of each entry in the vector table (6 bytes) is known in trapinit().
 */
TEXT _strayintr(SB), $0
	PUSHL	AX			/* save AX */
	MOVL	4(SP), AX		/* return PC from vectortable(SB) */
	JMP	intrcommon

TEXT _strayintrx(SB), $0
	XCHGL	AX, (SP)		/* swap AX with vectortable CALL PC */
intrcommon:
	PUSHL	DS			/* save DS */
	PUSHL	$(KDSEL)
	POPL	DS			/* fix up DS */
	MOVBLZX	(AX), AX		/* trap type -> AX */
	XCHGL	AX, 4(SP)		/* exchange trap type with saved AX */

	PUSHL	ES			/* save ES */
	PUSHL	$(KDSEL)
	POPL	ES			/* fix up ES */

	PUSHL	FS			/* save the rest of the Ureg struct */
	PUSHL	GS
	PUSHAL

	PUSHL	SP			/* Ureg* argument to trap */
	CALL	trap(SB)

TEXT forkret(SB), $0
	POPL	AX
	POPAL
	POPL	GS
	POPL	FS
	POPL	ES
	POPL	DS
	ADDL	$8, SP			/* pop error code and trap type */
	IRETL

TEXT vectortable(SB), $0
	CALL _strayintr(SB); BYTE $0x00		/* divide error */
	CALL _strayintr(SB); BYTE $0x01		/* debug exception */
	CALL _strayintr(SB); BYTE $0x02		/* NMI interrupt */
	CALL _strayintr(SB); BYTE $0x03		/* breakpoint */
	CALL _strayintr(SB); BYTE $0x04		/* overflow */
	CALL _strayintr(SB); BYTE $0x05		/* bound */
	CALL _strayintr(SB); BYTE $0x06		/* invalid opcode */
	CALL _strayintr(SB); BYTE $0x07		/* no coprocessor available */
	CALL _strayintrx(SB); BYTE $0x08	/* double fault */
	CALL _strayintr(SB); BYTE $0x09		/* coprocessor segment overflow */
	CALL _strayintrx(SB); BYTE $0x0A	/* invalid TSS */
	CALL _strayintrx(SB); BYTE $0x0B	/* segment not available */
	CALL _strayintrx(SB); BYTE $0x0C	/* stack exception */
	CALL _strayintrx(SB); BYTE $0x0D	/* general protection error */
	CALL _strayintrx(SB); BYTE $0x0E	/* page fault */
	CALL _strayintr(SB); BYTE $0x0F		/*  */
	CALL _strayintr(SB); BYTE $0x10		/* coprocessor error */
	CALL _strayintrx(SB); BYTE $0x11	/* alignment check */
	CALL _strayintr(SB); BYTE $0x12		/* machine check */
	CALL _strayintr(SB); BYTE $0x13
	CALL _strayintr(SB); BYTE $0x14
	CALL _strayintr(SB); BYTE $0x15
	CALL _strayintr(SB); BYTE $0x16
	CALL _strayintr(SB); BYTE $0x17
	CALL _strayintr(SB); BYTE $0x18
	CALL _strayintr(SB); BYTE $0x19
	CALL _strayintr(SB); BYTE $0x1A
	CALL _strayintr(SB); BYTE $0x1B
	CALL _strayintr(SB); BYTE $0x1C
	CALL _strayintr(SB); BYTE $0x1D
	CALL _strayintr(SB); BYTE $0x1E
	CALL _strayintr(SB); BYTE $0x1F
	CALL _strayintr(SB); BYTE $0x20		/* VectorLAPIC */
	CALL _strayintr(SB); BYTE $0x21
	CALL _strayintr(SB); BYTE $0x22
	CALL _strayintr(SB); BYTE $0x23
	CALL _strayintr(SB); BYTE $0x24
	CALL _strayintr(SB); BYTE $0x25
	CALL _strayintr(SB); BYTE $0x26
	CALL _strayintr(SB); BYTE $0x27
	CALL _strayintr(SB); BYTE $0x28
	CALL _strayintr(SB); BYTE $0x29
	CALL _strayintr(SB); BYTE $0x2A
	CALL _strayintr(SB); BYTE $0x2B
	CALL _strayintr(SB); BYTE $0x2C
	CALL _strayintr(SB); BYTE $0x2D
	CALL _strayintr(SB); BYTE $0x2E
	CALL _strayintr(SB); BYTE $0x2F
	CALL _strayintr(SB); BYTE $0x30
	CALL _strayintr(SB); BYTE $0x31
	CALL _strayintr(SB); BYTE $0x32
	CALL _strayintr(SB); BYTE $0x33
	CALL _strayintr(SB); BYTE $0x34
	CALL _strayintr(SB); BYTE $0x35
	CALL _strayintr(SB); BYTE $0x36
	CALL _strayintr(SB); BYTE $0x37
	CALL _strayintr(SB); BYTE $0x38
	CALL _strayintr(SB); BYTE $0x39
	CALL _strayintr(SB); BYTE $0x3A
	CALL _strayintr(SB); BYTE $0x3B
	CALL _strayintr(SB); BYTE $0x3C
	CALL _strayintr(SB); BYTE $0x3D
	CALL _strayintr(SB); BYTE $0x3E
	CALL _strayintr(SB); BYTE $0x3F
	CALL _syscallintr(SB); BYTE $0x40	/* VectorSYSCALL */
	CALL _strayintr(SB); BYTE $0x41
	CALL _strayintr(SB); BYTE $0x42
	CALL _strayintr(SB); BYTE $0x43
	CALL _strayintr(SB); BYTE $0x44
	CALL _strayintr(SB); BYTE $0x45
	CALL _strayintr(SB); BYTE $0x46
	CALL _strayintr(SB); BYTE $0x47
	CALL _strayintr(SB); BYTE $0x48
	CALL _strayintr(SB); BYTE $0x49
	CALL _strayintr(SB); BYTE $0x4A
	CALL _strayintr(SB); BYTE $0x4B
	CALL _strayintr(SB); BYTE $0x4C
	CALL _strayintr(SB); BYTE $0x4D
	CALL _strayintr(SB); BYTE $0x4E
	CALL _strayintr(SB); BYTE $0x4F
	CALL _strayintr(SB); BYTE $0x50
	CALL _strayintr(SB); BYTE $0x51
	CALL _strayintr(SB); BYTE $0x52
	CALL _strayintr(SB); BYTE $0x53
	CALL _strayintr(SB); BYTE $0x54
	CALL _strayintr(SB); BYTE $0x55
	CALL _strayintr(SB); BYTE $0x56
	CALL _strayintr(SB); BYTE $0x57
	CALL _strayintr(SB); BYTE $0x58
	CALL _strayintr(SB); BYTE $0x59
	CALL _strayintr(SB); BYTE $0x5A
	CALL _strayintr(SB); BYTE $0x5B
	CALL _strayintr(SB); BYTE $0x5C
	CALL _strayintr(SB); BYTE $0x5D
	CALL _strayintr(SB); BYTE $0x5E
	CALL _strayintr(SB); BYTE $0x5F
	CALL _strayintr(SB); BYTE $0x60
	CALL _strayintr(SB); BYTE $0x61
	CALL _strayintr(SB); BYTE $0x62
	CALL _strayintr(SB); BYTE $0x63
	CALL _strayintr(SB); BYTE $0x64
	CALL _strayintr(SB); BYTE $0x65
	CALL _strayintr(SB); BYTE $0x66
	CALL _strayintr(SB); BYTE $0x67
	CALL _strayintr(SB); BYTE $0x68
	CALL _strayintr(SB); BYTE $0x69
	CALL _strayintr(SB); BYTE $0x6A
	CALL _strayintr(SB); BYTE $0x6B
	CALL _strayintr(SB); BYTE $0x6C
	CALL _strayintr(SB); BYTE $0x6D
	CALL _strayintr(SB); BYTE $0x6E
	CALL _strayintr(SB); BYTE $0x6F
	CALL _strayintr(SB); BYTE $0x70
	CALL _strayintr(SB); BYTE $0x71
	CALL _strayintr(SB); BYTE $0x72
	CALL _strayintr(SB); BYTE $0x73
	CALL _strayintr(SB); BYTE $0x74
	CALL _strayintr(SB); BYTE $0x75
	CALL _strayintr(SB); BYTE $0x76
	CALL _strayintr(SB); BYTE $0x77
	CALL _strayintr(SB); BYTE $0x78
	CALL _strayintr(SB); BYTE $0x79
	CALL _strayintr(SB); BYTE $0x7A
	CALL _strayintr(SB); BYTE $0x7B
	CALL _strayintr(SB); BYTE $0x7C
	CALL _strayintr(SB); BYTE $0x7D
	CALL _strayintr(SB); BYTE $0x7E
	CALL _strayintr(SB); BYTE $0x7F
	CALL _strayintr(SB); BYTE $0x80		/* Vector[A]PIC */
	CALL _strayintr(SB); BYTE $0x81
	CALL _strayintr(SB); BYTE $0x82
	CALL _strayintr(SB); BYTE $0x83
	CALL _strayintr(SB); BYTE $0x84
	CALL _strayintr(SB); BYTE $0x85
	CALL _strayintr(SB); BYTE $0x86
	CALL _strayintr(SB); BYTE $0x87
	CALL _strayintr(SB); BYTE $0x88
	CALL _strayintr(SB); BYTE $0x89
	CALL _strayintr(SB); BYTE $0x8A
	CALL _strayintr(SB); BYTE $0x8B
	CALL _strayintr(SB); BYTE $0x8C
	CALL _strayintr(SB); BYTE $0x8D
	CALL _strayintr(SB); BYTE $0x8E
	CALL _strayintr(SB); BYTE $0x8F
	CALL _strayintr(SB); BYTE $0x90
	CALL _strayintr(SB); BYTE $0x91
	CALL _strayintr(SB); BYTE $0x92
	CALL _strayintr(SB); BYTE $0x93
	CALL _strayintr(SB); BYTE $0x94
	CALL _strayintr(SB); BYTE $0x95
	CALL _strayintr(SB); BYTE $0x96
	CALL _strayintr(SB); BYTE $0x97
	CALL _strayintr(SB); BYTE $0x98
	CALL _strayintr(SB); BYTE $0x99
	CALL _strayintr(SB); BYTE $0x9A
	CALL _strayintr(SB); BYTE $0x9B
	CALL _strayintr(SB); BYTE $0x9C
	CALL _strayintr(SB); BYTE $0x9D
	CALL _strayintr(SB); BYTE $0x9E
	CALL _strayintr(SB); BYTE $0x9F
	CALL _strayintr(SB); BYTE $0xA0
	CALL _strayintr(SB); BYTE $0xA1
	CALL _strayintr(SB); BYTE $0xA2
	CALL _strayintr(SB); BYTE $0xA3
	CALL _strayintr(SB); BYTE $0xA4
	CALL _strayintr(SB); BYTE $0xA5
	CALL _strayintr(SB); BYTE $0xA6
	CALL _strayintr(SB); BYTE $0xA7
	CALL _strayintr(SB); BYTE $0xA8
	CALL _strayintr(SB); BYTE $0xA9
	CALL _strayintr(SB); BYTE $0xAA
	CALL _strayintr(SB); BYTE $0xAB
	CALL _strayintr(SB); BYTE $0xAC
	CALL _strayintr(SB); BYTE $0xAD
	CALL _strayintr(SB); BYTE $0xAE
	CALL _strayintr(SB); BYTE $0xAF
	CALL _strayintr(SB); BYTE $0xB0
	CALL _strayintr(SB); BYTE $0xB1
	CALL _strayintr(SB); BYTE $0xB2
	CALL _strayintr(SB); BYTE $0xB3
	CALL _strayintr(SB); BYTE $0xB4
	CALL _strayintr(SB); BYTE $0xB5
	CALL _strayintr(SB); BYTE $0xB6
	CALL _strayintr(SB); BYTE $0xB7
	CALL _strayintr(SB); BYTE $0xB8
	CALL _strayintr(SB); BYTE $0xB9
	CALL _strayintr(SB); BYTE $0xBA
	CALL _strayintr(SB); BYTE $0xBB
	CALL _strayintr(SB); BYTE $0xBC
	CALL _strayintr(SB); BYTE $0xBD
	CALL _strayintr(SB); BYTE $0xBE
	CALL _strayintr(SB); BYTE $0xBF
	CALL _strayintr(SB); BYTE $0xC0
	CALL _strayintr(SB); BYTE $0xC1
	CALL _strayintr(SB); BYTE $0xC2
	CALL _strayintr(SB); BYTE $0xC3
	CALL _strayintr(SB); BYTE $0xC4
	CALL _strayintr(SB); BYTE $0xC5
	CALL _strayintr(SB); BYTE $0xC6
	CALL _strayintr(SB); BYTE $0xC7
	CALL _strayintr(SB); BYTE $0xC8
	CALL _strayintr(SB); BYTE $0xC9
	CALL _strayintr(SB); BYTE $0xCA
	CALL _strayintr(SB); BYTE $0xCB
	CALL _strayintr(SB); BYTE $0xCC
	CALL _strayintr(SB); BYTE $0xCD
	CALL _strayintr(SB); BYTE $0xCE
	CALL _strayintr(SB); BYTE $0xCF
	CALL _strayintr(SB); BYTE $0xD0
	CALL _strayintr(SB); BYTE $0xD1
	CALL _strayintr(SB); BYTE $0xD2
	CALL _strayintr(SB); BYTE $0xD3
	CALL _strayintr(SB); BYTE $0xD4
	CALL _strayintr(SB); BYTE $0xD5
	CALL _strayintr(SB); BYTE $0xD6
	CALL _strayintr(SB); BYTE $0xD7
	CALL _strayintr(SB); BYTE $0xD8
	CALL _strayintr(SB); BYTE $0xD9
	CALL _strayintr(SB); BYTE $0xDA
	CALL _strayintr(SB); BYTE $0xDB
	CALL _strayintr(SB); BYTE $0xDC
	CALL _strayintr(SB); BYTE $0xDD
	CALL _strayintr(SB); BYTE $0xDE
	CALL _strayintr(SB); BYTE $0xDF
	CALL _strayintr(SB); BYTE $0xE0
	CALL _strayintr(SB); BYTE $0xE1
	CALL _strayintr(SB); BYTE $0xE2
	CALL _strayintr(SB); BYTE $0xE3
	CALL _strayintr(SB); BYTE $0xE4
	CALL _strayintr(SB); BYTE $0xE5
	CALL _strayintr(SB); BYTE $0xE6
	CALL _strayintr(SB); BYTE $0xE7
	CALL _strayintr(SB); BYTE $0xE8
	CALL _strayintr(SB); BYTE $0xE9
	CALL _strayintr(SB); BYTE $0xEA
	CALL _strayintr(SB); BYTE $0xEB
	CALL _strayintr(SB); BYTE $0xEC
	CALL _strayintr(SB); BYTE $0xED
	CALL _strayintr(SB); BYTE $0xEE
	CALL _strayintr(SB); BYTE $0xEF
	CALL _strayintr(SB); BYTE $0xF0
	CALL _strayintr(SB); BYTE $0xF1
	CALL _strayintr(SB); BYTE $0xF2
	CALL _strayintr(SB); BYTE $0xF3
	CALL _strayintr(SB); BYTE $0xF4
	CALL _strayintr(SB); BYTE $0xF5
	CALL _strayintr(SB); BYTE $0xF6
	CALL _strayintr(SB); BYTE $0xF7
	CALL _strayintr(SB); BYTE $0xF8
	CALL _strayintr(SB); BYTE $0xF9
	CALL _strayintr(SB); BYTE $0xFA
	CALL _strayintr(SB); BYTE $0xFB
	CALL _strayintr(SB); BYTE $0xFC
	CALL _strayintr(SB); BYTE $0xFD
	CALL _strayintr(SB); BYTE $0xFE
	CALL _strayintr(SB); BYTE $0xFF
