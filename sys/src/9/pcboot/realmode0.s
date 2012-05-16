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

TEXT m0rgdtptr(SB), $0
	WORD	$(NGDT*8-1)
	LONG	$(CPU0GDT-KZERO)

TEXT m0gdtptr(SB), $0
	WORD	$(NGDT*8-1)
	LONG	$CPU0GDT

TEXT m0idtptr(SB), $0
	WORD $(256*8-1)
	LONG $IDTADDR

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
 *	- no device interrupts are going to come in
 *	- 0-16MB is identity mapped in page tables
 *	- l.s real-mode code is in low memory already but
 *	  may need to be copied into the first 64K (if loaded by pbs)
 *	- can use code segment rmseg in real mode to get at l.s code
 *	- the above Chinese puzzle pretty much forces RMUADDR to be 0x8000 or 0
 *	  and rmseg to be 0x800 or 0.
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
	MOVL	$RMSTACK, SP
	PUSHL	AX
	
	/* paranoia: make sure modified INT & JMPFAR instr.s are seen below */
	BYTE $0x0f; BYTE $0xae; BYTE $0xf8	/* SFENCE */
	BYTE $0x0f; BYTE $0xae; BYTE $0xe8	/* LFENCE */
	BYTE $0x0f; BYTE $0xae; BYTE $0xf0	/* MFENCE */

	/* change gdt to physical pointer */
	MOVL	m0rgdtptr-KZERO(SB), GDTR

	/* load IDT with real-mode version*/
	MOVL	realmodeidtptr-KZERO(SB), IDTR

	/* disable paging */
	MOVL	CR0, AX
	ANDL	$(~PG), AX
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

	/* JMPFAR rmseg:now16real */
	 BYTE $0xEA
	 WORD	$now16real-KZERO(SB)
TEXT rmseg(SB), $0
	 WORD	$0

TEXT now16real(SB), $0
	/* copy the registers for the bios call */
	CLR(rAX)
	MTSR(rAX, rSS)			/* 0000 -> rSS */
	LWI((RMSTACK-4), rSP)		/* preserve AX pushed in physcode */

	LWI((RMUADDR-KZERO), rBP)

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

	/* assume that SP and SS persist across INT */

TEXT realmodeintrinst(SB), $0
	INT $0x00
	CLI			/* who knows what evil the bios got up to */
	/* save the registers after the call */

//	CLR(rBP)
//	MTSR(rBP, rSS)			/* 0000 -> rSS */
//	LWI((RMSTACK-4), rSP)

	OPSIZE; PUSHFL
	OPSIZE; PUSHL AX

	LWI((RMUADDR-KZERO), rBP)
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
	ORL	$(PG|0x10000), AX	/* PG|WP */
	MOVL	AX, CR0
	LEAL	again32kzero(SB), AX
	JMP*	AX

TEXT again32kzero(SB), $0
	/* breathe a sigh of relief - back in 32-bit protected mode */

	/* switch to old stack */	
	PUSHL	AX	/* match popl below for 8l */
	MOVL	$(RMSTACK-4), SP
	POPL	SP

	/* restore idt */
	MOVL	m0idtptr(SB),IDTR

	/* restore gdt */
	MOVL	m0gdtptr(SB), GDTR

	CALL	restoreregs(SB)
	RET

TEXT realmodeend(SB), $0

/*
 * Must be 4-byte aligned & within 8K of the image's start to be seen.
 */
	NOP
	NOP
	NOP
TEXT _multibootheader(SB), $0			/* CHECK alignment (4) */
	LONG	$0x1BADB002			/* magic */
	LONG	$0x00010003			/* flags */
	LONG	$-(0x1BADB002 + 0x00010003)	/* checksum */
	LONG	$_multibootheader-KZERO(SB)	/* header_addr */
	LONG	$_start32p-KZERO(SB)		/* load_addr */
	LONG	$edata-KZERO(SB)		/* load_end_addr */
	LONG	$end-KZERO(SB)			/* bss_end_addr */
	LONG	$_start32p-KZERO(SB)		/* entry_addr */
	LONG	$0				/* mode_type */
	LONG	$0				/* width */
	LONG	$0				/* height */
	LONG	$0				/* depth */

	LONG	$0				/* +48: saved AX - magic */
	LONG	$0				/* +52: saved BX - info* */

/*
 * There's no way with 8[al] to make this into local data, hence
 * the TEXT definitions. Also, it should be in the same segment as
 * the LGDT instruction.
 * In real mode only 24-bits of the descriptor are loaded so the
 * -KZERO is superfluous for the usual mappings.
 * The segments are
 *	NULL
 *	DATA 32b 4GB PL0
 *	EXEC 32b 4GB PL0
 *	EXEC 16b 4GB PL0
 */
TEXT _gdt16r(SB), $0
	LONG $0x0000; LONG $0
	LONG $0xFFFF; LONG $(SEGG|SEGB|(0xF<<16)|SEGP|SEGPL(0)|SEGDATA|SEGW)
	LONG $0xFFFF; LONG $(SEGG|SEGD|(0xF<<16)|SEGP|SEGPL(0)|SEGEXEC|SEGR)
	LONG $0xFFFF; LONG $(SEGG     |(0xF<<16)|SEGP|SEGPL(0)|SEGEXEC|SEGR)
TEXT _gdtptr16r(SB), $0
	WORD	$(4*8)
	LONG	$_gdt16r-KZERO(SB)
