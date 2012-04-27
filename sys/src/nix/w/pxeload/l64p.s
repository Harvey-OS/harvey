#include "mem.h"

/*
 * Some machine instructions not handled by 8[al].
 */
#define WRMSR		BYTE $0x0F; BYTE $0x30	/* WRMSR, lo argument in AX */
						/*	  hi argument in DX */
#define RDMSR		BYTE $0x0F; BYTE $0x32	/* RDMSR, lo result in AX */
						/*	  hi result in DX */
#define FARJUMP(s, o)	BYTE $0xEA;		/* far jump to ptr32:16 */\
			LONG $o; WORD $s

/*
 * Macro for calculating offset within the page directory base.
 * Note that this is assembler-specific hence the '<<2'.
 */
#define PDO(a)		(((((a))>>22) & 0x03FF)<<2)

TEXT _warp64(SB), $0
	CLI
	MOVL	cr3+0(FP), BP			/* pml4 */

	MOVL	CR3, CX				/* load address of PDB */
	ADDL	$KZERO, CX
	MOVL	PDO(KZERO)(CX), DX		/* double-map KZERO at 0 */
	MOVL	DX, PDO(0)(CX)

	MOVL	CR3, CX
	MOVL	CX, CR3				/* load and flush the mmu */

	MOVL	$_start32id<>-KZERO(SB), AX
	JMP*	AX				/* jump to identity-map */

TEXT _start32id<>(SB), $0
	MOVL	CR0, DX				/* turn off paging */
	ANDL	$~0x80000000, DX		/* ~(PG) */

	MOVL	$_stop32pg<>-KZERO(SB), AX
	MOVL	DX, CR0
	JMP*	AX				/* forward into the past */

TEXT _stop32pg<>(SB), $0
MOVL $multibootheader-KZERO(SB), BX
MOVL $0x2badb002, AX
MOVL $0x00110000, CX
JMP* CX
	MOVL	CR4, AX
	ORL	$0x00000020, AX			/* Pae */
	MOVL	AX, CR4

	MOVL	BP, CR3				/* pml4 */

	MOVL	$0xC0000080, CX			/* Extended Feature Enable */
	RDMSR
	ORL	$0x00000100, AX			/* Long Mode Enable */
	MOVL	$0xC0000080, CX
	WRMSR

	MOVL	CR0, DX				/* enable paging */
	ORL	$0x80010000, DX
	MOVL	$_start32cm<>-KZERO(SB), AX
	MOVL	DX, CR0
	JMP*	AX

TEXT _start32cm<>(SB), $0			/* compatibility mode */
	MOVL	$_gdtptr64<>-KZERO(SB), AX	/* load a long-mode GDT */
	MOVL	(AX), GDTR
	MOVL	$_start64lm<>-KZERO(SB), AX
	JMP*	AX

TEXT _start64lm<>(SB), $0			/* finally - long mode */
	MOVL	$multibootheader-KZERO(SB), BX	/* multiboot data pointer */
	MOVL	$0x2badb002, AX			/* multiboot magic */
	FARJUMP((1<<3), 0x00110000)		/* selector 1, in  GDT, RPL 0 */
_idle:
	JMP	_idle

TEXT _gdt64<>(SB), $0
	LONG	$0x00000000; LONG $0x00000000	/* NULL descriptor */
	LONG	$0x00000000; LONG $0x00209800	/* CS */
	LONG	$0x00000000; LONG $0x00008000	/* DS */

TEXT _gdtptr64<>(SB), $0
	WORD	$(3*8-1)
	LONG	$_gdt64<>-KZERO(SB)
