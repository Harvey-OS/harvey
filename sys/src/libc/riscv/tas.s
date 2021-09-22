/*
 *	risc-v test-and-set
 *	assumes A extension
 *	LR/SC only work on cached regions
 */

#define ARG	8

#define SYNC	WORD $0xf		/* FENCE */
#define LRW(rs2, rs1, rd) \
	WORD $((2<<27)|(    0<<20)|((rs1)<<15)|(2<<12)|((rd)<<7)|057)
#define SCW(rs2, rs1, rd) \
	WORD $((3<<27)|((rs2)<<20)|((rs1)<<15)|(2<<12)|((rd)<<7)|057)

/* atomically set (RARG) non-zero and return previous contents */
	TEXT	_tas(SB), $-4
	MOV	R(ARG), R12		/* address of key */
	MOV	$1, R10
	SYNC
tas1:
	LRW(0, 12, ARG)			/* (R12) -> R(ARG) */
	SCW(10, 12, 14)			/* R10 -> (R12) maybe, R14=0 if ok */
	BNE	R14, tas1
	RET
