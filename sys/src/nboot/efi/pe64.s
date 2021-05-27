TEXT mzhdr(SB), 1, $0
	BYTE $'M'; BYTE $'Z'

	WORD $0		/* e_cblp UNUSED */
	WORD $0		/* e_cp UNUSED */
	WORD $0		/* e_crlc UNUSED */
	WORD $0		/* e_cparhdr UNUSED */
	WORD $0		/* e_minalloc UNUSED */
	WORD $0		/* e_maxalloc UNUSED */
	WORD $0		/* e_ss UNUSED */
	WORD $0		/* e_sp UNUSED */
	WORD $0		/* e_csum UNUSED */
	WORD $0		/* e_ip UNUSED */
	WORD $0		/* e_cs UNUSED */
	WORD $0		/* e_lsarlc UNUSED */
	WORD $0		/* e_ovno UNUSED */

	WORD $0		/* e_res UNUSED */
	WORD $0
	WORD $0
	WORD $0
	WORD $0

	WORD $0		/* e_oemid UNUSED */

	WORD $0		/* e_res2 UNUSED */
	WORD $0
	WORD $0
	WORD $0
	WORD $0
	WORD $0
	WORD $0
	WORD $0
	WORD $0
	WORD $0

	LONG $pehdr-IMAGEBASE(SB)	/* offset to pe header */

TEXT pehdr(SB), 1, $0
	BYTE $'P'; BYTE $'E'; BYTE $0; BYTE $0

	WORD $0x8664		/* Machine (AMD64) */
	WORD $1			/* NumberOfSections */
	LONG $0			/* TimeDateStamp UNUSED */
	LONG $0			/* PointerToSymbolTable UNUSED */
	LONG $0			/* NumberOfSymbols UNUSED */
	WORD $0xF0		/* SizeOfOptionalHeader */
	WORD $2223		/* Characteristics */

	WORD $0x20B		/* Magic (PE32+) */
    	BYTE $9			/* MajorLinkerVersion UNUSED */
	BYTE $0			/* MinorLinkerVersion UNUSED */
	LONG $0			/* SizeOfCode UNUSED */
	LONG $0			/* SizeOfInitializedData UNUSED */
	LONG $0			/* SizeOfUninitializedData UNUSED */
	LONG $start-IMAGEBASE(SB)/* AddressOfEntryPoint */
	LONG $0			/* BaseOfCode UNUSED */

	QUAD $IMAGEBASE		/* ImageBase */
	LONG $0x200		/* SectionAlignment */
	LONG $0x200		/* FileAlignment */
	WORD $4			/* MajorOperatingSystemVersion UNUSED */
	WORD $0			/* MinorOperatingSystemVersion UNUSED */
	WORD $0			/* MajorImageVersion UNUSED */
	WORD $0			/* MinorImageVersion UNUSED */
	WORD $4			/* MajorSubsystemVersion */
	WORD $0			/* MinorSubsystemVersion UNUSED */
	LONG $0			/* Win32VersionValue UNUSED */
	LONG $end-IMAGEBASE(SB)	/* SizeOfImage */
 	LONG $start-IMAGEBASE(SB)/* SizeOfHeaders */
 	LONG $0			/* CheckSum UNUSED */
	WORD $10		/* Subsystem (10 = efi application) */
	WORD $0			/* DllCharacteristics UNUSED */
	QUAD $0			/* SizeOfStackReserve UNUSED */
	QUAD $0			/* SizeOfStackCommit UNUSED */
	QUAD $0			/* SizeOfHeapReserve UNUSED */
	QUAD $0			/* SizeOfHeapCommit UNUSED */
	LONG $0			/* LoaderFlags UNUSED */
	LONG $16		/* NumberOfRvaAndSizes UNUSED */

	LONG $0; LONG $0
	LONG $0; LONG $0
	LONG $0; LONG $0		/* RVA */
	LONG $0; LONG $0		/* RVA */
	LONG $0; LONG $0		/* RVA */
	LONG $0; LONG $0		/* RVA */
	LONG $0; LONG $0		/* RVA */
	LONG $0; LONG $0		/* RVA */
	LONG $0; LONG $0		/* RVA */
	LONG $0; LONG $0		/* RVA */
	LONG $0; LONG $0		/* RVA */
	LONG $0; LONG $0		/* RVA */
	LONG $0; LONG $0		/* RVA */
	LONG $0; LONG $0		/* RVA */
	LONG $0; LONG $0		/* RVA */
	LONG $0; LONG $0		/* RVA */

	BYTE $'.'; BYTE $'t'; BYTE $'e'; BYTE $'x'
	BYTE $'t'; BYTE $0;   BYTE $0;   BYTE $0
	LONG $edata-(IMAGEBASE+0x200)(SB)		/* VirtualSize */
	LONG $start-IMAGEBASE(SB)			/* VirtualAddress */
	LONG $edata-(IMAGEBASE+0x200)(SB)		/* SizeOfData */
	LONG $start-IMAGEBASE(SB)			/* PointerToRawData */
	LONG $0			/* PointerToRelocations UNUSED */
	LONG $0			/* PointerToLinenumbers UNUSED */
	WORD $0			/* NumberOfRelocations UNUSED */
	WORD $0			/* NumberOfLinenumbers UNUSED */
	LONG $0x86000020	/* Characteristics (code, execute, read, write) */

	/* padding to get start(SB) at IMAGEBASE+0x200 */
	LONG $0; LONG $0; LONG $0; LONG $0;
	LONG $0; LONG $0; LONG $0; LONG $0;
	LONG $0; LONG $0; LONG $0; LONG $0;
	LONG $0; LONG $0; LONG $0; LONG $0;
	LONG $0; LONG $0; LONG $0; LONG $0;
	LONG $0; LONG $0; LONG $0; LONG $0;
	LONG $0; LONG $0; LONG $0; LONG $0;
	LONG $0; LONG $0; LONG $0; LONG $0;
	LONG $0; LONG $0; LONG $0; LONG $0

MODE $64

TEXT start(SB), 1, $-4
	/* spill arguments */
	MOVQ CX, 8(SP)
	MOVQ DX, 16(SP)

	CALL reloc(SP)

TEXT reloc(SB), 1, $-4
	MOVQ 0(SP), SI
	SUBQ $reloc-IMAGEBASE(SB), SI
	MOVQ $IMAGEBASE, DI
	MOVQ $edata-IMAGEBASE(SB), CX
	CLD
	REP; MOVSB

	MOVQ 16(SP), BP
	MOVQ $efimain(SB), DI
	MOVQ DI, (SP)
	RET

TEXT eficall(SB), 1, $-4
	MOVQ SP, SI
	MOVQ SP, DI
	MOVL $(8*16), CX
	SUBQ CX, DI
	ANDQ $~15ULL, DI
	LEAQ 16(DI), SP
	CLD
	REP; MOVSB
	SUBQ $(8*16), SI

	MOVQ 0(SP), CX
	MOVQ 8(SP), DX
	MOVQ 16(SP), R8
	MOVQ 24(SP), R9
	CALL BP

	MOVQ SI, SP
	RET

#include "mem.h"

TEXT jump(SB), 1, $-4
	CLI

	/* load zero length idt */
	MOVL	$_idtptr64p<>(SB), AX
	MOVL	(AX), IDTR

	/* load temporary gdt */
	MOVL	$_gdtptr64p<>(SB), AX
	MOVL	(AX), GDTR

	/* load CS with 32bit code segment */
	PUSHQ	$SELECTOR(3, SELGDT, 0)
	PUSHQ	$_warp32<>(SB)
	RETFQ

MODE $32

TEXT	_warp32<>(SB), 1, $-4

	/* load 32bit data segments */
	MOVL	$SELECTOR(2, SELGDT, 0), AX
	MOVW	AX, DS
	MOVW	AX, ES
	MOVW	AX, FS
	MOVW	AX, GS
	MOVW	AX, SS

	/* turn off paging */
	MOVL	CR0, AX
	ANDL	$0x7fffffff, AX		/* ~(PG) */
	MOVL	AX, CR0

	MOVL	$0, AX
	MOVL	AX, CR3

	/* disable long mode */
	MOVL	$0xc0000080, CX		/* Extended Feature Enable */
	RDMSR
	ANDL	$0xfffffeff, AX		/* Long Mode Disable */
	WRMSR

	/* diable pae */
	MOVL	CR4, AX
	ANDL	$0xffffff5f, AX		/* ~(PAE|PGE) */
	MOVL	AX, CR4

	JMP	*BP

TEXT _gdt<>(SB), 1, $-4
	/* null descriptor */
	LONG	$0
	LONG	$0

	/* (KESEG) 64 bit long mode exec segment */
	LONG	$(0xFFFF)
	LONG	$(SEGL|SEGG|SEGP|(0xF<<16)|SEGPL(0)|SEGEXEC|SEGR)

	/* 32 bit data segment descriptor for 4 gigabytes (PL 0) */
	LONG	$(0xFFFF)
	LONG	$(SEGG|SEGB|(0xF<<16)|SEGP|SEGPL(0)|SEGDATA|SEGW)

	/* 32 bit exec segment descriptor for 4 gigabytes (PL 0) */
	LONG	$(0xFFFF)
	LONG	$(SEGG|SEGD|(0xF<<16)|SEGP|SEGPL(0)|SEGEXEC|SEGR)

TEXT _gdtptr64p<>(SB), 1, $-4
	WORD	$(4*8-1)
	QUAD	$_gdt<>(SB)

TEXT _idtptr64p<>(SB), 1, $-4
	WORD	$0
	QUAD	$0
