#include "mem.h"
#include "amd64l.h"

/* instructions not known to assembler */
#define MONITOR	BYTE $0x0f; BYTE $0x01; BYTE $0xc8
#define MWAIT	BYTE $0x0f; BYTE $0x01; BYTE $0xc9
#define PAUSE	BYTE $0xf3; BYTE $0x90

MODE $64

/*
 * Port I/O.  Welcome to 1969 (the 4004), at the latest; probably actually
 * to 1948.  In 1970, the PDP-11 provided memory-mapped I/O registers, even
 * for DMA.  Intel was slow on the uptake, unsurprisingly.
 *
 * We don't use the string variants of in & out, other than a few by sdata.c.
 */

TEXT inb(SB), 1, $-4
	MOVL	RARG, DX			/* MOVL	port+0(FP), DX */
	XORL	AX, AX
	INB
	RET

TEXT ins(SB), 1, $-4
	MOVL	RARG, DX
	XORL	AX, AX
	INW
	RET

TEXT inl(SB), 1, $-4
	MOVL	RARG, DX
	INL
	RET

TEXT outb(SB), 1, $-1
	MOVL	RARG, DX			/* MOVL	port+0(FP), DX */
	MOVL	byte+8(FP), AX
	OUTB
	RET

TEXT outs(SB), 1, $-4
	MOVL	RARG, DX
	MOVL	short+8(FP), AX
	OUTW
	RET

TEXT outl(SB), 1, $-4
	MOVL	RARG, DX
	MOVL	long+8(FP), AX
	OUTL
	RET

/*
 * only sdata.c uses these string variants.
 */
#ifdef SDATA
TEXT inss(SB), 1, $-4
	MOVL	RARG, DX
	MOVQ	address+8(FP), DI
	MOVL	count+16(FP), CX
	CLD
	REP;	INSW
	RET

TEXT outss(SB), 1, $-4
	MOVL	RARG, DX
	MOVQ	address+8(FP), SI
	MOVL	count+16(FP), CX
	CLD
	REP; OUTSW
	RET
#endif

/*
 * Load/store segment descriptor tables:
 *	GDT - global descriptor table
 *	IDT - interrupt descriptor table
 *	TR - task register
 * GDTR and LDTR take an m16:m64 argument,
 * so shuffle the stack arguments to
 * get it in the right format.
 */
TEXT gdtget(SB), 1, $-4
	MOVL	GDTR, (RARG)			/* Note: 10 bytes returned */
	RET

TEXT gdtput(SB), 1, $-4
	SHLQ	$48, RARG
	MOVQ	RARG, m16+0(FP)
	LEAQ	m16+6(FP), RARG

	MOVL	(RARG), GDTR

	XORQ	AX, AX
	MOVW	AX, DS
	MOVW	AX, ES
	MOVW	AX, FS
	MOVW	AX, GS
	MOVW	AX, SS

	POPQ	AX
	MOVWQZX	cs+16(FP), BX
	PUSHQ	BX
	PUSHQ	AX
	RETFQ

TEXT idtput(SB), 1, $-4
	SHLQ	$48, RARG
	MOVQ	RARG, m16+0(FP)
	LEAQ	m16+6(FP), RARG
	MOVL	(RARG), IDTR
	RET

TEXT trput(SB), 1, $-4
	MOVW	RARG, TASK
	RET

/*
 * Read/write various system registers.
 */
TEXT cr0get(SB), 1, $-4				/* Processor Control */
	MOVQ	CR0, AX
	RET

TEXT cr0put(SB), 1, $-4
	MOVQ	RARG, AX
	MOVQ	AX, CR0
	RET

TEXT cr2get(SB), 1, $-4				/* #PF Linear Address */
	MOVQ	CR2, AX
	RET

TEXT cr3get(SB), 1, $-4				/* PML4 Base */
	MOVQ	CR3, AX
	RET

TEXT cr3put(SB), 1, $-4
	MOVQ	RARG, AX
	MOVQ	AX, CR3
	RET

TEXT cr4get(SB), 1, $-4				/* Extensions */
	MOVQ	CR4, AX
	RET

TEXT cr4put(SB), 1, $-4
	MOVQ	RARG, AX
	MOVQ	AX, CR4
	RET

/* tsc is reset only by processor reset */
TEXT rdtsc(SB), 1, $-4				/* Time Stamp Counter */
	LFENCE
	RDTSC
	LFENCE
	JMP	_swap

TEXT rdmsr(SB), 1, $-4				/* Model-Specific Register */
	MOVL	RARG, CX
	RDMSR
_swap:
	XCHGL	DX, AX				/* swap lo/hi, zero-extend */
	SHLQ	$32, AX				/* hi<<32 */
	ORQ	DX, AX				/* (hi<<32)|lo */
	RET

TEXT wrmsr(SB), 1, $-4
	MOVL	RARG, CX
	MOVL	lo+8(FP), AX
	MOVL	hi+12(FP), DX
	WRMSR
	RET

/*
 * cache or tlb invalidation
 */
TEXT invlpg(SB), 1, $-4
	MOVQ	RARG, va+0(FP)
	INVLPG	va+0(FP)
	RET

TEXT wbinvd(SB), 1, $-4
	WBINVD
	RET

/*
 * Serialisation.
 */
TEXT mfence(SB), 1, $-4
	MFENCE
	RET
#ifdef unused
TEXT lfence(SB), 1, $-4
	LFENCE
	RET
#endif

/*
 * Note: CLI and STI are not serialising instructions.
 * Is that assumed anywhere?
 * Added fences to test this, 20 july 2021.  Seems like a good idea.
 */
TEXT splhi(SB), 1, $-4
_splhi:
	PUSHFQ
	POPQ	AX
	TESTQ	$If, AX				/* If - Interrupt enable Flag */
	JEQ	_spldone			/* already disabled? done */
	MOVQ	(SP), BX
	MOVQ	BX, 8(RMACH) 			/* save PC in m->splpc */
	CLI					/* disable intrs */
_spldone:
	MFENCE
	RET

TEXT spllo(SB), 1, $-4				/* marker for devkprof */
_spllo:
	PUSHFQ
	POPQ	AX
	TESTQ	$If, AX				/* If - Interrupt enable Flag */
	JNZ	_spldone			/* already enabled? done */
	MOVQ	$0, 8(RMACH)			/* clear m->splpc */
	MFENCE
	STI					/* enable intrs */
	RET

/* assumed between spllo and spldone by devkprof */
TEXT splx(SB), 1, $-4
	TESTQ	$If, RARG			/* If - Interrupt enable Flag */
	JNZ	_spllo				/* want intrs enabled again? */
	JMP	_splhi				/* want intrs disabled */

TEXT spldone(SB), 1, $-4			/* marker for devkprof */
	RET

TEXT islo(SB), 1, $-4
	PUSHFQ
	POPQ	AX
	ANDQ	$If, AX				/* If - Interrupt enable Flag */
	RET

/*
 * atomic operations
 */

TEXT aincnonneg(SB), 1, $0			/* int aincnonneg(int*); */
	CALL	ainc(SB)
	JGT	_aincdone
	/* <= 0 after incr, so overflowed, so blow up */
	XORQ	BX, BX
	MOVQ	(BX), BX			/* over under sideways down */
_aincdone:
	RET

TEXT adecnonneg(SB), 1, $0			/* int adecnonneg(int*); */
	CALL	adec(SB)
	JGE	_aincdone
	/* <0, so underflow.  use own copy of XORQ/MOVQ to disambiguate PC */
	XORQ	BX, BX
	MOVQ	(BX), BX			/* over under sideways down */
	RET

TEXT aadd(SB), 1, $-4				/* int aadd(int*, int); */
	MOVL	addend+8(FP), AX	
	MOVL	AX, BX
	MFENCE
	LOCK; XADDL BX, (RARG)
	MFENCE
	ADDL	BX, AX
	RET

/*
 * void monitor(void* address, u32int extensions, u32int hints);
 * void mwait(u32int extensions, u32int hints);
 *
 * Note: extensions and hints are only 32-bits.
 * There are no extensions or hints defined yet for MONITOR,
 * but MWAIT can have both.
 * These functions and prototypes may change.
 */
TEXT monitor(SB), 1, $-4
	MOVQ	RARG, AX			/* address */
	MOVL	extensions+8(FP), CX		/* (c|sh)ould be 0 currently */
	MOVL	hints+16(FP), DX		/* (c|sh)ould be 0 currently */
	MONITOR
	RET

TEXT mwait(SB), 1, $-4
	MOVL	RARG, CX			/* extensions */
	MOVL	hints+8(FP), AX
	MFENCE
	MWAIT	/* an interrupt or any store to monitored word will resume */
	MFENCE
	RET

/*
 * int k10waitfor(int* address, int val);
 *
 * Suspend the CPU until an interrupt or *address changes from val.
 * Combined, thought to be usual, case of monitor+mwait
 * with no extensions or hints, and return on interrupt even
 * if val didn't change.
 * This function and prototype may change.
 */
TEXT k10waitfor(SB), 1, $-4
	MOVL	val+8(FP), R8

	CMPL	(RARG), R8			/* already changed? */
	JNE	_wwdone

	MOVQ	RARG, AX			/* linear address to monitor */
	XORL	CX, CX				/* no optional extensions yet */
	XORL	DX, DX				/* no optional hints yet */
	MONITOR

	CMPL	(RARG), R8			/* changed yet? */
	JNE	_wwdone

	XORL	AX, AX				/* no optional hints yet */
	MFENCE
	MWAIT		/* an interrupt or any store to (RARG) will resume */
	MFENCE

_wwdone:
	MOVL	(RARG), AX
	RET

/* really only useful on old, uniprocessor VMs.  returns spllo(). */
TEXT halt(SB), 1, $-4
	CLI
	CMPL	nrdy(SB), $0
	JEQ	_nothingready
	STI
	RET

_nothingready:
	MFENCE
	STI
	HLT
	RET

/*
 * Label consists of a stack pointer and a programme counter.
 */
TEXT gotolabel(SB), 1, $-4
	MOVQ	0(RARG), SP			/* restore SP */
	MOVQ	8(RARG), AX			/* put return PC on the stack */
	MOVQ	AX, 0(SP)
	MOVL	$1, AX				/* return 1 */
	RET

TEXT setlabel(SB), 1, $-4
	MOVQ	SP, 0(RARG)			/* store SP */
	MOVQ	0(SP), BX			/* store return PC */
	MOVQ	BX, 8(RARG)
	XORL	AX, AX				/* return 0 */
	RET

TEXT mul64fract(SB), 1, $-4
	MOVQ	a+8(FP), AX
	MULQ	b+16(FP)			/* a*b */
	SHRQ	$32, AX:DX
	MOVQ	AX, (RARG)
	RET

/*
 * Miscellany
 */

/*
 * The CPUID instruction is always supported on the amd64.
 * It's a serialising instruction and can be executed in user mode.
 */
TEXT cpuid(SB), $-4
	MOVL	RARG, AX			/* function in AX */
	MOVLQZX	cx+8(FP), CX			/* iterator/index/etc. */

	CPUID

	MOVQ	info+16(FP), BP
	MOVL	AX, 0(BP)
	MOVL	BX, 4(BP)
	MOVL	CX, 8(BP)
	MOVL	DX, 12(BP)
	RET

/*
 * Basic timing loop to determine CPU frequency.
 * The AAM instruction is not available in 64-bit mode.
 * It was really slow (Intel Skylake-x cpus issued 11 µops).  Divide
 * instructions are as slow or slower, but not many others are
 * (other than the simd/vector/graphics instructions).
 * LOOP lab is equivalent to DECQ CX; JNZ lab, but much slower.
 */
TEXT aamloop(SB), 1, $-4
	MOVLQZX	RARG, CX
aaml1:
//	AAM; LOOP	aaml1		/* old 386 version */
	PAUSE
	PAUSE
	DECQ	CX
	JNZ	aaml1
	RET

TEXT pause(SB), 1, $-4
	MFENCE
	PAUSE
	RET

TEXT bsr(SB), $0
	BSRQ	RARG, AX		/* return bit index of leftmost 1 bit */
	RET

///*
// * Testing.
// */
//TEXT ud2(SB), $-4
//	BYTE $0x0f; BYTE $0x0b
//	RET
