#include "mem.h"
#include "amd64l.h"

MODE $32

#define pFARJMP32(s, o)	BYTE $0xea;		/* far jump to ptr32:16 */\
			LONG $o; WORD $s

/*
 * Enter here in 32-bit protected mode. Welcome to 1982.
 * Make sure the GDT is set as it should be:
 *	disable interrupts;
 *	load the GDT with the table in _gdt32p;
 *	load all the data segments
 *	load the code segment via a far jump.
 */
TEXT _protected<>(SB), 1, $-4
	CLI
	BYTE $0xe9; LONG $0x00000058;		/* JMP _endofheader */

_startofheader:
	BYTE	$0x90				/* NOP */
	BYTE	$0x90				/* NOP */

TEXT _multibootheader<>(SB), 1, $-4		/* must be 4-byte aligned */
	LONG	$0x1badb002			/* magic */
	LONG	$0x00000003			/* flags */
	LONG	$-(0x1badb002 + 0x00000003)	/* checksum */

TEXT _gdt32p<>(SB), 1, $-4
	QUAD	$0x0000000000000000		/* NULL descriptor */
	QUAD	$0x00cf9a000000ffff		/* CS */
	QUAD	$0x00cf92000000ffff		/* DS */
	QUAD	$0x0020980000000000		/* Long mode CS */

TEXT _gdtptr32p<>(SB), 1, $-4
	WORD	$(4*8-1)
	LONG	$_gdt32p<>-KZERO(SB)

TEXT _gdt64<>(SB), 1, $-4
	QUAD	$0x0000000000000000		/* NULL descriptor */
	QUAD	$0x0020980000000000		/* CS */

TEXT _gdtptr64p<>(SB), 1, $-4
	WORD	$(2*8-1)
	QUAD	$_gdt64<>-KZERO(SB)

TEXT _gdtptr64v<>(SB), 1, $-4
	WORD	$(3*8-1)
	QUAD	$_gdt64<>(SB)

_endofheader:
	MOVL	AX, BP				/* possible passed-in magic */

	MOVL	$_gdtptr32p<>-KZERO(SB), AX
	MOVL	(AX), GDTR

	MOVL	$SSEL(SiDS, SsTIGDT|SsRPL0), AX
	MOVW	AX, DS
	MOVW	AX, ES
	MOVW	AX, FS
	MOVW	AX, GS
	MOVW	AX, SS

	pFARJMP32(SSEL(SiCS, SsTIGDT|SsRPL0), _warp64<>-KZERO(SB))

/*
 * Make the basic page tables for CPU0 to map 0-4MiB physical
 * to KZERO, and include an identity map for the switch from protected
 * to paging mode. There's an assumption here that the creation and later
 * removal of the identity map will not interfere with the KZERO mappings;
 * the conditions for clearing the identity map are
 *	clear PML4 entry when (KZERO & 0x0000ff8000000000) != 0;
 *	clear PDP entry when (KZERO & 0x0000007fc0000000) != 0;
 *	don't clear PD entry when (KZERO & 0x000000003fe00000) == 0;
 * the code below assumes these conditions are met.
 *
 * Assume a recent processor with Page Size Extensions
 * and use two 2MiB entries.
 */
/*
 * The layout is decribed in dat.h:
 *	_protected:	start of kernel text
 *	- 4*KiB		unused
 *	- 4*KiB		unused
 *	- 4*KiB		ptrpage
 *	- 4*KiB		syspage
 *	- MACHSZ	m
 *	- 4*KiB		vsvmpage for gdt, tss
 *	- PTSZ		PT for PMAPADDR		unused - assumes in KZERO PD
 *	- PTSZ		PD
 *	- PTSZ		PDP
 *	- PTSZ		PML4
 *	- MACHSTKSZ	stack
 */

/*
 * Macros for accessing page table entries; change the
 * C-style array-index macros into a page table byte offset
 */
#define PML4O(v)	((PTLX((v), 3))<<3)
#define PDPO(v)		((PTLX((v), 2))<<3)
#define PDO(v)		((PTLX((v), 1))<<3)
#define PTO(v)		((PTLX((v), 0))<<3)

TEXT _warp64<>(SB), 1, $-4
	MOVL	$_protected<>-(MACHSTKSZ+4*PTSZ+5*(4*KiB)+MACHSZ+KZERO)(SB), SI

	MOVL	SI, DI
	XORL	AX, AX
	MOVL	$((MACHSTKSZ+4*PTSZ+5*(4*KiB)+MACHSZ)>>2), CX

	CLD
	REP;	STOSL				/* stack, P*, vsvm, m, sys */

	MOVL	SI, AX				/* sys-KZERO */
	ADDL	$(MACHSTKSZ), AX		/* PML4 */
	MOVL	AX, CR3				/* load the mmu */
	MOVL	AX, DX
	ADDL	$(PTSZ|PteRW|PteP), DX		/* PDP at PML4 + PTSZ */
	MOVL	DX, PML4O(0)(AX)		/* PML4E for identity map */
	MOVL	DX, PML4O(KZERO)(AX)		/* PML4E for KZERO, PMAPADDR */

	ADDL	$PTSZ, AX			/* PDP at PML4 + PTSZ */
	ADDL	$PTSZ, DX			/* PD at PML4 + 2*PTSZ */
	MOVL	DX, PDPO(0)(AX)			/* PDPE for identity map */
	MOVL	DX, PDPO(KZERO)(AX)		/* PDPE for KZERO, PMAPADDR */

	ADDL	$PTSZ, AX			/* PD at PML4 + 2*PTSZ */
	MOVL	$(PtePS|PteRW|PteP), DX
	MOVL	DX, PDO(0)(AX)			/* PDE for identity 0-[24]MiB */
	MOVL	DX, PDO(KZERO)(AX)		/* PDE for KZERO 0-[24]MiB */
	ADDL	$PGLSZ(1), DX
	MOVL	DX, PDO(KZERO+PGLSZ(1))(AX)	/* PDE for KZERO [24]-[48]MiB */

	MOVL	AX, DX				/*  */
	ADDL	$(PTSZ|PteRW|PteP), DX		/* PT at PML4 + 3*PTSZ */
	MOVL	DX, PDO(PMAPADDR)(AX)		/* PDE for PMAPADDR */

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
 * load a proper long mode GDT.
 */
MODE $64

TEXT _identity<>(SB), 1, $-4
	MOVQ	$_start64v<>(SB), AX
	JMP*	AX

TEXT _start64v<>(SB), 1, $-4
	MOVQ	$_gdtptr64v<>(SB), AX
	MOVL	(AX), GDTR

	XORQ	DX, DX
	MOVW	DX, DS				/* not used in long mode */
	MOVW	DX, ES				/* not used in long mode */
	MOVW	DX, FS
	MOVW	DX, GS
	MOVW	DX, SS				/* not used in long mode */

	MOVLQZX	SI, SI				/* sys-KZERO */
	MOVQ	SI, AX
	ADDQ	$KZERO, AX
	MOVQ	AX, sys(SB)			/* sys */

	ADDQ	$(MACHSTKSZ), AX		/* PML4 and top of stack */
	MOVQ	AX, SP				/* set stack */

_zap0pml4:
	CMPQ	DX, $PML4O(KZERO)		/* KZERO & 0x0000ff8000000000 */
	JEQ	_zap0pdp
	MOVQ	DX, PML4O(0)(AX) 		/* zap identity map PML4E */
_zap0pdp:
	ADDQ	$PTSZ, AX			/* PDP at PML4 + PTSZ */
	CMPQ	DX, $PDPO(KZERO)		/* KZERO & 0x0000007fc0000000 */
	JEQ	_zap0pd
	MOVQ	DX, PDPO(0)(AX)			/* zap identity map PDPE */
_zap0pd:
	ADDQ	$PTSZ, AX			/* PD at PML4 + 2*PTSZ */
	CMPQ	DX, $PDO(KZERO)			/* KZERO & 0x000000003fe00000 */
	JEQ	_zap0done
	MOVQ	DX, PDO(0)(AX)			/* zap identity map PDE */
_zap0done:

	ADDQ	$(MACHSTKSZ), SI		/* PML4-KZERO */
	MOVQ	SI, CR3				/* flush TLB */

	ADDQ	$(2*PTSZ+4*KiB), AX		/* PD+PT+vsvm */
	MOVQ	AX, RMACH			/* Mach */
	MOVQ	DX, RUSER

	PUSHQ	DX				/* clear flags */
	POPFQ

	MOVLQZX	BX, BX				/* push multiboot args */
	PUSHQ	BX				/* multiboot info* */
	MOVLQZX	RARG, RARG
	PUSHQ	RARG				/* multiboot magic */

	CALL	main(SB)

TEXT ndnr(SB), 1, $-4				/* no deposit, no return */
_dnr:
	STI
	HLT
	JMP	_dnr				/* do not resuscitate */

