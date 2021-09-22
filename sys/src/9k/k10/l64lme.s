/*
 * fragment to #include.  starts MODE $32, leaves in MODE $64.
 * On return, BX is unchanged, DX is 0,
 * SI is reduced to a physical address and AX is SI+KZERO.
 * also defines _gdt* descriptors.
 * used by cpu0 and AP start up.
 */

/*
 * Enable and activate Long Mode. From the manual:
 * 	make sure Page Size Extentions are off, and Page Global
 *	Extensions and Physical Address Extensions are on in CR4;
 *	set Long Mode Enable in the Extended Feature Enable MSR;
 *	set Paging Enable in CR0;
 *	make an inter-segment jump to the Long Mode code.
 * It's all in 32-bit mode until the jump is made.
 */
MODE $32

TEXT _lme<>(SB), 1, $-4
DBGPUT('M')
	MOVL	$Efer, CX			/* Extended Feature Enable */
	RDMSR
	MOVL	AX, DX
	ANDL	$Lme, DX			/* Long Mode Enable bit */
	CMPL	DX, $0
	JEQ	_32p
	/* we're already in long mode.  skip protected mode set up */
	JMP	_identity<>-KZERO(SB)

_32p:
DBGPUT('p')
	MOVL	CR4, AX
	ANDL	$~Pse, AX			/* Page Size */
	ORL	$(Pge|Pae), AX			/* Page Global, Phys. Address */
	MOVL	AX, CR4

	MOVL	$Efer, CX			/* Extended Feature Enable */
	RDMSR
	ORL	$Lme, AX			/* set Long Mode Enable */
	WRMSR

	MOVL	CR0, DX
	ANDL	$~(Cd|Nw|Ts|Mp), DX		/* caches on */
	ORL	$(Pg|Wp), DX			/* Paging Enable */
/**/	MOVL	DX, CR0

	/* we're now in (64-bit) compatibility mode of long mode */
	pFARJMP32(SSEL(3, SsTIGDT|SsRPL0), _identity<>-KZERO(SB))

/*
 * Long mode. Welcome to 2003.
 * Jump out of the identity map space into KZERO;
 * load a proper long mode GDT, zero segment registers.
 */
MODE $64

TEXT _gdt32p<>(SB), 1, $-4
	QUAD	$0				/* NULL descriptor */
	QUAD	$(SdG|SdD|Sd4G|SdP|SdS|SdCODE|SdW|0xffff) /* CS */
	QUAD	$(SdG|SdD|Sd4G|SdP|SdS|SdW|0xffff)	/* DS */
	QUAD	$(SdL|SdP|SdS|SdCODE)		/* Long mode CS */

TEXT _gdtptr32p<>(SB), 1, $-4
	WORD	$(4*8-1)			/* includes long mode */
	LONG	$_gdt32p<>-KZERO(SB)

TEXT _gdt64<>(SB), 1, $-4
	QUAD	$0				/* NULL descriptor */
	QUAD	$(SdL|SdP|SdS|SdCODE)		/* Long mode CS */
	QUAD	$SdP				/* DS */

TEXT _gdtptr64p<>(SB), 1, $-4
	WORD	$(2*8-1)
	QUAD	$_gdt64<>-KZERO(SB)

TEXT _gdtptr64v<>(SB), 1, $-4
	WORD	$(3*8-1)
	QUAD	$_gdt64<>(SB)

/* we're in long mode */
TEXT _identity<>(SB), 1, $-4
DBGPUT('I')
	MOVQ	$_start64v<>(SB), AX
	JMP*	AX			/* jump into KZERO space */

TEXT _start64v<>(SB), 1, $-4
DBGPUT('v')
	MOVQ	$_gdtptr64v<>(SB), AX
/**/	MOVL	(AX), GDTR		/* enter 64-bit submode of long mode */

DBGPUT('G')
	XORQ	DX, DX				/* DX is 0 from here on */
	MOVW	DX, DS				/* not used in long mode */
	MOVW	DX, ES				/* not used in long mode */
	MOVW	DX, FS
	MOVW	DX, GS
	MOVW	DX, SS				/* not used in long mode */

	MOVLQZX	SI, SI				/* reduce to physical */
	MOVQ	$KZERO, AX
	ADDQ	SI, AX				/* virtual sys or PML4 */
