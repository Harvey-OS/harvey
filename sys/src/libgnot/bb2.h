typedef	ushort	Type;

/*
 * See the comments at the beginning of bb.c and bbc.h
 * for an outline of how this bitblt works
 
 * Registers
 *   in addition to the registers of the abstract machine,
 *   we use RY as a scratch register, and AT to hold the
 *   address of the table given in Inittab.
 */
/* A registers */
#define As	0
#define Ad	1
#define AT	2	/* conversion table */
#define Ao	3	/* Ro in abstract machine */
#define Au	4	/* Ru in abstract machine */
/* D registers */
#define	Rs	0
#define	Rd	1
#define Rt	2
#define Ri	3
#define Ra	4	/* shift a */
#define Rb	5	/* shift b */
#define RX	6	/* scratch */
#define RY	7	/* scratch */

/*
 * Addressing modes
 */
/* use Rx by itself to get Dn mode */
#define AR(n)		((1<<3)|n)	/* An */
#define AI(n)		((2<<3)|n)	/* (An) */
#define AIPOST(n)	((3<<3)|n)	/* (An)+ */
#define AIPRE(n)	((4<<3)|n)	/* -(An) */
#define AID(n)		((5<<3)|n)	/* d16(An) */
#define AIX(n)		((6<<3)|n)	/* (An+Xm) with following word */
#define AIXEXT(m)	((m<<12)|(1<<11))		/* scale factor 1 */
#define AIX2EXT(m)	((m<<12)|(1<<11)|(1<<9))	/* scale factor 2 */
#define AIX4EXT(m)	((m<<12)|(1<<11)|(2<<9))	/* scale factor 4 */
#define IMML		((7<<3)|4)	/* immediate long (following) */
#define ABSW		((7<<3)|0)	/* absolute short address */

/* generate `a', `size' bits wide, into bit position `shift' */
/* the SMM version is used when there might be extra bits in `a' */
#define	SM(a,size,shift)	((a)<<(shift))
#define	SMM(a,size,shift)	(((a)&(1<<(size))-1)<<(shift))
/*
 * Instructions
 *   They operate on long words, unless suffixed by 'w' or 'b'
 *   The 'I' instructions must be followed by immediate data (bigendian)
 */

#define MOVE(dmode,smode) (SM(2,4,12)|SMM(dmode,3,9)|SMM((dmode>>3),3,6)|SM(smode,6,0))
#define MOVEw(dmode,smode) (SM(3,4,12)|SMM(dmode,3,9)|SMM((dmode>>3),3,6)|SM(smode,6,0))
#define MOVEb(dmode,smode) (SM(1,4,12)|SMM(dmode,3,9)|SMM((dmode>>3),3,6)|SM(smode,6,0))
#define MOVEQ(dreg,v)	(SM(0x7,4,12)|SM(dreg,3,9)|SM(v,8,0))
	/* MOVEQ sign-extends the 8-bit value; make sure it is masked to 8 bits */

#define AND(dreg,smode)	(SM(0xC,4,12)|SM(dreg,3,9)|SM(2,3,6)|SM(smode,6,0))
#define ANDI(dmode)	(SM(0x02,8,8)|SM(2,2,6)|SM(dmode,6,0))
#define ANDIw(dmode)	(SM(0x02,8,8)|SM(1,2,6)|SM(dmode,6,0))

/* the 68020 gnot.h defines an OR */
#undef OR
#define OR(dreg,smode)	(SM(0x8,4,12)|SM(dreg,3,9)|SM(2,3,6)|SM(smode,6,0))
#define XOR(dmode,sreg)	(SM(0xB,4,12)|SM(sreg,3,9)|SM(6,3,6)|SM(dmode,6,0))
#define NOT(dmode)	(SM(0x46,8,8)|SM(2,2,6)|SM(dmode,6,0))
#define CLR(dmode)	(SM(0x42,8,8)|SM(2,2,6)|SM(dmode,6,0))

/* LS[LR] can shift 1-8 (n==8 gets smashed to 0, which is the right encoding) */
/* else have to put shift amount in reg and use LS[LR]R */
#define LSL(dreg,n)	(SM(0xE,4,12)|SMM(n,3,9)|SM(1,1,8)|SM(0x11,5,3)|SM(dreg,3,0))
#define LSLR(dreg,rn)	(SM(0xE,4,12)|SM(rn,3,9)|SM(1,1,8)|SM(0x15,5,3)|SM(dreg,3,0))

#define LSR(dreg,n)	(SM(0xE,4,12)|SMM(n,3,9)|SM(0,1,8)|SM(0x11,5,3)|SM(dreg,3,0))
#define LSRR(dreg,rn)	(SM(0xE,4,12)|SM(rn,3,9)|SM(0,1,8)|SM(0x15,5,3)|SM(dreg,3,0))

#define ADDA(dareg,smode) (SM(0xD,4,12)|SM(dareg,3,9)|SM(7,3,6)|SM(smode,6,0))
#define ADDQ(dmode,v)	(SM(0x5,4,12)|SMM(v,3,9)|SM(0,1,8)|SM(2,2,6)|SM(dmode,6,0))
#define SUBQ(dmode,v)	(SM(0x5,4,12)|SMM(v,3,9)|SM(1,1,8)|SM(2,2,6)|SM(dmode,6,0))
#define LEA(dareg,smode) (SM(0x4,4,12)|SM(dareg,3,9)|SM(7,3,6)|SM(smode,6,0))

/* BFEXTU and BFINS each must be followed by BFIELD */
#define BFEXTU(smode)	(SM(0xE9,8,8)|SM(3,2,6)|SM(smode,6,0))
#define BFINS(dmode)	(SM(0xEF,8,8)|SM(3,2,6)|SM(dmode,6,0))

/* low part of r is dst for BFEXTU, src for BFINS */
/* offset is from high order bit; w==0 means width of 32 */
#define BFIELD(r,o,w)	(SM(r,3,12)|SM(o,5,6)|SMM(w,5,0))

#define DBF(countreg)	(SM(0x5,4,12)|SM(0x39,6,3)|SM(countreg,3,0))

#define BCC(cond,d8)	(SM(0x6,4,12)|SM(cond,4,8)|SMM(d8,8,0))

#define RTS		0x4E75

/*
 * Macros for assembling the operations of the abstract machine.
 * Each assumes that ushort *p points to the next location where
 * an instruction should be assembled.
 * These macros can use RY as a scratch register, but no others.
 */

#define Two(h,l)	*(long *)p = (long)(((h)<<16)|(l)); p += 2
#define Lsh(r,n)	if(n != 0) { \
				if(n <= 8) \
					*p++ = LSL(r,n); \
				else { \
					*p++ = MOVEQ(RX,n); *p++ = LSLR(r,RX); \
				} \
			}
#define Rsh(r,n)	if(n != 0) { \
				if(n <= 8) \
					*p++ = LSR(r,n); \
				else { \
					*p++ = MOVEQ(RX,n); *p++ = LSRR(r,RX); \
				} \
			}
#define Lsha(r)		*p++ = LSLR(r,Ra)
#define Lshb(r)		*p++ = LSLR(r,Rb)
#define Rsha(r)		*p++ = LSRR(r,Ra)
#define Rshb(r)		*p++ = LSRR(r,Rb)
#define Andcon(r,c)	*p++ = ANDI(r); *(long *)p = (long)(c); p += 2
#define Movecon(r,c)	*p++ = MOVE(r,IMML); *(long *)p = (long)(c); p += 2
#define Moveacon(dareg,c) *p++ = MOVE(AR(dareg),IMML);  *(long *)p = (long)(c); p += 2
/* Addacon assumes value added will fit in a short */
#define Addacon(dareg,c) *p++ = LEA(dareg,AID(dareg)); *p++ = c
/* Bcc assumes distance to branch is less than short offset */
#define Bcc(cond,lp)	*p++ = BCC(cond,0); *p++ = ((char*)(lp))-(((char*)p))
#define Ofield(c)	*p++ = XOR(Rs,Rd); Andcon(Rs,c); *p++ = XOR(Rs,Rd)
#define Olsha_RsRt	*p++ = MOVE(Rs,Rt); Lsha(Rs)
#define Olshb_RsRt	*p++ = MOVE(Rs,Rt); Lshb(Rs)
#define Olsh_RsRd(c)	*p++ = MOVE(Rs,Rd); Lsh(Rs,c)
#define Olsh_RtRt(c)	Lsh(Rt,c)
#define Olsha_RtRt	Lsha(Rt)
#define Olsha_RtRu	Two(MOVE(Rt,AR(Au)),LSLR(Rt,Ra))
#define Olshb_RtRu	Two(MOVE(Rt,AR(Au)),LSLR(Rt,Rb))
#define Orsha_RsRt	Two(MOVE(Rs,Rt),LSRR(Rs,Ra))
#define Orshb_RsRt	Two(MOVE(Rs,Rt),LSRR(Rs,Rb))
#define Orsha_RtRu	Two(MOVE(Rt,AR(Au)),LSRR(Rt,Ra))
#define Orshb_RtRu	Two(MOVE(Rt,AR(Au)),LSRR(Rt,Rb))
#define Oorlsha_RsRt	Two(MOVE(RY,Rt),LSLR(RY,Ra)); *p++ = OR(Rs,RY)
#define Oorlshb_RsRt	Two(MOVE(RY,Rt),LSLR(RY,Rb)); *p++ = OR(Rs,RY)
#define Oorlsh_RsRd(c)	*p++ = MOVE(RY,Rd); Lsh(RY,c); *p++ = OR(Rs,RY)
#define Oorrsha_RsRt	Two(MOVE(RY,Rt),LSRR(RY,Ra)); *p++ = OR(Rs,RY)
#define Oorrshb_RsRt	Two(MOVE(RY,Rt),LSRR(RY,Rb)); *p++ = OR(Rs,RY)
#define Oorrsha_RtRu	Two(MOVE(RY,AR(Au)),LSRR(RY,Ra)); *p++ = OR(Rt,RY)
#define Oorrshb_RtRu	Two(MOVE(RY,AR(Au)),LSRR(RY,Rb)); *p++ = OR(Rt,RY)
#define Oor_RsRd	*p++ = OR(Rs,Rd)
#define Add_As(c)	Addacon(As,c)
#define Add_Ad(c)	Addacon(Ad,c)
#define Initsd(s,d)	*p++ = MOVE(AR(As),IMML); *(long *)p = (long) (s); p += 2; \
			*p++ = MOVE(AR(Ad),IMML); *(long *)p = (long) (d); p += 2
#define Initsh(a,b)	*p++ = MOVEQ(Ra,a); *p++ = MOVEQ(Rb,b)

/*
 * use DBF instruction. assume counts < 16k
 */
#define Ilabel(c)	tmp = (c)-1; Movecon(Ri,tmp)
#define Olabel(c)	Moveacon(Ao,(c))
#define Iloop(lp)	*p++ = DBF(Ri); *p++ = ((char*)(lp))-((char*)p)
#define Oloop(lp)	*p++ = SUBQ(AR(Ao),1); *p++ = MOVE(Rs,AR(Ao)); \
			Bcc(0x6,lp) /* Bne */

#define Orts		*p++ = RTS
#define Nop		/* nothing */

/*
 * Load and Fetch macros: arg is ignored for this architecture
 */

#define Load_Rs_P	*p++ = MOVE(Rs,AIPOST(As))
#define Load_Rt_P	*p++ = MOVE(Rt,AIPOST(As))
#define Loadzx_Rt_P	*p++ = MOVE(Rt,AIPOST(As))
#define Loador_Rt_P	*p++ = MOVE(Rt,AIPOST(As))
#define Load_Ru_P	*p++ = MOVE(AR(Au),AIPOST(As))
#define Load_Rd_D(f)	*p++ = MOVE(Rd,AIPRE(As))
#define Load_Rs_D(f)	*p++ = MOVE(Rs,AIPRE(As))
#define Load_Rt_D(f)	*p++ = MOVE(Rt,AIPRE(As))
#define Loadzx_Rt_D(f)	*p++ = MOVE(Rt,AIPRE(As))
#define Load_Rd(f)	*p++ = MOVE(Rd,AI(As))
#define Load_Rs(f)	*p++ = MOVE(Rs,AI(As))
#define Load_Rt(f)	*p++ = MOVE(Rt,AI(As))
#define Loadzx_Rt(f)	*p++ = MOVE(Rt,AI(As))
#define Fetch_Rd_P(f)	*p++ = MOVE(Rd,AIPOST(Ad))
#define Fetch_Rd_D(f)	*p++ = MOVE(Rd,AIPRE(Ad))
#define Fetch_Rd(f)	*p++ = MOVE(Rd,AI(Ad))
#define Store_Rs_P	*p++ = MOVE(AIPOST(Ad),Rs)
#define Store_Rs_D	*p++ = MOVE(AIPRE(Ad),Rs)
#define Store_Rs	*p++ = MOVE(AI(Ad),Rs)

#define HAVEBF
#define Bfextu_RdAd(o,w) *p++ = BFEXTU(AI(Ad)); *p++ = BFIELD(Rd,o,w)
#define Bfextu_RsAs(o,w) *p++ = BFEXTU(AI(As)); *p++ = BFIELD(Rs,o,w)
#define Bfins_AdRs(o,w)  *p++ = BFINS(AI(Ad)); *p++ = BFIELD(Rs,o,w)

#define Inittab(t,s)	*p++ = MOVE(AR(AT),IMML); *(long *)p = (long) t; p += 2

/* look up n bits at offset o in Rt; move table entry (1<<l bytes long) to Rd */
#define Table_RdRt(o,n,l) Two(BFEXTU(Rt),BFIELD(Rd,o,n)); \
			if(osiz==1) { \
				Two(MOVEb(Rd,AIX(AT)),AIXEXT(Rd)); \
			} else if(osiz==2) { \
				Two(MOVEw(Rd,AIX(AT)),AIX2EXT(Rd)); \
			} else { \
				Two(MOVE(Rd,AIX(AT)),AIX4EXT(Rd)); \
			}

#define Table_RsRt(o,n,l) Two(BFEXTU(Rt),BFIELD(Rs,o,n)); \
			if(osiz==1) { \
				Two(MOVEb(Rs,AIX(AT)),AIXEXT(Rs)); \
			} else if(osiz==2) { \
				Two(MOVEw(Rs,AIX(AT)),AIX2EXT(Rs)); \
			} else { \
				Two(MOVE(Rs,AIX(AT)),AIX4EXT(Rs)); \
			}

/* emit code to assemble low n bits of Rd into offset o in Rs */
#define Assemble(o,n)				\
	if((o) == 0) {				\
		Olsh_RsRd(32-(n));		\
	} else if((o) == 32-(n)) {		\
		*p++ = OR(Rs,Rd);		\
	} else {				\
		Oorlsh_RsRd(32-((o)+(n)));	\
	}
/* emit code to assemble low n bits of Rd into offset o in Rs.
   this works by shifting Rd as we go, it only works if
   the whole word will eventually be filled */
#define Assemblex(o,n)				\
	if((o) == 0) {				\
		*p++ = MOVE(Rs,Rd);		\
	} else {				\
		Lsh(Rs,n);			\
		*p++ = OR(Rs,Rd);		\
	}

#define Extrainit

extern void	flushvirtpage(void *);
extern void	bbdflush(void *, int);

#define Execandfree(memstart,onstack)					\
			if(onstack) {					\
				flushvirtpage(memstart);		\
				(*(void (*)(void))memstart)();		\
			} else {					\
				tmp = (p-memstart) * sizeof(Type);	\
				bbdflush(memstart, tmp);		\
				(*(void (*)(void))memstart)();		\
				bbfree(memstart, tmp);			\
			}

/* emit code seq at fi (at most 3 shorts) */
#define Emitop					\
	*(long *)p = *(long *)fi;		\
	p[2] = fi[2];				\
	p = (Type*)(((char *)p)+fin)

typedef struct	Fstr
{
	short	fetchs;
	short	fetchd;
	int	n;
	Type	instr[4];
} Fstr;

Fstr	fstr[16] =
{
[0]	0,0,2,		/* Zero */
	{ CLR(Rs), 0, 0, 0 },

[1]	1,1,4,		/* DnorS */
	{ OR(Rs,Rd), NOT(Rs), 0, 0},

[2]	1,1,4,		/* DandnotS */
	{ NOT(Rs), AND(Rs,Rd), 0, 0},

[3]	1,0,2,		/* notS */
	{ NOT(Rs), 0, 0, 0},

[4]	1,1,6,		/* notDandS */
	{ NOT(Rs), OR(Rs,Rd), NOT(Rs), 0},

[5]	0,1,4,		/* notD */
	{ MOVE(Rs,Rd), NOT(Rs), 0, 0},

[6]	1,1,2,		/* DxorS */
	{ XOR(Rs,Rd), 0, 0, 0},

[7]	1,1,4,		/* DnandS */
	{ AND(Rs,Rd), NOT(Rs), 0, 0},

[8]	1,1,2,		/* DandS */
	{ AND(Rs,Rd), 0, 0, 0},

[9]	1,1,4,		/* DxnorS */
	{ XOR(Rs,Rd), NOT(Rs), 0, 0},

[10]	0,1,2,		/* D */
	{ MOVE(Rs,Rd), 0, 0, 0},

[11]	1,1,4,		/* DornotS */
	{ NOT(Rs), OR(Rs,Rd), 0, 0},

[12]	1,0,0,		/* S */
	{ 0, 0, 0, 0},

[13]	1,1,6,		/* notDorS */
	{ NOT(Rs), AND(Rs,Rd), NOT(Rs), 0},

[14]	1,1,2,		/* DorS */
	{ OR(Rs,Rd), 0, 0, 0},

[15]	0,0,4,		/* F */
	{ CLR(Rs), NOT(Rs), 0, 0},
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
	Progmax = 1600,		/* max number of Type units in a bitblt prog */
				/* should be bigger if want 0->5 conversion */
	Progmaxnoconv = 80,	/* max number of Type units when no conversion */
};

#ifdef TEST

void
prprog(void)
{
	abort();	/* use db */
}

#endif
