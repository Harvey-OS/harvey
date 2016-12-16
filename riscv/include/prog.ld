OUTPUT_ARCH("riscv")
ENTRY(main)

SECTIONS
{
	. = 0x200000;
	.text : {
		*(.text .stub .text.* .gnu.linkonce.t.*)
	}
	PROVIDE(etext = .);	/* Define the 'etext' symbol to this value */

	.rodata : {
		*(.rodata .rodata.* .gnu.linkonce.r.*)
	}

	/* Adjust the address for the data segment to the next page */
	. = ALIGN(0x200000);

	/* The data segment */
	.data : {
		*(.data)
	}

	PROVIDE(edata = .);


	.bss : {
		*(.bss)
		*(COMMON)
	}

	PROVIDE(end = .);
	/DISCARD/ : {
		*(.eh_frame .note.GNU-stack)
	}
}
