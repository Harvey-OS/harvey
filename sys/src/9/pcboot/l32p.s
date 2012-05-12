#include "mem.h"

#define KB		1024
#define MB		(1024*1024)

/*
 * Some machine instructions not handled by 8[al].
 */
#define	DELAY		BYTE $0xEB; BYTE $0x00	/* JMP .+2 */
#define FARJUMP(s, o)	BYTE $0xEA;		/* far jump to ptr32:16 */\
			LONG $o; WORD $s

#define NOP		BYTE $0x90
#define HLT		BYTE $0xF4

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
/* if distance to _start32p32 changes, update the offset after 0xEB. */
//	jmp	.+32 (_start32p32).
	BYTE $0xEB; BYTE $(2+3*2*4+2+4)
	NOP; NOP

TEXT _gdt32p(SB), $0
	LONG	$0x0000; LONG $0
	LONG	$0xFFFF; LONG $(SEGG|SEGB|(0xF<<16)|SEGP|SEGPL(0)|SEGDATA|SEGW)
	LONG	$0xFFFF; LONG $(SEGG|SEGD|(0xF<<16)|SEGP|SEGPL(0)|SEGEXEC|SEGR)

TEXT _gdtptr32p(SB), $0
	WORD	$(3*8)
	LONG	$_gdt32p-KZERO(SB)

_start32p32:
	CLI

	MOVL	AX, _multibootheader+(48-KZERO)(SB)
	MOVL	BX, _multibootheader+(52-KZERO)(SB)

	MOVL	$_gdtptr32p-KZERO(SB), AX
	MOVL	(AX), GDTR

	MOVL	$KDSEL, AX
	MOVW	AX, DS
	MOVW	AX, ES
	MOVW	AX, FS
	MOVW	AX, GS
	MOVW	AX, SS

	DELAY
	FARJUMP(KESEL, _start32pg-KZERO(SB))

/*
 * Make the basic page tables for processor 0. Eight pages are needed for
 * the basic set:
 * a page directory, 5 pages pf page tables for mapping the first 20MB of
 * physical memory, a single physical and virtual page for the Mach structure,
 * and a page to be used later for the GDT.
 *
 * The remaining PTEs will be allocated later when memory is sized.
 * Could clear BSS here too when clearing the space for the tables,
 * but that would violate the separation of church and state.
 * The first aligned page after end[] was used for passing BIOS parameters
 * by the real-mode startup, now it's BIOSTABLES.
 */
TEXT _start32pg(SB), $0			/* CHECK alignment (16) */
	DELAY

	MOVL	$end-KZERO(SB), DX	/* clear pages for the tables etc. */
	/* start must be page aligned, skip boot params page */
	ADDL	$(2*BY2PG-1), DX
	ANDL	$~(BY2PG-1), DX

	/*
	 * zero mach page & gdt in low memory
	 */
	MOVL	$(CPU0MACH-KZERO), DI
	XORL	AX, AX
	MOVL	$(2*BY2PG), CX		/* mach (phys & virt) & gdt */
	SHRL	$2, CX
	CLD
	REP;	STOSL			/* zero mach & gdt pages */

	/*
	 * zero pdb and pte for low memory
	 */
	MOVL	DX, DI			/* first page after end & boot params */
	XORL	AX, AX
	MOVL	$((1+LOWPTEPAGES)*BY2PG), CX /* pdb, n pte */
	SHRL	$2, CX
	CLD
	REP;	STOSL			/* zero pdb & pte pages */

	/*
	 * populate pdb for low memory (20MB)
	 */
	MOVL	DX, AX			/* bootstrap processor PDB (page 0) */
	MOVL	DX, DI			/* save it for later */
	MOVL	$(PTEWRITE|PTEVALID), BX/* page permissions */

	ADDL	$BY2PG, DX		/* -> PA of first page of page table (page 1)  */
	ADDL	$PDO(KZERO), AX		/* page directory offset for KZERO */
	MOVL	DX, (AX)		/* PTE's for KZERO */
	ORL	BX, (AX)

	/* should be LOWPTEPAGES-1 repetitions of this fragment */
	ADDL	$BY2PG, DX		/* -> PA of second page of page table (page 2)  */
	ADDL	$4, AX			/* page dir. offset for KZERO+4MB */
	MOVL	DX, (AX)		/* PTE's for KZERO */
	ORL	BX, (AX)

	ADDL	$BY2PG, DX		/* -> PA of third page of page table (page 3)  */
	ADDL	$4, AX			/* page dir. offset for KZERO+8MB */
	MOVL	DX, (AX)		/* PTE's for KZERO */
	ORL	BX, (AX)

	ADDL	$BY2PG, DX		/* -> PA of fourth page of page table (page 4)  */
	ADDL	$4, AX			/* page dir. offset for KZERO+12MB */
	MOVL	DX, (AX)		/* PTE's for KZERO */
	ORL	BX, (AX)

	ADDL	$BY2PG, DX		/* -> PA of fifth page of page table (page 5)  */
	ADDL	$4, AX			/* page dir. offset for KZERO+16MB */
	MOVL	DX, (AX)		/* PTE's for KZERO */
	ORL	BX, (AX)

	/*
	 * populate page tables for low memory
	 */
	MOVL	DI, AX
	ADDL	$BY2PG, AX		/* PA of first page of page table */
	MOVL	$(MemMin/BY2PG), CX
_setpte:
	MOVL	BX, (AX)
	ADDL	$(1<<PGSHIFT), BX
	ADDL	$4, AX
	LOOP	_setpte

	/*
	 * map the Mach page
	 */
	MOVL	DI, AX
	ADDL	$BY2PG, AX		/* PA of first page of page table */
	MOVL	$(CPU0MACH-KZERO), DX	/* -> PA of Mach structure */
	MOVL	$CPU0MACH, BX		/* VA of Mach structure */
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
	/* double-map first 20 MB, since we are running at 7 or 9 MB */
	/* should be LOWPTEPAGES repetitions */
	MOVL	PDO(KZERO)(CX), DX	/* double-map KZERO at 0 */
	MOVL	DX, PDO(0)(CX)
	MOVL	PDO(KZERO+4*MB)(CX), DX
	MOVL	DX, PDO(4*MB)(CX)
	MOVL	PDO(KZERO+8*MB)(CX), DX
	MOVL	DX, PDO(8*MB)(CX)
	MOVL	PDO(KZERO+12*MB)(CX), DX
	MOVL	DX, PDO(12*MB)(CX)
	MOVL	PDO(KZERO+16*MB)(CX), DX
	MOVL	DX, PDO(16*MB)(CX)

	MOVL	CR0, DX
	MOVL	DX, AX
	ANDL	$PG, AX			/* check paging not already on */
	JNE	_nocr3load
	MOVL	CX, CR3			/* paging off, safe to load cr3 */
_nocr3load:
	ORL	$(PG|0x10000), DX	/* PG|WP */
	ANDL	$~0x6000000A, DX	/* ~(CD|NW|TS|MP) */

	MOVL	$_startpg(SB), AX
	TESTL	$KZERO, AX		/* want to run protected or virtual? */
	JEQ	_to32v			/* protected */
	MOVL	DX, CR0			/* turn on paging */
	JMP*	AX			/* headfirst into the new world */

TEXT _startpg(SB), $0
//	MOVL	$0, PDO(0)(CX)		/* undo double-map of KZERO at 0 */
	MOVL	CX, CR3			/* load and flush the mmu */
_to32v:
	MOVL	$_start32v(SB), AX
	JMP*	AX			/* into the dorkness */
