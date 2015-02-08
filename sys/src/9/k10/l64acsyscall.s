#include "mem.h"
#include "amd64l.h"

MODE $64

/*
 */
TEXT acsyscallentry(SB), 1, $-4
	SWAPGS
	BYTE $0x65; MOVQ 0, RMACH		/* m-> (MOVQ GS:0x0, R15) */
	MOVQ	16(RMACH), RUSER		/* m->proc */
	MOVQ	24(RUSER), R12		/* m->proc->dbgregs */

	/* save sp to r13; set up kstack so we can call acsyscall */
	MOVQ	SP, R13
	MOVQ	24(RMACH), SP			/* m->stack */
	ADDQ	$MACHSTKSZ, SP

	MOVQ	$SSEL(SiUDS, SsRPL3), BX		/* old stack segment */
	MOVQ	BX, 176(R12)				/* save ss */
	MOVQ	R13, 168(R12)				/* old sp */
	MOVQ	R11, 160(R12)				/* old flags */
	MOVQ	$SSEL(SiUCS, SsRPL3), BX		/* old code segment */
	MOVQ	BX, 152(R12)				/* save cs */
	MOVQ	CX, 144(R12)				/* old ip */

	MOVW	$SSEL(SiUDS, SsRPL3), 120(R12)
	MOVW	ES,  122(R12)
	MOVW	FS,  124(R12)
	MOVW	GS,  126(R12)

	MOVQ	RARG, 	0(R12)			/* system call number: up->dbgregs->ax  */
	CALL	acsyscall(SB)
NDNR:	JMP NDNR

TEXT _acsysret(SB), 1, $-4
	CLI
	SWAPGS

	MOVQ	24(RUSER), R12			/* m->proc->dbgregs */
	MOVQ	0(R12), AX			/* m->proc->dbgregs->ax */
	MOVQ	(6*8)(R12),	BP		/* m->proc->dbgregs->bp */
	ADDQ	$(15*8), R12			/* after ax--r15, 8 bytes each */

	MOVW	0(R12), DS
	MOVW	2(R12), ES
	MOVW	4(R12), FS
	MOVW	6(R12), GS

	MOVQ	24(R12), CX			/* ip */
	MOVQ	40(R12), R11			/* flags */

	MOVQ	48(R12), SP			/* sp */

	BYTE $0x48; SYSRET			/* SYSRETQ */

/*
 * Return from an exec() system call that we never did,
 * DX is ar0->p by the time we call it. See syscall()
 */
TEXT xactouser(SB), 1, $-4
	CLI
	BYTE $0x65; MOVQ 0, RMACH		/* m-> (MOVQ GS:0x0, R15) */
	MOVQ	16(RMACH), RUSER		/* m->proc */
	MOVQ	24(RUSER), R12			/* m->proc->dbgregs */
	MOVQ	144(R12), CX			/* old ip */
	MOVQ	0(R12), BX				/* save AX */
	SWAPGS
	MOVQ	$SSEL(SiUDS, SsRPL3), AX
	MOVW	AX, DS
	MOVW	AX, ES
	MOVW	AX, FS
	MOVW	AX, GS

	MOVQ	BX, AX			/* restore AX */
	MOVQ	$If, R11			/* flags */

	MOVQ	RARG, SP			/* sp */

	BYTE $0x48; SYSRET			/* SYSRETQ */
