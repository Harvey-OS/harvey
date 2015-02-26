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
	// WTF is it called again?
	//movl	%ip, %ebp
	/* when you execute this instruction, bp as the value
	 * of the ip at the start of this instruction.
	 * So add the length of this instruction and the
	 * 5 bytes of the jmp that follows it.
	 * It will then point to start of header.
	 */
	addl $7, %ebp
	/* Now make it point to gdt32p (gdt, 32 bits, physical)
	 */
	addl $14, %ebp
	.byte $0xe9; .long $0x00000058;		/* JMP _endofheader */

_startofheader:
	.byte	$0x90				/* NOP */
	.byte	$0x90				/* NOP */

_multibootheader:	/* must be 4-byte aligned */
	.long	$0x1badb002			/* magic */
	.long	$0x00000003			/* flags */
	.long	$-(0x1badb002 + 0x00000003)	/* checksum */

gdt32p:	
	.quad	$0x0000000000000000		/* NULL descriptor */
	.quad	$0x00cf9a000000ffff		/* CS */
	.quad	$0x00cf92000000ffff		/* DS */
	.quad	$0x0020980000000000		/* Long mode CS */

_gdtptr32p:	
	.word	31 #$4*8-1
	.long	$_gdt32p

_gdt64:	
	.quad	$0x0000000000000000		/* NULL descriptor */
	.quad	$0x0020980000000000		/* CS */

_gdtptr64p:	
	.word	15 # $2*8-1
	.quad	$_gdt64

_gdtptr64v:	
	.word	23 # $3*8-1
	.quad	$_gdt64

_endofheader:
	movl	%eAX, %ebp				/* possible passed-in magic */

	movl	$_gdtptr32p, %eax
//	lgdt

	//movl	$SSEL(SiDS, SsTIGDT|SsRPL0), %AX
	movw	%ax, ds
	movw	%ax, es
	movw	%ax, fs
	movw	%ax, gs
	movw	%ax, ss

//	pFARJMP32(SSEL(SiCS, SsTIGDT|SsRPL0), _warp64<>-KZERO(SB))

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

_warp64:	
//	movl	$_protected<>-(MACHSTKSZ+4*PTSZ+5*(4*KiB)+MACHSZ+KZERO)(SB), SI

	movl	%esi, %edi
	xorl	%eax, %eax
//	movl	$((MACHSTKSZ+4*PTSZ+5*(4*KiB)+MACHSZ)>>2), CX

	cld
	rep;	stosl				/* stack, P*, vsvm, m, sys */

	movl	%esi, %eax				/* sys-KZERO */
	addl	$(MACHSTKSZ), %eax		/* PML4 */
	movl	%eAX, %CR3				/* load the mmu */
	movl	%eAX, %edx
//	addl	$(PTSZ|PteRW|PteP), %edx		/* PDP at PML4 + PTSZ */
//	movl	%edx, PML4O(0)(%AX)		/* PML4E for identity map */
//	movl	%edx, PML4O(KZERO)(%AX)		/* PML4E for KZERO, PMAPADDR */

	addl	$PTSZ, %eax			/* PDP at PML4 + PTSZ */
	addl	$PTSZ, %edx			/* PD at PML4 + 2*PTSZ */
//	movl	%edx, PDPO(0)(%eax)			/* PDPE for identity map */
//	movl	%edx, PDPO(KZERO)(%eax)		/* PDPE for KZERO, PMAPADDR */

	addl	$PTSZ, %eax			/* PD at PML4 + 2*PTSZ */
//	movl	$(PtePS|PteRW|PteP), %edx
//	movl	%edx, PDO(0)(%eax)			/* PDE for identity 0-[24]MiB */
//	movl	%edx, PDO(KZERO)(%eax)		/* PDE for KZERO 0-[24]MiB */
//	addl	$PGLSZ(1), %edx
/	movl	%edx, PDO(KZERO+PGLSZ(1))(%eax)	/* PDE for KZERO [24]-[48]MiB */

	movl	%eax, %edx				/* PD at PML4 + 2*PTSZ */
//	addl	$(PTSZ|PteRW|PteP), %edx		/* PT at PML4 + 3*PTSZ */
//	movl	%edx, PDO(PMAPADDR)(%eax)		/* PDE for PMAPADDR */

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
	movl	%cr4, %eax
//	ANDL	$~Pse, %eax			/* Page Size */
//	ORL	$(Pge|Pae), %eax			/* Page Global, Phys. Address */
	movl	%eax, %cr4

	movl	$Efer, %ecx			/* Extended Feature Enable */
	RDMSR
	ORL	$Lme, %eax			/* Long Mode Enable */
	WRMSR

	movl	CR0, %edx
//	ANDL	$~(Cd|Nw|Ts|Mp), %edx
//	ORL	$(Pg|Wp), %edx			/* Paging Enable */
	movl	%edx, %cr0

//	pFARJMP32(SSEL(3, SsTIGDT|SsRPL0), _identity<>-KZERO(SB))

/*
 * Long mode. Welcome to 2003.
 * Jump out of the identity map space;
 * load a proper long mode GDT.
 */
.code64

_identity:
	movq	$_start64v, %rax
	JMP	*%rax

_start64v:
	movq	$_gdtptr64v, %rax
//	movl	0(%eax), GDTR

	XORQ	%rdx, %rdx
	movw	%dx, %ds				/* not used in long mode */
	movw	%dx, %es				/* not used in long mode */
	movw	%dx, %fs
	movw	%dx, %gs
	movw	%dx, %ss				/* not used in long mode */

//	movlQZX	%rsi, %rsi				/* sys-KZERO */
	movq	%rsi, %rax
	addq	$KZERO, %rax
	movq	%rax, sys			/* sys */

	addq	$(MACHSTKSZ), %rax		/* PML4 and top of stack */
	movq	%rax, %rsp				/* set stack */

_zap0pml4:
//	cmpq	%rdx, $PML4O(KZERO)		/* KZER0 & 0x0000ff8000000000 */
	JE	_zap0pdp
//	movq	%rdx, PML4O(0)(%rax) 		/* zap identity map PML4E */
_zap0pdp:
	addq	$PTSZ, %rax			/* PDP at PML4 + PTSZ */
//	cmpq	%rdx, $PDPO(KZERO)		/* KZER0 & 0x0000007fc0000000 */
	JE	_zap0pd
//	movq	%rdx, PDPO(0)(%AX)			/* zap identity map PDPE */
_zap0pd:
	addq	$PTSZ, %rax			/* PD at PML4 + 2*PTSZ */
//	cmpq	%rdx, $PDO(KZERO)			/* KZER0 & 0x000000003fe00000 */
	JE	_zap0done
//	movq	%rdx, PDO(0)(%rax)			/* zap identity map PDE */
_zap0done:

	addq	$(MACHSTKSZ), %rsi		/* PML4-KZERO */
	movq	%rsi, %CR3				/* flush TLB */

//	addq	$(2*PTSZ+4*KiB), %rax		/* PD+PT+vsvm */
	movq	%rax, %r15 # we thinkgRMACH			/* Mach */
	movq	%rdx, %r13 # we think RUSER

	PUSHQ	%rdx				/* clear flags */
	POPFQ

//	movlQZX	%rbx, %rbx				/* push multiboot args */
	PUSHQ	%rbx				/* multiboot info* */
//	movlQZX	%r3, %r3
//	pushq	%rarg				/* multiboot magic */

	CALL	main

ndnr:	/* no deposit, no return */
	/* do not resuscitate */
_dnr:
	sti
	hlt
	JMP	_dnr				/* do not resuscitate */

