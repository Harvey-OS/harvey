#include "mem.h"
#include "vmspal.h"

#define SP		R30

TEXT	_main(SB), $-8
	MOVQ	$setSB(SB), R29
	MOVQ	$edata(SB), R1
	MOVQ	$end(SB), R2
loop2:
	MOVQ	R31, (R1)
	ADDQ	$8, R1
	CMPUGT	R1, R2, R3
	BEQ	R3, loop2

	JSR	main(SB)

TEXT	firmware(SB), $-8
	CALL_PAL $PALhalt
	MOVQ	$_divq(SB), R31		/* touch _divq etc.; doesn't need to execute */
	MOVQ	$_divl(SB), R31		/* touch _divl etc.; doesn't need to execute */
	RET

TEXT	mb(SB), $-8
	MB
	RET

TEXT	icflush(SB), $-8
	CALL_PAL $PALimb
	RET

TEXT	tlbflush(SB), $-8
	CALL_PAL $PALmtpr_tbia
	RET

TEXT	gendispatch(SB), $-8
	MOVQ	8(FP), R16
	MOVQ	16(FP), R17
	MOVQ	24(FP), R18
	MOVQ	32(FP), R19
	MOVQ	40(FP), R20
	MOVQ	R26, R1
	JSR	(R0)
	MOVQ	R1, R26
	RET					/* 7a bug: should be RET (R1) */

TEXT	rdv(SB), $-8
	MOVQ	(R0), R0
	RET

TEXT	wrv(SB), $-8
	MOVQ	8(FP), R1
	MOVQ	R1, (R0)
	RET

TEXT	ipl(SB), $-8
	CALL_PAL $PALmfpr_ipl
	RET

TEXT	mces(SB), $-8
	CALL_PAL $PALmfpr_mces
	RET

TEXT	setipl(SB), $-8
	MOVQ	R0, R16
	CALL_PAL $PALmtpr_ipl
	RET

TEXT	setmces(SB), $-8
	MOVQ	R0, R16
	CALL_PAL $PALmtpr_mces
	RET

TEXT	ldqp(SB), $-8
	MOVQ	R0, R16
	CALL_PAL $PALldqp
	RET

TEXT	stqp(SB), $-8
	MOVQ	R0, R16
	MOVQ	8(FP), R17
	CALL_PAL $PALstqp
	RET

TEXT	getptbr(SB), $-8
	CALL_PAL $PALmfpr_ptbr
	RET

TEXT	swppal(SB), $-8
	MOVQ	R0, R16			/* which PALcode */
	MOVQ	8(FP), R17		/* new PC */
	MOVQ	16(FP), R18		/* PCBB (physical) */
	MOVQ	24(FP), R19		/* VPTB */
	MOVQ	32(FP), R20		/* new KSP */
	CALL_PAL $PALswppal
	RET

TEXT	pcc_cnt(SB), $-8
	MOVQ	PCC, R1
	MOVL	R1, R0
	RET
