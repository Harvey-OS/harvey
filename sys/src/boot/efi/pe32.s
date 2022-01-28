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

	WORD $0x014C		/* Machine (Intel 386) */
	WORD $1			/* NumberOfSections */
	LONG $0			/* TimeDateStamp UNUSED */
	LONG $0			/* PointerToSymbolTable UNUSED */
	LONG $0			/* NumberOfSymbols UNUSED */
	WORD $0xE0		/* SizeOfOptionalHeader */
	WORD $2103		/* Characteristics (no relocations, executable, 32 bit) */

	WORD $0x10B		/* Magic (PE32) */
    	BYTE $9			/* MajorLinkerVersion UNUSED */
	BYTE $0			/* MinorLinkerVersion UNUSED */
	LONG $0			/* SizeOfCode UNUSED */
	LONG $0			/* SizeOfInitializedData UNUSED */
	LONG $0			/* SizeOfUninitializedData UNUSED */
	LONG $start-IMAGEBASE(SB)/* AddressOfEntryPoint */
	LONG $0			/* BaseOfCode UNUSED */
	LONG $0			/* BaseOfData UNUSED */
	LONG $IMAGEBASE		/* ImageBase */
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
	LONG $0			/* SizeOfStackReserve UNUSED */
	LONG $0			/* SizeOfStackCommit UNUSED */
	LONG $0			/* SizeOfHeapReserve UNUSED */
	LONG $0			/* SizeOfHeapCommit UNUSED */
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
	LONG $0; LONG $0; LONG $0; LONG $0;
	LONG $0; LONG $0; LONG $0; LONG $0;

TEXT start(SB), 1, $0
	CALL reloc(SP)

TEXT reloc(SB), 1, $0
	MOVL 0(SP), SI
	SUBL $reloc-IMAGEBASE(SB), SI
	MOVL $IMAGEBASE, DI
	MOVL $edata-IMAGEBASE(SB), CX
	CLD
	REP; MOVSB
	MOVL $efimain(SB), DI
	MOVL DI, (SP)
	RET

TEXT jump(SB), $0
	CLI
	MOVL 4(SP), AX
	JMP *AX

TEXT eficall(SB), 1, $0
	MOVL SP, SI
	MOVL SP, DI
	MOVL $(4*16), CX
	SUBL CX, DI
	ANDL $~15ULL, DI
	SUBL $8, DI

	MOVL 4(SI), AX
	LEAL 8(DI), SP

	CLD
	REP; MOVSB
	SUBL $(4*16), SI

	CALL AX

	MOVL SI, SP
	RET
