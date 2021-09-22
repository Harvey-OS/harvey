/*
 * atomic memory operations and fences for rv64a
 *
 *	assumes the standard A extension
 *	LR/SC only work on cached regions
 */
#define	Amoadd	0
#define	Amoswap	1
#define	Amolr	2
#define	Amosc	3
#define	Amoxor	4
#define	Amoor	010
#define	Amoand	014
#define	Amomin	020
#define	Amomax	024
#define	Amominu	030
#define	Amomaxu	034

/* AMO operand widths */
#define	Amow	2		/* long */
#define	Amod	3		/* vlong */

/* setting AQ and RL produces sequential consistency */
#define AQ	(1<<26)			/* acquire */
#define RL	(1<<25)			/* release */

/* instructions unknown to the assembler */
/* atomically (rd = (rs1); (rs1) = rd func rs2) */
#define AMOW(func, opts, rs2, rs1, rd) \
	WORD $(((func)<<27)|((rs2)<<20)|((rs1)<<15)|(Amow<<12)|((rd)<<7)|057|(opts))
/* these choices of AQ and RL produce sequential consistency */
#define LRW(rs1, rd)		AMOW(Amolr, AQ, 0, rs1, rd)
#define SCW(rs2, rs1, rd)	AMOW(Amosc, AQ|RL, rs2, rs1, rd)

/* all i/o, mem, r & w ops before & after */
#define FENCE	WORD $(0xf | 0xff<<20)
#define PAUSE	WORD $(0xf | 1<<24)	/* FENCE pred=W, Zihintpause ext'n */
#define FENCE_I	WORD $(0xf | 1<<12)
#define SFENCE_VMA(rs2, rs1) WORD $(011<<25|(rs2)<<20|(rs1)<<15|0<<7|SYSTEM)

#define SEXT_W(r) ADDW R0, r

/* rv64 instructions */
#define AMOD(func, opts, rs2, rs1, rd) \
	WORD $(((func)<<27)|((rs2)<<20)|((rs1)<<15)|(Amod<<12)|((rd)<<7)|057|(opts))
#define LRD(rs1, rd)		AMOD(Amolr, AQ, 0, rs1, rd)
#define SCD(rs2, rs1, rd)	AMOD(Amosc, AQ|RL, rs2, rs1, rd)

#define	SLLI64(rs2, rs1, rd) \
	WORD $((rs2)<<20 | (rs1)<<15 | 1<<12 | (rd)<<7 | 0x13)
#define	SRLI64(rs2, rs1, rd) \
	WORD $((rs2)<<20 | (rs1)<<15 | 5<<12 | (rd)<<7 | 0x13)
