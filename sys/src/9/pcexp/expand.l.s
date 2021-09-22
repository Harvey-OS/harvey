/*
 * Machine start up and assist for bootstrap loader decompressor.
 * Starts at 0x10000 (where pbs puts it) or 0x7c00 (where pxe puts it).
 */
#include "mem.h"

/* (mostly) real-mode instructions */
#include "/sys/src/boot/pc/x16.h"

#define CMPL(r0, r1)	OPSIZE; CMP(r0, r1)
#define LLI(i, rX)	OPSIZE;	BYTE $(0xB8+rX); LONG $i	/* i -> rX */	

#define CPUID		BYTE $0x0F; BYTE $0xA2	/* CPUID, argument in AX */
#define WBINVD		BYTE $0x0F; BYTE $0x09

#define GDTPTR loadgdtptr

/*
 * This assumes that we are in real address mode, i.e.,
 * that we look like an 8086 (i.e., it's 1978).
 */
TEXT _start16r(SB), $0
	CLI				/* turn off interrupts */
	OPSIZE; XORL DI, DI
	JEQ	pastaligned		/* skip data & prevent 8l code motion */
TEXT _align(SB), $0
	/* align gdt on 16-byte multiple; some cpus care. */
	HLT; HLT; HLT; HLT
	HLT; HLT; HLT; HLT
	HLT; HLT
/*
 *  gdt to get us to 32-bit/segmented/unpaged mode.
 *  descriptors are for 4 gigabytes (PL 0).
 *  0xFFFF is seg limit low bits; high bits are low base-address bits.
 *  0xF<<16 is seg limit high bits.  units are pages.
 */
TEXT	loadgdt(SB),$0
	LONG $0; LONG $0		/* null descriptor */
	LONG $0xFFFF; LONG $(SEGG|SEGB|(0xF<<16)|SEGP|SEGPL(0)|SEGDATA|SEGW)
	LONG $0xFFFF; LONG $(SEGG|SEGD|(0xF<<16)|SEGP|SEGPL(0)|SEGEXEC|SEGR)
	/* exec segment 16-bit */
	LONG $0xFFFF; LONG $(SEGG|     (0xF<<16)|SEGP|SEGPL(0)|SEGEXEC|SEGR)

TEXT	loadgdtptr(SB),$0
	WORD	$(4*2*BY2WD-1)		/* gdt is 4 pairs of longs */
	LONG	$loadgdt(SB)

pastaligned:
/*
 * Start in 16-bit real mode:
 *	disable interrupts;
 *	set segment selectors;
 *	create temporary stack,
 *	try to enable A20 address line.
 *
 * Do any real-mode BIOS calls before going to protected mode.
 * APM data and the E820 memory map will be stored at BIOSTABLES,
 * not ROUND(end, BY2PG) as before.
 *
 * Install a basic GDT, enable 32-bit protected mode,
 * and set 32-bit segment selectors for new GDT, call _main.
 */
	CLI				/* turn interrupts off */
	CLD				/* string ops increment SI or DI */

	/*
	 * if cpu is in protected mode, bios INT calls won't work, so
	 * assume that this code has already been executed in real mode
	 * or we've been started from UEFI.
	 */
	MFCR(rCR0, rAX)
	ANDI(PE, rAX)
	/*
	 * conditional jumps only take byte displacements, so for targets
	 * farther away, we must use an unconditional long jump.
	 */
	JEQ	notprotected
	JMP	protected
notprotected:
	/*
	 * Make sure the segments are reasonable.
	 * If we were started directly from the BIOS
	 * (i.e. no MS-DOS) then DS may not be right.
	 *
	 * we were loaded by pbs or by pxe but not expanded: in 16-bit mode.
	 * the plan 9 pbs will have set CS, DS and DI to 0x1000 (load addr/16).
	 * PXE spec requires CS to be 0 here.
	 */
	MOVW	CS, AX
	MTSR(rAX, rDS)
	MTSR(rAX, rES)
	MTSR(rAX, rFS)
	MTSR(rAX, rGS)
	DELAY				/* flush prefetch queue */

/*
 * finish initialisation: set up stack, try to enable A20 via bios call.
 */
	LLI(0, rAX)			/* always put stack in first 64k */
	MTSR(rAX, rSS)
	DELAY				/* JMP .+2; flush prefetch queue */
	LWI(_start16r-BY2WD(SB), rSP)	/* set stack pointer */
	MW(rSP, rBP)			/* set the indexed-data pointer */

	/* we access misaligned data in bda, so clear Ac, at least */
	OPSIZE; XORL	AX, AX
	OPSIZE; PUSHL	AX
	OPSIZE; POPFL			/* set EFLAGS */

	/* prevent NMIs */
	LWI(NVRADDR, rDX)
	CLR(rAX)
	INB				/* read port NVRADDR into AX */
	ORI(0x80, rAX)			/* set no-NMI bit */
	LWI(NVRADDR, rDX)
	OUTB				/* write port NVRADDR from AX */

#include "bios.s"

/*
 * Done with BIOS calls for now.
 *
 * Load a basic GDT to map 4GB, turn on the protected mode bit in CR0,
 * set all the segments to point to the new GDT.
 */
	LGDT(GDTPTR(SB))		/* load a basic gdt */
	DELAY				/* flush prefetch queue */

	/* prevent NMIs */
	LWI(NVRADDR, rDX)
	CLR(rAX)
	INB				/* read port NVRADDR into AX */
	ORI(0x80, rAX)			/* set no-NMI bit */
	LWI(NVRADDR, rDX)
	OUTB				/* write port NVRADDR from AX */

	WBINVD
	OPSIZE; XORL AX, AX
	OPSIZE; MOVL AX, CR4
	OPSIZE; MOVL $PE, AX		/* NB: CD|NW are zero, so caches on */
/**/	OPSIZE; MOVL AX, CR0  		/* turn on protected mode */
	/*
	 * we are now in 32-bit protected mode (i.e., 1985).
	 * intel says we must immediately perform a far jump or call.
	 */
	FARJUMP32(KESEL, mode32bit(SB))

TEXT	mode32bit(SB),$0
	CLI
	WBINVD
/*
 *	set all segment selectors except cs, which was set by far jump.
 */
 	MOVW	$KDSEL, CX
 	MOVW	CX, DS
 	MOVW	CX, ES
 	MOVW	CX, FS
 	MOVW	CX, GS
 	MOVW	CX, SS
 	DELAY
	XORL	DX, DX			/* signal "not EUFI" to _start32p */
protected:
	/*
	 * although we're in 32-bit protected mode, the A20 line may still
	 * not be enabled, so move the stack pointer to an even MB so that
	 * we can try to enable A20 in C.
	 */
	MOVL	$(MemMin-MB-BY2WD), SP
	CALL	_main(SB)	/* jump to C in expand.c, never to return */

	CLI
loop:
	HLT
	JMP	loop

TEXT _hello(SB), $0
	BYTE $'.'; BYTE $'.'; BYTE $'.'; BYTE $'\z'

/* offset into bios tables using segment ES.  stos? use (es,di) */
TEXT _DI(SB), $0
	LONG	$0
/* segment address of bios tables (BIOSTABLES >> 4) */
TEXT _ES(SB), $0
	LONG	$0

/*
 * C-callable machine primitives
 */

/*
 *  output a byte
 */
TEXT	outb(SB),$0
	MOVL	p+0(FP),DX
	MOVL	b+4(FP),AX
	OUTB
	RET

/*
 *  input a byte
 */
TEXT	inb(SB),$0
	MOVL	p+0(FP),DX
	XORL	AX,AX
	INB
	RET

TEXT	splhi(SB),$0
	PUSHFL
	POPL	AX
	CLI
	RET

TEXT	mb586(SB), $0
	XORL	AX, AX
	CPUID
	RET

TEXT	wbinvd(SB), $0
	WBINVD
	RET

/* arg is entry point */
TEXT 	enternextkernel(SB), $0
	MOVL	entry+0(FP), DX
	MOVL	multibootheader(SB), BX		/* multiboot data pointer */
	/* BX should now contain 0x500 (MBOOTTAB-KZERO) */
	MOVL	$MBOOTREGMAG, AX		/* multiboot magic */
	WBINVD
	JMP*	DX				/* into the loaded kernel */
idle:
	HLT
	JMP	idle
