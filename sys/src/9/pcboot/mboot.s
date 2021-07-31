/* included by l16r.s and mbootstart.s */

/*
 * Must be 4-byte aligned & within 8K of the image's start.
 */
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
