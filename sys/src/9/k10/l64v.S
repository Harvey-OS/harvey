#include "amd64l.h"

.code64

/*
 * Port I/O.
 */
.global inb
inb:
	MOVL	%edi, %edx			/* MOVL	port+0(FP), DX */
	XORL	%eax, %eax
	INB	%dx
	RET

.global insb
insb:
	MOVL	%edi, %edx			/* MOVL	port+0(FP), DX */
	MOVQ	%rsi, %rdi
	MOVL	%edx, %ecx
	CLD
	REP;	INSB
	RET

.global ins
ins:
	MOVL	%edi, %edx			/* MOVL	port+0(FP), DX */
	XORL	%eax, %eax
	INW	%dx
	RET

.global inss
inss:
	MOVL	%edi, %edx			/* MOVL	port+0(FP), DX */
	MOVQ	%rsi, %rdi
	MOVL	%edx, %ecx
	CLD
	REP;	INSW
	RET

.global inl
inl:
	MOVL	%edi, %edx			/* MOVL	port+0(FP), DX */
	INL	%dx
	RET

.global insl
insl:
	MOVL	%edi, %edx			/* MOVL	port+0(FP), DX */
	MOVQ	%rsi, %rdi
	MOVL	%edx, %ecx
	CLD
	REP; INSL
	RET

.global outb
outb:
	MOVL	%edi, %edx			/* MOVL	port+0(FP), DX */
	MOVL	%esi, %eax
	OUTB	%dx
	RET

.global outsb
outsb:
	MOVL	%edi, %edx			/* MOVL	port+0(FP), DX */
	MOVQ	%rsi, %rdi
	MOVL	%edx, %ecx
	CLD
	REP; OUTSB
	RET

.global outs
outs:
	MOVL	%edi, %edx			/* MOVL	port+0(FP), DX */
	MOVL	%esi, %eax
	OUTW	%dx
	RET

.global outss
outss:
	MOVL	%edi, %edx			/* MOVL	port+0(FP), DX */
	MOVL	%edx, %ecx
	CLD
	REP; OUTSW
	RET

.global outl
outl:
	MOVL	%edi, %edx			/* MOVL	port+0(FP), DX */
	MOVL	%esi, %eax
	OUTL	%dx
	RET

.global outsl
outsl:
	MOVL	%edi, %edx			/* MOVL	port+0(FP), DX */
	MOVQ	%rsi, %rdi
	MOVL	%edx, %ecx
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
.global gdtget
gdtget:
	// panic.
	mov 0, %rax
	//sgdt	%rdi
//	MOVL	GDTR, (RARG)			/* Note: 10 bytes returned */
	RET

	// Called with the address of gdt in rdi.
// Load the gdt, then do a ret which will use the argument on the stack as
// a segment #. This stuff is just crazy.
// We have to push %rsi, then 16 bits(really!) of %rdi.
.global gdtput
gdtput:
	pushq	%rsi
	movq	%rdi, %rax
	pushw	%ax
	mov	%rsp, %rax
	lgdt	(%rax)
	popw 	%ax
	popq	%rax
	XORQ	%rax, %rax
	MOVW	%ax, %ds
	MOVW	%ax, %es
	MOVW	%ax, %fs
	MOVW	%ax, %gs
	MOVW	%ax, %ss
	POPQ	%rax
	PUSHQ	%rdx
	PUSHQ	%rax
	lretq

.global idtput
idtput:
	// save %rdi, since we are going to modify it.
	pushq	%rdi
	// Push the two quads onto the stack,
	// which arranges them in memory.
	pushq	%rsi
	shlq	$48, %rdi
	pushq	%rdi
	movq	%rsp, %rax
	addq	$6, %rax
	lidt	(%rax)
	popq	%rdi
	popq	%rsi
	popq	%rdi
	RET

.global trput
trput:
//	MOVW	%di, TASK
	RET

/*
 * Read/write various system registers.
 */
.global cr0get
cr0get:
	MOVQ	%cr0, %rax
	RET

.global cr0put
cr0put:
	MOVQ	%rdi, %rax
	MOVQ	%rax, %cr0
	RET

.global cr2get
cr2get:
	MOVQ	%cr2, %rax
	RET

.global cr3get
cr3get:
	MOVQ	%cr3, %rax
	RET

.global cr3put
cr3put:
	MOVQ	%rdi, %rax
	MOVQ	%rax, %CR3
	RET

.global cr4get
cr4get:
	MOVQ	%CR4, %rax
	RET

.global cr4put
cr4put:
	MOVQ	%rdi, %rax
	MOVQ	%rax, %CR4
	RET

.global rdtsc
rdtsc:
	RDTSC
						/* u64int rdtsc(void); */
	XCHGL	%edx, %eax				/* swap lo/hi, zero-extend */
	SHLQ	$32, %rax				/* hi<<32 */
	ORQ	%rdx, %rax				/* (hi<<32)|lo */
	RET

.global rdmsr
rdmsr:
	PUSHQ	%rcx
	PUSHQ	%rdx
	MOVL	%edi, %ecx

	RDMSR
						/* u64int rdmsr(u32int); */
	XCHGL	%edx, %eax				/* swap lo/hi, zero-extend */
	SHLQ	$32, %rax				/* hi<<32 */
	ORQ	%rdx, %rax				/* (hi<<32)|lo */
	POPQ	%rdx
	POPQ	%rcx
	RET

.global wrmsr
wrmsr:
	PUSHQ	%rax	// do we need to do this?
	PUSHQ	%rcx
	PUSHQ	%rdx
	MOVL	%edi, %ecx
	MOVL	%esi, %eax
	MOVQ	%rsi, %rdx
	SHRQ	$32, %rdx

	WRMSR

	POPQ	%rdx
	POPQ	%rcx
	POPQ	%rax
	RET

.global invlpg
invlpg:
#	MOVQ	%rdi, va+0(FP)

#	INVLPG	va+0(FP)

	RET

.global wbinvd
wbinvd:
	WBINVD
	RET

/*
 * Serialisation.
 */
.global lfence
lfence:
	LFENCE
	RET

.global mfence
mfence:
	MFENCE
	RET

.global sfence
sfence:
	SFENCE
	RET

/*
 * Note: CLI and STI are not serialising instructions.
 * Is that assumed anywhere?
 */
.global splhi
splhi:
_splhi:
	PUSHFQ
	POPQ	%rax
	TESTQ	$If, %rax				/* If - Interrupt Flag */
	JZ	_alreadyhi			/* use CMOVLEQ etc. here? */

	MOVQ	%gs:0, %rax
	// Until we know we have a register we can trash, we have to
	// save and restore this one. Callee save
	PUSHQ	%rbx
	MOVQ	8(%rsp), %rbx
	MOVQ	%rbx, 8(%rax) 			/* save PC in m->splpc */
	POPQ	%rbx

_alreadyhi:
	CLI
	RET

.global spllo
spllo:
_spllo:
	PUSHFQ
	POPQ	%rax
	TESTQ	$If, %rax				/* If - Interrupt Flag */
	JNZ	_alreadylo			/* use CMOVLEQ etc. here? */
	MOVQ	%gs:0, %rax
	MOVQ	$0, 8(%rax)			/* clear m->splpc */

_alreadylo:
	STI
	RET

.global splx
splx:
	TESTQ	$If, %rdi			/* If - Interrupt Flag */
	JNZ	_spllo
	JMP	_splhi

.global spldone
spldone:
	RET

.global islo
islo:
	PUSHFQ
	POPQ	%rax
	ANDQ	$If, %rax				/* If - Interrupt Flag */
	RET

/*
 * Synchronisation
 */
.global ainc
ainc:
	MOVL	$1, %eax
	LOCK; XADDL %eax, (%rdi)
	ADDL	$1, %eax				/* overflow if -ve or 0 */
	JG	_return
_trap:
	XORQ	%rbx, %rbx
	MOVQ	(%rbx), %rbx			/* over under sideways down */
_return:
	RET

.global adec
adec:
	MOVL	$-1, %eax
	LOCK; XADDL %eax, (%rdi)
	SUBL	$1, %eax				/* underflow if -ve */
	JL	_trap

	RET

/*
 * Semaphores rely on negative values for the counter,
 * and don't have the same overflow/underflow conditions
 * as ainc/adec.
 */
.global semainc
semainc:
	MOVL	$1, %eax
	LOCK; XADDL %eax, (%rdi)
	ADDL	$1, %eax
	RET

.global semadec
semadec:
	MOVL	$-1, %eax
	LOCK; XADDL %eax, (%rdi)
	SUBL	$1, %eax
	RET

.global tas32
tas32:
	MOVL	$0xdeaddead, %eax
	XCHGL	%eax, (%rdi)			/*  */
	RET

.global fas64
fas64:
	MOVQ	%rdi, %rax
	//LOCK; XCHGQ	%eax, (%rdi)			/*  */
	RET

.global cas32
cas32:
	MOVL	%edi, %eax
	MOVL	%esi, %ebx
	LOCK; CMPXCHGL %ebx, (%rdi)
	MOVL	$1, %eax				/* use CMOVLEQ etc. here? */
	JNZ	_cas32r0
_cas32r1:
	RET
_cas32r0:
	DECL	%eax
	RET

.global cas64
cas64:
	MOVQ	%rdi, %rax
	MOVQ	%rsi, %rbx
	LOCK; CMPXCHGQ %rbx, (%rdi)
	MOVL	$1, %eax				/* use CMOVLEQ etc. here? */
	JNZ	_cas64r0
_cas64r1:
	RET
_cas64r0:
	DECL	%eax
	RET

/*
 * Label consists of a stack pointer and a programme counter
 * 0(%rdi) is the SP, 8(%rdi) is the PC
 */
.global gotolabel
gotolabel:
	MOVQ	%rdi, %rax
	MOVQ	0(%rdi), %rsp

	MOVQ	(16+0*8)(%rdi), %rBX
	MOVQ	(16+1*8)(%rdi), %rCX
	MOVQ	(16+2*8)(%rdi), %rDX
	MOVQ	(16+3*8)(%rdi), %rSI
	MOVQ	(16+5*8)(%rdi), %rBP
	MOVQ	(16+6*8)(%rdi), %r8
	MOVQ	(16+7*8)(%rdi), %r9
	MOVQ	(16+8*8)(%rdi), %r10
	MOVQ	(16+9*8)(%rdi), %r11
	MOVQ	(16+10*8)(%rdi), %r12
	MOVQ	(16+11*8)(%rdi), %r13
	MOVQ	(16+12*8)(%rdi), %r14
	MOVQ	(16+13*8)(%rdi), %r15
	MOVQ	(16+4*8)(%rdi), %rDI

	MOVQ	8(%rax), %rax			/* put return PC on the stack */
						/* NOTE: replaces previous caller? */
	MOVQ	%rax, (%rSP)
	MOVQ	$1, %rax			/* return 1 */
	RET

	/* save all registers on this stack, the save stack
	* in the label struct.
	*/
.global setlabel
setlabel:
	// %rax is trashable.
	MOVQ	0(%rSP), %rax			/* store return PC */
	MOVQ	%rax, 8(%rdi)

	MOVQ	%rBX, (16+0*8)(%rdi)
	MOVQ	%rCX, (16+1*8)(%rdi)
	MOVQ	%rDX, (16+2*8)(%rdi)
	MOVQ	%rSI, (16+3*8)(%rdi)
	MOVQ	%rDI, (16+4*8)(%rdi)
	MOVQ	%rBP, (16+5*8)(%rdi)
	MOVQ	%r8, (16+6*8)(%rdi)
	MOVQ	%r9, (16+7*8)(%rdi)
	MOVQ	%r10, (16+8*8)(%rdi)
	MOVQ	%r11, (16+9*8)(%rdi)
	MOVQ	%r12, (16+10*8)(%rdi)
	MOVQ	%r13, (16+11*8)(%rdi)
	MOVQ	%r14, (16+12*8)(%rdi)
	MOVQ	%r15, (16+13*8)(%rdi)

	MOVQ	%rSP, 0(%rdi)	/* store SP */
	MOVL	$0, %eax	/* return 0 */
	RET

.global hardhalt
hardhalt:
	STI
	HLT
	RET

.global _monitor
_monitor:
	MOVQ	%rdi, %rax			/* linear address to monitor */
	XORQ	%rcx, %rcx				/* no optional extensions yet */
	XORQ	%rdx, %rdx				/* no optional hints yet */
	.byte 0x0f; .byte 0x01; .byte 0xc8	/* MONITOR */
	RET

.global _mwait
_mwait:
	movq	%rdi, %rcx			/* optional extensions */
	.byte 0x0f; .byte 0x01; .byte 0xc9	/* MWAIT */
	RET

.global k10mwait
k10mwait:
k10mwloop:
	MOVQ	%rdi,%rcx
	MOVQ	(%rcx), %rax
	CMPQ	$0, %rax
	JNE		k10mwdone
	MOVQ	%rdi, %rax			/* linear address to monitor */
	XORQ	%rcx, %rcx				/* no optional extensions yet */
	XORQ	%rdx, %rdx				/* no optional hints yet */
	.byte 0x0f; .byte 0x01; .byte 0xc8	/* MONITOR */
	MOVQ	%rdi, %rcx
	MOVQ	0(%rcx), %rax
	CMPQ	$0, %rax
	JNE		k10mwdone
	XORQ %rcx, %rcx			/* optional extensions */
	.byte 0x0f; .byte 0x01; .byte 0xc9	/* MWAIT */
	JMP		k10mwloop
k10mwdone:
	RET

.global machp
machp:
	movq	%gs:0, %rax
	ret

/* not needed.
.global mul64fract
mul64fract:
	MOVQ	%rdi, %rax
	MULQ	%rsi			/ * a*b *
	SHRQ	$32, %rax:DX
	MOVQ	%rax, (%rdi)
	RET
*/

///*
// * Testing.
// */
//.global ud2
ud2:
//	BYTE $0x0f; BYTE $0x0b
//	RET
//
