#include "mem.h"

#define PADDR(a)	((a) & ~KZERO)
#define KADDR(a)	(KZERO|(a))

/*
 * Some machine instructions not handled by 8[al].
 */
#define OP16		BYTE $0x66
#define	DELAY		BYTE $0xEB; BYTE $0x00	/* JMP .+2 */
#define CPUID		BYTE $0x0F; BYTE $0xA2	/* CPUID, argument in AX */
#define WRMSR		BYTE $0x0F; BYTE $0x30	/* WRMSR, argument in AX/DX (lo/hi) */
#define RDMSR		BYTE $0x0F; BYTE $0x32	/* RDMSR, result in AX/DX (lo/hi) */
#define RDTSC 		BYTE $0x0F; BYTE $0x31
#define WBINVD		BYTE $0x0F; BYTE $0x09
#define HLT		BYTE $0xF4
#define SFENCE		BYTE $0x0F; BYTE $0xAE; BYTE $0xF8

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
TEXT _start0x80100020(SB),$0
	MOVL	$_start0x00100020(SB), AX
	ANDL	$~KZERO, AX
	JMP*	AX
/*
 * In protected mode with paging turned off and segment registers setup to linear map all memory.
 * Entered via a jump to 0x00100020, the physical address of the virtual kernel entry point of 0x80100020
 * Make the basic page tables for processor 0. Four pages are needed for the basic set:
 * a page directory, a page table for mapping the first 4MB of physical memory to KZERO,
 * and virtual and physical pages for mapping the Mach structure.
 * The remaining PTEs will be allocated later when memory is sized.
 * An identity mmu map is also needed for the switch to virtual mode.  This
 * identity mapping is removed once the MMU is going and the JMP has been made
 * to virtual memory.
 */
TEXT _start0x00100020(SB),$0
	CLI					/* make sure interrupts are off */

	MOVL	$PADDR(CPU0PDB), DI		/* clear 4 pages for the tables etc. */
	XORL	AX, AX
	MOVL	$(4*BY2PG), CX
	SHRL	$2, CX

	CLD
	REP;	STOSL

	MOVL	$PADDR(CPU0PDB), AX
	ADDL	$PDO(KZERO), AX			/* page directory offset for KZERO */
	MOVL	$PADDR(CPU0PTE), (AX)		/* PTE's for 0x80000000 */
	MOVL	$(PTEWRITE|PTEVALID), BX	/* page permissions */
	ORL	BX, (AX)

	MOVL	$PADDR(CPU0PTE), AX		/* first page of page table */
	MOVL	$1024, CX			/* 1024 pages in 4MB */
_setpte:
	MOVL	BX, (AX)
	ADDL	$(1<<PGSHIFT), BX
	ADDL	$4, AX
	LOOP	_setpte

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

TEXT outsb(SB),$0
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

TEXT rdtsc(SB), $0				/* time stamp counter; cycles since power up */
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

TEXT wbinvd(SB), $0
	WBINVD
	RET

TEXT sfence(SB), $0
	SFENCE
	RET

/*
 * Try to determine the CPU type which requires fiddling with EFLAGS.
 * If the Id bit can be toggled then the CPUID instruciton can be used
 * to determine CPU identity and features. First have to check if it's
 * a 386 (Ac bit can't be set). If it's not a 386 and the Id bit can't be
 * toggled then it's an older 486 of some kind.
 *
 *	cpuid(id[], &ax, &dx);
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

	MOVL	$0, AX
	CPUID
	MOVL	id+0(FP), BP
	MOVL	BX, 0(BP)			/* "Genu" "Auth" "Cyri" */
	MOVL	DX, 4(BP)			/* "ineI" "enti" "xIns" */
	MOVL	CX, 8(BP)			/* "ntel" "cAMD" "tead" */

	MOVL	$1, AX
	CPUID
	JMP	_cpuid

_cpu486:
	MOVL	$0x400, AX
	MOVL	$0, DX
	JMP	_cpuid

_cpu386:
	MOVL	$0x300, AX
	MOVL	$0, DX

_cpuid:
	MOVL	ax+4(FP), BP
	MOVL	AX, 0(BP)
	MOVL	dx+8(FP), BP
	MOVL	DX, 0(BP)
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
 */
#define	FPOFF								;\
	WAIT								;\
	MOVL	CR0, AX							;\
	ANDL	$~0x4, AX			/* EM=0 */		;\
	ORL	$0x28, AX			/* NE=1, TS=1 */	;\
	MOVL	AX, CR0

#define	FPON								;\
	MOVL	CR0, AX							;\
	ANDL	$~0xC, AX			/* EM=0, TS=0 */	;\
	MOVL	AX, CR0
	
TEXT fpoff(SB), $0				/* disable */
	FPOFF
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

TEXT fpsave(SB), $0				/* save state and disable */
	MOVL	p+0(FP), AX
	FSAVE	0(AX)				/* no WAIT */
	FPOFF
	RET

TEXT fprestore(SB), $0				/* enable and restore state */
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
	FSTENV	0(AX)
	RET

/*
 */
TEXT splhi(SB), $0
	MOVL	$(MACHADDR+0x04), AX 		/* save PC in m->splpc */
	MOVL	(SP), BX
	MOVL	BX, (AX)

	PUSHFL
	POPL	AX
	CLI
	RET

TEXT spllo(SB), $0
	PUSHFL
	POPL	AX
	STI
	RET

TEXT splx(SB), $0
	MOVL	$(MACHADDR+0x04), AX 		/* save PC in m->splpc */
	MOVL	(SP), BX
	MOVL	BX, (AX)

TEXT splxpc(SB), $0				/* for iunlock */
	MOVL	s+0(FP), AX
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
 * Test-And-Set
 */
TEXT tas(SB), $0
	MOVL	$0xDEADDEAD, AX
	MOVL	lock+0(FP), BX
	XCHGL	AX, (BX)			/* lock->key */
	RET

TEXT wbflush(SB), $0
	CPUID
	RET

TEXT xchgw(SB), $0
	MOVL	v+4(FP), AX
	MOVL	p+0(FP), BX
	XCHGW	AX, (BX)
	RET

/*
TEXT xchgl(SB), $0
	MOVL	v+4(FP), AX
	MOVL	p+0(FP), BX
	XCHGL	AX, (BX)
	RET
 */

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

TEXT swaplabel(SB), $0
	MOVL	label+0(FP), AX
	MOVL	pc+4(FP), BX
	MOVL	0(AX), SP			/* restore sp */
	MOVL	4(AX), CX			/* put return pc on the stack */
	MOVL	CX, 0(SP)
	MOVL	BX, 4(AX)			/* put caller pc where return pc was */
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
	ADDL	$8, SP				/* pop error code and trap type */
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
