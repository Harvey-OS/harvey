/*
 * pc machine-language assist
 * shutdown and restart code, mostly taken from older pcboot.
 */
#include "mem.h"
#include "/sys/src/boot/pc/x16.h"

#define WBINVD	BYTE $0x0F; BYTE $0x09

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
	WORD	$(4*8-1)
	LONG	$_gdt16r-KZERO(SB)

TEXT pagingoff(SB), $0
	DELAY				/* JMP .+2 */

	/*
	 *  use a jump to an absolute location to get the PC out of
	 *  KZERO.  first establishes double mapping of first few MB.
	 */
	CLI
	MOVL	CR3, DI				/* load address of PDB */
	ADDL	$KZERO, DI
	CALL	map0pages(SB)
	MOVL	CR3, DI				/* use physical pdb addr */
/**/	MOVL	DI, CR3				/* load and flush the mmu */
	DELAY					/* JMP .+2 */

	MOVL	entry+0(FP), DX
	LEAL	_nopaging-KZERO(SB),AX
	JMP*	AX				/* jump to identity-map */

TEXT _nopaging(SB), $0
	DELAY				/* JMP .+2 */

#ifdef PARANOID				/* not needed so far */
	MOVL	$RMSTACK, SP		/* switch to low stack */
	MOVL	_gdtptr16r-KZERO(SB), GDTR /* change gdt to physical pointer */
#endif

	/*
	 * turn off paging, stay in protected mode
	 */
	MOVL	CR0,AX
	ANDL	$~PG, AX
/**/	MOVL	AX,CR0
	DELAY				/* JMP .+2 */

	MOVL	$_stop32pg-KZERO(SB), AX
	JMP*	AX				/* forward into the past */

TEXT _stop32pg(SB), $0
	DELAY				/* JMP .+2 */

	MOVL	multibootheader-KZERO(SB), BX	/* multiboot data pointer */
	/* BX should now contain 0x500 (MBOOTTAB-KZERO) */
	MOVL	$MBOOTREGMAG, AX		/* multiboot magic */
	WBINVD
	JMP*	DX				/* into the loaded kernel */
	JMP	idle(SB)
