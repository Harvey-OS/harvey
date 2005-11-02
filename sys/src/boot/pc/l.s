#include "x16.h"
#include "mem.h"

#define WRMSR		BYTE $0x0F; BYTE $0x30	/* WRMSR, argument in AX/DX (lo/hi) */
#define RDTSC 		BYTE $0x0F; BYTE $0x31	/* RDTSC, result in AX/DX (lo/hi) */
#define RDMSR		BYTE $0x0F; BYTE $0x32	/* RDMSR, result in AX/DX (lo/hi) */

#ifdef PXE
#define PDB		0x90000		/* temporary page tables (24KB) */
#else
#define PDB		0x08000
#endif PXE

#define NoScreenBlank	1
/*#define ResetDiscs	1*/

TEXT origin(SB), $0
	/*
	 * This part of l.s is used only in the boot kernel.
	 * It assumes that we are in real address mode, i.e.,
	 * that we look like an 8086.
	 *
	 * Make sure the segments are reasonable.
	 * If we were started directly from the BIOS
	 * (i.e. no MS-DOS) then DS may not be
	 * right.
	 */
	MOVW	CS, AX
	MOVW	AX, DS

#ifdef NoScreenBlank
	/*
	 * Get the current video mode. If it isn't mode 3,
	 * set text mode 3.
	 * Well, no. Windows95 won't co-operate here so we have
	 * to explicitly set mode 3.
	 */
	XORL	AX, AX
	MOVB	$0x0F, AH
	INT	$0x10			/* get current video mode in AL */
	CMPB	AL, $03
	JEQ	sayhello
#endif /* NoScreenBlank */
	XORL	AX, AX
	MOVB	$0x03, AL
	INT	$0x10			/* set video mode in AL */

sayhello:
	LWI(hello(SB), rSI)
	CALL16(biosputs(SB))

#ifdef ResetDiscs
	XORL	AX, AX			/* reset disc system */
	XORL	DX, DX
	MOVB	$0x80, DL
	INT	$0x13
#endif /* ResetDiscs */

#ifdef DOTCOM
/*
 *	relocate everything to a half meg and jump there
 *	- looks weird because it is being assembled by a 32 bit
 *	  assembler for a 16 bit world
 *
 *	only b.com does this - not 9load
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

	/*
	 * Jump to the copied image;
	 * fix up the DS for the new location.
	 */
	FARJUMP16(0x8000, _start8000(SB))

TEXT _start8000(SB), $0
	MFSR(rCS, rAX)			/* fix up DS, ES (0x8000) */
	MTSR(rAX, rDS)
	MTSR(rAX, rES)

	/*
	 * If we are already in protected mode, have to get back
	 * to real mode before trying any privileged operations
	 * (like going into protected mode...).
	 * Try to reset with a restart vector.
	 */
	MFCR(rCR0, rAX)			/* are we in protected mode? */
	ANDI(0x0001, rAX)
	JEQ	_real

	CLR(rBX)
	MTSR(rBX, rES)

	LWI(0x0467, rBX)		/* reset entry point */
	LWI(_start8000(SB), rAX)	/* offset within segment */
	BYTE	$0x26
	BYTE	$0x89
	BYTE	$0x07			/* MOVW	AX, ES:[BX] */
	LBI(0x69, rBL)
	MFSR(rCS, rAX)			/* segment */
	BYTE	$0x26
	BYTE	$0x89
	BYTE	$0x07			/* MOVW	AX, ES:[BX] */

	CLR(rDX)
	OUTPORTB(0x70, 0x8F)
	OUTPORTB(0x71, 0x0A)

	FARJUMP16(0xFFFF, 0x0000)		/* reset */
#endif /* DOTCOM */

_real:

/*
 *	do things that need to be done in real mode.
 *	the results get written to CONFADDR (0x1200)
 *	in a series of <4-byte-magic-number><block-of-data>
 *	the data length is dependent on the magic number.
 *
 *	this gets parsed by conf.c:/^readlsconf
 *
 *	N.B. CALL16 kills rDI, so we can't call anything.
 */
	LWI(0x0000, rAX)
	MTSR(rAX, rES)
	LWI(0x1200, rDI)

/*
 *	turn off interrupts
 */
	CLI

/*
 *	detect APM1.2 bios support
 */
	/* save DI */
	SW(rDI, rock(SB))

	/* disconnect anyone else */
	LWI(0x5304, rAX)
	LWI(0x0000, rBX)
	INT $0x15

	/* connect */
	CLC
	LWI(0x5303, rAX)
	LWI(0x0000, rBX)
	INT $0x15
	CLI	/* apm put interrupts back? */

	JC noapm

	OPSIZE; PUSHR(rSI)
	OPSIZE; PUSHR(rBX)
	PUSHR(rDI)
	PUSHR(rDX)
	PUSHR(rCX)
	PUSHR(rAX)

	/* put DI, ES back */
	LW(rock(SB), rDI)
	LWI(0x0000, rAX)
	MTSR(rAX, rES)

	/*
	 * write APM data.  first four bytes are APM\0.
	 */
	LWI(0x5041, rAX)
	STOSW

	LWI(0x004d, rAX)
	STOSW

	LWI(8, rCX)
apmmove:
	POPR(rAX)
	STOSW
	LOOP apmmove

noapm:

/*
 *	end of real mode hacks: write terminator, put ES back.
 */
	LWI(0x0000, rAX)
	STOSW
	STOSW

	MFSR(rCS, rAX)			/* fix up ES (0x8000) */
	MTSR(rAX, rES)

/*
 * 	goto protected mode
 */
/*	MOVL	tgdtptr(SB),GDTR /**/
	 BYTE	$0x0f
	 BYTE	$0x01
	 BYTE	$0x16
	 WORD	$tgdtptr(SB)

	LWI(1, rAX)
	/* MOV AX,MSW */
	BYTE $0x0F; BYTE $0x01; BYTE $0xF0

/*
 *	clear prefetch queue (weird code to avoid optimizations)
 */
	/* JMP .+2 */
	BYTE $0xEB
	BYTE $0x00

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
	MOVW	AX,FS
	MOVW	AX,GS

/*	JMPFAR	SELECTOR(2, SELGDT, 0):$mode32bit(SB) /**/
	 BYTE	$0x66
	 BYTE	$0xEA
	 LONG	$mode32bit-KZERO(SB)
	 WORD	$SELECTOR(2, SELGDT, 0)

TEXT	mode32bit(SB),$0
	/*
	 *  make a bottom level page table page that maps the first
	 *  16 meg of physical memory
	 */
	MOVL	$PDB, DI			/* clear 6 pages for the tables etc. */
	XORL	AX, AX
	MOVL	$(6*BY2PG), CX
	SHRL	$2, CX

	CLD
	REP;	STOSL

	MOVL	$PDB, AX		/* phys addr of temporary page table */
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
	 *
	 *  this produces the apparently harmless "VMX|F(125):468 Dis 0x0:0x0"
	 *  message in the VMware log.
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

/*
 * When we load 9load from DOS, the bootstrap jumps
 * to the instruction right after `JUMP', which gets
 * us into kzero.
 *
 * The name prevents it from being optimized away.
 */
TEXT jumplabel(SB), $0
	BYTE $'J'; BYTE $'U'; BYTE $'M'; BYTE $'P'

	LEAL	tokzero(SB),AX
	JMP*	AX

TEXT	tokzero(SB),$0
	/*
	 * Clear BSS
	 */
	LEAL	edata(SB),SI
	MOVL	SI,DI
	ADDL	$4,DI
	MOVL	$0,AX
	MOVL	AX,(SI)
	LEAL	end(SB),CX
	SUBL	DI,CX
	SHRL	$2,CX
	CLD
	REP
	MOVSL

	/*
	 *  stack and mach
	 */
	MOVL	$mach0(SB),SP
	MOVL	SP,m(SB)
	MOVL	$0,0(SP)
	ADDL	$(MACHSIZE-4),SP	/* start stack above machine struct */

	CALL	main(SB)

loop:
	JMP	loop

GLOBL	mach0+0(SB), $MACHSIZE
GLOBL	m(SB), $4

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

	/* exec segment descriptor for 4 gigabytes (PL 0) 16-bit */
	LONG	$(0xFFFF)
	LONG	$(SEGG|(0xF<<16)|SEGP|SEGPL(0)|SEGEXEC|SEGR)

/*
 *  pointer to initial gdt
 */
TEXT	tgdtptr(SB),$0
	WORD	$(4*8)
	LONG	$tgdt-KZERO(SB)

/*
 * Output a string to the display.
 * String argument is in rSI.
 */
TEXT biosputs(SB), $0
	PUSHA
	CLR(rBX)
_BIOSputs:
	LODSB
	ORB(rAL, rAL)
	JEQ _BIOSputsret

	LBI(0x0E, rAH)
	BIOSCALL(0x10)
	JMP _BIOSputs

_BIOSputsret:
	POPA
	RET

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

TEXT mb386(SB), $0
	POPL	AX				/* return PC */
	PUSHFL
	PUSHL	CS
	PUSHL	AX
	IRETL

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
 * Try to determine the CPU type which requires fiddling with EFLAGS.
 * If the Id bit can be toggled then the CPUID instruciton can be used
 * to determine CPU identity and features. First have to check if it's
 * a 386 (Ac bit can't be set). If it's not a 386 and the Id bit can't be
 * toggled then it's an older 486 of some kind.
 *
 *	cpuid(id[], &ax, &dx);
 */
#define CPUID		BYTE $0x0F; BYTE $0xA2	/* CPUID, argument in AX */
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
 *  basic timing loop to determine CPU frequency
 */
TEXT	aamloop(SB),$0

	MOVL	c+0(FP),CX
aaml1:
	AAM
	LOOP	aaml1
	RET

TEXT hello(SB), $0
	BYTE $'P'; BYTE $'l'; BYTE $'a'; BYTE $'n';
	BYTE $' '; BYTE $'9'; BYTE $' '; BYTE $'f';
	BYTE $'r'; BYTE $'o'; BYTE $'m'; BYTE $' ';
	BYTE $'B'; BYTE $'e'; BYTE $'l'; BYTE $'l';
	BYTE $' '; BYTE $'L'; BYTE $'a'; BYTE $'b';
	BYTE $'s'; 
	BYTE $'\r';
	BYTE $'\n';
	BYTE $'\z';

TEXT rock(SB), $0
	BYTE $0; BYTE $0; BYTE $0; BYTE $0;

GLOBL pxe(SB), $4
#ifdef PXE
DATA	pxe+0(SB)/4, $1
#else
DATA	pxe+0(SB)/4, $0
#endif /* PXE */

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
 *	- can use code segment 0x1000 in real mode
 *		to get at l.s code
 */
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

	/* load IDT with real-mode version; GDT already fine */
	MOVL	realmodeidtptr(SB), IDTR

	/* edit INT $0x00 instruction below */
	MOVL	realmodeintr(SB), AX
	MOVB	AX, realmodeintrinst+1(SB)

	/* disable paging */
	MOVL	CR0, AX
	ANDL	$0x7FFFFFFF, AX
	MOVL	AX, CR0
	/* JMP .+2 to clear prefetch queue*/
	BYTE $0xEB; BYTE $0x00

	/* jump to 16-bit code segment */
/*	JMPFAR	SELECTOR(3, SELGDT, 0):$again16bit(SB) /**/
	 BYTE	$0xEA
	 LONG	$again16bit-KZERO(SB)
	 WORD	$SELECTOR(3, SELGDT, 0)

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

	/* JMPFAR 0x1000:now16real */
	 BYTE $0xEA
	 WORD	$now16real-KZERO(SB)
	 WORD	$0x1000

TEXT now16real(SB), $0
	/* copy the registers for the bios call */
	LWI(0x1000, rAX)
	MOVW	AX,SS
	LWI(realmoderegs(SB), rBP)
	
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

	/* save the registers after the call */

	LWI(0x7bfc, rSP)
	OPSIZE; PUSHFL
	OPSIZE; PUSHL AX

	LWI(0x1000, rAX)
	MOVW	AX,SS
	LWI(realmoderegs(SB), rBP)
	
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
	
/*	JMPFAR	SELECTOR(2, SELGDT, 0):$again32bit(SB) /**/
	 OPSIZE
	 BYTE $0xEA
	 LONG	$again32bit-KZERO(SB)
	 WORD	$SELECTOR(2, SELGDT, 0)

TEXT again32bit(SB), $0
	MOVW	$SELECTOR(1, SELGDT, 0),AX
	MOVW	AX,DS
	MOVW	AX,SS
	MOVW	AX,ES
	MOVW	AX,FS
	MOVW	AX,GS

	/* enable paging and jump to kzero-address code */
	MOVL	CR0, AX
	ORL	$0x80000000, AX
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
	MOVL	idtptr(SB),IDTR

	CALL	restoreregs(SB)
	RET

TEXT realmoderegs(SB), $0
	LONG $0; LONG $0; LONG $0; LONG $0
	LONG $0; LONG $0; LONG $0; LONG $0
	LONG $0; LONG $0; LONG $0; LONG $0
	LONG $0; LONG $0; LONG $0; LONG $0
	LONG $0; LONG $0; LONG $0; LONG $0

TEXT realmodeintr(SB), $0
	LONG $0

