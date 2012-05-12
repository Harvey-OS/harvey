#include "mem.h"

/*
 * Macro for calculating offset within the page directory base.
 * Note that this is assembler-specific hence the '<<2'.
 */
#define PDO(a)		(((((a))>>22) & 0x03FF)<<2)

TEXT _warp64(SB), $0
	CLI
	MOVL	entry+0(FP), BP			/* entry */

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
	ANDL	$~PG, DX

	MOVL	$_stop32pg<>-KZERO(SB), AX
	MOVL	DX, CR0
	JMP*	AX				/* forward into the past */

TEXT _stop32pg<>(SB), $0
	MOVL multibootheader-KZERO(SB), BX
	MOVL $0x2badb002, AX

	JMP* BP
