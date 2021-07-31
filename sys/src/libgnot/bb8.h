/*
 * See the comments at the beginning of bb.c and bbc.h
 * for an outline of how this bitblt works
 
 * The VGA screen on the 386 has an awful bit/byte order:
 * the high order bit of a byte is leftmost, but the low
 * order byte of a word is leftmost.  This causes havoc
 * if the source is shifted relative to the destination.
 * So we make a 'word' be a byte.  That changes the converting
 * bitblt considerably, so that Ru is no longer needed in
 * the abstract machine.
 */
#define WBITS	8
#define LWBITS	3
#define W2L	4
#define WMASK	0xFF
#define BYTEREV
typedef uchar	*WType;

typedef	uchar	Type;

/*
 * Registers:
 */
#define As	6	/* ESI */
#define Ad	7	/* EDI */
#define	Rs	0	/* EAX: FIELD works better with this here */
#define	Rd	3	/* EBX */
#define Rt	2	/* EDX */
#define Ri	1	/* ECX: LOOP depends on this here */
#define RX	5	/* EBP */
#define SP	4	/* ESP */
/* Ro is top-of-stack, if needed */

/*
 * Macros for assembling 386 instructions
 */
#define MODRM(mod,rm,reg)	(((mod)<<6)|((reg)<<3)|(rm))
#define RMR(rm,reg)		MODRM(3,rm,reg)
#define	IRMR(rm,reg)		MODRM(0,rm,reg)
#define DB(b1,b2,b3,b4)		((b1)|((b2)<<8)|((b3)<<16)|((b4)<<24))
#define SB(b1,b2)		((b1)|((b2)<<8))

#define OR(src,dst)		SB(0x0B, RMR(src,dst))
#define XOR(src,dst)		SB(0x31, RMR(dst,src))
#define AND(src,dst)		SB(0x21, RMR(dst,src))
#define NOT(r)			SB(0xF7, RMR(r,2))
#define MOV(src,dst)		SB(0x89, RMR(dst,src))

#define Immd(v)			*(long*)p = (long)(v); p += 4
#define Immdb(b1,b2,b3,b4)	Immd(DB(b1,b2,b3,b4))
#define Imms(v)			*(short*)p = (v); p += 2
#define	Immsb(b1,b2)		Imms(SB(b1,b2))

#define Loadb(areg,dst)		Immsb(0x8A, IRMR(areg,dst))
#define Loadbzx(areg,dst)	Immsb(0x0F, 0xB6); *p++ = IRMR(areg,dst);
#define Storeb(src,areg)	Immsb(0x88, IRMR(areg,src))
#define Mov(src,dst)		Imms(MOV(src,dst))
#define Movd(v,dst)		*p++ = (0xB8 + (dst)); Immd(v)
#define Imovzx(a,i,r)		Immdb(0x0F, 0xB6, IRMR(4,r), IRMR(5,i)); Immd(a)
#define Or(src,dst)		Imms(OR(src,dst))
#define Xor(src,dst)		Imms(XOR(src,dst))
#define And(src,dst)		Imms(AND(src,dst))
#define Andd(v,dst)		if(dst==Rs) {\
					*p++ = 0x25;\
				} else {\
					Immsb(0x81, RMR(dst,4));\
				}\
				Immd(v)
#define And8(v,dst)		if(dst==Rs) {\
					*p++ = 0x24;\
				} else {\
					Immsb(0x80, RMR(dst,4));\
				}\
				*p++ = (v)
#define Not(r)			Imms(NOT(r))
#define Inc(r)			*p++ = (0x40 + (r))
#define Dec(r)			*p++ = (0x48 + (r))
#define Decsp			Immsb(0xFF, IRMR(4,1)); *p++ = 0x24
#define Shl(r,sh)		Immsb(0xC1, RMR(r,4)); *p++ = (sh)
#define Shr(r,sh)		Immsb(0xC1, RMR(r,5)); *p++ = (sh)
#define Addd(v,r)		Immsb(0x81, RMR(r,0)); Immd(v)
#define Addsp(v)		Immsb(0x83, RMR(4,0)); *p++ = v
#define Pushd(v)		*p++ = 0x68; Immd(v)
#define Loop(d)			*p = 0xE2; *(p+1) = d; p += 2
#define Jmp8(d)			*p = 0xEB; *(p+1) = d; p += 2
#define Jmp(d)			*p++ = 0xE9; Immd(d)
#define Jg8(d)			*p = 0x7F; *(p+1) = d; p += 2
#define Jle8(d)			*p = 0x7E; *(p+1) = d; p += 2

/*
 * Macros for assembling the operations of the abstract machine.
 * Each assumes that uchar *p points to the next location where
 * an instruction should be assembled.  Some of them assume
 * that Arg *a points to the Arg structure describing the current
 * bitblt (see bb.c).
 * Some of the operations only care about the low order bytes of
 * the registers; others have to zero the high order bytes because
 * they might get shifted down.
 */

#define Ofield(c)	Xor(Rd,Rs); And8(c,Rs); Xor(Rd,Rs)
#define Olsha_RsRt	Mov(Rt,Rs); Shl(Rs,sha)
#define Olshb_RsRt	Mov(Rt,Rs); Shl(Rs,shb)
#define Olsh_RsRd(sh)	Mov(Rd,Rs); Shl(Rs,sh)
#define Olsh_RtRt(sh)	Shl(Rt,sh)
#define Olsha_RtRt	Shl(Rt,sha)
#define Olsha_RtRu	/* Ru not needed */
#define Olshb_RtRu	/* Ru not needed */
#define Orsha_RsRt	Mov(Rt,Rs); Shr(Rs,sha)
#define Orshb_RsRt	Mov(Rt,Rs); Shr(Rs,shb)
#define Orsha_RtRu	/* Ru not needed */
#define Orshb_RtRu	/* Ru not needed */
#define Oorlsha_RsRt	Mov(Rt,RX); Shl(RX,sha); Or(RX,Rs)
#define Oorlshb_RsRt	Mov(Rt,RX); Shl(RX,shb); Or(RX,Rs)
#define Oorlsh_RsRd(sh)	Mov(Rd,RX); Shl(RX,sh); Or(RX,Rs)
#define Oorrsha_RsRt	Mov(Rt,RX); Shr(RX,sha); Or(RX,Rs)
#define Oorrshb_RsRt	Mov(Rt,RX); Shr(RX,shb); Or(RX,Rs)
#define Oorrsha_RtRu	/* Ru not needed */
#define Oorrshb_RtRu	/* Ru not needed */
#define Oor_RsRd	Or(Rd,Rs)
#define Add_As(v)	Addd(v,As)
#define Add_Ad(v)	Addd(v,Ad)
#define Initsd(s,d)	Movd(s,As); Movd(d,Ad)
#define Ilabel(c)	Movd(c,Ri)
#define Olabel(c)	Pushd(c)
#define Iloop(lp)	tmp = (lp)-(p+2);		\
			if(tmp > -128) {		\
				Loop(tmp);		\
			} else {			\
				Loop(2);		\
				Jmp8(5);		\
				tmp = (lp) - (p+5);	\
				Jmp(tmp);		\
			}
#define Oloop(lp)	Decsp;				\
			tmp = (lp)-(p+2);		\
			if(tmp > -128) {		\
				Jg8(tmp);		\
			} else {			\
				Jle8(5);		\
				tmp = (lp) - (p+5);	\
				Jmp(tmp);		\
			}				\
			Addsp(4)
#define Orts		*p++ = 0xC3
/* load */
#define Load_Rs_P	Loadb(As,Rs); Inc(As)
#define Load_Rt_P	Loadb(As,Rt); Inc(As)
#define Loadzx_Rt_P	Loadbzx(As,Rt); Inc(As)
#define Loador_Rt_P	Loadb(As,Rt); Inc(As)
#define Load_Ru_P	/* Ru not needed */
#define Load_Rd_D(f)	Dec(As); Loadb(As,Rd)
#define Load_Rs_D(f)	Dec(As); Loadb(As,Rs)
#define Load_Rt_D(f)	Dec(As); Loadb(As,Rt)
#define Loadzx_Rt_D(f)	Dec(As); Loadbzx(As,Rt)
#define Load_Rd(f)	Loadb(As,Rd)
#define Load_Rs(f)	Loadb(As,Rs)
#define Load_Rt(f)	Loadb(As,Rt)
#define Loadzx_Rt(f)	Loadbzx(As,Rt)
/* fetch */
#define Fetch_Rd_P(f)	Loadb(Ad,Rd); Inc(Ad)
#define Fetch_Rd_D(f)	Dec(Ad); Loadb(Ad,Rd)
#define Fetch_Rd(f)	Loadb(Ad,Rd)
/* store */
#define Store_Rs_P	Storeb(Rs,Ad); Inc(Ad)
#define Store_Rs_D	Dec(Ad); Storeb(Rs,Ad)
#define Store_Rs	Storeb(Rs,Ad)

#define Inittab(t,s)
#define Initsh(a,b)

/* emit code to look up n bits of Rt at offset o, answer byte in Rd */
/* l is always 0 when WBITS==8 */
#define Table_RdRt(o,n,l)		\
	Mov(Rt,Rd);			\
	tmp = 32-((o)+(n));		\
	if(tmp > 0) {			\
		Shr(Rd,tmp);		\
	}				\
	Andd(((1<<(n))-1),Rd);		\
	Imovzx(tab,Rd,Rd)

#define Table_RsRt(o,n,l)		\
	Mov(Rt,Rs);			\
	tmp = 32-((o)+(n));		\
	if(tmp > 0) {			\
		Shr(Rs,tmp);		\
	}				\
	Andd(((1<<(n))-1),Rs);		\
	Imovzx(tab,Rs,Rs)

/* emit code to assemble low n bits of Rd into offset o in low byte in Rs */
#define Assemble(o,n)				\
	if((o) == 0) {				\
		Olsh_RsRd(8-(n));		\
	} else if((o) == 8-(n)) {		\
		Or(Rd,Rs);			\
	} else {				\
		Oorlsh_RsRd(8-((o)+(n)));	\
	}

#define Assemblex(o,n)				\
	if((o) == 0) {				\
		Mov(Rs,Rd);			\
	} else {				\
		Shl(Rs,n);			\
		Or(Rd,Rs);			\
	}

#define Nop

#ifdef TEST
#define Extrainit		\
	Movd(lrand(),Rs);	\
	Movd(lrand(),Rd);	\
	Movd(lrand(),Rt);	\
	Movd(lrand(),Ri);	\
	Movd(lrand(),RX)
#else
#define Extrainit
#endif

#define Execandfree(memstart,onstack)				\
	(*(void (*)(void))memstart)();				\
	if(!onstack)						\
		bbfree(memstart, (p-memstart) * sizeof(Type));

/* emit code seq at fi (at most 3 shorts) */
#define Emitop						\
	*(long *)p = *(long *)fi;			\
	*(short *)(p+4) = *(short *)(((char *)fi)+4);	\
	p = (Type*)(((char *)p)+fin)

typedef struct	Fstr
{
	short	fetchs;
	short	fetchd;
	int	n;
	short	instr[4];
} Fstr;

Fstr	fstr[16] =
{
[0]	0,0,2,		/* Zero */
	{ XOR(Rs,Rs), 0, 0, 0 },

[1]	1,1,4,		/* DnorS */
	{ OR(Rd,Rs), NOT(Rs), 0, 0},

[2]	1,1,4,		/* DandnotS */
	{ NOT(Rs), AND(Rd,Rs), 0, 0},

[3]	1,0,2,		/* notS */
	{ NOT(Rs), 0, 0, 0},

[4]	1,1,6,		/* notDandS */
	{ NOT(Rs), OR(Rd,Rs), NOT(Rs), 0},

[5]	0,1,4,		/* notD */
	{ MOV(Rd,Rs), NOT(Rs), 0, 0},

[6]	1,1,2,		/* DxorS */
	{ XOR(Rd,Rs), 0, 0, 0},

[7]	1,1,4,		/* DnandS */
	{ AND(Rd,Rs), NOT(Rs), 0, 0},

[8]	1,1,2,		/* DandS */
	{ AND(Rd,Rs), 0, 0, 0},

[9]	1,1,4,		/* DxnorS */
	{ XOR(Rd,Rs), NOT(Rs), 0, 0},

[10]	0,1,2,		/* D */
	{ MOV(Rd,Rs), 0, 0, 0},

[11]	1,1,4,		/* DornotS */
	{ NOT(Rs), OR(Rd,Rs), 0, 0},

[12]	1,0,0,		/* S */
	{ 0, 0, 0, 0},

[13]	1,1,6,		/* notDorS */
	{ NOT(Rs), AND(Rd,Rs), NOT(Rs), 0},

[14]	1,1,2,		/* DorS */
	{ OR(Rd,Rs), 0, 0, 0},

[15]	0,0,4,		/* F */
	{ XOR(Rs,Rs), NOT(Rs), 0, 0},
};

#include "tabs.h"

static uchar *tabs[4][4] =
{
	{	     0, (uchar*)tab01b, 0, (uchar*)tab03b},
	{(uchar*)tab10b,	    0,  0,	       0},
	{	     0,		    0,  0,	       0},
	{(uchar*)tab30b,	    0,  0,	       0},
};

static uchar tabosiz[4][4] = /* size in bytes of entries */
{
	{	     0, 	    1,  0,	       1},
	{	     1,		    0,  0, 	       0},
	{	     0,		    0,  0,	       0},
	{	     1,		    0,  0,	       0},
};

enum {
	Progmax = 1000,		/* max number of bytes in a bitblt prog */
	Progmaxnoconv = 168,	/* max number when not converting */
};

#ifdef TEST
void
prprog(void)
{
	abort();	/* use db */
}

#endif
