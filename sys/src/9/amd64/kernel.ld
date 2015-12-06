/* Simple linker script for the ROS kernel.
   See the GNU ld 'info' manual ("info ld") to learn the syntax. */

/* This script needs to be invoked with -z max-page-size=0x1000.  Otherwise,
 * ld will offset our first section to 1MB within the actual file.  Multiboot
 * requires the header to be in the first two pages. */

OUTPUT_FORMAT("elf64-x86-64", "elf64-x86-64", "elf64-x86-64")
ENTRY(_start)
/* start the kernel at 0x110000. 
 * That way we can use lower ram for critical structures
 */
KERN_LOAD_ADDR = 0xfffffffff0000000;

SECTIONS
{
	/* Entry Linked and loaded at 0x00100000 (includes multiboot) */
	. = 0x00110000;

	.bootstrap : {
		*(.boottext .bootdata)
	}

	/* Link the main kernel for the space after entry + KERN_LOAD_ADDR.  We'll
	 * still load it adjacent in physical memory */
	. += KERN_LOAD_ADDR;

	.text : AT(ADDR(.text) - KERN_LOAD_ADDR) {
		*(.text .stub .text.* .gnu.linkonce.t.*)
	}

	PROVIDE(etext = .);	/* Define the 'etext' symbol to this value */

	/* Linker-made tables.  Our tables (e.g. devtab) are 2^6 aligned,
	 * independently of us aligning '.'.  We align '.' to get the right start,
	 * e.g.  __devtabstart. */
	. = ALIGN(64);
	/* We shouldn't have to use PROVIDE, but if we don't, we get the wrong
	 * value for '.'.  And items with empty tables get the KLA (basically 0) */
	PROVIDE(__devtabstart = .);
	PROVIDE(devtab = .);
	.devtab : {
		*(.devtab)
	}
	PROVIDE(__devtabend = .);

	.rodata : {
		*(.rodata .rodata.* .gnu.linkonce.r.*)
	}

	/* TODO: add some debug info.  i hear stabs are 32 bit only, so we'll need
	 * to bring in some dwarves.  for now, hack in the symbols to compile. */
	PROVIDE(__STAB_BEGIN__ = .);
	PROVIDE(__STAB_END__ = .);
	PROVIDE(__STABSTR_BEGIN__ = .);
	PROVIDE(__STABSTR_END__ = .);

	/* Adjust the address for the data segment to the next page */
	. = ALIGN(0x1000);

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
