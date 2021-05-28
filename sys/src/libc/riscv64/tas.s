/*
 *	risc-v test-and-set
 *	assumes the standard A extension
 */

#define ARG	8

#define MASK(w)	((1<<(w))-1)
#define FENCE	WORD $(0xf | MASK(8)<<20)  /* all i/o, mem ops before & after */
#define AQ	(1<<26)			/* acquire */
#define RL	(1<<25)			/* release */
#define LRW(rs1, rd) \
	WORD $((2<<27)|(    0<<20)|((rs1)<<15)|(2<<12)|((rd)<<7)|057|AQ)
#define SCW(rs2, rs1, rd) \
	WORD $((3<<27)|((rs2)<<20)|((rs1)<<15)|(2<<12)|((rd)<<7)|057|AQ|RL)

/* atomically set *keyp non-zero and return previous contents */
TEXT	_tas(SB), $-4		/* int _tas(ulong *keyp) */
	MOV	R(ARG), R12		/* address of key */
	MOV	$1, R10
	FENCE
tas1:
	LRW(12, ARG)			/* (R12) -> R(ARG) */
	SCW(10, 12, 14)			/* R10 -> (R12) maybe, R14=0 if ok */
	BNE	R14, tas1
	RET
