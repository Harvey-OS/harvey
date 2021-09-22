/*
 *	risc-v test-and-set
 */
#include <atom.h>

#define ARG	8

/*
 * atomically set *keyp non-zero and return previous contents.
 * the mips code does LL/SC in a loop.
 */
TEXT _tas(SB), $-4			/* int _tas(ulong *keyp) */
	MOV	R(ARG), R12		/* address of key */
	MOV	$1, R10
#ifdef NEWWAY
	FENCE
	AMOW(Amoswap, AQ|RL, 10, 12, ARG) /* R10 -> (R12), old (R12) -> ARG */
#else
tas1:
	FENCE
	LRW(12, ARG)			/* (R12) -> R(ARG) */
	SCW(10, 12, 14)			/* R10 -> (R12) maybe, R14=0 if ok */
	BNE	R14, tas1
#endif
	FENCE
	RET
