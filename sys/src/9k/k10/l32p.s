/*
 * initialise cpu0 enough to call main.
 */
#include "mem.h"
#include "amd64l.h"

MODE $32

/* illegal op-code in long mode */
#define pFARJMP32(s, o)	BYTE $0xea;		/* far jump to ptr32:16 */\
			LONG $o; WORD $s
#define	DELAY		BYTE $0xeb;		/* JMP .+2 */		\
			BYTE $0x00
/*
 * use this (data) definition instead of the built-in instruction
 * to avoid 8l deleting NOPs after a jump.  this matters to alignment.
 */
#define NOP		BYTE $0x90

/*
 * Enter here in 32-bit protected mode, either from 9boot or expand.
 * Welcome to 1982.
 * Make sure the GDT is set as it should be:
 *	disable interrupts;
 *	load the GDT with the table in _gdt32p;
 *	load all the data segments
 *	load the code segment via a far jump.
 */
TEXT _main(SB), 1, $-4				/* avoid libc's main9.s */
TEXT _protected<>(SB), 1, $-4
	CLI
	/* possible passed-in multiboot magic; BX may have Mbi addr. */
	MOVL	AX, BP
	BYTE $0xe9; LONG $(3*4); /* JMP _endofheader; not recognised as JMP */

TEXT _multibootheader<>(SB), 1, $-4		/* must be 4-byte aligned */
	LONG	$0x1badb002			/* magic */
	LONG	$0x00000003			/* flags */
	LONG	$-(0x1badb002 + 0x00000003)	/* checksum */

_endofheader:
	MOVL	$_gdtptr32p<>-KZERO(SB), AX	/* in l64lme.s */
	MOVL	(AX), GDTR

	MOVL	$SSEL(SiDS, SsTIGDT|SsRPL0), AX
	MOVW	AX, DS
	MOVW	AX, ES
	MOVW	AX, FS
	MOVW	AX, GS
	MOVW	AX, SS

	DELAY

	MOVL	CR0, AX
	ANDL	$~(Cd|Nw), AX			/* caches on */
/**/	MOVL	AX, CR0
	DELAY

/*
 * what if we were entered in long mode because we were loaded by a previous
 * 9k kernel? assume that we're in the identity map.
 */
	MOVL	$Efer, CX		/* Extended Feature Enable */
	RDMSR
	ANDL	$Lme, AX		/* Long Mode Enable */
	CMPL	AX, $0
	JNE	_long1

	pFARJMP32(SSEL(SiCS, SsTIGDT|SsRPL0), _warp64<>-KZERO(SB))

	/*
	 * we're already in long mode.  skip protected set up, and do some
	 * of the things that _warp64 would have done.
	 * we don't currently use this; we always start in 32-bit
	 * protected mode.
	 */
MODE $64
_long1:
	MOVQ	$_protected<>-(LOW0END+KZERO)(SB), SI	/* sys-KZERO */
DBGPUT('L')

	MOVL	SI, SP
	ADDL	$(MACHSTKSZ-8), SP	/* establish stack in sys */

	/* zero sys except page tables since MMU is on. */
	XORL	AX, AX
	CLD

	MOVL	SI, DI
	MOVL	$(MACHSTKSZ>>2), CX
	REP;	STOSL			/* zero stack */

	MOVL	SI, DI
	ADDL	$(MACHSTKSZ + 4*PTSZ), DI /* skip stack & page tables */
	MOVL	$((MACHSZ + 3*PGSZ)>>2), CX
	REP;	STOSL			/* zero vsvm, mach, syspage, ptrpage */

	MOVQ	$_identity<>-KZERO(SB), AX
	ORQ	AX, AX			/* NOP: avoid link bug */
	JMP*	AX			/* into l64lme.s below */

/*
 * Make the basic page tables for CPU0 to map 0-4MB physical
 * to KZERO, and include an identity map for the switch from protected
 * to paging mode. There's an assumption here that the creation and later
 * removal of the identity map will not interfere with the KZERO mappings;
 * the conditions for clearing the identity map are
 *	clear PML4 entry when (KZER0 & 0x0000ff8000000000) != 0; // 47-39
 *	clear PDP entry when (KZER0 & 0x0000007fc0000000) != 0;	 // 38-30
 *	don't clear PD entry when (KZER0 & 0x000000003fe00000) == 0; // 29-21
 * the code below assumes these conditions are met.
 * Also assumes KSEG0SIZE is 2GB.
 *
 * Assume a recent processor with Page Size Extensions
 * and use two 2MB entries.
 */
/*
 * The layout is described in dat.h, in reverse order by address:
 *	_protected:	start of kernel text
 *	- PTSZ		PT for 2nd GB		PMAPADDR PT may be here
 *	- PTSZ		PD for 2nd GB
 *	- PGSZ		ptrpage
 *	- PGSZ		syspage
 *	- MACHSZ	m
 *	- PGSZ		vsvmpage for gdt, tss
 *	- PTSZ		PT for PMAPADDR		unused - assumes in KZERO PD(!)
 *	- PTSZ		PD
 *	- PTSZ		PDP
 *	- PTSZ		PML4
 *	- MACHSTKSZ	stack
 */

MODE $32

TEXT _warp64<>(SB), 1, $-4
	MOVL	$_protected<>-(LOW0END+KZERO)(SB), SI	/* sys-KZERO */
DBGPUT('z')
	MOVL	SI, DI
	XORL	AX, AX
	MOVL	$(LOW0ZEROEND>>2), CX
	CLD
	REP;	STOSL	/* zero stack, P*, vsvm, mach, syspage, ptrpage */

DBGPUT('m')
	LEAL	LOWPML4(SI), AX
/**/	MOVL	AX, CR3			/* load the mmu */

PUSHL	AX
DBGPUT('1')
POPL	AX
	/* point 0 & KZERO & PMAPADDR PML4 512G entries at PDP */
	MOVL	SI, DX
	ADDL	$(LOWPDP|PteRW|PteP), DX
	MOVL	DX, PML4O(0)(AX)	/* PML4E for identity map (512GB) */
	MOVL	DX, PML4O(KZERO)(AX)	/* PML4E for KZERO, PMAPADDR */

	/* point 0 & KZERO & PMAPADDR PDP 1G entries at PD */
	LEAL	LOWPDP(SI), AX
	MOVL	SI, DX
	ADDL	$(LOWPD|PteRW|PteP), DX
	MOVL	DX, PDPO(0)(AX)		/* PDPE for identity map (1GB) */
	MOVL	DX, PDPO(KZERO)(AX)	/* PDPE for KZERO, PMAPADDR */
	/*
	 * KSEG0SIZE is 2GB, so add KZERO+GB mapping using another PD & PT
	 * pages.  this is only possible if KSEG0 is -2GB or lower.
	 */
	MOVL	SI, DX
	ADDL	$(LOW0PD2G|PteRW|PteP), DX
	MOVL	DX, PDPO(KZERO+GB)(AX)	/* PDPE for PMAPADDR in 2nd GB */

	/*
	 * point low 8MB at 0 & KZERO to PD with 4 2MB pages each.
	 * we don't use the first MB, and 9k10fs is ~4MB.
	 */
	LEAL	LOWPD(SI), AX
	MOVL	$(PtePS|PteRW|PteP), DX
	/* match with l16sipi.s mappings in _protected */
	MOVL	DX, PDO(0)(AX)			/* PDE for identity 0-2MB*/
	MOVL	DX, PDO(KZERO)(AX)		/* PDE for KZERO 0-2MB */
	ADDL	$PGLSZ(1), DX
	MOVL	DX, PDO(PGLSZ(1))(AX)		/* PDE for identity 2-4MB*/
	MOVL	DX, PDO(KZERO+PGLSZ(1))(AX)	/* PDE for KZERO 2-4MB */
	ADDL	$PGLSZ(1), DX
	MOVL	DX, PDO(2*PGLSZ(1))(AX)		/* PDE for identity 4-6MB*/
	MOVL	DX, PDO(KZERO+2*PGLSZ(1))(AX)	/* PDE for KZERO 4-6MB */
	ADDL	$PGLSZ(1), DX
	MOVL	DX, PDO(3*PGLSZ(1))(AX)		/* PDE for identity 6-8MB*/
	MOVL	DX, PDO(KZERO+3*PGLSZ(1))(AX)	/* PDE for KZERO 6-8MB */

	/*
	 * KSEG0SIZE is 2GB, so point high 2MB of KZERO at PMAPADDR to PD or
	 * PD2GB page, with 2MB pages.
	 */
	/* switch to PDP entry for 2nd GB of KZERO, for PDMAP & PMAPADDR */
	LEAL	LOW0PD2G(SI), AX
	ADDL	$(507*PGLSZ(1)), DX
	MOVL	DX, PDO(KZERO+508*PGLSZ(1))(AX)	/* PDE for KZERO last 8MB */
	ADDL	$PGLSZ(1), DX
	MOVL	DX, PDO(KZERO+509*PGLSZ(1))(AX)	/* PDE for KZERO last 8MB */
	ADDL	$PGLSZ(1), DX
	MOVL	DX, PDO(KZERO+510*PGLSZ(1))(AX)	/* PDE for KZERO last 8MB */
	ADDL	$PGLSZ(1), DX
	MOVL	DX, PDO(KZERO+511*PGLSZ(1))(AX)	/* PDE for KZERO last 2MB */

	LEAL	LOW0PT2G(SI), DX
	ADDL	$(PTSZ|PteRW|PteP), DX
	MOVL	DX, PDO(PMAPADDR)(AX)		/* PDE for PMAPADDR */

#include "l64lme.s"
	/* now in long mode and MODE $64 and in KZERO space. */
	/* BX is unchanged, DX is 0, SI is sys-KZERO and AX is sys. */

/*
 * zap the identity map;
 * initialise the stack and call the C startup code for cpu0.
 */
	MOVQ	AX, sys(SB)			/* set sys for C */

	MOVQ	AX, SP
	ADDQ	$(LOWMACHSTK+MACHSTKSZ-8), SP	/* set stack */

PUSHQ	AX
PUSHQ	DX
DBGPUT('Z')
POPQ	DX
POPQ	AX

_zap0pml4:
	ADDQ	$LOWPML4, AX
	CMPQ	DX, $PML4O(KZERO)		/* KZER0 & 0x0000ff8000000000 */
	JEQ	_zap0pdp
	MOVQ	DX, PML4O(0)(AX) 		/* zap identity map PML4E */
_zap0pdp:
	MOVQ	sys(SB), AX
	ADDQ	$LOWPDP, AX
	CMPQ	DX, $PDPO(KZERO)		/* KZER0 & 0x0000007fc0000000 */
	JEQ	_zap0pd
	MOVQ	DX, PDPO(0)(AX)			/* zap identity map PDPE */
_zap0pd:
	MOVQ	sys(SB), AX
	ADDQ	$LOWPD, AX
	CMPQ	DX, $PDO(KZERO)			/* KZER0 & 0x000000003fe00000 */
	JEQ	_zap0done
	MOVQ	DX, PDO(0)(AX)			/* zap identity map PDE */
_zap0done:

DBGPUT('m')
	LEAQ	LOWPML4(SI), SI			/* pml4 physical */
	MOVQ	SI, CR3				/* flush TLB */

DBGPUT('1')
	XORQ	DX, DX
	PUSHQ	DX				/* clear flags */
	POPFQ

	MOVLQZX	BX, BX				/* push multiboot args */
	PUSHQ	BX				/* multiboot info* */
	MOVLQZX	RARG, RARG			/* RARG is BP */
	PUSHQ	RARG				/* multiboot magic */

	CALL	main(SB)			/* escape to C at last */

TEXT ndnr(SB), 1, $-4				/* no deposit, no return */
_dnr:
	STI
	HLT
	JMP	_dnr				/* do not resuscitate */
