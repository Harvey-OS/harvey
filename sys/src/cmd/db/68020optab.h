#define	MAXRAND	4

struct m68020optab {
	ushort opcode;
	ushort mask;
	ushort op2;
	ushort mk2;
	char	*opname;
	char	flags;
	char	nrand;
	short	rand[MAXRAND];
};

/*
 * operand types
 */

#define	DSREG	00	/* special registers */
#define	DEA	01	/* E.A. to low order 6 bits */
#define	DRG	02	/* register to low order 3 bits */
#define	DRGL	03	/* register to bits 11-9 */
#define	DBR	04	/* branch offset (short) */
#define	DMQ	05	/* move-quick 8-bit value */
#define	DAQ	06	/* add-quick 3-bit value in 11-9 */
#define	DIM	07	/* Immediate value, according to size */
#define	DEAM	010	/* E.A. to bits 11-6 as in move */
#define	DBCC	011	/* branch address as in "dbcc" */
#define	DTRAP	012	/* immediate in low 4 bits */
#define D2L	013	/* register to bits 0-2 of next word */
#define D2H	014	/* register to bits 12-14 of next word */
#define DBL	015	/* qty in bits 0-5 of next word */
#define DBH	016	/* qty in bits 6-11 of next word */
#define DCR	017	/* control reg a bit combination in 0-11 */
#define	DBKPT	020	/* immediate in low three bits */
#define	DFSRC	021	/* floating source specifier */
#define	DFDRG	022	/* floating destination register */
#define	DFSRG	023	/* floating source register */
#define	DFCR	024	/* floating point constant register */
#define	DFBR	025	/* floating branch offset */
#define	DFMRGM	026	/* FMOVE register mask */
#define	DFMCRGM	027	/* FMOVEC register mask */
#define	DCH	030	/* cache indicator */
#define	DCHRGI	031	/* cache indicator plus indirect address register */

#define	DMASK	037

/*
 * operand flags
 */

#define	ADEC	0100	/* funny auto-decrement */
#define	AINC	0200	/* funny auto-increment */
#define	AAREG	0400	/* address register */
#define	ADREG	01000	/* data register */
#define	AONE	02000	/* immediate always 1 */
#define	AWORD	04000	/* immediate always two-byte */
#define	AIAREG	010000	/* indirect through address register */

/*
 * special registers
 */

#define	C	0100	/* the condition code register */
#define	SR	0200	/* the status register */
#define	U	0400	/* the user stack pointer */

/*
 * flags
 */

#define	B	0	/* byte */
#define	W	1	/* word */
#define	L	2	/* long */
#define	D	3	/* double float */
#define	F	4	/* single float */
#define	NZ	010	/* no size */
#define	SZ	017

#define I2W	020		/* two word instruction code */
