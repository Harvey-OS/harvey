/*
 * non-startup assembly-language assist
 */

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

/*
 * Macros for calculating offsets within the page directory base
 * and page tables. Note that these are assembler-specific hence
 * the '<<2'.
 */
#define PDO(a)		(((((a))>>22) & 0x03FF)<<2)
#define PTO(a)		(((((a))>>12) & 0x03FF)<<2)

TEXT pagingoff(SB), $0
	DELAY				/* JMP .+2 */

	/*
	 *  use a jump to an absolute location to get the PC out of
	 *  KZERO.  first establishes double mapping of first few MB.
	 */
	MOVL	CR3, CX				/* load address of PDB */
	ADDL	$KZERO, CX
	MOVL	PDO(KZERO)(CX), DX		/* double-map KZERO at 0 */
	MOVL	DX, PDO(0)(CX)

	MOVL	CR3, CX
	MOVL	CX, CR3				/* load and flush the mmu */

	MOVL	  entry+0(FP), DX

	LEAL	_nopaging-KZERO(SB),AX
	JMP*	AX				/* jump to identity-map */

TEXT _nopaging(SB), $0
	DELAY				/* JMP .+2 */

	/* switch to low stack */
	MOVL	SP, AX
	MOVL	$RMSTACK, SP
//	PUSHL	AX

	/* change gdt to physical pointer */
	MOVL	_gdtptr16r-KZERO(SB), GDTR

	/*
	 * turn off paging
	 */
	MOVL	CR0,AX
	ANDL	$~PG, AX
	MOVL	AX,CR0
	DELAY				/* JMP .+2 */

	MOVL	$_stop32pg-KZERO(SB), AX
	JMP*	AX				/* forward into the past */

TEXT _stop32pg(SB), $0
	MOVL	multibootheader-KZERO(SB), BX	/* multiboot data pointer */
	MOVL	$0x2badb002, AX			/* multiboot magic */

	JMP*	DX				/* into the loaded kernel */

_idle:
	HLT
	JMP	_idle

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

TEXT cgapost2(SB), 0, $16
	MOVL	$0xb8000,CX
	MOVL	CX,(SP)
	CALL	,kaddr+0(SB)
	MOVL	code+0(FP),BP
	MOVL	AX,BX
	MOVL	BP,CX
	SARL	$4,CX
	ANDL	$15,CX
	MOVBLZX	hex(SB)(CX*1),AX
	MOVB	AX,3996(BX)
	MOVB	$7,3997(BX)
	MOVL	BP,DX
	ANDL	$15,DX
	MOVBLZX	hex(SB)(DX*1),CX
	MOVB	CX,3998(BX)
	MOVB	$7,3999(BX)
	RET

/*
 * Read/write various system registers.
 * CR4 and the 'model specific registers' should only be read/written
 * after it has been determined the processor supports them
 */
TEXT ltr(SB), $0				/* TR - task register */
	MOVL	tptr+0(FP), AX
	MOVW	AX, TASK
	RET

TEXT invlpg(SB), $0
	/* 486+ only */
	MOVL	va+0(FP), CX
	INVLPG
	RET

TEXT wbinvd(SB), $0
	WBINVD
	RET

/*
 * stub for:
 * time stamp counter; low-order 32 bits of 64-bit cycle counter
 * Runs at fasthz/4 cycles per second (m->clkin>>3)
 */
TEXT lcycles(SB),1,$0
	RDTSC
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
	MOVL	fn+0(FP), AX
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
	CLI					/* interrupts off */
	CMPL	nrdy(SB), $0
	JEQ	_nothingready
	STI					/* interrupts on */
	RET

_nothingready:
	STI			/* interrupts on: service before rescheduling */
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
//	CALL _syscallintr(SB); BYTE $0x40	/* VectorSYSCALL */
	CALL _strayintr(SB); BYTE $0x40
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
