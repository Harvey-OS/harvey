typedef	long	Type;

/*
 * See the comments at the beginning of gbitblt.c
 * for an outline of how this bitblt works
 
 * Registers
 *   in addition to the registers of the abstract machine,
 *   we use RF to hold ~0 always, RX as a scratch register,
 *   and AT to hold the address of the table given in Inittab.
 */
#define As	5
#define Ad	6
#define	Rs	7
#define	Rd	8
#define Rt	9
#define Ru	10
#define Ri	11
#define Ro	12
#define RF	1	/* ~0 */
#define RX	2	/* scratch */
#define AT	3	/* conversion table */

/*
 * Macros for assembling MIPS instructions
 */

/* generate `a', `size' bits wide, into bit position `shift' */
/* the SMM version is used when there might be extra bits in `a' */
#define	SM(a,size,shift)	((a)<<(shift))
#define	SMM(a,size,shift)	(((a)&(1<<(size))-1)<<(shift))

/* Make sure im fits in 16 bits and sh fits in 5 bits */
#define Iinst(op,rs,rt,im)	(SM(op,6,26)|SM(rs,5,21)|SM(rt,5,16)|(im))
#define Rinst(op,rs,rt,rd,sh,f)	(SM(op,6,26)|SM(rs,5,21)|SM(rt,5,16)|SM(rd,5,11)|SM(sh,5,6)|(f))

/*
 * Instructions
 */

#define SLL(rs,rd,sh)	Rinst(0,0,rs,rd,sh,0)
#define SRL(rs,rd,sh)	Rinst(0,0,rs,rd,sh,2)
#define ADDU(rs,rt,rd)	Rinst(0,rs,rt,rd,0,041)
#define ADDIU(rs,rd,v)	Iinst(011,rs,rd,v)
#define AND(rs,rt,rd)	Rinst(0,rs,rt,rd,0,044)
#define ANDI(rs,rd,v)	Iinst(014,rs,rd,v)
#define OR(rs,rt,rd)	Rinst(0,rs,rt,rd,0,045)
#define ORI(rs,rd,v)	Iinst(015,rs,rd,v)
#define XOR(rs,rt,rd)	Rinst(0,rs,rt,rd,0,046)
#define XORI(rs,rd,v)	Iinst(016,rs,rd,v)
#define NOR(rs,rt,rd)	Rinst(0,rs,rt,rd,0,047)
#define NOP		Rinst(0,0,0,0,0,047)
#define LUI(r,v)	Iinst(017,0,r,v)
#define LW(as,rd)	Iinst(043,as,rd,0)
#define LBU(as,rd)	Iinst(044,as,rd,0)
#define LHU(as,rd)	Iinst(045,as,rd,0)
#define SW(ad,rs)	Iinst(053,ad,rs,0)
#define BGTZ(r,d)	Iinst(007,r,0,d)
#define JR(r)		Rinst(0,r,0,0,0,010)

/*
 * Macros for assembling the operations of the abstract machine.
 * Each assumes that ulong *p points to the next location where
 * an instruction should be assembled.
 * These macros can use RX as a scratch register, but no others.
 * They can assume RF holds ~0.
 */

#define Ofield(c)	*p++ = XOR(Rs,Rd,Rs);				\
			if((c)&0xFFFF0000) {				\
				*p++ =  LUI(RX,((ulong)(c))>>16);	\
				*p++ = ORI(RX,RX,(c)&0xFFFF);		\
				*p++ = AND(RX,Rs,Rs);			\
			} else						\
				*p++ = ANDI(Rs,Rs,(c));			\
			*p++ = XOR(Rs,Rd,Rs)

#define Olsha_RsRt	*p++ = SLL(Rt,Rs,sha)
#define Olshb_RsRt	*p++ = SLL(Rt,Rs,shb)
#define Olsh_RsRd(c)	*p++ = SLL(Rd,Rs,c)
#define Olsh_RtRt(c)	*p++ = SLL(Rt,Rt,c)
#define Olsha_RtRt	*p++ = SLL(Rt,Rt,sha)
#define Olsha_RtRu	*p++ = SLL(Ru,Rt,sha)
#define Olshb_RtRu	*p++ = SLL(Ru,Rt,shb)
#define Orsha_RsRt	*p++ = SRL(Rt,Rs,sha)
#define Orshb_RsRt	*p++ = SRL(Rt,Rs,shb)
#define Orsha_RtRu	*p++ = SRL(Ru,Rt,sha)
#define Orshb_RtRu	*p++ = SRL(Ru,Rt,shb)
#define Oorlsha_RsRt	*p++ = SLL(Rt,RX,sha); *p++ = OR(RX,Rs,Rs)
#define Oorlshb_RsRt	*p++ = SLL(Rt,RX,shb); *p++ = OR(RX,Rs,Rs)
#define Oorlsh_RsRd(c)	*p++ = SLL(Rd,RX,c); *p++ = OR(RX,Rs,Rs)
#define Oorrsha_RsRt	*p++ = SRL(Rt,RX,sha); *p++ = OR(RX,Rs,Rs)
#define Oorrshb_RsRt	*p++ = SRL(Rt,RX,shb); *p++ = OR(RX,Rs,Rs)
#define Oorrsha_RtRu	*p++ = SRL(Ru,RX,sha); *p++ = OR(RX,Rt,Rt)
#define Oorrshb_RtRu	*p++ = SRL(Ru,RX,shb); *p++ = OR(RX,Rt,Rt)
#define Oor_RsRd	*p++ = OR(Rs,Rd,Rs)

/*
 * Try to use smaller instructions when the constant is small.
 * Note that MIPS sign extends immediate operands
 */
#define Add_As(c)	if((c)&0xFFFF8000) { \
				if(!((c)&0xFFFF0000)) { \
					*p++ = ORI(0,RX,(c)&0xFFFF); \
					*p++ = ADDU(RX,As,As); \
				} else { \
					*p++ = LUI(RX,((ulong)(c))>>16); \
					*p++ = ORI(RX,RX,(c)&0xFFFF); \
					*p++ = ADDU(RX,As,As); \
				} \
			} else \
				*p++ = ADDIU(As,As,c)

#define Add_Ad(c)	if((c)&0xFFFF8000) { \
				if(!((c)&0xFFFF0000)) { \
					*p++ = ORI(0,RX,(c)&0xFFFF); \
					*p++ = ADDU(RX,Ad,Ad); \
				} else { \
					*p++ = LUI(RX,((ulong)(c))>>16); \
					*p++ = ORI(RX,RX,(c)&0xFFFF); \
					*p++ = ADDU(RX,Ad,Ad); \
				} \
			} else \
				*p++ = ADDIU(Ad,Ad,c)

#define Initsd(s,d)	*p++ = LUI(As,((ulong)(s))>>16); \
			*p++ = ORI(As,As,((ulong)(s))&0xFFFF); \
			*p++ = LUI(Ad,((ulong)(d))>>16); \
			*p++ = ORI(Ad,Ad,((ulong)(d))&0xFFFF)

#define Initsh(a,b)

/* Put all ones in RF */
#define Extrainit	*p++ = ADDIU(0,RF,0xFFFF)

/*
 * We put one less than the loop count into R[io], so that
 * the loop instructions can decrement after the test instead
 * of before
 */
#define Ilabel(c)	tmp = (c)-1; \
			if(tmp&0xFFFF8000) { \
				*p++ = LUI(Ri,((ulong)tmp)>>16); \
				*p++ = ORI(Ri,Ri,tmp&0xFFFF); \
			} else \
				*p++ = ADDIU(0,Ri,tmp)

#define Olabel(c)	tmp = (c)-1; \
			if(tmp&0xFFFF8000) { \
				*p++ = LUI(Ro,((ulong)tmp)>>16); \
				*p++ = ORI(Ro,Ro,tmp&0xFFFF); \
			} else \
				*p++ = ADDIU(0,Ro,tmp)

/*
 * The decrement is done after the test instead of before
 * so that the required delay slot can be filled with
 * the decrement (counts were adjusted by [IO]label)
 */
#define Iloop(lp)	*p = BGTZ(Ri,(lp-(p+1))&0xFFFF); p++; \
			*p++ = ADDIU(Ri,Ri,0xFFFF)

#define Oloop(lp)	*p = BGTZ(Ro,(lp-(p+1))&0xFFFF); p++; \
			*p++ = ADDIU(Ro,Ro,0xFFFF)

#define Orts		*p++ = JR(31); *p++ = NOP

/*
 * Load and Fetch macros: arg should be 1 if following instr might use loaded value
 * In the predecrement versions, it's as easy to do the decrment afterwards in
 * the delay slot
 */

#define Load_Rs_P	*p++ = LW(As,Rs); *p++ = ADDIU(As,As,4)
#define Load_Rt_P	*p++ = LW(As,Rt); *p++ = ADDIU(As,As,4)
#define Loadzx_Rt_P	*p++ = LW(As,Rt); *p++ = ADDIU(As,As,4)
#define Loador_Rt_P	*p++ = LW(As,Rt); *p++ = ADDIU(As,As,4)
#define Load_Ru_P	*p++ = LW(As,Ru); *p++ = ADDIU(As,As,4)
#define Load_Rd_D(f)	*p++ = ADDIU(As,As,-4&0xFFFF); *p++ = LW(As,Rd); \
			if(f) *p++ = NOP
#define Load_Rs_D(f)	*p++ = ADDIU(As,As,-4&0xFFFF); *p++ = LW(As,Rs); \
			if(f) *p++ = NOP
#define Load_Rt_D(f)	*p++ = ADDIU(As,As,-4&0xFFFF); *p++ = LW(As,Rt); \
			if(f) *p++ = NOP
#define Loadzx_Rt_D(f)	*p++ = ADDIU(As,As,-4&0xFFFF); *p++ = LW(As,Rt); \
			if(f) *p++ = NOP
#define Load_Rd(f)	*p++ = LW(As,Rd); if(f) *p++ = NOP
#define Load_Rs(f)	*p++ = LW(As,Rs); if(f) *p++ = NOP
#define Load_Rt(f)	*p++ = LW(As,Rt); if(f) *p++ = NOP
#define Loadzx_Rt(f)	*p++ = LW(As,Rt); if(f) *p++ = NOP
#define Fetch_Rd_P(f)	*p++ = LW(Ad,Rd); *p++ = ADDIU(Ad,Ad,4)
#define Fetch_Rd_D(f)	*p++ = ADDIU(Ad,Ad,-4&0xFFFF); *p++ = LW(Ad,Rd); \
			if(f) *p++ = NOP
#define Fetch_Rd(f)	*p++ = LW(Ad,Rd); if(f) *p++ = NOP
#define Store_Rs_P	*p++ = SW(Ad,Rs); *p++ = ADDIU(Ad,Ad,4)
#define Store_Rs_D	*p++ = ADDIU(Ad,Ad,-4&0xFFFF); *p++ = SW(Ad,Rs)
#define Store_Rs	*p++ = SW(Ad,Rs)
#define Nop		*p++ = NOP

#define Inittab(t,s)	*p++ = LUI(AT,((ulong)(t))>>16); \
			*p++ = ORI(AT,AT,((ulong)(t))&0xFFFF)

/* emit code to look up n bits at offset o; table entries are 1<<l bytes long */
#define Table_RdRt(o,n,l)			\
	tmp = 32-((o)+(n))-(l);			\
	if(tmp > 0)				\
		*p++ = SRL(Rt,Rd,tmp);		\
	else if((l) > 0)			\
		*p++ = SLL(Rt,Rd,l);		\
	else					\
		*p++ = ADDU(Rt,0,Rd);		\
	*p++ = ANDI(Rd,Rd,((1<<(n))-1)<<(l));	\
	*p++ = ADDU(Rd,AT,Rd);			\
	if(osiz==1) *p++ = LBU(Rd,Rd);		\
	else if(osiz==2) *p++ = LHU(Rd,Rd);	\
	else *p++ = LW(Rd,Rd);			\
	*p++ = NOP

#define Table_RsRt(o,n,l)			\
	tmp = 32-((o)+(n))-(l);			\
	if(tmp > 0)				\
		*p++ = SRL(Rt,Rs,tmp);		\
	else if((l) > 0)			\
		*p++ = SLL(Rt,Rs,l);		\
	else					\
		*p++ = ADDU(Rt,0,Rs);		\
	*p++ = ANDI(Rd,Rs,((1<<(n))-1)<<(l));	\
	*p++ = ADDU(Rs,AT,Rs);			\
	if(osiz==1) *p++ = LBU(Rs,Rs);		\
	else if(osiz==2) *p++ = LHU(Rs,Rs);	\
	else *p++ = LW(Rs,Rs);			\
	*p++ = NOP

/* emit code to assemble low n bits of Rd into offset o in Rs */
#define Assemble(o,n)				\
	if((o) == 0) {				\
		Olsh_RsRd(32-(n));		\
	} else if((o) == 32-(n)) {		\
		*p++ = OR(Rs,Rd,Rs);		\
	} else {				\
		Oorlsh_RsRd(32-((o)+(n)));	\
	}

/* emit code to assemble low n bits of Rd into offset o in Rs.
   this works by shifting Rd as we go, it only works if
   the whole word will eventually be filled */
#define Assemblex(o,n)				\
	if((o) == 0) {				\
		*p++ = ADDU(Rd,0,Rs);		\
	} else {				\
		*p++ = SLL(Rs,Rs,n);		\
		*p++ = OR(Rd,Rs,Rs);		\
	}

extern void	wbflush(void);
extern void	icflush(void *, int);
#define Execandfree(memstart,onstack)			\
	tmp = (p-memstart) * sizeof(Type);		\
	if(onstack) {					\
		icflush(memstart, tmp);			\
		(*(void (*)(void))memstart)();		\
	} else {					\
		wbflush();				\
		(*(void (*)(void))memstart)();		\
		bbfree(memstart, tmp);			\
	}

#define Emitop			\
	p[0] = fi[0];		\
	p[1] = fi[1];		\
	p = (Type*)(((char *)p)+fin)

typedef struct	Fstr
{
	char	fetchs;
	char	fetchd;
	short	n;
	Type	instr[2];
} Fstr;

Fstr	fstr[16] =
{
[0]	0,0,4,		/* Zero */
	{ LUI(Rs,0), 0 },

[1]	1,1,4,		/* DnorS */
	{ NOR(Rs,Rd,Rs), 0 },

[2]	1,1,8,		/* DandnotS */
	{ XOR(Rs,RF,Rs), AND(Rs,Rd,Rs) },

[3]	1,0,4,		/* notS */
	{ XOR(Rs,RF,Rs), 0 },

[4]	1,1,8,		/* notDandS */
	{ XOR(Rs,RF,Rs), NOR(Rd,Rs,Rs) },

[5]	0,1,4,		/* notD */
	{ XOR(Rd,RF,Rs), 0 },

[6]	1,1,4,		/* DxorS */
	{ XOR(Rd,Rs,Rs), 0 },

[7]	1,1,8,		/* DnandS */
	{ AND(Rd,Rs,Rs), XOR(Rs,RF,Rs) },

[8]	1,1,4,		/* DandS */
	{ AND(Rd,Rs,Rs), 0 },

[9]	1,1,8,		/* DxnorS */
	{ XOR(Rd,Rs,Rs), XOR(Rs,RF,Rs) },

[10]	0,1,4,		/* D */
	{ ADDU(Rd,0,Rs), 0 },

[11]	1,1,8,		/* DornotS */
	{ XOR(Rs,RF,Rs), OR(Rs,Rd,Rs) },

[12]	1,0,0,		/* S */
	{0, 0},

[13]	1,1,8,		/* notDorS */
	{ XOR(Rd,RF,RX), OR(Rs,RX,Rs) },

[14]	1,1,4,		/* DorS */
	{ OR(Rs,Rd,Rs), 0 },

[15]	0,0,4,		/* F */
	{ OR(RF,0,Rs), 0 },
};

#include "tabs.h"
static uchar *tabs[4][4] =
{
	{	     0, (uchar*)tab01, (uchar*)tab02, (uchar*)tab03},
	{(uchar*)tab10,		    0, (uchar*)tab12, (uchar*)tab13},
	{(uchar*)tab20, (uchar*)tab21,		   0, (uchar*)tab23},
	{(uchar*)tab30,	(uchar*)tab31, (uchar*)tab32,		  0},
};

static uchar tabosiz[4][4] = /* size in bytes of entries */
{
	{ 0, 2, 4, 4},
	{ 1, 0, 2, 4},
	{ 1, 1, 0, 2},
	{ 1, 1, 1, 0},
};

enum {
	Progmax = 900,		/* max number of Type units in a bitblt prog */
	Progmaxnoconv = 64,	/* max number of Type units when no conversion */
};

void
prprog(void)
{
	abort();	/* use db */
}
