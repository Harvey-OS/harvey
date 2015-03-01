#include "mem.h"
#include "amd64l.h"
#ifndef __ASSEMBLER__
#define __ASSEMBLER__
#endif

// It gets REALLY ugly to try  to link this at some low address and then have the rest of the
// kernel linked high. Really, really ugly. And that defines any attempt to load at a randome
// address. So, you have to learn to write position independent code here. 
// It will make you stronger. Assuming you survive the training.
.code32

#define pFARJMP32(s, o)	.byte 0xea;	.long o; .word s	/* far jump to ptr32:16 */
			

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
#define MULTIBOOT_PAGE_ALIGN  (1<<0)
#define MULTIBOOT_MEMORY_INFO (1<<1)
#define MULTIBOOT_HEADER_MAGIC (0x1BADB002)
#define MULTIBOOT_HEADER_FLAGS (MULTIBOOT_MEMORY_INFO | MULTIBOOT_PAGE_ALIGN)
#define CHECKSUM (-(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS))

# The kernel bootstrap (this code) is linked and loaded at physical address
# 0x00100000 (1MB), which is the start of extended memory.  (See kernel.ld)

# Flagging boottext to be text.  Check out:
# http://sourceware.org/binutils/docs/as/Section.html
.section .boottext, "awx"

.code32
.align 4
_protected:
multiboot_header:
.long MULTIBOOT_HEADER_MAGIC
.long MULTIBOOT_HEADER_FLAGS
.long CHECKSUM

	.globl _start
_start:
	cli
	jmp 1f


	/* This is the GDT for the ROM stage part of coreboot. It
	 * is different from the RAM stage GDT which is defined in
	 * c_start.S
	 */

	.align	4
.globl gdtptr
gdt:
gdtptr:
	.word	gdt_end - gdt -1 /* compute the table limit */
	.long	gdt		 /* we know the offset */
	.word	0

	/* selgdt 0x08, flat code segment */
	.word	0xffff, 0x0000
	.byte	0x00, 0x9b, 0xcf, 0x00 /* G=1 and 0x0f, So we get 4Gbytes for limit */

	/* selgdt 0x10,flat data segment */
	.word	0xffff, 0x0000
	.byte	0x00, 0x93, 0xcf, 0x00

	/* long mode code segment. */
	.quad	0x0020980000000000		/* Long mode CS */

gdt_end:


/*
 *	When we come here we are in protected mode. We expand
 *	the stack and copies the data segment from ROM to the
 *	memory.
 *
 *	After that, we call the chipset bootstrap routine that
 *	does what is left of the chipset initialization.
 *
 *	NOTE aligned to 4 so that we are sure that the prefetch
 *	cache will be reloaded.
 */
	.align	4
1: 
//	jmp 1b
.globl protected_start
protected_start:

	lgdt	%cs:gdtptr
	ljmp	$8, $__protected_start

__protected_start:
	/* Save the BIST value */
	movl	%eax, %ebp
	movw	$0x10, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %ss
	movw	%ax, %fs
	movw	%ax, %gs

	/* Restore the BIST value to %eax */
	movl	%ebp, %eax
	
entry32:
1:
	movb	$0x30, %al
	movw $0x30, %dx
	outb %dx
// This gets us into a reasonable mode. We can skip the plan 9 gdt code.
	call 1f
1:
	popl	%ebp
	/* when you execute this instruction, bp has the value
	 * of 1f.
	 * So add the length of this instruction and the
	 * 5 bytes of the jmp that follows it.
	 * It will then point to start of header.
	 */
	addl $12, %ebp
	/* Now make it point to gdt32p (gdt, 32 bits, physical)
	 */
	addl $14, %ebp
	JMP _endofheader

_startofheader:
	.byte	0x90				/* NOP */
	.byte	0x90				/* NOP */

_multibootheader:	/* must be 4-byte aligned */
	.long	0x1badb002			/* magic */
	.long	0x00000003			/* flags */
	.long	-(0x1badb002 + 0x00000003)	/* checksum */

_gdt32p:	
	.quad	0x0000000000000000		/* NULL descriptor */
	.quad	0x00cf9a000000ffff		/* CS */
	.quad	0x00cf92000000ffff		/* DS */
	.quad	0x0020980000000000		/* Long mode CS */

_gdtptr32p:	
	.word	4*8-1
	.long	0 // _gdt32p

_gdt64:	
	.quad	0x0000000000000000		/* NULL descriptor */
	.quad	0x0020980000000000		/* CS */

_gdtptr64p:	
	.word	2*8-1
	.quad	0 // _gdt64

_gdtptr64v:	
	.word	3*8-1
	.quad	0 // _gdt64

_endofheader:
	pushl %eax				/* possible passed-in magic */

/*
 * Make the basic page tables for CPU0 to map 0-4MiB physical
 * to KZERO, and include an identity map for the switch from protected
 * to paging mode. There`s an assumption here that the creation and later
 * removal of the identity map will not interfere with the KZERO mappings;
 * the conditions for clearing the identity map are
 *	clear PML4 entry when (KZER0 & 0x0000ff8000000000) != 0;
 *	clear PDP entry when (KZER0 & 0x0000007fc0000000) != 0;
 *	don`t clear PD entry when (KZER0 & 0x000000003fe00000) == 0;
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
	movl	$_protected-(MACHSTKSZ+4*PTSZ+5*(4*KiB)+MACHSZ/*+KZERO*/), %esi

	movl	%esi, %edi
	xorl	%eax, %eax
	movl	$((MACHSTKSZ+4*PTSZ+5*(4*KiB)+MACHSZ)>>2), %ecx

	cld
	rep;	stosl				/* stack, P*, vsvm, m, sys */

	movl	%esi, %eax				/* sys-KZERO */
	addl	$(MACHSTKSZ), %eax		/* PML4 */
	movl	%eax, %CR3				/* load the mmu */
	movl	%eax, %edx
	addl	$(PTSZ|PteRW|PteP), %edx		/* PDP at PML4 + PTSZ */
	movl	%edx, PML4O(0)(%eax)		/* PML4E for identity map */
	movl	%edx, PML4O(KZERO)(%eax)		/* PML4E for KZERO, PMAPADDR */

	addl	$PTSZ, %eax			/* PDP at PML4 + PTSZ */
	addl	$PTSZ, %edx			/* PD at PML4 + 2*PTSZ */
	movl	%edx, PDPO(0)(%eax)			/* PDPE for identity map */
	movl	%edx, PDPO(KZERO)(%eax)		/* PDPE for KZERO, PMAPADDR */

	addl	$PTSZ, %eax			/* PD at PML4 + 2*PTSZ */
	movl	$(PtePS|PteRW|PteP), %edx
	movl	%edx, PDO(0)(%eax)			/* PDE for identity 0-[24]MiB */
	movl	%edx, PDO(KZERO)(%eax)		/* PDE for KZERO 0-[24]MiB */
	addl	$PGLSZ(1), %edx
	movl	%edx, PDO(KZERO+PGLSZ(1))(%eax)	/* PDE for KZERO [24]-[48]MiB */

	movl	%eax, %edx				/* PD at PML4 + 2*PTSZ */
	addl	$(PTSZ|PteRW|PteP), %edx		/* PT at PML4 + 3*PTSZ */
	movl	%edx, PDO(PMAPADDR)(%eax)		/* PDE for PMAPADDR */
1: 
	jmp 1b
// CODE IS KNOWN GOOD TO THIS POINT.

/*
 * Enable and activate Long Mode. From the manual:
 * 	make sure Page Size Extentions are off, and Page Global
 *	Extensions and Physical Address Extensions are on in CR4;
 *	set Long Mode Enable in the Extended Feature Enable MSR;
 *	set Paging Enable in CR0;
 *	make an inter-segment jump to the Long Mode code.
 * It`s all in 32-bit mode until the jump is made.
 */
lme:
	movl	%cr4, %eax
	ANDL	$~Pse, %eax			/* Page Size */
//	ORL	$(Pge|Pae), %eax			/* Page Global, Phys. Address */
	movl	%eax, %cr4

//	movl	$Efer, %ecx			/* Extended Feature Enable */
	RDMSR
//	ORL	$Lme, %eax			/* Long Mode Enable */
	WRMSR

	movl	%cr0, %edx
	//ANDL	$~(Cd|Nw|Ts|Mp), %edx
	//ORL	$(Pg|Wp), %edx			/* Paging Enable */
	movl	%edx, %cr0

	//pFARJMP32(SSEL(3, SsTIGDT|SsRPL0), _identity-KZERO)

/*
 * Long mode. Welcome to 2003.
 * Jump out of the identity map space;
 * load a proper long mode GDT.
 */
.code64

_identity:
	//movq	$_start64v, %rax
	JMP	*%rax

// At this point, we are safe to use kernel addresses, as we are in
// kernel virtual address space.
_start64v:
	movq	$_gdtptr64v, %rax
	lgdt	(%eax)
//	movl	0(%eax), GDTR

	XORQ	%rdx, %rdx
	movw	%dx, %ds				/* not used in long mode */
	movw	%dx, %es				/* not used in long mode */
	movw	%dx, %fs
	movw	%dx, %gs
	movw	%dx, %ss				/* not used in long mode */

	movq	%rsi, %rsi				/* sys-KZERO */
	movq	%rsi, %rax
	addq	$KZERO, %rax
	movq	%rax, sys			/* sys */

	addq	$(MACHSTKSZ), %rax		/* PML4 and top of stack */
	movq	%rax, %rsp				/* set stack */

_zap0pml4:
	cmpq	$PML4O(KZERO), %rdx		/* KZER0 & 0x0000ff8000000000 */
	JE	_zap0pdp
	movq	%rdx, PML4O(0)(%rax) 		/* zap identity map PML4E */
_zap0pdp:
	addq	$PTSZ, %rax			/* PDP at PML4 + PTSZ */
	cmpq	$PDPO(KZERO), %rdx 		/* KZER0 & 0x0000007fc0000000 */
	JE	_zap0pd
	movq	%rdx, PDPO(0)(%rax)			/* zap identity map PDPE */
_zap0pd:
	addq	$PTSZ, %rax			/* PD at PML4 + 2*PTSZ */
	cmpq	$PDO(KZERO), %rdx			/* KZER0 & 0x000000003fe00000 */
	JE	_zap0done
	movq	%rdx, PDO(0)(%rax)			/* zap identity map PDE */
_zap0done:

	addq	$(MACHSTKSZ), %rsi		/* PML4-KZERO */
	movq	%rsi, %CR3				/* flush TLB */

	addq	$(2*PTSZ+4*KiB), %rax		/* PD+PT+vsvm */
	movq	%rax, %r15 # we thinkgRMACH			/* Mach */
	movq	%rdx, %r13 # we think RUSER

	PUSHQ	%rdx				/* clear flags */
	POPFQ

	movq	%rbx, %rbx				/* push multiboot args */
	PUSHQ	%rbx				/* multiboot info* */
	movq	%rax, %rax
	pushq	%rax				/* multiboot magic */

	CALL	main

.globl ndnr
ndnr:	/* no deposit, no return */
	/* do not resuscitate */
_dnr:
	sti
	hlt
	JMP	_dnr				/* do not resuscitate */

