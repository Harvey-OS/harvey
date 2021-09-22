/*
 * Reboot trampoline code.  Copies new kernel into place over old one.
 * This is copied to REBOOTADDR before executing it.
 * This version switches to 32-bit protected mode.
 */
#include "mem.h"
#include "amd64l.h"

#undef USE_IPI

#undef DBGPUT
/* trashes AX, DX */
#define DBGPUT(c) MOVL $0x3F8, DX; MOVL $(c), AX; OUTB
// #define DBGPUT(c)

/* trashes CX */
#define BUSY(n, l)	MOVL $(n), CX; l: LOOP l

#define CLRPREFQ(lab)	CMPL AX, AX; JEQ lab; MFENCE; lab:  /* 64-bit safe */
#define NOP		BYTE 0x90

MODE $64

/*
 * Turn off MMU, then memmove the new kernel to its correct location
 * in physical memory.  Then jump to the start of the kernel.
 * entered with CX pointing at a Sipireboot and in the identity map.
 */

// amd says revert from long to protected modes this way:
//
// [must already be in identity map.]
// switch to compatibility mode & put cpu at CPL 0.
// deactivate long mode by zeroing CR0.PG (cpu will zero LMA bit).
// (load cr3 with 3-level page table, including identity map.)
// zero efer.lme.
// (set CR0.PG then JMP.)

/*
 * if entered via _protected in l16sipi.s,
 * CX will contain sipi->args.
 * otherwise, we are called in the identity map in long mode with
 *	(*tramp)(phyentry, (void *)PADDR(code), size);
 */
TEXT	main(SB),$0
	CLI
#ifdef USE_IPI
	/* into identity map */
	MOVQ	$intozeromap(SB), DX
	ORQ	DX, DX			/* NOP: avoid link bug */
	JMP*	DX			/* KZERO to identity map */

TEXT	intozeromap(SB),$-4	/* this may mess up (FP) references */
	CLRPREFQ(pref1)
	MOVL	CX, savecx(SB)		/* sipi->arg */
#else
DBGPUT('l'); BUSY(33000, elab)
	MOVQ	RARG, DI		/* destination */
	MOVQ	DI, AX			/* also entry point */
	MOVQ	p2+8(FP), SI		/* source */
	MOVL	n+16(FP), CX		/* byte count */
	MOVL	AX, saveax(SB)		/* entry */
	MOVL	CX, savecx(SB)		/* count */
#endif
DBGPUT('a'); BUSY(33000, alab)
	CLI				/* align data below */

	XORQ	CX, CX

	/* prepare to switch to long compatibility mode at CPL0 (desc. SdL=0) */
	MOVQ	$_gdtptr32p<>(SB), AX
/**/	MOVL	(AX), GDTR

	PUSHQ	$SSEL(SiCS, SsTIGDT|SsRPL0) /* compat code segment index */
	PUSHQ	$compat32(SB)
DBGPUT('u'); BUSY(33000, plab)
/**/	RETFQ				/* switch to compatibility mode */

TEXT saveax(SB), 1, $-4
	LONG	$0
TEXT savecx(SB), 1, $-4
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

	CLRPREFQ(pref2)

	/* disable paging by zeroing CR0.PG (cpu will zero LMA bit). */
	CLI
	MOVL	$0x7c00, SP		/* new stack below pxe base */
	MFENCE
DBGPUT('n'); BUSY(33000, clab)

	MOVL	CR0, CX
	MOVL	$(Pg|Wp), AX
	NOTL	AX
	ANDL	AX, CX			/* clear Pg */
/**/	MOVL	CX, CR0			/* paging off */

	/*
	 * now behaving as 386 (32-bit).
	 *
	 * Intel says a jump is needed after disablilng paging,
	 * but this one crashes.
	 */
//	JMP	clrprefq(SB)

TEXT forcejmp(SB), 1, $-4
	MFENCE

// TEXT clrprefq(SB), 1, $-4
DBGPUT('c'); BUSY(33000, dashlab)
	MOVL	$0x7c00, SP		/* new stack below pxe base */

	/* deactivate long mode by zeroing efer.lme. */
	MOVL	$Efer, CX			/* Extended Feature Enable */
	RDMSR
	ANDL	$~Lme, AX			/* zero Long Mode Enable */
/**/	WRMSR
DBGPUT('h'); BUSY(33000, dotlab)
DBGPUT('\n'); BUSY(33000, nllab)

	/*
	 * we're now in legacy 32-bit protected mode.
	 */
	MOVL	saveax(SB), AX
	MOVL	savecx(SB), CX
#ifdef USE_IPI
//	MOVL	RBTRAMP(CX), AX		/* main(SB) */
	MOVL	RBPHYSENTRY(CX), AX	/* kernel entry */
	MOVL	AX, DI			/* destination and entry point */
	MOVL	RBSRC(CX), SI		/* kernel source addr */
	MOVL	RBLEN(CX), CX		/* byte count */
#endif

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
	REP;	MOVSB

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
