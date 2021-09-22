#define EBREAK	WORD	$(0x73 | 1<<20)
#define STIMER(rs,rd)	WORD	$(0xB | (rd)<<7 | (rs)<<15 | 0x5<<25 | 6<<12)

TEXT start(SB), $0
	/* set static base */
	MOVW	$setSB(SB), R3

	/* set stack pointer */
	MOVW	$(512*1024-16),R2

	/* clear bss */
	MOVW	$edata(SB), R1
	MOVW	$end(SB), R2
	MOVW	R0, 0(R1)
	ADD	$4, R1
	BLT	R2, R1, -2(PC)

#ifdef nope
	/* set watchdog timer */
	MOVW	$20000000, R10		/* 1 seconds */
	STIMER(10,10)
#endif

	/* but0 interrupt on rising edge */
	MOVW	$0x20020000, R1
	MOVW	$2, R2
	MOVW	R2, 0(R1)

	/* call main */
	JAL	R1, main(SB)

TEXT	abort(SB), $-4
	EBREAK
	RET
