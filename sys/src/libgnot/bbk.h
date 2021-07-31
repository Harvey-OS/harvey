typedef	long	Type;

/*
 * See the comments at the beginning of bb.c and bbc.h
 * for an outline of how this bitblt works
 
 * Registers
 *   in addition to the registers of the abstract machine,
 *   we use RF to hold ~0 always, RX as a scratch register,
 *   Rc as a scratch register for forming constants, and
 *   AT to hold the address of the table given in Inittab.
 */
#define	R0	0
#define As	19
#define Ad	20
#define	Rs	21
#define	Rd	22
#define Rt	23
#define Ru	24
#define Ri	25
#define Ro	26
#define	Rc	27	/* const temporary */
#define RF	16	/* ~0 */
#define RX	17	/* scratch */
#define AT	18	/* conversion table */

/*
 * Instructions
 */
/* Inst2a */
#define	SETHI	0x04
/* Inst2b */
#define	BG	0x0A
/* Inst3 2 */
#define	ADD	0x00
#define	SUB	0x04
#define	AND	0x01
#define	OR	0x02
#define	XOR	0x03
#define	ANDN	0x05
#define	ORN	0x06
#define	SUBcc	0x14
#define	SLL	0x25
#define	SRL	0x26
#define	JMPL	0x38
/* Inst3 3 */
#define	LD	0x00
#define	LDUB	0x01
#define	LDUH	0x02
#define	ST	0x04

/*
 * Generate `a', `size' bits wide, into bit position `shift'.
 * (to save execution time, the onus is on the caller to make sure that
 * `a' fits in `size')
 */
#define	SM(a,size,shift)	((a)<<(shift))

#define	Inst2a(rd,op2,i)	(SM(rd,5,25)|SM(op2,4,22)|SM(i,22,0))
#define	Inst2b(a,cond,op2,d)	Inst2a(SM(a,1,4)|SM(cond,4,0), op2, d)

#define	Inst3X(op,rd,op3,rs1)	(SM(op,2,30)|SM(rd,5,25)|SM(op3,6,19)|SM(rs1,5,14))
#define	Inst3a(op,rd,op3,rs1,i,asi,rs2)	(Inst3X(op,rd,op3,rs1)|SM(i,1,13)|SM(asi,8,5)|SM(rs2,5,0))
#define	Inst3b(op,rd,op3,rs1,i,si)	(Inst3X(op,rd,op3,rs1)|SM(i,1,13)|SM(si,13,0))
#define	Inst3c(op,rd,op3,rs1,opf,rs2)	(Inst3X(op,rd,op3,rs1)|SM(opf,9,5)|SM(rs2,5,0))

#define	OpConst(Rdst,Rsrc,op,c)		/* Rdst = Rsrc op c */ \
			if(((ulong)(c)) <= 0xFFF) \
				*p++ = Inst3b(2,Rdst,op,Rsrc,1,(c)); \
			else { \
				RConst(Rc,(c)); \
				*p++ = Inst3a(2,Rdst,op,Rsrc,0,0,Rc); \
			}	

#define	RConst(Rdst,c)			/* Rdst = c; c is large */ \
			*p++ = Inst2a(Rdst,SETHI,((ulong)(c))>>10); \
			*p++ = Inst3b(2,Rdst,OR,Rdst,1,(c)&0x3FF)


/*
 * Macros for assembling the operations of the abstract machine.
 * Each assumes that ulong *p points to the next location where
 * an instruction should be assembled.
 * These macros can use RX as a scratch register, but no others.
 * They can assume RF holds ~0 and R0 holds 0.
 */

#define Ofield(c)	*p++ = Inst3a(2,Rs,XOR,Rs,0,0,Rd); \
			RConst(Rc,(c)); \
			*p++ = Inst3a(2,Rs,AND,Rs,0,0,Rc); \
			*p++ = Inst3a(2,Rs,XOR,Rs,0,0,Rd)

#define Olsha_RsRt	*p++ = Inst3a(2,Rs,SLL,Rt,1,0,sha)
#define Olshb_RsRt	*p++ = Inst3a(2,Rs,SLL,Rt,1,0,shb)
#define Olsh_RsRd(c)	*p++ = Inst3a(2,Rs,SLL,Rd,1,0,(c))
#define Olsh_RtRt(c)	*p++ = Inst3a(2,Rt,SLL,Rt,1,0,(c))
#define Olsha_RtRt	*p++ = Inst3a(2,Rt,SLL,Rt,1,0,sha)
#define Olsha_RtRu	*p++ = Inst3a(2,Rt,SLL,Ru,1,0,sha)
#define Olshb_RtRu	*p++ = Inst3a(2,Rt,SLL,Ru,1,0,shb)
#define Orsha_RsRt	*p++ = Inst3a(2,Rs,SRL,Rt,1,0,sha)
#define Orshb_RsRt	*p++ = Inst3a(2,Rs,SRL,Rt,1,0,shb)
#define Orsha_RtRu	*p++ = Inst3a(2,Rt,SRL,Ru,1,0,sha)
#define Orshb_RtRu	*p++ = Inst3a(2,Rt,SRL,Ru,1,0,shb)
#define Oorlsha_RsRt	*p++ = Inst3a(2,RX,SLL,Rt,1,0,sha); \
			*p++ = Inst3a(2,Rs,OR,Rs,0,0,RX)
#define Oorlshb_RsRt	*p++ = Inst3a(2,RX,SLL,Rt,1,0,shb); \
			*p++ = Inst3a(2,Rs,OR,Rs,0,0,RX)
#define Oorlsh_RsRd(c)	*p++ = Inst3a(2,RX,SLL,Rd,1,0,(c)); \
			*p++ = Inst3a(2,Rs,OR,Rs,0,0,RX)
#define Oorrsha_RsRt	*p++ = Inst3a(2,RX,SRL,Rt,1,0,sha); \
			*p++ = Inst3a(2,Rs,OR,Rs,0,0,RX)
#define Oorrshb_RsRt	*p++ = Inst3a(2,RX,SRL,Rt,1,0,shb); \
			*p++ = Inst3a(2,Rs,OR,Rs,0,0,RX)
#define Oorrsha_RtRu	*p++ = Inst3a(2,RX,SRL,Ru,1,0,sha); \
			*p++ = Inst3a(2,Rt,OR,Rt,0,0,RX)
#define Oorrshb_RtRu	*p++ = Inst3a(2,RX,SRL,Ru,1,0,shb); \
			*p++ = Inst3a(2,Rt,OR,Rt,0,0,RX)
#define Oor_RsRd	*p++ = Inst3a(2,Rs,OR,Rd,0,0,Rs)

#define Add_As(c)	OpConst(As,As,ADD,(c))

#define Add_Ad(c)	OpConst(Ad,Ad,ADD,(c))

#define Initsd(s,d)	RConst(As,((ulong)(s))); \
			RConst(Ad,((ulong)(d)))

#define Initsh(a,b)

/* Put all ones in RF */
#define Extrainit	*p++ = Inst3b(2,RF,ORN,R0,1,0)

#define Ilabel(c)	RConst(Ri,(c))

#define Olabel(c)	RConst(Ro,(c))

#define Iloop(lp)	*p++ = Inst3b(2,Ri,SUBcc,Ri,1,1); \
			*p   = Inst2b(0,BG,0x2,((lp)-p)&0x3FFFFF); p++; \
			Nop

#define Oloop(lp)	*p++ = Inst3b(2,Ro,SUBcc,Ro,1,1); \
			*p   = Inst2b(0,BG,0x2,((lp)-p)&0x3FFFFF); p++; \
			Nop

#define Orts		*p++ = Inst3b(2,R0,JMPL,15,1,8); \
			Nop

/*
 * In the predecrement versions, it's as easy to do the decrement afterwards
 * in the (virtual) delay slot, which will go faster on some versions
 */

#define Load_Rs_P	*p++ = Inst3b(3,Rs,LD,As,1,0); *p++ = Inst3b(2,As,ADD,As,1,4)
#define Load_Rt_P	*p++ = Inst3b(3,Rt,LD,As,1,0); *p++ = Inst3b(2,As,ADD,As,1,4)
#define Loadzx_Rt_P	*p++ = Inst3b(3,Rt,LD,As,1,0); *p++ = Inst3b(2,As,ADD,As,1,4)
#define Loador_Rt_P	*p++ = Inst3b(3,Rt,LD,As,1,0); *p++ = Inst3b(2,As,ADD,As,1,4)
#define Load_Ru_P	*p++ = Inst3b(3,Ru,LD,As,1,0); *p++ = Inst3b(2,As,ADD,As,1,4)
#define Load_Rd_D(f)	*p++ = Inst3b(3,Rd,LD,As,1,(-4)&0x1FFF); *p++ = Inst3b(2,As,SUB,As,1,4)
#define Load_Rs_D(f)	*p++ = Inst3b(3,Rs,LD,As,1,(-4)&0x1FFF); *p++ = Inst3b(2,As,SUB,As,1,4)
#define Load_Rt_D(f)	*p++ = Inst3b(3,Rt,LD,As,1,(-4)&0x1FFF); *p++ = Inst3b(2,As,SUB,As,1,4)
#define Loadzx_Rt_D(f)	*p++ = Inst3b(3,Rt,LD,As,1,(-4)&0x1FFF); *p++ = Inst3b(2,As,SUB,As,1,4)
#define Load_Rd(f)	*p++ = Inst3b(3,Rd,LD,As,1,0)
#define Load_Rs(f)	*p++ = Inst3b(3,Rs,LD,As,1,0)
#define Load_Rt(f)	*p++ = Inst3b(3,Rt,LD,As,1,0)
#define Loadzx_Rt(f)	*p++ = Inst3b(3,Rt,LD,As,1,0)
#define Fetch_Rd_P(f)	*p++ = Inst3b(3,Rd,LD,Ad,1,0); *p++ = Inst3b(2,Ad,ADD,Ad,1,4)
#define Fetch_Rd_D(f)	*p++ = Inst3b(3,Rd,LD,Ad,1,(-4)&0x1FFF); *p++ = Inst3b(2,Ad,SUB,Ad,1,4)
#define Fetch_Rd(f)	*p++ = Inst3b(3,Rd,LD,Ad,1,0);
#define Store_Rs_P	*p++ = Inst3b(3,Rs,ST,Ad,1,0); *p++ = Inst3b(2,Ad,ADD,Ad,1,4)
#define Store_Rs_D	*p++ = Inst3b(3,Rs,ST,Ad,1,(-4)&0x1FFF); *p++ = Inst3b(2,Ad,SUB,Ad,1,4)
#define Store_Rs	*p++ = Inst3b(3,Rs,ST,Ad,1,0)
#define Nop		*p++ = Inst3a(2,R0,OR,R0,0,0,R0)

#define Inittab(t,s)	RConst(AT,((ulong)(t)))

/* emit code to look up n bits at offset o; table entries are 1<<l bytes long */
#define Table_RdRt(o,n,l)					\
	tmp = 32-((o)+(n))-(l);					\
	if(tmp >= 0)						\
		*p++ = Inst3a(2,Rd,SRL,Rt,1,0,tmp); 		\
	else if((l) > 0)					\
		*p++ = Inst3a(2,Rd,SLL,Rt,1,0,l); 		\
	else							\
		*p++ = Inst3a(2,Rd,ADD,Rt,0,0,R0);		\
	*p++ = Inst3b(2,Rd,AND,Rd,1,((1<<(n))-1)<<(l));		\
	if(osiz==1) *p++ = Inst3b(3,Rd,LDUB,Rd,0,AT);		\
	else if(osiz==2) *p++ = Inst3b(3,Rd,LDUH,Rd,0,AT);	\
	else *p++ = Inst3b(3,Rd,LD,Rd,0,AT);			\

#define Table_RsRt(o,n,l)					\
	tmp = 32-((o)+(n))-(l);					\
	*p++ = Inst3a(2,Rs,SRL,Rt,1,0,tmp); 			\
	if(tmp >= 0)						\
		*p++ = Inst3a(2,Rs,SRL,Rt,1,0,tmp); 		\
	else if((l) > 0)					\
		*p++ = Inst3a(2,Rs,SLL,Rt,1,0,l); 		\
	else							\
		*p++ = Inst3a(2,Rs,ADD,Rt,0,0,R0);		\
	if(osiz==1) *p++ = Inst3b(3,Rs,LDUB,Rs,0,AT);		\
	else if(osiz==2) *p++ = Inst3b(3,Rs,LDUH,Rs,0,AT);	\
	else *p++ = Inst3b(3,Rs,LD,Rs,0,AT);			\

/* emit code to assemble low n bits of Rd into offset o in Rs */
#define Assemble(o,n)				\
	if((o) == 0) {				\
		Olsh_RsRd(32-(n));		\
	} else if((o) == 32-(n)) {		\
		*p++ = Inst3a(2,Rs,OR,Rs,0,0,Rd); \
	} else {				\
		Oorlsh_RsRd(32-((o)+(n)));	\
	}

/* emit code to assemble low n bits of Rd into offset o in Rs.
   this works by shifting Rd as we go, it only works if
   the whole word will eventually be filled */
#define Assemblex(o,n)				\
	if((o) == 0) {				\
		*p++ = Inst3a(2,Rs,OR,Rd,0,0,R0); \
	} else {				\
		*p++ = Inst3a(2,Rs,SLL,Rs,1,0,n); \
		*p++ = Inst3a(2,Rs,OR,Rd,0,0,Rs); \
	}

#define Execandfree(memstart,onstack)				\
	(*(void (*)(void))memstart)();				\
	if(!onstack)						\
		bbfree(memstart, (p-memstart) * sizeof(Type));

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
	{ Inst3a(2,Rs,ADD,R0,0,0,R0), 0 },

[1]	1,1,8,		/* DnorS */
	{ Inst3a(2,Rs,OR,Rs,0,0,Rd), Inst3a(2,Rs,XOR,Rs,0,0,RF) },

[2]	1,1,4,		/* DandnotS */
	{ Inst3a(2,Rs,ANDN,Rd,0,0,Rs), 0 },

[3]	1,0,4,		/* notS */
	{ Inst3a(2,Rs,XOR,Rs,0,0,RF), 0 },

[4]	1,1,4,		/* notDandS */
	{ Inst3a(2,Rs,ANDN,Rs,0,0,Rd), 0 },

[5]	0,1,4,		/* notD */
	{ Inst3a(2,Rs,XOR,Rd,0,0,RF), 0 },

[6]	1,1,4,		/* DxorS */
	{ Inst3a(2,Rs,XOR,Rd,0,0,Rs), 0 },

[7]	1,1,8,		/* DnandS */
	{ Inst3a(2,Rs,AND,Rd,0,0,Rs), Inst3a(2,Rs,XOR,Rs,0,0,RF) },

[8]	1,1,4,		/* DandS */
	{ Inst3a(2,Rs,AND,Rd,0,0,Rs), 0 },

[9]	1,1,8,		/* DxnorS */
	{ Inst3a(2,Rs,XOR,Rd,0,0,Rs), Inst3a(2,Rs,XOR,Rs,0,0,RF) },

[10]	0,1,4,		/* D */
	{ Inst3a(2,Rs,ADD,Rd,0,0,R0), 0 },

[11]	1,1,4,		/* DornotS */
	{ Inst3a(2,Rs,ORN,Rd,0,0,Rs), 0 },

[12]	1,0,0,		/* S */
	{0, 0},

[13]	1,1,4,		/* notDorS */
	{ Inst3a(2,Rs,ORN,Rs,0,0,Rd), 0 },

[14]	1,1,4,		/* DorS */
	{ Inst3a(2,Rs,OR,Rd,0,0,Rs), 0 },

[15]	0,0,4,		/* F */
	{ Inst3a(2,Rs,OR,R0,0,0,RF), 0 },
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
	Progmax = 1000,		/* max number of bytes in a bitblt prog */
	Progmaxnoconv = 70,	/* max number of Type units when no conversion */
};


#ifdef TEST
void
prprog(void)
{
	abort();	/* use db */
}

#endif
