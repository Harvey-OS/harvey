#include "mem.h"
#include "amd64l.h"

.code32

#define pFARJMP32(s, o)	.byte $0xea;		/* far jump to ptr32:16 */\
			.long $o; .word $s

/* do we enter in 16-bit mode? If so, take the code from coreboot that goes from
 * 16->32
 */
/*
 * Enter here in 32-bit protected mode. Welcome to 1982.
 * Make sure the GDT is set as it should be:
 *	disable interrupts;
 *	load the GDT with the table in _gdt32p;
 *	load all the data segments
 *	load the code segment via a far jump.
 */
entry32: 
	cli
	/* save the IP. We will then use this to get the
	 * physical address of the gdt.
	 */
	movl	%ip, %bp
	/* when you execute this instruction, bp as the value
	 * of the ip at the start of this instruction.
	 * So add the length of this instruction and the
	 * 5 bytes of the jmp that follows it.
	 * It will then point to start of header.
	 */
	addl $7, %bp
	/* Now make it point to gdt32p (gdt, 32 bits, physical)
	 */
	addl $14, %bp
	.byte $0xe9; .long $0x00000058;		/* JMP _endofheader */

_startofheader:
	.byte	$0x90				/* NOP */
	.byte	$0x90				/* NOP */

TEXT _multibootheader<>(SB), 1, $-4		/* must be 4-byte aligned */
	.long	$0x1badb002			/* magic */
	.long	$0x00000003			/* flags */
	.long	$-(0x1badb002 + 0x00000003)	/* checksum */

TEXT _gdt32p<>(SB), 1, $-4
	.quad	$0x0000000000000000		/* NULL descriptor */
	.quad	$0x00cf9a000000ffff		/* CS */
	.quad	$0x00cf92000000ffff		/* DS */
	.quad	$0x0020980000000000		/* Long mode CS */

TEXT _gdtptr32p<>(SB), 1, $-4
	.word	$4*8-1
	.long	$_gdt32p<>-KZERO(SB)

TEXT _gdt64<>(SB), 1, $-4
	.quad	$0x0000000000000000		/* NULL descriptor */
	.quad	$0x0020980000000000		/* CS */

TEXT _gdtptr64p<>(SB), 1, $-4
	.word	$(2*8-1)
	.quad	$_gdt64<>-KZERO(SB)

TEXT _gdtptr64v<>(SB), 1, $-4
	.word	$(3*8-1)
	.quad	$_gdt64<>(SB)

_endofheader:
	movl	%eAX, %ebp				/* possible passed-in magic */

	movl	$_gdtptr32p<>-KZERO(SB), %eAX
	lgdt

	//movl	$SSEL(SiDS, SsTIGDT|SsRPL0), %AX
	movw	%ax, ds
	movw	%ax, es
	movw	%ax, fs
	movw	%ax, gs
	movw	%ax, ss

	pFARJMP32(SSEL(SiCS, SsTIGDT|SsRPL0), _warp64<>-KZERO(SB))

/*
 * Make the basic page tables for CPU0 to map 0-4MiB physical
 * to KZERO, and include an identity map for the switch from protected
 * to paging mode. There's an assumption here that the creation and later
 * removal of the identity map will not interfere with the KZERO mappings;
 * the conditions for clearing the identity map are
 *	clear PML4 entry when (KZER0 & 0x0000ff8000000000) != 0;
 *	clear PDP entry when (KZER0 & 0x0000007fc0000000) != 0;
 *	don't clear PD entry when (KZER0 & 0x000000003fe00000) == 0;
 * the code below assumes these conditions are met.
 *
 * Assume a recent processor with Page Size Extensions
 * and use two 2MiB entries.
 */
/*
 * The layout is decribed in data.h:
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
	movl	$_protected<>-(MACHSTKSZ+4*PTSZ+5*(4*KiB)+MACHSZ+KZERO)(SB), SI

	movl	%esi, %edi
	XORL	%eax, %eax
	movl	$((MACHSTKSZ+4*PTSZ+5*(4*KiB)+MACHSZ)>>2), CX

	cld
	rep;	stosl				/* stack, P*, vsvm, m, sys */

	movl	SI, %AX				/* sys-KZERO */
	ADDL	$(MACHSTKSZ), %AX		/* PML4 */
	movl	%AX, CR3				/* load the mmu */
	movl	%AX, DX
	ADDL	$(PTSZ|PteRW|PteP), DX		/* PDP at PML4 + PTSZ */
	movl	DX, PML4O(0)(%AX)		/* PML4E for identity map */
	movl	DX, PML4O(KZERO)(%AX)		/* PML4E for KZERO, PMAPADDR */

	ADDL	$PTSZ, %AX			/* PDP at PML4 + PTSZ */
	ADDL	$PTSZ, DX			/* PD at PML4 + 2*PTSZ */
	movl	DX, PDPO(0)(%AX)			/* PDPE for identity map */
	movl	DX, PDPO(KZERO)(%AX)		/* PDPE for KZERO, PMAPADDR */

	ADDL	$PTSZ, %AX			/* PD at PML4 + 2*PTSZ */
	movl	$(PtePS|PteRW|PteP), DX
	movl	DX, PDO(0)(%AX)			/* PDE for identity 0-[24]MiB */
	movl	DX, PDO(KZERO)(%AX)		/* PDE for KZERO 0-[24]MiB */
	ADDL	$PGLSZ(1), DX
	movl	DX, PDO(KZERO+PGLSZ(1))(%AX)	/* PDE for KZERO [24]-[48]MiB */

	movl	%AX, DX				/* PD at PML4 + 2*PTSZ */
	ADDL	$(PTSZ|PteRW|PteP), DX		/* PT at PML4 + 3*PTSZ */
	movl	DX, PDO(PMAPADDR)(%AX)		/* PDE for PMAPADDR */

/*
 * Enable and activate Long Mode. From the manual:
 * 	make sure Page Size Extentions are off, and Page Global
 *	Extensions and Physical Address Extensions are on in CR4;
 *	set Long Mode Enable in the Extended Feature Enable MSR;
 *	set Paging Enable in CR0;
 *	make an inter-segment jump to the Long Mode code.
 * It's all in 32-bit mode until the jump is made.
 */
lme:
	movl	CR4, %AX
	ANDL	$~Pse, %AX			/* Page Size */
	ORL	$(Pge|Pae), %AX			/* Page Global, Phys. Address */
	movl	%AX, CR4

	movl	$Efer, CX			/* Extended Feature Enable */
	RDMSR
	ORL	$Lme, %AX			/* Long Mode Enable */
	WRMSR

	movl	CR0, DX
	ANDL	$~(Cd|Nw|Ts|Mp), DX
	ORL	$(Pg|Wp), DX			/* Paging Enable */
	movl	DX, CR0

	pFARJMP32(SSEL(3, SsTIGDT|SsRPL0), _identity<>-KZERO(SB))

/*
 * Long mode. Welcome to 2003.
 * Jump out of the identity map space;
 * load a proper long mode GDT.
 */
.code64

_identity:
	movq	$_start64v<>(SB), %AX
	JMP*	%AX

_start64v:
	movq	$_gdtptr64v<>(SB), %AX
	movl	(%AX), GDTR

	XORQ	DX, DX
	movw	DX, DS				/* not used in long mode */
	movw	DX, ES				/* not used in long mode */
	movw	DX, FS
	movw	DX, GS
	movw	DX, SS				/* not used in long mode */

	movlQZX	SI, SI				/* sys-KZERO */
	movq	SI, %AX
	addq	$KZERO, %AX
	movq	%AX, sys(SB)			/* sys */

	addq	$(MACHSTKSZ), %AX		/* PML4 and top of stack */
	movq	%AX, SP				/* set stack */

_zap0pml4:
	cmpq	DX, $PML4O(KZERO)		/* KZER0 & 0x0000ff8000000000 */
	JEQ	_zap0pdp
	movq	DX, PML4O(0)(%AX) 		/* zap identity map PML4E */
_zap0pdp:
	addq	$PTSZ, %AX			/* PDP at PML4 + PTSZ */
	cmpq	DX, $PDPO(KZERO)		/* KZER0 & 0x0000007fc0000000 */
	JEQ	_zap0pd
	movq	DX, PDPO(0)(%AX)			/* zap identity map PDPE */
_zap0pd:
	addq	$PTSZ, %AX			/* PD at PML4 + 2*PTSZ */
	cmpq	DX, $PDO(KZERO)			/* KZER0 & 0x000000003fe00000 */
	JEQ	_zap0done
	movq	DX, PDO(0)(%AX)			/* zap identity map PDE */
_zap0done:

	addq	$(MACHSTKSZ), SI		/* PML4-KZERO */
	movq	SI, CR3				/* flush TLB */

	addq	$(2*PTSZ+4*KiB), %AX		/* PD+PT+vsvm */
	movq	%AX, RMACH			/* Mach */
	movq	DX, RUSER

	PUSHQ	DX				/* clear flags */
	POPFQ

	movlQZX	BX, BX				/* push multiboot args */
	PUSHQ	BX				/* multiboot info* */
	movlQZX	RARG, RARG
	PUSHQ	RARG				/* multiboot magic */

	CALL	main(SB)

TEXT ndnr(SB), 1, $-4				/* no deposit, no return */
_dnr:
	STI
	HLT
	JMP	_dnr				/* do not resuscitate */

