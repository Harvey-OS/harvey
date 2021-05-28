typedef struct MMUMcrChain15 MMUMcrChain15;

/* Version of the 15 Chain for mcr access */
struct MMUMcrChain15 {
	uchar	rw;		/* 1bit */
	uchar	op1;		/* 3 bits */
	uchar	op2;		/* 3 bits */
	uchar	crn;		/* 4 bits */
	uchar	crm;		/* 4 bits */
	uchar	access;	/* 1 bit */
	u32int	data;		/* 32 bits */
};

enum {
	/* other sizes taken from icert.h */
	AccessSz		= 1,
	OpSz		= 3,
	CrxSz		= 4,
};

enum{
	ARMMCROP		= 0xee000010,
	ARMMRCLFLAG	= 0x00100000,
};

#define C(cr)	((cr)&0x7)
#define ARMMCR(cp, op1, rd, crn, crm, op2) (ARMMCROP | \
				crm | (op2 << 5) | (cp<<8) | \
				(rd<<12) | (crn<<16) | (op1<<21))


#define ARMMRC(cp, op1, rd, crn, crm, op2) (ARMMRCLFLAG | \
				ARMMCR(cp, op1, rd, crn, crm, op2))

extern int	mmurdregs(JMedium *jmed, MMURegs *regs);
extern char *	printmmuregs(MMURegs *mmuregs, char *s, int ssz);
