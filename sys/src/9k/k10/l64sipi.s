/*
 * Start-up request IPI handler.
 *
 * This code is executed on an application processor in response to receiving
 * a Start-up IPI (SIPI) from another processor.
 * This must be placed on a 4KiB boundary
 * somewhere in the 1st MiB of conventional memory. However,
 * due to some shortcuts below it's restricted further to within the 1st 64KiB.
 * The AP starts in real-mode, with
 *   CS selector set to the startup memory address/16;
 *   CS base set to startup memory address;
 *   CS limit set to 64KiB;
 *   CPL and IP set to 0.
 * Parameters are passed to this code via a vector in low memory
 * indexed by the APIC number of the processor. The layout, size,
 * and location have to be kept in sync with the setup in sipi.s.
 */
#include "mem.h"
#include "amd64l.h"

/*
 * Some machine instructions not handled well by [68][al].
 * This is a messy piece of code, requiring instructions in real mode,
 * protected mode (+long mode on amd64). The MODE pseudo-op of 6[al] handles
 * the latter two OK, but 'MODE $16' is incomplete, e.g. it does
 * not truncate operands appropriately, hence the ugly 'rMOVAX' macro.
 * Fortunately, the only other instruction executed in real mode that
 * could cause a problem (ORL) is encoded such that it will work OK.
 */
#define	DELAY		BYTE $0xeb;		/* JMP .+2 */		\
			BYTE $0x00
#define NOP		BYTE $0x90		/* NOP */

#define pFARJMP32(s, o)	BYTE $0xea;		/* far jmp ptr32:16 */	\
			LONG $o; WORD $s

#define rFARJMP16(s, o)	BYTE $0xea;		/* far jump ptr16:16 */	\
			WORD $o; WORD $s;
#define rFARJMP32(s, o)	BYTE $0x66;		/* far jump ptr32:16 */	\
			pFARJMP32(s, o)
#define rLGDT(gdtptr)	BYTE $0x0f;		/* LGDT */		\
			BYTE $0x01; BYTE $0x16;				\
			WORD $gdtptr
#define rMOVAX(i)	BYTE $0xb8;		/* i -> AX */		\
			WORD $i;

/*
 * Real mode. Welcome to 1978.
 * Load a basic GDT, turn on protected mode and make
 * inter-segment jump to the protected mode code.
 */
MODE $16

TEXT _real<>(SB), 1, $-4
	rFARJMP16(0, _endofheader<>-KZERO(SB))	/*  */

_startofheader:
	NOP; NOP; NOP
	QUAD	$0xa5a5a5a5a5a5a5a5

TEXT _gdt32p<>(SB), 1, $-4
	QUAD	$0x0000000000000000		/* NULL descriptor */
	QUAD	$0x00cf9a000000ffff		/* CS */
	QUAD	$0x00cf92000000ffff		/* DS */
	QUAD	$0x0020980000000000		/* Long mode CS */

TEXT _gdtptr32p<>(SB), 1, $-4
	WORD	$(4*8-1)			/* includes long mode */
	LONG	$_gdt32p<>-KZERO(SB)

TEXT _gdt64<>(SB), 1, $-4
	QUAD	$0x0000000000000000		/* NULL descriptor */
	QUAD	$0x0020980000000000		/* CS */
	QUAD	$0x0000800000000000		/* DS */

TEXT _gdtptr64v<>(SB), 1, $-4
	WORD	$(3*8-1)
	QUAD	$_gdt64<>(SB)

TEXT _endofheader<>(SB), 1, $-4
	MOVW	CS, AX
	MOVW	AX, DS				/* initialise DS */

	rLGDT(_gdtptr32p<>-KZERO(SB))		/* load a basic gdt */

	MOVL	CR0, AX
	ORL	$Pe, AX
	MOVL	AX, CR0				/* turn on protected mode */
	DELAY					/* JMP .+2 */

	rMOVAX	(SSEL(SiDS, SsTIGDT|SsRPL0))	/*  */
	MOVW	AX, DS
	MOVW	AX, ES
	MOVW	AX, FS
	MOVW	AX, GS
	MOVW	AX, SS

	rFARJMP32(SSEL(SiCS, SsTIGDT|SsRPL0), _protected<>-KZERO(SB))

/*
 * Protected mode. Welcome to 1982.
 * Get the local APIC ID from the memory mapped APIC
 * and use it to locate the index to the parameter vector;
 * load the PDB with the page table address from the
 * information vector;
 * make an identity map for the inter-segment jump below,
 * using the stack space to hold a temporary PDP and PD;
 * enable and activate long mode;
 * make an inter-segment jump to the long mode code.
 */
MODE $32

/*
 * Macros for accessing page table entries; must turn
 * the C-style array-index macros into a page table byte
 * offset.
 */
#define PML4O(v)	((PTLX((v), 3))<<3)
#define PDPO(v)		((PTLX((v), 2))<<3)
#define PDO(v)		((PTLX((v), 1))<<3)
#define PTO(v)		((PTLX((v), 0))<<3)

TEXT _protected<>(SB), 1, $-4
	MOVL	$0xfee00000, BP			/* apicbase */
	MOVL	0x20(BP), BP			/* Id */
	SHRL	$24, BP				/* becomes RARG later */

	MOVL	BP, AX				/* apicno */
	IMULL	$32, AX				/* [apicno] */
	MOVL	$_real<>-KZERO(SB), BX
	ADDL	$4096, BX			/* sipi */
	ADDL	AX, BX				/* sipi[apicno] */

	MOVL	0(BX), SI			/* sipi[apicno].pml4 */
	
	MOVL	SI, AX
	MOVL	AX, CR3				/* load the mmu */

	MOVL	AX, DX
	SUBL	$MACHSTKSZ, DX			/* PDP for identity map */
	ADDL	$(PteRW|PteP), DX
	MOVL	DX, PML4O(0)(AX)		/* PML4E for identity map */

	SUBL	$MACHSTKSZ, AX			/* PDP for identity map */
	ADDL	$PTSZ, DX
	MOVL	DX, PDPO(0)(AX)			/* PDPE for identity map */
	MOVL	$(PtePS|PteRW|PteP), DX
	ADDL	$PTSZ, AX			/* PD for identity map */
	MOVL	DX, PDO(0)(AX)			/* PDE for identity 0-[24]MiB */


/*
 * Enable and activate Long Mode. From the manual:
 * 	make sure Page Size Extentions are off, and Page Global
 *	Extensions and Physical Address Extensions are on in CR4;
 *	set Long Mode Enable in the Extended Feature Enable MSR;
 *	set Paging Enable in CR0;
 *	make an inter-segment jump to the Long Mode code.
 * It's all in 32-bit mode until the jump is made.
 */
TEXT _lme<>(SB), 1, $-4
	MOVL	CR4, AX
	ANDL	$~Pse, AX			/* Page Size */
	ORL	$(Pge|Pae), AX			/* Page Global, Phys. Address */
	MOVL	AX, CR4

	MOVL	$Efer, CX			/* Extended Feature Enable */
	RDMSR
	ORL	$Lme, AX			/* Long Mode Enable */
	WRMSR

	MOVL	CR0, DX
	ANDL	$~(Cd|Nw|Ts|Mp), DX
	ORL	$(Pg|Wp), DX			/* Paging Enable */
	MOVL	DX, CR0

	pFARJMP32(SSEL(3, SsTIGDT|SsRPL0), _identity<>-KZERO(SB))

/*
 * Long mode. Welcome to 2003.
 * Jump out of the identity map space;
 * load a proper long mode GDT;
 * zap the identity map;
 * initialise the stack, RMACH, RUSER,
 * and call the C startup code.
 */
MODE $64

TEXT _identity<>(SB), 1, $-4
	MOVQ	$_start64v<>(SB), AX
	JMP*	AX

TEXT _start64v<>(SB), 1, $-4
	MOVQ	$_gdtptr64v<>(SB), AX
	MOVL	(AX), GDTR

	XORQ	DX, DX				/* DX is 0 from here on */
	MOVW	DX, DS				/* not used in long mode */
	MOVW	DX, ES				/* not used in long mode */
	MOVW	DX, FS
	MOVW	DX, GS
	MOVW	DX, SS				/* not used in long mode */

	MOVLQZX	SI, SI				/* sipi[apicno].pml4 */
	MOVQ	SI, AX
	ADDQ	$KZERO, AX			/* PML4 */
	MOVQ	DX, PML4O(0)(AX)		/* zap identity map */
	MOVQ	SI, CR3				/* flush TLB */

	ADDQ	$KZERO, BX			/* &sipi[apicno] */

	MOVQ	8(BX), SP			/* sipi[apicno].stack */

	PUSHQ	DX				/* clear flags */
	POPFQ
	MOVLQZX	RARG, RARG			/* APIC ID */
	PUSHQ	RARG				/* apicno */

	MOVQ	16(BX), RMACH			/* sipi[apicno].mach */
	MOVQ	DX, RUSER
	MOVQ	24(BX), AX			/* sipi[apicno].pc */
	CALL*	AX				/* (*sipi[apicno].pc)(apicno) */

_ndnr:
	JMP	_ndnr
