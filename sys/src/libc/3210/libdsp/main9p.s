#define STACK	(0x5003fff0)
#define	PCW	12
#define	mypcw	0x6215
#define NOOP	BRA	FALSE, (R0)
#define	COUNTER	0x06000000

TEXT	_mainp(SB), 1, $-4
	MOVW	$STACK, R1
	NOOP
	MOVW	$mypcw, R3
	MOVH	R3, C(PCW)
	NOOP
	NOOP
	NOOP
	NOOP
	NOOP
	NOOP
	NOOP
	/*
	 * move stuff to internal ram
	 */
	MOVW	$0x5003e000, R4
	MOVW	$_ramsize(SB), R6
	CMP	R6,R0
	BRA	EQ,nocopy

	MOVW	$edata(SB), R5
	SRL	$2, R6
	SUB	$1, R6
	BMOVW	R6, (R5)+, (R4)+

	MOVW	$edata(SB), R5
cloop:	DO	R6,clear
	MOVW	R0,(R5)+
clear:	DOEND	cloop

nocopy:
	/*
	 * set the clocks running
	 */
	MOVW	$0x04000000, R3
	MOVW	(R3), R4
	OR	$0x80, R4
	MOVW	R4, (R3)

	/*
	 * set _clock and clear the counters
	 */
	MOVW	$(9*4), R3
	MOVW	R3, _lastclock(SB)
	MOVW	$COUNTER, R3
	ADD	$0, R3, R4		/* clock/4 */
/*	ADD	$4, R3, R4		/* bus grants */
/*	ADD	$8, R3, R4		/* locked bus cycles */
/*	ADD	$12, R3	, R4		/* unlocked bus cycles */
	MOVW	$_clock(SB), R5
	MOVW	R4, (R5)
	MOVW	(R3)+, R0
	MOVW	(R3)+, R0
	MOVW	(R3)+, R0
	MOVW	(R3)+, R0

	CALL	_profmain(SB)

	MOVW	$(8*4), R4
	MOVW	R3, (R4)		/* stash pointer to links in well-known location */

	MOVW	__prof+4(SB), R3
	MOVW	R3, __prof+0(SB)

	MOVW	$main(SB), R4
	CALL	(R4)

	CALL	_exits(SB)

TEXT	_exits(SB), 1, $0
	CALL	_profdump(SB)
	MOVW	$(10*4), R4
	MOVW	R3, (R4)

loop:
	MOVW	$_profin(SB), R3	/* force loading of profile */
	JMP	loop

TEXT	_savearg(SB), 1, $0
	RETURN

TEXT	_callpc(SB), 1, $0
	ADD	$4, R3
	MOVW	(R3), R3
	RETURN
