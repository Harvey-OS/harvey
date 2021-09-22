/*
 * Trap catcher for machine mode of RV64.
 *
 * There is no paging in machine mode, so this handler, at least,
 * must be in identity-mapped memory.
 *
 * Note well that the exact stack layout of a trap is known
 * in various other places too (e.g., main.c, mmu.c, trap.c).
 * The intended layout is: a Ureg struct at the very end of up->kstack if
 * trapped from user mode, or somewhere on m->stack if trapped from kernel,
 * with a pointer to that Ureg just before (below) it, for the
 * argument to trap().
 *
 * all interrupts and most exceptions are delegated, if the system allows.
 * so we should at most get timer, IPI and external interrupts
 * and environment calls from super in machine mode.
 */

#include "mem.h"
#include "riscv64l.h"

/* registers */
#define RSYS	29
#define UART0	30
#define	TMP	31

#define ENSURELOW(r)	MOV $~KSEG0, R(TMP); AND R(TMP), r

/* 0x20 in Lsr (uart reg 5) is Thre (Thr empty). assumes R(UART0) is set. */
#define CONSPUT(c) \
	FENCE; \
 	MOV	$PAUart0, R(UART0);	/* now safe to print on PAUart0 */ \
	MOVWU	(5*4)(R(UART0)), R(TMP); \
	AND	$0x20, R(TMP); \
	BEQ	R(TMP), -3(PC); \
	MOV	c, R(TMP); \
	MOVW	R(TMP), (R(UART0))

#define SPLHI	CSRRC	CSR(MSTATUS), $Mie, R0

#define CALLTRAP \
	SPLHI;\
	MOV	R2, R(ARG);		/* Ureg* */\
	SUB	$(2*XLEN), R2;	/* leave needed room (for return PC & what?) */\
/**/	JAL	LINK, trap(SB);		/* to C: trap(sp) */\
	ADD	$(2*XLEN), R2;\
	SPLHI

/* side-effect: patches Ureg with regsave's R2 */
#define PUSHALLM \
	SUB	$UREGSIZE, R2;		/* room for pc, regs, csrs */\
	MOV	$((1ull<<38)-1), R9;\
	AND	R9, R2;			/* convert sp to physical */ \
	MOV	SAVEMR9(R(MACH)), R9;\
	SAVER1_31;\
	/* patch up Ureg on stack */\
	MOV	SAVEMR2(R(MACH)), R9;\
	MOV	R9, (2*XLEN)(R2);	/* push saved R2 from regsave */\
	/* save CSRs */\
	MOV	CSR(MEPC), R9;		MOV R9, 0(R2);	/* push saved pc */\
	MOV	CSR(MSTATUS), R9;	MOV R9, (32*XLEN)(R2);\
	MOV	CSR(MIE), R9;		MOV R9, (33*XLEN)(R2);\
	MOV	CSR(MCAUSE), R9;	MOV R9, (34*XLEN)(R2);\
	MOV	CSR(MTVAL), R9;		MOV R9, (35*XLEN)(R2);\
	MOV	$Mppmach, R9;		MOV R9, (36*XLEN)(R2);\
	MOV	CSR(MSCRATCH), R9;	MOV R9, (37*XLEN)(R2)

#define POPMOSTREGS \
	POPR8_31; \
	MOV	(1*XLEN)(R2), R1;	/* restore link reg */ \
	MOV	(4*XLEN)(R2), R4;	/* restore saved temporary */ \
	MOV	(5*XLEN)(R2), R5

/*
 * mips ports restore only M(STATUS) and M(EPC), but has no *IE.
 * there should be no need to restore MIE.
 */
#define POPCSRM \
	MOV         0(R2), R4; MOV R4, CSR(MEPC); \
	MOV	$dummysc(SB), R9; \
	SCW(0, 9, 0); \
	MOV (32*XLEN)(R2), R4; MOV R4, CSR(MSTATUS)	/**/

/*
 * cope with machine-mode faults, from user or kernel mode.
 * the trap mechanism saved the return address in CSR(MEPC) and masked
 * interrupts.
 *
 * NB: set SB before MOVing 64-bit constants.
 */
TEXT mtrap(SB), 1, $-4
	CSRRW	CSR(MSCRATCH), R(MACH), R(MACH)	/* save old m, load new m */
	FENCE
	/* stash a few regs in m->regsave to free working registers */
	MOV	R2, SAVEMR2(R(MACH))	/* save SP */
	MOV	R3, SAVEMR3(R(MACH))	/* save SB */
	MOV	R4, SAVEMR4(R(MACH))	/* save R4 temporary */
	MOV	R6, SAVEMR6(R(MACH))	/* save R6 up */
	MOV	R9, SAVEMR9(R(MACH))

	MOV	CSR(MCAUSE), R6
	MOV	$setSB(SB), R3
	MOV	$~KSEG0, R9
	AND	R9, R3			/* force SB to physical address */
	MOV	$Rv64intr, R9
	AND	R6, R9
	BEQ	R9, except

	/*
	 * punt interrupts to supervisor mode.
	 * this should run to MRET without interruption.
	 */
intr:
	AND	$077, R6		/* clear Rv64intr bit */
	MOV	$Machie, R9
	MOV	CSR(MIP), R2		/* intrs pending */
	AND	R9, R2			/* isolate mach intr bits */
	/* clear MIP *ie bits to dismiss (but they are mostly readonly) */
	CSRRC	CSR(MIP), R2, R0
	SRL	$2, R2
	CSRRS	CSR(SIP), R2, R0 /* set corr. SIP bits to punt to super */

	/* clock interrupt? */
	MOV	$Suptmrintr, R4
	BEQ	R6, R4, clock
	MOV	$Mchtmrintr, R4
	BNE	R6, R4, notclock
clock:
	/* clock interrupt */
	MOVWU	MACHHARTID(R(MACH)), R2
	MOV	$BY2V, R9
	MUL	R9, R2
	MOV	$(PAClint+4*4096), R9	/* phys addr in machine mode */
	ADD	R2, R9			/* &clint->mtimecmp[m->hartid] */
	MOV	$VMASK(63), R2
	/* cater to tinyemu with 32-bit stores */
	SRL	$32, R2, R6		/* get high ulong */
	MOVW	R6, 4(R9)		/* extinguish timer intr source */
	MOVW	R2, (R9)		/* extinguish timer intr source */
notclock:
	MOV	$dummysc(SB), R9
	SCW(0, 9, 0)			/* clear any LR reservation */
	MOV	$(Sum|Mxr), R9
	CSRRS	CSR(SSTATUS), R9, R0
	MOV	SAVEMR9(R(MACH)), R9
	MOV	SAVEMR6(R(MACH)), R6	/* R6 up */
	MOV	SAVEMR4(R(MACH)), R4	/* R4 temporary */
	MOV	SAVEMR3(R(MACH)), R3	/* SB */
	MOV	SAVEMR2(R(MACH)), R2	/* SP */
	CSRRW	CSR(MSCRATCH), R(MACH), R(MACH)	/* restore r7, reload saved m */
	FENCE
	MRET			/* the super intr is awaiting our return */

	/*
	 * exception from kernel (super) or user mode.
	 * going to have to save & restore almost all the registers.
	 * should be rare.  shouldn't get here from user mode.
	 */
except:
	MOV	CSR(MSTATUS), R9
	MOV	$Mpp, R4
	AND	R4, R9
	MOV	SAVEMR4(R(MACH)), R4	/* restore R4 temporary */
	BEQ	R9, mfromuser		/* 0 == Mppuser */

	/*
	 * exception from kernel mode: SB, MACH, R2, USER were all okay,
	 * but we stepped on MACH (a no-op) and SP above.
	 */
	MOV	SAVEMR9(R(MACH)), R9
	MOV	SAVEMR6(R(MACH)), R6	/* R6 up */
	MOV	SAVEMR3(R(MACH)), R3	/* SB */
	MOV	SAVEMR2(R(MACH)), R2	/* SP */

	MOV	CSR(MCAUSE), R6
	MOV	$~Rv64intr, R9
	AND	R9, R6			/* R6 is cause */
	MOV	$Envcallsup, R4
	BEQ	R6, R4, totramp		/* we're rebooting? */

	PUSHALLM

	MOV	$setSB(SB), R3
	MOV	$~KSEG0, R9
	AND	R9, R3			/* force SB to physical address */
/**/	CALLTRAP

	/* pop from Ureg regs except MACH, USER, SB, R2; pop CSRs */
	POPCSRM
	POPMOSTREGS
	/*
	 * be sure to pop registers from Ureg, so that changes to this
	 * Ureg elsewhere (e.g. trap) are reflected.
	 * MACH, R2, SB are still correct; USER might not be.
	 */
	MOV	MACHPROC(R(MACH)), R(USER) /*  up = m->proc */
	JMP	return			/* to kernel */

	/*
	 * exception from user mode: SB, MACH, R2, USER all needed reloading.
	 * old MACH value is in CSR(MSCRATCH).  R2 is saved in regsave.
	 */
mfromuser:
	MOV	MACHPROC(R(MACH)), R(USER) /*  up = m->proc */
	MOV	(2*XLEN)(R(USER)), R2	/*  switch to up->kstack */
	ADD	$KSTACK, R2
	AND	$~7, R2			/* vlong alignment */

	MOV	SAVEMR9(R(MACH)), R9
	MOV	SAVEMR4(R(MACH)), R4	/* restore R4 temporary */
	MOV	SAVEMR3(R(MACH)), R3	/* SB */
	PUSHALLM			/* patches R2 from regsave in Ureg */
	MOV	SAVEMR6(R(MACH)), R3
	MOV	R3, (6*XLEN)(R2)	/* patch old r6 into Ureg */

	MOV	$setSB(SB), R3
	MOV	$~KSEG0, R9
	AND	R9, R3			/* force SB to physical address */
/**/	CALLTRAP
	SPLHI

	/* pop from Ureg */
	POPCSRM
	POPMOSTREGS

	MOV	(3*XLEN)(R2), R3	/* restore user's SB */
	MOV	(6*XLEN)(R2), R6
return:
	MOV	(2*XLEN)(R2), R2	/* restore SP */
	CSRRW	CSR(MSCRATCH), R(MACH), R(MACH) /* restore m, load saved m */
	FENCE
	MRET				/* to CSR(MEPC) address */

totramp:
	MOV	$~KSEG0, R9
	AND	R9, R3			/* force SB to physical */
	MOV	sys(SB), R(RSYS)
	AND	R9, R(RSYS)		/* sys physical */
	MOV	$LOW0SYSPAGE, R10
	ADD	R10, R(RSYS), R14	/* &sys->Reboot physical */
	MOV	(R14), R15		/* tramp address */
	AND	R9, R15			/* tramp physical */
	MOV	R14, R8
	CONSPUT($'\n')
	JMP	(R15)			/* sys->tramp(&sys->Reboot) */
