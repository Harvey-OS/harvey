/*
 * Reboot trampoline code.  Copies new kernel into place over old one.
 * This is copied to REBOOTADDR before executing it.
 * This version switches to 32-bit protected mode.
 * It's fragile, due to subtleties of Intel's architecture,
 * so be warned if modifying it.
 *
 * If you change this, pad after CLI as needed.
 */
#include "mem.h"
#include "amd64l.h"

#define	DELAY		BYTE $0xeb;	/* JMP .+2; flushes prefetch queue */ \
			BYTE $0x00

#undef DBGPUT
// #define DBGPUT(c, l)
/* trashes AX, CX, DX */
#define DBGPUT(c, l) MOVL $0x3F8, DX; MOVL $(c), AX; OUTB; BUSY(33000, l)

/* trashes CX */
#define BUSY(n, l)	MOVL $(n), CX; l: LOOP l

MODE $64

/*
 * Turn off MMU, then memmove the new kernel to its correct location
 * in physical memory.  Then jump to the start of the kernel.
 * we are called in the identity map in long mode with
 *	(*tramp)(phyentry, (void *)PADDR(code), size);
 */
TEXT	main(SB),$0
	CLI
	MOVQ	RARG, DI		/* destination */
	MOVQ	DI, AX			/* also entry point */
	MOVQ	p2+8(FP), SI		/* source */
	MOVL	n+16(FP), CX		/* byte count */
	MOVL	AX, entry<>(SB)
	MOVL	CX, count<>(SB)
	CLI				/* adjust to align data below */

	/* prepare to switch to long compatibility mode at CPL0 (desc. SdL=0) */
	MOVQ	$_gdtptr32p<>(SB), AX
/**/	MOVL	(AX), GDTR

	PUSHQ	$SSEL(SiCS, SsTIGDT|SsRPL0) /* compat code segment index */
	PUSHQ	$compat32(SB)
/**/	RETFQ				/* switch to compatibility mode */

TEXT entry<>(SB), 1, $-4
	LONG	$0
TEXT count<>(SB), 1, $-4
	LONG	$0

/*
 * originally copied from l64lme.s; no -KZERO.
 * CS and DS are for legacy or long compatibility modes.
 */
TEXT _gdt32p<>(SB), 1, $-4
	QUAD	$0				/* NULL descriptor */
	QUAD	$(SdG|SdD|Sd4G|SdP|SdS|SdCODE|SdW|0xffff) /* CS long compat */
	QUAD	$(SdG|SdD|Sd4G|SdP|SdS|SdW|0xffff)	/* DS long compat */
	QUAD	$(SdL|SdP|SdS|SdCODE)		/* Long mode 64-bit CS */

TEXT _gdtptr32p<>(SB), 1, $-4
	WORD	$(4*8-1)			/* includes long mode */
	QUAD	$_gdt32p<>(SB)			/* not LONG! */

/*
 * We're in long mode's 32-bit compatibility submode.
 */
MODE $32

TEXT compat32(SB), 1, $-4
	MOVL	$SSEL(SiDS, SsTIGDT|SsRPL0), AX
	MOVW	AX, DS
	MOVW	AX, ES
	MOVW	AX, FS
	MOVW	AX, GS
	MOVW	AX, SS
	DELAY

	/* disable paging by zeroing CR0.PG (cpu will zero LMA bit). */
	CLI
	MOVL	$0x7c00, SP		/* new stack below pxe base */
	MFENCE
DBGPUT('3', c3lab)

	MOVL	CR0, CX
	MOVL	$(Pg|Wp), AX
	NOTL	AX
	ANDL	AX, CX			/* clear Pg */
/**/	MOVL	CX, CR0			/* paging off */

	/*
	 * now behaving as 386 (32-bit).
	 *
	 * Intel says a jump is needed after disablilng paging,
	 * but this one crashes, at least on AMD cpus.
	 */
//	JMP	clrprefq(SB)

	DELAY
	MFENCE
DBGPUT('2', c2lab)
	MOVL	$0x7c00, SP		/* new stack below pxe base */

	/* deactivate long mode by zeroing efer.lme. */
	MOVL	$Efer, CX			/* Extended Feature Enable */
	RDMSR
	ANDL	$~Lme, AX			/* zero Long Mode Enable */
/**/	WRMSR
DBGPUT('.', dotlab)
DBGPUT('\n', nllab)

	/*
	 * we're now in legacy 32-bit protected mode.
	 */
	MOVL	entry<>(SB), AX
	MOVL	count<>(SB), CX

/*
 * the source and destination may overlap.
 * determine whether to copy forward or backwards
 */
	CLD			/* assume forward copy */
	CMPL	SI, DI
	JGT	_copy		/* src > dest, copy forward */
	MOVL	SI, DX
	ADDL	CX, DX
	CMPL	DX, DI
	JLE	_copy		/* src+len <= dest, copy forward */
_back:
	ADDL	CX, DI
	ADDL	CX, SI
	SUBL	$1, DI
	SUBL	$1, SI
	STD
_copy:
	REP;	MOVSB		/* copy new kernel into place */

/*
 * JMP to kernel entry point.  Note the true kernel entry point is
 * the virtual address KZERO|AX, but this must wait until
 * the MMU is enabled by the kernel in l.s.
 *
 * Set up AX & BX for multiboot.
 */
	MOVL	AX, DX			/* entry address */
	MOVL	$MBOOTREGMAG, AX	/* multiboot magic */
	MOVL	$(MBOOTTAB-KZERO), BX	/* multiboot data pointer */
	WBINVD
	ORL	DX, DX			/* NOP: avoid link bug */
	JMP*	DX
