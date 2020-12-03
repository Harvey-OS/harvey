/*
 * Linker script for Harvey.
 */

ENTRY(_start)

SECTIONS {
	/*
	 * start the kernel at 0x0xffff_8000_0020_0000
	 * This preserves some RAM between 1MiB and the
	 * start of the kernel for critical structures.
	 */
	. = 0xffff800000200000;

	PROVIDE(text = .);
	.text : ALIGN(4096) {
		*(.boottext .bootdata)
		*(.text* .stub .gnu.linkonce.t.*)
	}
	. = ALIGN(2097152);
	PROVIDE(etext = .);	/* Define the 'etext' symbol to this value */

	.rodata : ALIGN(4096) {
		*(.rodata* .gnu.linkonce.r.*)
	}
	. = ALIGN(2097152);
	PROVIDE(erodata = .);

	.data : ALIGN(2097152) {
		*(.data*)
	}
	.got : ALIGN(4096) {
		*(.got)
	}
	.got.plt : ALIGN(4096) {
		*(.got.plt)
	}
	. = ALIGN(4096);
	PROVIDE(edata = .);

	.bss : ALIGN(2097152) {
		*(.bss*)
		*(COMMON)
	}
	. = ALIGN(4096);
	PROVIDE(end = .);

	/DISCARD/ : {
		*(.eh_frame .note.GNU-stack)
		*(.debug*)
	}
}
