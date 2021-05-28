#include "amd64l.h"

MODE $64

/*
 * Port I/O.
 */
TEXT inb(SB), 1, $-4
	MOVL	RARG, DX			/* MOVL	port+0(FP), DX */
	XORL	AX, AX
	INB
	RET

TEXT insb(SB), 1, $-4
	MOVL	RARG, DX
	MOVQ	address+8(FP), DI
	MOVL	count+16(FP), CX
	CLD
	REP;	INSB
	RET

TEXT ins(SB), 1, $-4
	MOVL	RARG, DX
	XORL	AX, AX
	INW
	RET

TEXT inss(SB), 1, $-4
	MOVL	RARG, DX
	MOVQ	address+8(FP), DI
	MOVL	count+16(FP), CX
	CLD
	REP;	INSW
	RET

TEXT inl(SB), 1, $-4
	MOVL	RARG, DX
	INL
	RET

TEXT insl(SB), 1, $-4
	MOVL	RARG, DX
	MOVQ	address+8(FP), DI
	MOVL	count+16(FP), CX
	CLD
	REP; INSL
	RET

TEXT outb(SB), 1, $-1
	MOVL	RARG, DX			/* MOVL	port+0(FP), DX */
	MOVL	byte+8(FP), AX
	OUTB
	RET

TEXT outsb(SB), 1, $-4
	MOVL	RARG, DX
	MOVQ	address+8(FP), SI
	MOVL	count+16(FP), CX
	CLD
	REP; OUTSB
	RET

TEXT outs(SB), 1, $-4
	MOVL	RARG, DX
	MOVL	short+8(FP), AX
	OUTW
	RET

TEXT outss(SB), 1, $-4
	MOVL	RARG, DX
	MOVQ	address+8(FP), SI
	MOVL	count+16(FP), CX
	CLD
	REP; OUTSW
	RET

TEXT outl(SB), 1, $-4
	MOVL	RARG, DX
	MOVL	long+8(FP), AX
	OUTL
	RET

TEXT outsl(SB), 1, $-4
	MOVL	RARG, DX
	MOVQ	address+8(FP), SI
	MOVL	count+16(FP), CX
	CLD
	REP; OUTSL
	RET

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

TEXT rdtsc(SB), 1, $-4				/* Time Stamp Counter */
	RDTSC
	XCHGL	DX, AX				/* swap lo/hi, zero-extend */
	SHLQ	$32, AX				/* hi<<32 */
	ORQ	DX, AX				/* (hi<<32)|lo */
	RET

TEXT rdmsr(SB), 1, $-4				/* Model-Specific Register */
	MOVL	RARG, CX
	RDMSR
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
TEXT lfence(SB), 1, $-4
	LFENCE
	RET

TEXT mfence(SB), 1, $-4
	MFENCE
	RET

TEXT sfence(SB), 1, $-4
	SFENCE
	RET

/*
 * Note: CLI and STI are not serialising instructions.
 * Is that assumed anywhere?
 */
TEXT splhi(SB), 1, $-4
	PUSHFQ
	POPQ	AX
	TESTQ	$If, AX				/* If - Interrupt Flag */
	JNZ	_splgohi
	RET

_splgohi:
	MOVQ	(SP), BX
	MOVQ	BX, 8(RMACH) 			/* save PC in m->splpc */
	CLI
	RET

TEXT spllo(SB), 1, $-4
	PUSHFQ
	POPQ	AX
	TESTQ	$If, AX				/* If - Interrupt Flag */
	JZ	_splgolo
	RET

_splgolo:
	MOVQ	$0, 8(RMACH)			/* clear m->splpc */
	STI
	RET

TEXT splx(SB), 1, $-4
	TESTQ	$If, RARG			/* If - Interrupt Flag */
	JNZ	_splxlo

	PUSHFQ
	POPQ	AX
	TESTQ	$If, AX				/* If - Interrupt Flag */
	JNZ	_splxgohi
	RET

_splxgohi:
	MOVQ	(SP), BX
	MOVQ	BX, 8(RMACH) 			/* save PC in m->splpc */
	CLI
	RET

_splxlo:
	PUSHFQ
	POPQ	AX
	TESTQ	$If, AX				/* If - Interrupt Flag */
	JZ	_splxgolo
	RET

_splxgolo:
	MOVQ	$0, 8(RMACH)			/* clear m->splpc */
	STI
	RET

TEXT spldone(SB), 1, $-4
	RET

TEXT islo(SB), 1, $-4
	PUSHFQ
	POPQ	AX
	ANDQ	$If, AX				/* If - Interrupt Flag */
	RET

/*
 * Synchronisation
 */
TEXT ainc(SB), 1, $-4				/* int ainc(int*); */
	MOVL	$1, AX
	LOCK; XADDL AX, (RARG)
	ADDL	$1, AX				/* overflow if -ve or 0 */
	JGT	_aincreturn
_ainctrap:
	XORQ	BX, BX
	MOVQ	(BX), BX			/* over under sideways down */
_aincreturn:
	RET

TEXT adec(SB), 1, $-4				/* int adec(int*); */
	MOVL	$-1, AX
	LOCK; XADDL AX, (RARG)
	SUBL	$1, AX				/* underflow if -ve */
	JGE	_adecreturn
_adectrap:
	XORQ	BX, BX
	MOVQ	(BX), BX			/* over under sideways down */
_adecreturn:
	RET

TEXT aadd(SB), 1, $-4				/* int aadd(int*, int); */
	MOVL	addend+8(FP), AX	
	MOVL	AX, BX
	LOCK; XADDL BX, (RARG)
	ADDL	BX, AX
	RET

TEXT tas32(SB), 1, $-4
	MOVL	$0xdeaddead, AX
	XCHGL	AX, (RARG)			/*  */
	RET

TEXT cas32(SB), 1, $-4
	MOVL	exp+8(FP), AX
	MOVL	new+16(FP), BX
	LOCK; CMPXCHGL BX, (RARG)
	MOVL	$1, AX
	JNZ	_cas32r0
_cas32r1:
	RET
_cas32r0:
	DECL	AX
	RET

TEXT cas64(SB), 1, $-4
	MOVQ	exp+8(FP), AX
	MOVQ	new+16(FP), BX
	LOCK; CMPXCHGQ BX, (RARG)
	MOVL	$1, AX
	JNZ	_cas64r0
_cas64r1:
	RET
_cas64r0:
	DECL	AX
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
	BYTE $0x0f; BYTE $0x01; BYTE $0xc8	/* MONITOR */
	RET

TEXT mwait(SB), 1, $-4
	MOVL	RARG, CX			/* extensions */
	MOVL	hints+8(FP), AX
	BYTE $0x0f; BYTE $0x01; BYTE $0xc9	/* MWAIT */
	RET

/*
 * int k10waitfor(int* address, int val);
 *
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
	BYTE $0x0f; BYTE $0x01; BYTE $0xc8	/* MONITOR */

	CMPL	(RARG), R8			/* changed yet? */
	JNE	_wwdone

	XORL	AX, AX				/* no optional hints yet */
	BYTE $0x0f; BYTE $0x01; BYTE $0xc9	/* MWAIT */

_wwdone:
	MOVL	(RARG), AX
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
	MOVL	$0, AX				/* return 0 */
	RET

TEXT pause(SB), 1, $-4
	PAUSE
	RET

TEXT halt(SB), 1, $-4
	CLI
	CMPL	nrdy(SB), $0
	JEQ	_nothingready
	STI
	RET

_nothingready:
	STI
	HLT
	RET

TEXT mul64fract(SB), 1, $-4
	MOVQ	a+8(FP), AX
	MULQ	b+16(FP)			/* a*b */
	SHRQ	$32, AX:DX
	MOVQ	AX, (RARG)
	RET

///*
// * Testing.
// */
//TEXT ud2(SB), $-4
//	BYTE $0x0f; BYTE $0x0b
//	RET
//
