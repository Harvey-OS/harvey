#include "mem.h"

/*
 * Some machine instructions not handled by 8[al].
 */
#define	DELAY		BYTE $0xEB; BYTE $0x00	/* JMP .+2 */
#define FARJUMP(s, o)	BYTE $0xEA;		/* far jump to ptr32:16 */\
			LONG $o; WORD $s

/*
 * Macro for calculating offset within the page directory base.
 * Note that this is assembler-specific hence the '<<2'.
 */
#define PDO(a)		(((((a))>>22) & 0x03FF)<<2)

/*
 * May enter here either from the 16-bit real-mode startup or
 * from other 32-bit protected mode code. For the latter case must
 * make sure the GDT is set as it would be by the 16-bit code:
 *	disable interrupts;
 *	load the GDT with the table in _gdt32p;
 *	load all the data segments
 *	load the code segment via a far jump.
 */
TEXT _start32p(SB), $0
	BYTE $0xEB; BYTE $0x58;			/* jmp .+ 0x58  (_start32p58) */
	BYTE $0x90; BYTE $0x90			/* nop */

/*
 * Must be 4-byte aligned.
 */
TEXT _multibootheader(SB), $0
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

TEXT _gdt32p(SB), $0
	LONG	$0x0000; LONG $0
	LONG	$0xFFFF; LONG $(SEGG|SEGB|(0xF<<16)|SEGP|SEGPL(0)|SEGDATA|SEGW)
	LONG	$0xFFFF; LONG $(SEGG|SEGD|(0xF<<16)|SEGP|SEGPL(0)|SEGEXEC|SEGR)

TEXT _gdtptr32p(SB), $0
	WORD	$(3*8)
	LONG	$_gdt32p-KZERO(SB)

_start32p58:
	CLI

	MOVL	AX, _multibootheader+(48-KZERO)(SB)
	MOVL	BX, _multibootheader+(52-KZERO)(SB)

	MOVL	$_gdtptr32p-KZERO(SB), AX
	MOVL	(AX), GDTR

	MOVL	$SELECTOR(1, SELGDT, 0), AX
	MOVW	AX, DS
	MOVW	AX, ES
	MOVW	AX, FS
	MOVW	AX, GS
	MOVW	AX, SS

	FARJUMP(SELECTOR(2, SELGDT, 0), _start32pg-KZERO(SB))

/*
 * Make the basic page tables for processor 0. Five pages are needed for
 * the basic set:
 * a page directory, a page table for mapping the first 4MB of physical
 * memory, a separate physical and virtual page for the Mach structure
 * and a page to be used later for the GDT.
 * The remaining PTEs will be allocated later when memory is sized.
 * Could clear BSS here too when clearing the space for the tables,
 * but that would violate the separation of church and state.
 * The first aligned page after end[] is where the real-mode startup
 * puts any BIOS parameters (APM, MAP).
 */
TEXT _start32pg(SB), $0
	MOVL	$end-KZERO(SB), DX	/* clear pages for the tables etc. */
	ADDL	$(2*BY2PG-1), DX	/* start must be page aligned */
	ANDL	$~(BY2PG-1), DX

	MOVL	DX, DI
	XORL	AX, AX
	MOVL	$(5*BY2PG), CX
	SHRL	$2, CX

	CLD
	REP;	STOSL

	MOVL	DX, AX			/* bootstrap processor PDB */
	MOVL	DX, DI			/* save it for later */
	ADDL	$BY2PG, DX		/* -> PA of first page of page table */
	ADDL	$PDO(KZERO), AX		/* page directory offset for KZERO */
	MOVL	DX, (AX)		/* PTE's for KZERO */
	MOVL	$(PTEWRITE|PTEVALID), BX/* page permissions */
	ORL	BX, (AX)

	MOVL	DX, AX			/* PA of first page of page table */
	MOVL	$1024, CX		/* 1024 pages in 4MB */
_setpte:
	MOVL	BX, (AX)
	ADDL	$(1<<PGSHIFT), BX
	ADDL	$4, AX
	LOOP	_setpte

	MOVL	DX, AX			/* PA of first page of page table */
	ADDL	$BY2PG, DX		/* -> PA of Mach structure */

	MOVL	DX, BX
	ADDL	$(KZERO+BY2PG), BX	/* VA of Mach structure */
	SHRL	$10, BX			/* create offset into PTEs */
	ANDL	$(0x3FF<<2), BX

	ADDL	BX, AX			/* PTE offset for Mach structure */
	MOVL	DX, (AX)		/* create PTE for Mach */
	MOVL	$(PTEWRITE|PTEVALID), BX/* page permissions */
	ORL	BX, (AX)

/*
 * Now ready to use the new map. Initialise CR0 (assume the BIOS gets
 * it mostly correct for this processor type w.r.t. caches and FPU).
 * It is necessary on some processors to follow mode switching
 * with a JMP instruction to clear the prefetch queues.
 * The instruction to switch modes and the following jump instruction
 * should be identity-mapped; to this end double map KZERO at
 * virtual 0 and undo the mapping once virtual nirvana has been attained.
 * If paging is already on, don't load CR3 before setting CR0, in which
 * case most of this is a NOP and the 2nd load of CR3 actually does
 * the business.
 */
	MOVL	DI, CX			/* load address of PDB */
	MOVL	PDO(KZERO)(CX), DX	/* double-map KZERO at 0 */
	MOVL	DX, PDO(0)(CX)

	MOVL	CR0, DX
	MOVL	DX, AX
	ANDL	$0x80000000, AX		/* check paging not already on */
	JNE	_nocr3load
	MOVL	CX, CR3

_nocr3load:
	ORL	$0x80010000, DX		/* PG|WP */
	ANDL	$~0x6000000A, DX	/* ~(CD|NW|TS|MP) */

	MOVL	$_startpg(SB), AX
	TESTL	$KZERO, AX		/* want to run protected or virtual? */
	JNE	_dorkness

	MOVL	$_start32v(SB), AX
	JMP*	AX			/* into the dorkness */

_dorkness:
	MOVL	DX, CR0			/* turn on paging */
	JMP*	AX			/* headfirst into the new world */

TEXT _startpg(SB), $0
//	MOVL	$0, PDO(0)(CX)		/* undo double-map of KZERO at 0 */
//	MOVL	CX, CR3			/* load and flush the mmu */

	MOVL	$_start32v(SB), AX
	JMP*	AX			/* into the dorkness */
