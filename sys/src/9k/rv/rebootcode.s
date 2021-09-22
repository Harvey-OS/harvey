/*
 * Reboot trampoline code.  Copies new kernel into place over old one.
 * This is copied to sys->reboottramp before executing it.
 * All cpus come here, secondaries spin until it's safe, then jump
 * to the entry point.
 */
#include "mem.h"
#include "riscvl.h"
#include "start.h"

#define ENSURELOW(r)	AND R(SEG0MASK), r
#define ENSUREHIGH(r)	MOV $~KSEG0, R(TMP); OR R(TMP), r

/* dedicated registers for reboot */
#define ENTRY	17
#define SEG0MASK 18
#define RMASK	19

/*
 * Copy the new kernel to its expected location in physical addresses.
 * Turn off MMU, and jump to the start of the kernel.
 * we are called in the identity map with
 *
 *	(*tramp)(Reboot *);
 *
 * The new kernel will start in the privilege mode that we are called in.
 * May be called from mtrap.s or main.c.
 */
TEXT main(SB), $0
	/*
	 * we could be in machine or super mode.
	 */
	CSRRC	CSR(SSTATUS), $(Mie|Sie), R0	/* splhi */
	MOV	$setSB(SB), R3
	MOVBU	MMACHMODE(R(MACH)), R(MACHMODE)
	BEQ	R(MACHMODE), superboot
	/* we're supposed to be in machine mode via ecall */
	CSRRC	CSR(MSTATUS), $Mie, R0		/* splhi */
superboot:
	MOV	$PAUart0, R(UART0)
	MOV	$~KSEG0, R(SEG0MASK)
	MOV	$((1ull<<32)-1), R(RMASK) /* 32-bit mask */

	/* go low */
	AND	R(SEG0MASK), R(ARG), R(SYSRB)	/* physical &sys->Reboot */
	AND	R(SEG0MASK), R2		/* sp */
TEXT rbalign(SB), 1, $-4
	AND	R(SEG0MASK), R(MACH)	/* m */

	/*
	 * on cpu0, we are about to possibly overwrite our page tables,
	 * and on other cpus, that will have already happened.
	 */
	MOV	R0, R9
	LOADSATP(R9)			/* paging off in super */
	MOV	$setSB(SB), R3
	ENSURELOW(R3)

CONSPUT($'T')
	MOV	R0, newkern(SB)

	MOV	$strap(SB), R(TMP)
	MOV	R(TMP), CSR(STVEC)
	MOV	R(TMP), CSR(SEPC)

	MOVW	MMACHNO(R(MACH)), R(MACHNO)

	/* copy args out of KSEG0 space */
	/* jl doesn't produce an extended header, so entry is only 32 bits */
	MOV	RBENTRY(R(SYSRB)), R9
	AND	R(RMASK), R9		/* destination va; make it physical */
	MOV	R9, R(ENTRY)		/* also entry point */
CONSPUT($'3')
	MOV	RBCODE(R(SYSRB)), R10	/* source va */
	AND	R(SEG0MASK), R10	/*   make it physical */
	MOV	RBSIZE(R(SYSRB)), R11	/* byte count */

	/* copy before possibly overwriting Mach */
	MOVWU	MACHHARTID(R(MACH)), R(HARTID)	/* emulate SBI start-up */

	BNE	R(MACHNO), secondary

/*
 * the source and destination may overlap.
 * determine whether to copy forward or backwards.
 * this copy may overwrite the old kernel's page tables.
 */
reboot0:
CONSPUT($'C')
	MOV	$BY2WD, R13
	BGTU	R9, R10, forw		/* src > dest, copy forward */
	ADD	R10, R11, R14
	BLEU	R9, R14, forw		/* src+len <= dest, copy forward */
	/* copy new kernel into place */
	ADD	R11, R9			/* start at the ends */
	ADD	R11, R10
	SUB	R13, R9
	SUB	R13, R10
back:
	MOVW	(R10), R14
	MOVW	R14, (R9)
	SUB	R13, R10
	SUB	R13, R9
	SUB	R13, R11
	BGT	R11, back
	JMP	done
forw:
	MOVW	(R10), R14
	MOVW	R14, (R9)
	ADD	R13, R10
	ADD	R13, R9
	SUB	R13, R11
	BGT	R11, forw
done:
	CONSPUT($'\n')
	FENCE
	FENCE_I
	MOV	$RBFLAGSET, R(TMP2)
	MOV	R(TMP2), newkern(SB)		/* release other cpus */
	JMP	newkernel

/*
 * secondary cpus spin here until sys->newkernel is true,
 * indicating that the new kernel is in place.
 */
secondary:
CONSPUT($'W')
	/* wait for cpu0 to load the new kernel */
	MOV	$newkern(SB), R(TMP2)
	MOV	$RBFLAGSET, R10
awaitkern:
	PAUSE
	FENCE
	MOV	(R(TMP2)), R(TMP)
	BNE	R(TMP), R10, awaitkern

/*
 * JMP to kernel entry point.  Note the true kernel entry point is
 * the virtual address KZERO|R(ENTRY), but on some systems, this must wait until
 * the MMU is enabled by the kernel in start.s.
 */
newkernel:
	FENCE
	FENCE_I
	MOV	R(HARTID), R10			/* emulate SBI start up */
	BEQ	R(MACHMODE), superentry
	/* we're supposed to be in machine mode via ecall */
	MOV	R(ENTRY), CSR(MTVEC)
	MOV	R(ENTRY), CSR(MEPC)
superentry:
	MOV	R(ENTRY), CSR(STVEC)
	MOV	R(ENTRY), CSR(SEPC)
/**/	JMP	(R(ENTRY))

TEXT strap(SB), 1, $-4
	CONSPUT($'!')
	MOV	CSR(SCAUSE), R8
	ADD	$'0', R8
	CONSPUT(R8)
wfiloop:
	FENCE
	WFI
	JMP	wfiloop

	GLOBL	newkern(SB), $8
	DATA	newkern(SB)/8, $0
