/*
 * PowerPC 405 definitions
 */
#include "mem.h"

#undef	PGROUND
#define PGROUND(s)	ROUNDUP(s, MiB)

/*
 * Special Purpose Registers of interest here
 */

#define SPR_SRR0	26		/* Save/Restore Register 0 */
#define SPR_SRR1	27		/* Save/Restore Register 1 */
#define SPR_TBU		85		/* Time Base Upper */
#define SPR_SPRG0	272		/* SPR General 0 */
#define SPR_SPRG1	273		/* SPR General 1 */
#define SPR_SPRG2	274		/* SPR General 2 */
#define SPR_SPRG3	275		/* SPR General 3 */
#define SPR_TBL		284		/* Time Base Lower */
#define	SPR_PVR		287		/* Processor Version */
#define SPR_IVOR(n)	(400+(n))	/* Interrupt Vector Offset 0-15 */
#define SPR_ZPR		944		/* Zone Protection Register */
#define SPR_PID		945		/* Process Identity */
#define SPR_MMUCR	946		/* MMU Control */
#define SPR_CCR0	947		/* Core Configuration Register 0 */
#define SPR_IAC3	948		/* Instruction Address Compare 3 */
#define SPR_IAC4	949		/* Instruction Address Compare 4 */
#define SPR_DVC1	950		/* Data Value Compare 1 */
#define SPR_DVC2	951		/* Data Value Compare 2 */
#define SPR_SGR		953		/* Store Guarded Register */
#define SPR_DCWR	954		/* Data Cache Write-through Register */
#define SPR_SLER	955		/* Storage Little Endian Register */
#define SPR_SU0R	956		/* Storage User-defined 0 Register */
#define SPR_DBCR1	957		/* Debug Control Register 1 */
#define SPR_ICDBDR	979		/* Instruction Cache Debug Data Register */
#define SPR_ESR		980		/* Exception Syndrome Register */
#define SPR_DEAR	981		/* Data Error Address Register */
#define SPR_EVPR	982		/* Exception Vector Prefix Register */
#define SPR_TSR		984		/* Time Status Register */
#define SPR_TCR		986		/* Time Control Register */
#define SPR_PIT		987		/* Programmable Interval Timer */
#define SPR_SRR2	990		/* Save/Restore Register 2 */
#define SPR_SRR3	991		/* Save/Restore Register 3 */
#define SPR_DBSR	1008		/* Debug Status Register */
#define SPR_DBCR0	1010		/* Debug Control Register 0 */
#define SPR_IAC1	1012		/* Instruction Address Compare 1 */
#define SPR_IAC2	1013		/* Instruction Address Compare 2 */
#define SPR_DAC1	1014		/* Data Address Compare 1 */
#define SPR_DAC2	1015		/* Data Address Compare 2 */
#define SPR_DCCR	1018		/* Data Cache Cachability Register */
#define SPR_ICCR	1019		/* Instruction Cache Cachability Register */

/*
 * Bit encodings for Memory Management Unit Control Register (MMUCR)
 */
#define MMUCR_STS	0x00010000	/* search translation space */
#define MMUCR_IULXE	0x00040000	/* instruction cache unlock exception enable */
#define MMUCR_DULXE	0x00080000	/* data cache unlock exception enable */
#define MMUCR_U3L2SWOAE	0x00100000	/* U3 L3 store without allocate enable */
#define MMUCR_U2SWOAE	0x00200000	/* U2 store without allocate enable */
#define MMUCR_U1TE	0x00400000	/* U1 transient enable */
#define MMUCR_SWOA	0x01000000	/* store without allocate */
#define MMUCR_L2SWOA	0x02000000	/* L2 store without allocate */
