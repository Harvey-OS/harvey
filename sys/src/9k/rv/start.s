/*
 * minimal risc-v assembly-language initialisation to enable
 * calling begin() in C.  this is the first kernel code executed.
 */
#include "mem.h"
#include "riscv64l.h"
#include "start.h"

#define Z(n)	MOV R0, R(n)

#define ENSURELOW(r)	MOV $~KSEG0, R(TMP); AND R(TMP), r

/*
 * stack pointer is unknown at entry, so use $-4 to not touch it
 * until we can establish a new stack.
 *
 * all cpus may be executing this code simultaneously.
 */
TEXT _main(SB), 1, $-4			/* _main avoids libc's main9.s */
	/* this entry point will work in machine or super mode */
	FENCE
	FENCE_I
	CSRRC	CSR(SSTATUS), $Sie, R0		/* super intrs off */
	MOV	R0, CSR(SSCRATCH)
	MOV	R10, R(HARTID)	/* save possible sbi hartid in a safe place */
	MOV	$MACHINITIAL, R(MACHMODE)
	BEQ	R(MACHMODE), superstart

	/*
	 * we're supposed to be in machine mode here.
	 * risc-v makes it hard to tell without trapping.
	 */
	CSRRC	CSR(MSTATUS), $(Mie|Sie), R0	/* all intrs off */
	MOV	R0, CSR(MSCRATCH)
	MOV	CSR(MHARTID), R(HARTID)
superstart:
	/*
	 * zero most registers to avoid possible non-determinacy.
	 * R3 is static base; R21 is MACHMODE; R27 is CSRZERO; R31 is hartid.
	 */
	Z(1); Z(2); Z(4); Z(5); Z(6); Z(7); Z(8); Z(9); Z(11); Z(12); Z(13)
	Z(14); Z(15); Z(16); Z(17); Z(18); Z(19); Z(20); Z(22); Z(23); Z(24)
	Z(25); Z(26); Z(27); Z(28); Z(29); Z(30)

	/*
	 * stop paging, if it's on.  must be in the identity map, but given
	 * the convention of starting kernels at 0x80000000, that's almost
	 * certain on risc-v systems intended for general unix use, as is
	 * ram starting there.  otherwise, traps to machine mode (with no
	 * virtual memory) would fault endlessly.
	 */
/**/	LOADSATP(R(CSRZERO))		/* paging off; rs can't be R0 */

	/* prepare the static base for its first use */
	MOV	$setSB(SB), R3
	MOV	$KSEG0, R(RKSEG0)
	/*
	 * Force SB to physical addresses.
	 * NB: doing so eliminates the need for "-KSEG0" in machine mode or
	 * when otherwise executing in low addresses.
	 */
	ENSURELOW(R3)

	MOV	$strap(SB), R9
	MOV	R9, CSR(STVEC)

	MOV	$PAUart0, R(UART0)	/* now safe to print on PAUart0 */
	MOV	R(UART0), phyuart(SB)
	MOV	R(UART0), uart0regs(SB)	/* will later be va */
	MOV	$printlck(SB), R(LOCK) /* now safe to lock cons uart */
	CONSPUT($'*')
	ADD	$'0', R(HARTID), R9
	CONSPUT(R9)

	MOV	$dummysc(SB), R12
	SCW(0, 12, 0)		/* discharge any lingering reservation */

//	MOVW	R(MACHMODE), bootmachmode(SB)	/* export to C */

	/*
	 * assign machnos sequentially from zero
	 */
	MOV	$hartcnt(SB), R9
	MOV	$1, R10
	/* after amo: old hartcnt in R12, updated hartcnt in memory */
	FENCE
	AMOW(Amoadd, AQ|RL, 10, 9, 12)
	FENCE
	MOV	R12, R(MACHNO)
	MOV	$MACHMAX, R(TMP)
	BGEU	R(TMP), R(MACHNO), nostack /* drat, no initial stack for this one */

	/* store hart id, indexed by machno */
	MOV	$r10(SB), R12
	MOV	$XLEN, R13
	MUL	R(MACHNO), R13
	ADD	R13, R12
	MOV	R(HARTID), (R12)	/* export hartid to C in r10[machno] */

	/*
	 * set up a temporary stack for this cpu, based on machno.
	 */
	MOV	$initstks(SB), R(TMP)
	MOV	$INITSTKSIZE, R10
	ADD	$1, R(MACHNO), R12
	MUL	R12, R10
	ADD	R10, R(TMP), R2
TEXT aligntmpstk(SB), 1, $XLEN		/* reset SP, FP for new R2 */

	CONSPUT($'S')
	MOV	R(MACHNO), R(ARG)
	JAL	LINK, begin(SB)		/* begin(machno); */
wfi:
	WFI
	JMP	wfi

nostack:
	CONSPUT($'!')
	JMP	wfi

	GLOBL	printlck(SB), $8
	DATA	printlck(SB)/8, $0
	GLOBL	initstks(SB), $(HARTMAX*INITSTKSIZE)
	/*
	 * since cpus may start in any order, this needs to be in the
	 * data segment, so that zeroing bss won't clobber stacks in  use.
	 */
	DATA	initstks(SB)/8, $1	/* force out of bss */
