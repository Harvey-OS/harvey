/*
 * See the comments at the beginning of bb.c and bbc.h
 * for an outline of how this bitblt works
 
 * The VGA screen on the 386 has an awful bit/byte order:
 * the high order bit of a byte is leftmost, but the low
 * order byte of a word is leftmost.
 * This is a totally littleendian bitblt, so the bytes will
 * have to be bit-reversed before putting them on the screen.
 */
#define LENDIAN
typedef	uchar	Type;

/*
 * Registers:
 */
#define As	6	/* ESI */
#define Ad	7	/* EDI */
#define	Rs	0	/* EAX: FIELD works better with this here */
#define	Rd	3	/* EBX */
#define Rt	2	/* EDX */
#define Ru	1	/* ECX */
#define RX	5	/* EBP */
#define SP	4	/* ESP */
/* Ri is top-of-stack, if needed */
/* Ro is second-top-of-stack, if needed */
 
/*
 * Macros for assembling 386 instructions
 */
#define MODRM(mod,rm,reg)	(((mod)<<6)|((reg)<<3)|(rm))
#define SIB(ss,ind,base)	(((ss)<<6)|((ind)<<3)|(base))
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

#define Load(areg,dst)		Immsb(0x8B, IRMR(areg,dst))
#define Store(src,areg)		Immsb(0x89, IRMR(areg,src))
#define Mov(src,dst)		Imms(MOV(src,dst))
#define Movd(v,dst)		*p++ = (0xB8 + (dst)); Immd(v)
#define Imov(a,i,r)		*p++ = 0x8B; Immsb(IRMR(4,r),SIB(2,i,5)); Immd(a)
#define Or(src,dst)		Imms(OR(src,dst))
#define Xor(src,dst)		Imms(XOR(src,dst))
#define And(src,dst)		Imms(AND(src,dst))
#define Andd(v,dst)		if(dst==Rs) {\
					*p++ = 0x25;\
				} else {\
					Immsb(0x81, RMR(dst,4));\
				}\
				Immd(v)
#define Not(r)			Imms(NOT(r))
#define Inc(r)			*p++ = (0x40 + (r))
#define Dec(r)			*p++ = (0x48 + (r))
#define Decsp			Immsb(0xFF, IRMR(4,1)); *p++ = SIB(0,4,4)
#define Shl(r,sh)		Immsb(0xC1, RMR(r,4)); *p++ = (sh)
#define Shr(r,sh)		Immsb(0xC1, RMR(r,5)); *p++ = (sh)
#define Addd(v,r)		Immsb(0x81, RMR(r,0)); Immd(v)
#define Addbsx(v,r)		Immsb(0x83, RMR(r,0)); *p++ = v
#define Addsp(v)		Immsb(0x83, RMR(4,0)); *p++ = v
#define Pushd(v)		*p++ = 0x68; Immd(v)
#define Loop(d)			*p = 0xE2; *(p+1) = d; p += 2
#define Jmp8(d)			*p = 0xEB; *(p+1) = d; p += 2
#define Jmp(d)			*p++ = 0xE9; Immd(d)
#define Jg8(d)			*p = 0x7F; *(p+1) = d; p += 2
#define Jle8(d)			*p = 0x7E; *(p+1) = d; p += 2

#define Ofield(c)	Xor(Rd,Rs); Andd(c,Rs); Xor(Rd,Rs)
#define Olsha_RsRt	Mov(Rt,Rs); Shr(Rs,sha)
#define Olshb_RsRt	Mov(Rt,Rs); Shr(Rs,shb)
#define Olsh_RsRd(sh)	Mov(Rd,Rs); Shr(Rs,sh)
#define Olsh_RtRt(sh)	Shr(Rt,sh)
#define Olsha_RtRt	Shr(Rt,sha)
#define Olsha_RtRu	Mov(Ru,Rt); Shr(Rt,sha)
#define Olshb_RtRu	Mov(Ru,Rt); Shr(Rt,shb)
#define Orsha_RsRt	Mov(Rt,Rs); Shl(Rs,sha)
#define Orshb_RsRt	Mov(Rt,Rs); Shl(Rs,shb)
#define Orsha_RtRu	Mov(Ru,Rt); Shl(Rt,sha)
#define Orshb_RtRu	Mov(Ru,Rt); Shl(Rt,shb)
#define Oorlsha_RsRt	Mov(Rt,RX); Shr(RX,sha); Or(RX,Rs)
#define Oorlshb_RsRt	Mov(Rt,RX); Shr(RX,shb); Or(RX,Rs)
#define Oorlsh_RsRd(sh)	Mov(Rd,RX); Shr(RX,sh); Or(RX,Rs)
#define Oorrsha_RsRt	Mov(Rt,RX); Shl(RX,sha); Or(RX,Rs)
#define Oorrshb_RsRt	Mov(Rt,RX); Shl(RX,shb); Or(RX,Rs)
#define Oorrsha_RtRu	Mov(Ru,RX); Shl(RX,sha); Or(RX,Rt)
#define Oorrshb_RtRu	Mov(Ru,RX); Shl(RX,shb); Or(RX,Rt)
#define Oor_RsRd	Or(Rd,Rs)
#define Add_As(v)	Addd(v,As)
#define Add_Ad(v)	Addd(v,Ad)
#define Initsd(s,d)	Movd(s,As); Movd(d,Ad)
#define Ilabel(c)	Pushd(c)
#define Olabel(c)	Pushd(c)
#define Iloop(lp)	Decsp;				\
			tmp = (lp)-(p+2);		\
			if(tmp > -128) {		\
				Jg8(tmp);		\
			} else {			\
				Jle8(5);		\
				tmp = (lp) - (p+5);	\
				Jmp(tmp);		\
			}				\
			Addsp(4)
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
#define Load_Rs_P	Load(As,Rs); Addbsx(4,As)
#define Load_Rt_P	Load(As,Rt); Addbsx(4,As)
#define Loadzx_Rt_P	Load(As,Rt); Addbsx(4,As)
#define Loador_Rt_P	Load(As,Rt); Addbsx(4,As)
#define Load_Ru_P	Load(As,Ru); Addbsx(4,As)
#define Load_Rd_D(f)	Addbsx(-4,As); Load(As,Rd)
#define Load_Rs_D(f)	Addbsx(-4,As); Load(As,Rs)
#define Load_Rt_D(f)	Addbsx(-4,As); Load(As,Rt)
#define Loadzx_Rt_D(f)	Addbsx(-4,As); Load(As,Rt)
#define Load_Rd(f)	Load(As,Rd)
#define Load_Rs(f)	Load(As,Rs)
#define Load_Rt(f)	Load(As,Rt)
#define Loadzx_Rt(f)	Load(As,Rt)
/* fetch */
#define Fetch_Rd_P(f)	Load(Ad,Rd); Addbsx(4,Ad)
#define Fetch_Rd_D(f)	Addbsx(-4,Ad); Load(Ad,Rd)
#define Fetch_Rd(f)	Load(Ad,Rd)
/* store */
#define Store_Rs_P	Store(Rs,Ad); Addbsx(4,Ad)
#define Store_Rs_D	Addbsx(-4,Ad); Store(Rs,Ad)
#define Store_Rs	Store(Rs,Ad)

#define Inittab(t,s)
#define Initsh(a,b)

/* emit code to look up n bits of Rt at offset o, answer ulong in Rd */
#define Table_RdRt(o,n,l)		\
	Mov(Rt,Rd);			\
	if(o) { Shr(Rd,(o)); }		\
	Andd(((1<<(n))-1),Rd);		\
	Imov(tab,Rd,Rd)

#define Table_RsRt(o,n,l)		\
	Mov(Rt,Rs);			\
	if(o) { Shr(Rs,o); }		\
	Andd(((1<<(n))-1),Rs);		\
	Imov(tab,Rs,Rs)

/* emit code to assemble high n bits of Rd into offset o in Rs */
#define Assemble(o,n)				\
	if((o) == 0) {				\
		Olsh_RsRd(32-(n));		\
	} else if((o) == 32-(n)) {		\
		Or(Rd,Rs);			\
	} else {				\
		Oorlsh_RsRd(32-((o)+(n)));	\
	}

/* concept of shifting-as-you-go doesn't work when filling from low end */
#define Assemblex(o,n) Assemble(o,n)

#define Nop

#ifdef TEST
#define Extrainit		\
	Movd(lrand(),Rs);	\
	Movd(lrand(),Rd);	\
	Movd(lrand(),Rt);	\
	Movd(lrand(),Ru);	\
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
	{	      0, (uchar*)tab01l, (uchar*)tab02l, (uchar*)tab03l},
	{(uchar*)tab10l,	      0, (uchar*)tab12l, (uchar*)tab13l},
	{(uchar*)tab20l, (uchar*)tab21l,	      0, (uchar*)tab23l},
	{(uchar*)tab30l, (uchar*)tab31l, (uchar*)tab32l,	      0},
};

static uchar tabosiz[4][4] = /* size in bytes of entries */
{
	{ 0, 2, 4, 4},
	{ 1, 0, 2, 4},
	{ 1, 1, 0, 2},
	{ 1, 1, 1, 0},
};

enum {
	Progmax = 3000,		/* max number of bytes in a bitblt prog */
	Progmaxnoconv = 200,	/* max number when not converting */
};

#ifdef TEST
void
prprog(void)
{
	abort();	/* use db */
}

#endif
