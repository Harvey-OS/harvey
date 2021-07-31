/*
 * this doesn't attempt to implement MIPS floating-point properties
 * that aren't visible in the Inferno environment.
 * all arithmetic is done in double precision.
 * the FP trap status isn't updated.
 *
 * we emulate the original MIPS FP register model: 32-bits each,
 * F(2n) and F(2n+1) are a double, with lower-order word first;
 * note that this is little-endian order, unlike the rest of the
 * machine, so double-word operations will need to swap the words
 * when transferring between FP registers and memory.
 *
 * on some machines, we can convert to an FP internal representation when
 * moving to FPU registers and back (to integer, for example) when moving
 * from them.  the MIPS is different: its conversion instructions operate
 * on FP registers only, and there's no way to tell if data being moved
 * into an FP register is integer or FP, so it must be possible to store
 * integers in FP registers without conversion.  Furthermore, pairs of FP
 * registers can be combined into a double.  So we keep the raw bits
 * around as the canonical representation and convert only to and from
 * Internal FP format when we must (i.e., before calling the common fpi
 * code).
 */
#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"ureg.h"
#include	"../port/fpi.h"
#include	<tos.h>

#ifdef FPEMUDEBUG
#define DBG(bits) (fpemudebug & (bits))
#define intpr _intpr
#define internsane _internsane
#define dbgstuck _dbgstuck
#else
#define DBG(bits) (0)
#define internsane(i, ur)	do { USED(ur); } while(0)
#define intpr(i, reg, fmt, ufp)	do {} while(0)
#define dbgstuck(pc, ur, ufp)	do {} while(0)
#endif

#define	OFR(memb) (uintptr)&((Ureg*)0)->memb	/* offset into Ureg of memb */
#define	REG(ur, r) *acpureg(ur, r)			/* cpu reg in Ureg */
#define	FREG(ufp, fr) (ufp)->reg[(fr) & REGMASK]	/* fp reg raw bits */

/*
 * instruction decoding for COP1 instructions; integer instructions
 * are laid out differently.
 */
#define OP(ul)	 ((ul) >> 26)
#define REGMASK MASK(5)				/* mask for a register number */
#define FMT(ul)	 (((ul) >> 21) & REGMASK)	/* data type */
#define REGT(ul) (((ul) >> 16) & REGMASK)	/* source2 register */
#define REGS(ul) (((ul) >> 11) & REGMASK)	/* source1 register */
#define REGD(ul) (((ul) >>  6) & REGMASK)	/* destination register */
#define FUNC(ul) ((ul) & MASK(6))

enum {
	Dbgbasic = 1<<0,	/* base debugging: ops, except'ns */
	Dbgmoves = 1<<1,	/* not very exciting usually */
	Dbgregs	 = 1<<2,	/* print register contents around ops */
	Dbgdelay = 1<<3,	/* branch-delay-slot-related machinery */

	/* fpimips status codes */
	Failed = -1,
	Advpc,				/* advance pc normally */
	Leavepc,			/* don't change the pc */
	Leavepcret,			/* ... and return to user mode now */
	Nomatch,

	/* no-ops */
	NOP	= 0x27,			/* NOR R0, R0, R0 */
	MIPSNOP = 0,			/* SLL R0, R0, R0 */

	/* fp op-codes */
	COP1	= 0x11,			/* fpu op */
	LWC1	= 0x31,			/* load float/long */
	LDC1	= 0x35,			/* load double/vlong */
	SWC1	= 0x39,			/* store float/long */
	SDC1	= 0x3d,			/* store double/vlong */

	N = 1<<31,			/* condition codes */
	Z = 1<<30,
	C = 1<<29,
	V = 1<<28,

	/* data types (format field values) */
	MFC1	= 0,			/* and func == 0 ... */
	DMFC1,				/* vlong move */
	CFC1,				/* ctl word move */
	MTC1	= 4,
	DMTC1,
	CTC1,				/* ... end `and func == 0' */
	BRANCH	= 8,
	Ffloat	= 16,
	Fdouble,
	Flong	= 20,
	Fvlong,

	/* fp control registers */
	Fpimp	= 0,
	Fpcsr	= 31,
};

typedef struct FP1 FP1;
typedef struct FP2 FP2;
typedef struct FPcvt FPcvt;
typedef struct Instr Instr;

struct Instr {	/* a COP1 instruction, broken out and registers converted */
	int	iw;		/* whole word */
	uintptr	pc;
	int	o;		/* opcode or cop1 func code */
	int	fmt;		/* operand format */
	int	rm;		/* first operand register */
	int	rn;		/* second operand register */
	int	rd;		/* destination register */

	Internal *fm;		/* converted from FREG(ufp, rm) */
	Internal *fn;
	char	*dfmt;
	FPsave	*ufp;		/* fp state, including fp registers */
	Ureg	*ur;		/* user registers */
};

struct FP2 {
	char*	name;
	void	(*f)(Internal*, Internal*, Internal*);
};

struct FP1 {
	char*	name;
	void	(*f)(Internal*, Internal*);
};

struct FPcvt {
	char*	name;
	void	(*f)(int, int, int, Ureg *, FPsave *);
};

static	int	roff[32] = {
	0,       OFR(r1), OFR(r2), OFR(r3),
	OFR(r4), OFR(r5), OFR(r6), OFR(r7),
	OFR(r8), OFR(r9), OFR(r10), OFR(r11),
	OFR(r12), OFR(r13), OFR(r14), OFR(r15),
	OFR(r16), OFR(r17), OFR(r18), OFR(r19),
	OFR(r20), OFR(r21), OFR(r22), OFR(r23),
	OFR(r24), OFR(r25), OFR(r26), OFR(r27),
	OFR(r28), OFR(sp),  OFR(r30), OFR(r31),
};

/*
 * plan 9 assumes F24 initialized to 0.0, F26 to 0.5, F28 to 1.0, F30 to 2.0.
 */
enum {
	FZERO = 24,
	FHALF = 26,
};
static Internal fpconst[Nfpregs] = {		/* indexed by register no. */
	/* s, e, l, h */
[FZERO]	{0, 0x1, 0x00000000, 0x00000000},	/* 0.0 */
[FHALF]	{0, 0x3FE, 0x00000000, 0x08000000},	/* 0.5 */
[28]	{0, 0x3FF, 0x00000000, 0x08000000},	/* 1.0 */
[30]	{0, 0x400, 0x00000000, 0x08000000},	/* 2.0 */
};

static char *fmtnames[] = {
[MFC1]	"MF",
[DMFC1]	"DMF",
[CFC1]	"CF",
[MTC1]	"MT",
[DMTC1]	"DMT",
[CTC1]	"CT",
[BRANCH]"BR",

[Ffloat]"F",
[Fdouble]"D",
[Flong]	"W",
[Fvlong]"L",
};

static char *prednames[] = {
[0]	"F",
[1]	"UN",
[2]	"EQ",
[3]	"UEQ",
[4]	"OLT",
[5]	"ULT",
[6]	"OLE",
[7]	"ULE",
[8]	"SF",
[9]	"NGLE",
[10]	"SEQ",
[11]	"NGL",
[12]	"LT",
[13]	"NGE",
[14]	"LE",
[15]	"NGT",
};

int fpemudebug = 0;			/* settable via /dev/archctl */

static ulong dummyr0;
static QLock watchlock;			/* lock for watch-points */

ulong	branch(Ureg*, ulong);
int	isbranch(ulong *);

static int	fpimips(ulong, ulong, Ureg *, FPsave *);

char *
fpemuprint(char *p, char *ep)
{
#ifdef FPEMUDEBUG
	return seprint(p, ep, "fpemudebug %d\n", fpemudebug);
#else
	USED(ep);
	return p;
#endif
}

static ulong *
acpureg(Ureg *ur, int r)
{
	r &= REGMASK;
	if (r == 0 || roff[r] == 0) {
		dummyr0 = 0;
		return &dummyr0;
	}
	return (ulong *)((char*)ur + roff[r]);
}

ulong *
reg(Ureg *ur, int r)		/* for faultmips */
{
	return &REG(ur, r);
}

static void
_internsane(Internal *i, Ureg *ur)
{
	static char buf[ERRMAX];

	USED(i);
	if (!(DBG(Dbgbasic)))
		return;
	if ((unsigned)i->s > 1) {
		snprint(buf, sizeof buf,
			"fpuemu: bogus Internal sign at pc=%#p", ur->pc);
		error(buf);
	}
	if ((unsigned)i->e > DoubleExpMax) {
		snprint(buf, sizeof buf,
			"fpuemu: bogus Internal exponent at pc=%#p", ur->pc);
		error(buf);
	}
}

/*
 * mips binary operations (d = n operator m)
 */

static void
fadd(Internal *m, Internal *n, Internal *d)
{
	(m->s == n->s? fpiadd: fpisub)(m, n, d);
}

static void
fsub(Internal *m, Internal *n, Internal *d)
{
	m->s ^= 1;
	(m->s == n->s? fpiadd: fpisub)(m, n, d);
}

/*
 * mips unary operations
 */

static void
frnd(Internal *m, Internal *d)
{
	short e;
	Internal tmp;

	tmp = fpconst[FHALF];
	(m->s? fsub: fadd)(&tmp, m, d);
	if(IsWeird(d))
		return;
	fpiround(d);
	e = (d->e - ExpBias) + 1;
	if(e <= 0)
		SetZero(d);
	else if(e > FractBits){
		if(e < 2*FractBits)
			d->l &= ~((1<<(2*FractBits - e))-1);
	}else{
		d->l = 0;
		if(e < FractBits)
			d->h &= ~((1<<(FractBits-e))-1);
	}
}

/* debugging: print internal representation of an fp reg */
static void
_intpr(Internal *i, int reg, int fmt, FPsave *ufp)
{
	USED(i);
	if (!(DBG(Dbgregs)))
		return;
	if (fmt == Fdouble && reg < 31)
		iprint("\tD%02d: l %08lux h %08lux =\ts %d e %04d h %08lux l %08lux\n",
			reg, FREG(ufp, reg), FREG(ufp, reg+1),
			i->s, i->e, i->h, i->l);
	else
		iprint("\tF%02d: %08lux =\ts %d e %04d h %08lux l %08lux\n",
			reg, FREG(ufp, reg),
			i->s, i->e, i->h, i->l);
	delay(75);
}

static void
dreg2dbl(Double *dp, int reg, FPsave *ufp)
{
	reg &= ~1;
	dp->l = FREG(ufp, reg);
	dp->h = FREG(ufp, reg+1);
}

static void
dbl2dreg(int reg, Double *dp, FPsave *ufp)
{
	reg &= ~1;
	FREG(ufp, reg)   = dp->l;
	FREG(ufp, reg+1) = dp->h;
}

static void
vreg2dbl(Double *dp, int reg, FPsave *ufp)
{
	reg &= ~1;
	dp->l = FREG(ufp, reg+1);
	dp->h = FREG(ufp, reg);
}

static void
dbl2vreg(int reg, Double *dp, FPsave *ufp)
{
	reg &= ~1;
	FREG(ufp, reg+1) = dp->l;
	FREG(ufp, reg)   = dp->h;
}

/* convert fmt (rm) to double (rd) */
static void
fcvtd(int fmt, int rm, int rd, Ureg *ur, FPsave *ufp)
{
	Double d;
	Internal intrn;

	switch (fmt) {
	case Ffloat:
		fpis2i(&intrn, &FREG(ufp, rm));
		internsane(&intrn, ur);
		fpii2d(&d, &intrn);
		break;
	case Fdouble:
		dreg2dbl(&d, rm, ufp);
		break;
	case Flong:
		fpiw2i(&intrn, &FREG(ufp, rm));
		internsane(&intrn, ur);
		fpii2d(&d, &intrn);
		break;
	case Fvlong:
		vreg2dbl(&d, rm, ufp);
		fpiv2i(&intrn, &d);
		internsane(&intrn, ur);
		fpii2d(&d, &intrn);
		break;
	}
	dbl2dreg(rd, &d, ufp);
	if (fmt != Fdouble && DBG(Dbgregs))
		intpr(&intrn, rm, Fdouble, ufp);
}

/* convert fmt (rm) to single (rd) */
static void
fcvts(int fmt, int rm, int rd, Ureg *ur, FPsave *ufp)
{
	Double d;
	Internal intrn;

	switch (fmt) {
	case Ffloat:
		FREG(ufp, rd) = FREG(ufp, rm);
		break;
	case Fdouble:
		dreg2dbl(&d, rm, ufp);
		fpid2i(&intrn, &d);
		break;
	case Flong:
		fpiw2i(&intrn, &FREG(ufp, rm));
		break;
	case Fvlong:
		vreg2dbl(&d, rm, ufp);
		fpiv2i(&intrn, &d);
		break;
	}
	if (fmt != Ffloat) {
		if(DBG(Dbgregs))
			intpr(&intrn, rm, Ffloat, ufp);
		internsane(&intrn, ur);
		fpii2s(&FREG(ufp, rd), &intrn);
	}
}

/* convert fmt (rm) to long (rd) */
static void
fcvtw(int fmt, int rm, int rd, Ureg *ur, FPsave *ufp)
{
	Double d;
	Internal intrn;

	switch (fmt) {
	case Ffloat:
		fpis2i(&intrn, &FREG(ufp, rm));
		break;
	case Fdouble:
		dreg2dbl(&d, rm, ufp);
		fpid2i(&intrn, &d);
		break;
	case Flong:
		FREG(ufp, rd) = FREG(ufp, rm);
		break;
	case Fvlong:
		vreg2dbl(&d, rm, ufp);
		fpiv2i(&intrn, &d);
		break;
	}
	if (fmt != Flong) {
		if(DBG(Dbgregs))
			intpr(&intrn, rm, Flong, ufp);
		internsane(&intrn, ur);
		fpii2w((long *)&FREG(ufp, rd), &intrn);
	}
}

/* convert fmt (rm) to vlong (rd) */
static void
fcvtv(int fmt, int rm, int rd, Ureg *ur, FPsave *ufp)
{
	Double d;
	Internal intrn;

	switch (fmt) {
	case Ffloat:
		fpis2i(&intrn, &FREG(ufp, rm));
		break;
	case Fdouble:
		dreg2dbl(&d, rm, ufp);
		fpid2i(&intrn, &d);
		break;
	case Flong:
		fpiw2i(&intrn, &FREG(ufp, rm));
		break;
	case Fvlong:
		vreg2dbl(&d, rm, ufp);
		dbl2vreg(rd, &d, ufp);
		break;
	}
	if (fmt != Fvlong) {
		if(DBG(Dbgregs))
			intpr(&intrn, rm, Fvlong, ufp);
		internsane(&intrn, ur);
		fpii2v((vlong *)&FREG(ufp, rd), &intrn);
	}
}

/*
 * MIPS function codes
 */

static	FP2	optab2[] = {	/* Fd := Fn OP Fm (binary) */
[0]	{"ADDF",	fadd},	/* can ignore fmt, just use doubles */
[1]	{"SUBF",	fsub},
[2]	{"MULF",	fpimul},
[3]	{"DIVF",	fpidiv},
};

static	FP1	optab1[32] = {	/* Fd := OP Fm (unary) */
[4]	{"SQTF",	/*fsqt*/0},
[5]	{"ABSF",	/*fabsf*/0},	/* inline in unaryemu... */
[6]	{"MOVF",	/*fmov*/0},
[7]	{"NEGF",	/*fmovn*/0},
[8]	{"ROUND.L",	/*froundl*/0},	/* 64-bit integer results ... */
[9]	{"TRUNC.L",	/*ftruncl*/0},
[10]	{"CEIL.L",	/*fceill*/0},
[11]	{"FLOOR.L",	/*ffloorl*/0},
[12]	{"ROUND.W",	frnd},		/* 32-bit integer results ... */
[13]	{"TRUNC.W",	/*ftrunc*/0},
[14]	{"CEIL.W",	/*fceil*/0},
[15]	{"FLOOR.W",	/*ffloor*/0},
/* 17—19 are newish MIPS32/64 conditional moves */
/* 21, 22, 28—31 are newish reciprocal or sqrt */
};

static	FPcvt	optabcvt[] = {	/* Fd := OP(fmt, Fm) (unary) */
[32]	{"CVT.S",	fcvts},		/* must honour fmt as src format */
[33]	{"CVT.D",	fcvtd},
[36]	{"CVT.W",	fcvtw},
[37]	{"CVT.L",	fcvtv},
};

/*
 * No type conversion is implied and the type of the cpu register is
 * unknown, so copy the bits into reg.
 * Later instructions will have to know the correct type and use the
 * right format specifier to convert to or from Internal FP.
 */
static void
fld(int d, ulong ea, int n, FPsave *ufp)
{
	if(DBG(Dbgmoves))
		iprint("MOV%c #%lux, F%d\n", n==8? 'D': 'F', ea, d);
	if (n == 4)
		memmove(&FREG(ufp, d), (void *)ea, 4);
	else if (n == 8){
		d &= ~1;
		/* NB: we swap order of the words */
		memmove(&FREG(ufp, d), (void *)(ea+4), 4);
		memmove(&FREG(ufp, d+1), (void *)ea, 4);
	} else
		panic("fld: n (%d) not 4 nor 8", n);
}

static void
fst(ulong ea, int s, int n, FPsave *ufp)
{
	if(DBG(Dbgmoves))
		iprint("MOV%c	F%d,#%lux\n", n==8? 'D': 'F', s, ea);
	if (n == 4)
		memmove((void *)ea, &FREG(ufp, s), 4);
	else if (n == 8){
		s &= ~1;
		/* NB: we swap order of the words */
		memmove((void *)(ea+4), &FREG(ufp, s), 4);
		memmove((void *)ea, &FREG(ufp, s+1), 4);
	} else
		panic("fst: n (%d) not 4 nor 8", n);
}

void
unimp(ulong pc, ulong op, char *msg)
{
	char buf[120];

	snprint(buf, sizeof(buf), "sys: fp: pc=%#lux unimp fp %#.8lux: %s",
		pc, op, msg);
	if(DBG(Dbgbasic))
		iprint("FPE: %s\n", buf);
	error(buf);
	/* no return */
}

static int
isfpop(ulong iw)
{
	switch (OP(iw)) {
	case COP1:
	case LWC1:
	case LDC1:
	case SWC1:
	case SDC1:
		return 1;
	default:
		return 0;
	}
}

static int
ldst(ulong op, Ureg *ur, FPsave *ufp)
{
	int rn, rd, o, size, wr;
	short off;
	ulong ea;

	/* we're using the COP1 macros, but the fields have diff'nt meanings */
	o = OP(op);
	rn = FMT(op);
	off = op;
	ea = REG(ur, rn) + off;
	rd = REGT(op);
//iprint("fpemu: ld/st (F%d)=%#lux + %d => ea %#lux\n", rn, REG(ur, rn), off, ea);

	size = 4;
	if (o == LDC1 || o == SDC1)
		size = 8;
	wr = (o == SWC1 || o == SDC1);
	validaddr(ea, size, wr);

	switch (o) {
	case LWC1:	/* load an fp register, rd, from memory */
	case LDC1:	/* load an fp register pair, (rd, rd+1), from memory */
		fld(rd, ea, size, ufp);
		break;
	case SWC1:	/* store an fp register, rd, into memory */
	case SDC1:	/* store an fp register pair, (rd, rd+1), into memory */
		fst(ea, rd, size, ufp);
		break;
	default:
		unimp(ur->pc, op, "unknown non-COP1 load or store");
		return Failed;
	}
	return Advpc;
}

static int
cop1mov(Instr *ip)
{
	int fs, rt;
	uvlong vl;
	FPsave *ufp;
	Ureg *ur;

	fs = ip->rm;		/* F(s) aka rm */
	rt = ip->rn;		/* R(t) aka rn */
	ur = ip->ur;
	ufp = ip->ufp;
//iprint("fpemu: cop1 prob ld/st (R%d)=%#lux FREG%d\n", rn, REG(ip->ur, rn), rm);

	/* MIPS fp register pairs are in little-endian order: low word first */
	switch (ip->fmt) {
	case MTC1:
		/* load an fp register, F(s), from cpu register R(t) */
		fld(fs, (uintptr)&REG(ur, rt), 4, ufp);
		return Advpc;
	case DMTC1:
		/*
		 * load an fp register pair, (F(s), F(s+1)),
		 * from cpu registers (rt, rt+1)
		 */
		iprint("fpemu: 64-bit DMTC1 may have words backward\n");
		rt &= ~1;
		vl = (uvlong)REG(ur, rt+1) << 32 | REG(ur, rt);
		fld(fs & ~1, (uintptr)&vl, 8, ufp);
		return Advpc;
	case MFC1:
		/* store an fp register, fs, into a cpu register rt */
		fst((uintptr)&REG(ur, rt), fs, 4, ufp);
		return Advpc;
	case DMFC1:
		/*
		 * store an fp register pair, (F(s), F(s+1)),
		 * into cpu registers (rt, rt+1)
		 */
		iprint("fpemu: 64-bit DMFC1 may have words backward\n");
		fst((uintptr)&vl, fs & ~1, 8, ufp);
		rt &= ~1;
		REG(ur, rt) = (ulong)vl;
		REG(ur, rt+1) = vl>>32;
		return Advpc;
	case CFC1:
		switch (fs) {
		case Fpimp:			/* MOVW FCR0,Rn */
			REG(ur, rt) = 0x500;	/* claim to be r4k */
			break;
		case Fpcsr:
			REG(ur, rt) = ufp->fpcontrol;
			break;
		}
		if(DBG(Dbgbasic))
			iprint("MOVW	FCR%d, R%d\n", fs, rt);
		return Advpc;
	case CTC1:
		switch (fs) {
		case Fpcsr:
			ufp->fpcontrol = REG(ur, rt);
			break;
		}
		if(DBG(Dbgbasic))
			iprint("MOVW	R%d, FCR%d\n", rt, fs);
		return Advpc;
	}
	return Nomatch;			/* not a load or store; keep looking */
}

static char *
decodefmt(int fmt)
{
	if (fmtnames[fmt])
		return fmtnames[fmt];
	else
		return "GOK";
}

static char *
predname(int pred)			/* predicate name */
{
	if (prednames[pred])
		return prednames[pred];
	else
		return "GOK";
}

static int
fcmpf(Internal m, Internal n, int, int cond)
{
	int i;

	if(IsWeird(&m) || IsWeird(&n)){
		/* BUG: should trap if not masked */
		return 0;
	}
	fpiround(&n);
	fpiround(&m);
	i = fpicmp(&m, &n);		/* returns -1, 0, or 1 */
	switch (cond) {
	case 0:			/* F - false */
	case 1:			/* UN - unordered */
		return 0;
	case 2:			/* EQ */
	case 3:			/* UEQ */
		return i == 0;
	case 4:			/* OLT */
	case 5:			/* ULT */
		return i < 0;
	case 6:			/* OLE */
	case 7:			/* ULE */
		return i <= 0;
	case 8:			/* SF */
	case 9:			/* NGLE - not >, < or = */
		return 0;
	case 10:		/* SEQ */
		return i == 0;
	case 11:		/* NGL */
		return i != 0;
	case 12:		/* LT */
	case 13:		/* NGE */
		return i < 0;
	case 14:		/* LE */
	case 15:		/* NGT */
		return i <= 0;
	}
	return 0;
}

/*
 * assuming that ur->pc points to a branch instruction,
 * change it to point to the branch's target and return it.
 */
static uintptr
followbr(Ureg *ur)
{
	uintptr npc;

	npc = branch(ur, up->fpsave.fpstatus);
	if(npc == 0)
		panic("fpemu: branch expected but not seen at %#p", ur->pc);
	ur->pc = npc;
	return npc;
}

/* emulate COP1 instruction in branch delay slot */
static void
dsemu(Instr *ip, ulong dsinsn, Ureg *ur, FPsave *ufp)
{
	uintptr npc;

	npc = ur->pc;		/* save ur->pc since fpemu will change it */
	if(DBG(Dbgdelay))
		iprint(">>> emulating br delay slot\n");

	fpimips(ip->pc + 4, dsinsn, ur, ufp);

	if(DBG(Dbgdelay))
		iprint("<<< done emulating br delay slot\n");
	ur->pc = npc;
}

/*
 * execute non-COP1 instruction in branch delay slot, in user mode with
 * user registers, then trap so we can finish up and take the branch.
 */
static void
dsexec(Instr *ip, Ureg *ur, FPsave *ufp)
{
	ulong dsaddr, wpaddr;
	Tos *tos;

	/*
	 * copy delay slot, EHB, EHB, EHB to tos->kscr, flush caches,
	 * point pc there, set watch point on tos->kscr[2], return.
	 * this is safe since we've already checked for branches (and FP
	 * instructions) in the delay slot, so the instruction can be
	 * executed at any address.
	 */
	dsaddr = ip->pc + 4;
	tos = (Tos*)(USTKTOP-sizeof(Tos));
	tos->kscr[0] = *(ulong *)dsaddr;
	tos->kscr[1] = 0xc0;		/* EHB; we could use some trap instead */
	tos->kscr[2] = 0xc0;			/* EHB */
	tos->kscr[3] = 0xc0;			/* EHB */
	dcflush(tos->kscr, sizeof tos->kscr);
	icflush(tos->kscr, sizeof tos->kscr);

	wpaddr = (ulong)&tos->kscr[2] & ~7;	/* clear I/R/W bits */
	ufp->fpdelayexec = 1;
	ufp->fpdelaypc = ip->pc;		/* remember branch ip->pc */
	ufp->fpdelaysts = ufp->fpstatus;	/* remember state of FPCOND */
	ur->pc = (ulong)tos->kscr;		/* restart in tos */
	qlock(&watchlock);			/* wait for first watchpoint */
	setwatchlo0(wpaddr | 1<<2);	/* doubleword addr(!); i-fetches only */
	setwatchhi0(TLBPID(tlbvirt())<<16);	/* asid; see mmu.c */
	if (DBG(Dbgdelay))
		iprint("fpemu: set %s watch point at %#lux, after br ds %#lux...",
			up->text, wpaddr, *(ulong *)dsaddr);
	/* return to user mode, await fpwatch() trap */
}

void
fpwatch(Ureg *ur)			/* called on watch-point trap */
{
	FPsave *ufp;

	ufp = &up->fpsave;
	if(ufp->fpdelayexec == 0)
		panic("fpwatch: unexpected watch trap");

	/* assume we got here after branch-delay-slot execution */
	ufp->fpdelayexec = 0;
	setwatchlo0(0);
	setwatchhi0(0);
	qunlock(&watchlock);

	ur->pc = ufp->fpdelaypc;	/* pc of fp branch */
	ur->cause &= BD;		/* take no chances */
	ufp->fpstatus = ufp->fpdelaysts;
	followbr(ur);			/* sets ur->pc to fp branch target */
	if (DBG(Dbgdelay))
		iprint("delay slot executed; resuming at %#lux\n", ur->pc);
}

static ulong
validiw(uintptr pc)
{
	validaddr(pc, 4, 0);
	return *(ulong*)pc;
}

/*
 * COP1 (6) | BRANCH (5) | cc (3) | likely | true | offset(16)
 *	cc = ip->rn >> 2;			// assume cc == 0
 */
static int
bremu(Instr *ip)
{
	int off, taken;
	ulong dsinsn;
	FPsave *ufp;
	Ureg *ur;

	if (ip->iw & (1<<17))
		error("fpuemu: `likely' fp branch (obs)");
	ufp = ip->ufp;
	if (ufp->fpstatus & FPCOND)
		taken = ip->iw & (1<<16);	/* taken iff BCT */
	else
		taken = !(ip->iw & (1<<16));	/* taken iff BCF */
	dsinsn = validiw(ip->pc + 4);		/* delay slot addressible? */
	if(DBG(Dbgdelay)){
		off = (short)(ip->iw & MASK(16));
		iprint("BFP%c\t%d(PC): %staken\n", (ip->iw & (1<<16)? 'T': 'F'),
			off, taken? "": "not ");
		iprint("\tdelay slot: %08lux\n", dsinsn);
		delay(75);
	}
	ur = ip->ur;
	assert(ur->pc == ip->pc);
	if(!taken)
		return Advpc;	/* didn't branch, so return to delay slot */

	/*
	 * fp branch taken; emulate or execute the delay slot, then jump.
	 */
	if(dsinsn == NOP || dsinsn == MIPSNOP){
		;				/* delay slot does nothing */
	}else if(isbranch((ulong *)(ip->pc + 4)))
		error("fpuemu: branch in fp branch delay slot");
	else if (isfpop(dsinsn))
		dsemu(ip, dsinsn, ur, ufp);	/* emulate delay slot */
	else{
		/*
		 * The hard case: we need to execute the delay slot
		 * in user mode with user registers.  Set a watch point,
		 * return to user mode, await fpwatch() trap.
		 */
		dsexec(ip, ur, ufp);
		return Leavepcret;
	}
	followbr(ur);
	return Leavepc;
}

/* interpret fp reg as fmt (float or double) and convert to Internal */
static void
reg2intern(Internal *i, int reg, int fmt, Ureg *ur)
{
	Double d;
	FPsave *ufp;

	/* we may see other fmt types on conversion or unary ops; ignore */
	ufp = &up->fpsave;
	switch (fmt) {
	case Ffloat:
		fpis2i(i, &FREG(ufp, reg));
		internsane(i, ur);
		break;
	case Fdouble:
		dreg2dbl(&d, reg, ufp);
		fpid2i(i, &d);
		internsane(i, ur);
		break;
	default:
		SetQNaN(i);		/* cause trouble if we try to use i */
		break;
	}
}

/* convert Internal to fp reg as fmt (float or double) */
static void
intern2reg(int reg, Internal *i, int fmt, Ureg *ur)
{
	Double d;
	FPsave *ufp;
	Internal tmp;

	ufp = &up->fpsave;
	tmp = *i;		/* make a disposable copy */
	internsane(&tmp, ur);
	switch (fmt) {
	case Ffloat:
		fpii2s(&FREG(ufp, reg), &tmp);
		break;
	case Fdouble:
		fpii2d(&d, &tmp);
		dbl2dreg(reg, &d, ufp);
		break;
	default:
		panic("intern2reg: bad fmt %d", fmt);
	}
}

/*
 * comparisons - encoded slightly differently than arithmetic:
 * COP1 (6) | fmt(5) | ft (5) | fs (5) | # same
 *	cc (3) | 0 | A=0 |		# diff, was REGD
 *	FC=11 | cond (4)		# FUNC
 */
static int
cmpemu(Instr *ip)
{
	int cc, cond;

	cc = ip->rd >> 2;
	cond = ip->o & MASK(4);
	reg2intern(ip->fn, ip->rn, ip->fmt, ip->ur);
	/* fpicmp args are swapped, so this is `n compare m' */
	if (fcmpf(*ip->fm, *ip->fn, cc, cond))
		ip->ufp->fpstatus |= FPCOND;
	else
		ip->ufp->fpstatus &= ~FPCOND;
	if(DBG(Dbgbasic))
		iprint("CMP%s.%s	F%d,F%d =%d\n", predname(cond), ip->dfmt,
			ip->rm, ip->rn, (ip->ufp->fpstatus & FPCOND? 1: 0));
	if(DBG(Dbgregs)) {
		intpr(ip->fm, ip->rm, ip->fmt, ip->ufp);
		intpr(ip->fn, ip->rn, ip->fmt, ip->ufp);
		delay(75);
	}
	return Advpc;
}

static int
binemu(Instr *ip)
{
	FP2 *fp;
	Internal fd, prfd;
	Internal *fn;

	fp = &optab2[ip->o];
	if(fp->f == nil)
		unimp(ip->pc, ip->iw, "missing binary op");

	/* convert the second operand */
	fn = ip->fn;
	reg2intern(fn, ip->rn, ip->fmt, ip->ur);
	if(DBG(Dbgregs))
		intpr(fn, ip->rn, ip->fmt, ip->ufp);

	if(DBG(Dbgbasic)){
		iprint("%s.%s\tF%d,F%d,F%d\n", fp->name, ip->dfmt,
			ip->rm, ip->rn, ip->rd);
		delay(75);
	}
	/*
	 * fn and fm are scratch Internals just for this instruction,
	 * so it's okay to let the fpi routines trash them in the course
	 * of operation.
	 */
	/* NB: fpi routines take m and n (s and t) in reverse order */
	(*fp->f)(fn, ip->fm, &fd);

	/* convert the result */
	if(DBG(Dbgregs))
		prfd = fd;			/* intern2reg modifies fd */
	intern2reg(ip->rd, &fd, ip->fmt, ip->ur);
	if(DBG(Dbgregs))
		intpr(&prfd, ip->rd, ip->fmt, ip->ufp);
	return Advpc;
}

static int
unaryemu(Instr *ip)
{
	int o;
	FP1 *fp;
	FPsave *ufp;

	o = ip->o;
	fp = &optab1[o];
	if(DBG(Dbgbasic)){
		iprint("%s.%s\tF%d,F%d\n", fp->name, ip->dfmt, ip->rm, ip->rd);
		delay(75);
	}
	if(o == 6){			/* MOV */
		int rm, rd;

		ufp = ip->ufp;
		rd = ip->rd;
		rm = ip->rm;
		if(ip->fmt == Fdouble){
			rd &= ~1;
			rm &= ~1;
			FREG(ufp, rd+1) = FREG(ufp, rm+1);
		}
		FREG(ufp, rd) = FREG(ufp, rm);
	}else{
		Internal fdint, prfd;
		Internal *fd;

		switch(o){
		case 5:			/* ABS */
			fd = ip->fm;	/* use src Internal as dest */
			fd->s = 0;
			break;
		case 7:			/* NEG */
			fd = ip->fm;	/* use src Internal as dest */
			fd->s ^= 1;
			break;
		default:
			if(fp->f == nil)
				unimp(ip->pc, ip->iw, "missing unary op");
			fd = &fdint;
			(*fp->f)(ip->fm, fd);
			break;
		}
		if(DBG(Dbgregs))
			prfd = *fd;		/* intern2reg modifies fd */
		intern2reg(ip->rd, fd, ip->fmt, ip->ur);
		if(DBG(Dbgregs))
			intpr(&prfd, ip->rd, ip->fmt, ip->ufp);
	}
	return Advpc;
}

static int
cvtemu(Instr *ip)
{
	FPcvt *fp;

	fp = &optabcvt[ip->o];
	if(fp->f == nil)
		unimp(ip->pc, ip->iw, "missing conversion op");
	if(DBG(Dbgbasic)){
		iprint("%s.%s\tF%d,F%d\n", fp->name, ip->dfmt, ip->rm, ip->rd);
		delay(75);
	}
	(*fp->f)(ip->fmt, ip->rm, ip->rd, ip->ur, ip->ufp);
	return Advpc;
}

static void
cop1decode(Instr *ip, ulong iw, ulong pc, Ureg *ur, FPsave *ufp,
	Internal *imp, Internal *inp)
{
	ip->iw = iw;
	ip->pc = pc;
	ip->ur = ur;
	ip->ufp = ufp;
	ip->fmt = FMT(iw);
	ip->rm = REGS(iw);		/* 1st operand */
	ip->rn = REGT(iw);		/* 2nd operand (ignored by unary ops) */
	ip->rd = REGD(iw);		/* destination */
	ip->o = FUNC(iw);
	ip->fm = imp;
	ip->fn = inp;
	if (DBG(Dbgbasic))
		ip->dfmt = decodefmt(ip->fmt);
}

void
fpstuck(uintptr pc, FPsave *fp)
{
	USED(pc);
	if(!(DBG(Dbgbasic)))
		return;
	if (fp->fppc == pc) {
		fp->fpcnt++;
		if (fp->fpcnt > 4)
			panic("fpuemu: cpu%d stuck at pid %ld %s pc %#p "
				"instr %#8.8lux", m->machno, up->pid, up->text,
				pc, *(ulong *)pc);
	} else {
		fp->fppc = pc;
		fp->fpcnt = 0;
	}
}

static void
_dbgstuck(ulong pc, Ureg *ur, FPsave *ufp)
{
	fpstuck(pc, ufp);
	if (DBG(Dbgdelay) && ur->cause & BD)
		iprint("fpuemu: FP in a branch delay slot\n");
}

/* decode the opcode and call common emulation code */
static int
fpimips(ulong pc, ulong op, Ureg *ur, FPsave *ufp)
{
	int r, o;
	Instr insn;
	Instr *ip;
	Internal im, in;

	/* note: would update fault status here if we noted numeric exceptions */
	dummyr0 = 0;
	switch (OP(op)) {
	case LWC1:
	case LDC1:
	case SWC1:
	case SDC1:
		dbgstuck(pc, ur, ufp);
		return ldst(op, ur, ufp);
	default:
		unimp(pc, op, "non-FP instruction");
		return Failed;
	case COP1:
		dbgstuck(pc, ur, ufp);
		break;
	}

	ip = &insn;
	cop1decode(ip, op, pc, ur, ufp, &im, &in);
	if (ip->fmt == BRANCH) {		/* FP conditional branch? */
		r = bremu(ip);
		if(DBG(Dbgdelay)){
			iprint("resuming after br, at %#lux", ur->pc);
			if (r == Leavepcret)
				iprint("...");	/* we'll be right back */
			else
				iprint("\n");
		}
		return r;
	}
	o = ip->o;
	if (o == 0 && ip->rd == 0) {	/* *[TF]C1 load or store? */
		r = cop1mov(ip);
		if (r != Nomatch)
			return r;
		/* else wasn't a [tf]c1 move */
	}
	/* don't decode & print rm yet; it might be an integer */
	if(o >= 32 && o < 40)		/* conversion? */
		return cvtemu(ip);

	/* decode the mandatory operand, rm */
	reg2intern(ip->fm, ip->rm, ip->fmt, ip->ur);
	if(DBG(Dbgregs))
		intpr(&im, ip->rm, ip->fmt, ip->ufp);

	/*
	 * arithmetic
	 * all operands must be of the same format
	 */
	if(o >= 4 && o < 32)		/* monadic */
		return unaryemu(ip);
	if(o < 4)			/* the few binary ops */
		return binemu(ip);

	if(o >= 48 && (ip->rd & MASK(2)) == 0)	/* comparison? */
		return cmpemu(ip);

	/* don't recognise the opcode */
	if(DBG(Dbgbasic))
		iprint("fp at %#lux: %#8.8lux BOGON\n", pc, op);
	unimp(pc, op, "unknown opcode");
	return Failed;
}

static FPsave *
fpinit(Ureg *ur)
{
	int i, n;
	Double d;
	FPsave *ufp;
	Internal tmp;

	/*
	 * because all the emulated fp state is in the proc structure,
	 * it need not be saved/restored
	 */
	ufp = &up->fpsave;
	switch(up->fpstate){
	case FPactive:
	case FPinactive:
		error("fpu (in)active but fp is emulated");
	case FPinit:
		up->fpstate = FPemu;
		ufp->fpcontrol = 0;
		ufp->fpstatus = 0;
		ufp->fpcnt = 0;
		ufp->fppc = 0;
		for(n = 0; n < Nfpregs-1; n += 2) {
			if (fpconst[n].h == 0)	/* uninitialised consts */
				i = FZERO;	/* treated as 0.0 */
			else
				i = n;
			tmp = fpconst[i];
			internsane(&tmp, ur);
			fpii2d(&d, &tmp);
			dbl2dreg(n, &d, ufp);
		}
		break;
	}
	return ufp;
}

/*
 * called from trap.c's CCPU case, only to deal with user-mode
 * instruction faults.  
 *
 * libc/mips/lock.c reads FCR0 to determine what kind of system
 * this is (and thus if it can use LL/SC or must use some
 * system-dependent method).  So we simulate the move from FCR0.
 * All modern mips have LL/SC, so just claim to be an r4k.
 */
int
fpuemu(Ureg *ureg)
{
	int s;
	uintptr pc;
	ulong iw, r;

	if(waserror()){
		postnote(up, 1, up->errstr, NDebug);
		return -1;
	}

	if(up->fpstate & FPillegal)
		error("floating point in note handler");
	if(up->fpsave.fpdelayexec)
		panic("fpuemu: entered with outstanding watch trap");

	pc = ureg->pc;
	validaddr(pc, 4, 0);
	/* only the first instruction can be in a branch delay slot */
	if(ureg->cause & BD) {
		pc += 4;
		validaddr(pc, 4, 0);		/* check branch delay slot */
	}
	iw = *(ulong*)pc;
	do {
		/* recognise & optimise a common case */
		if (iw == 0x44410000){		/* MOVW FCR0,R1 (CFC1) */
			ureg->r1 = 0x500;	/* claim an r4k */
			r = Advpc;
			if (DBG(Dbgbasic))
				iprint("faked MOVW FCR0,R1\n");
		}else{
			s = spllo();
			if(waserror()){
				splx(s);
				nexterror();
			}
			r = fpimips(pc, iw, ureg, fpinit(ureg));
			splx(s);
			poperror();
			if (r == Failed || r == Leavepcret)
				break;
		}
		if (r == Advpc)	/* simulation succeeded, advance the pc? */
			if(ureg->cause & BD)
				followbr(ureg);
			else
				ureg->pc += 4;
		ureg->cause &= ~BD;

		pc = ureg->pc;
		iw = validiw(pc);
		while (iw == NOP || iw == MIPSNOP) {	/* skip NOPs */
			pc += 4;
			ureg->pc = pc;
			iw = validiw(pc);
		}
		/* is next ins'n also FP? */
	} while (isfpop(iw));
	if (r == Failed){
		iprint("fpuemu: fp emulation failed for %#lux"
			" at pc %#p in %lud %s\n",
			iw, ureg->pc, up->pid, up->text);
		unimp(ureg->pc, iw, "no fp instruction");
		/* no return */
	}
	ureg->cause &= ~BD;
	poperror();
	return 0;
}

int
isbranch(ulong *pc)
{
	ulong iw;

	iw = *(ulong*)pc;
	/*
	 * Integer unit jumps first
	 */
	switch(iw>>26){
	case 0:			/* SPECIAL: JR or JALR */
		switch(iw&0x3F){
		case 0x09:	/* JALR */
		case 0x08:	/* JR */
			return 1;
		default:
			return 0;
		}
	case 1:			/* BCOND */
		switch((iw>>16) & 0x1F){
		case 0x10:	/* BLTZAL */
		case 0x00:	/* BLTZ */
		case 0x11:	/* BGEZAL */
		case 0x01:	/* BGEZ */
			return 1;
		default:
			return 0;
		}
	case 3:			/* JAL */
	case 2:			/* JMP */
	case 4:			/* BEQ */
	case 5:			/* BNE */
	case 6:			/* BLEZ */
	case 7:			/* BGTZ */
		return 1;
	}
	/*
	 * Floating point unit jumps
	 */
	if((iw>>26) == COP1)
		switch((iw>>16) & 0x3C1){
		case 0x101:	/* BCT */
		case 0x181:	/* BCT */
		case 0x100:	/* BCF */
		case 0x180:	/* BCF */
			return 1;
		}
	return 0;
}

/*
 * if current instruction is a (taken) branch, return new pc and,
 * for jump-and-links, set r31.
 */
ulong
branch(Ureg *ur, ulong fcr31)
{
	ulong iw, npc, rs, rt, rd, offset, targ, next;

	iw = ur->pc;
	iw = *(ulong*)iw;
	rs = (iw>>21) & 0x1F;
	if(rs)
		rs = REG(ur, rs);
	rt = (iw>>16) & 0x1F;
	if(rt)
		rt = REG(ur, rt);
	offset = iw & ((1<<16)-1);
	if(offset & (1<<15))	/* sign extend */
		offset |= ~((1<<16)-1);
	offset <<= 2;
	targ = ur->pc + 4 + offset;	/* branch target */
	/* ins'n after delay slot (assumes delay slot has already been exec'd) */
	next = ur->pc + 8;
	/*
	 * Integer unit jumps first
	 */
	switch(iw>>26){
	case 0:			/* SPECIAL: JR or JALR */
		switch(iw&0x3F){
		case 0x09:	/* JALR */
			rd = (iw>>11) & 0x1F;
			if(rd)
				REG(ur, rd) = next;
			/* fall through */
		case 0x08:	/* JR */
			return rs;
		default:
			return 0;
		}
	case 1:			/* BCOND */
		switch((iw>>16) & 0x1F){
		case 0x10:	/* BLTZAL */
			ur->r31 = next;
			/* fall through */
		case 0x00:	/* BLTZ */
			if((long)rs < 0)
				return targ;
			return next;
		case 0x11:	/* BGEZAL */
			ur->r31 = next;
			/* fall through */
		case 0x01:	/* BGEZ */
			if((long)rs >= 0)
				return targ;
			return next;
		default:
			return 0;
		}
	case 3:			/* JAL */
		ur->r31 = next;
		/* fall through */
	case 2:			/* JMP */
		npc = iw & ((1<<26)-1);
		npc <<= 2;
		return npc | (ur->pc&0xF0000000);
	case 4:			/* BEQ */
		if(rs == rt)
			return targ;
		return next;
	case 5:			/* BNE */
		if(rs != rt)
			return targ;
		return next;
	case 6:			/* BLEZ */
		if((long)rs <= 0)
			return targ;
		return next;
	case 7:			/* BGTZ */
		if((long)rs > 0)
			return targ;
		return next;
	}
	/*
	 * Floating point unit jumps
	 */
	if((iw>>26) == COP1)
		switch((iw>>16) & 0x3C1){
		case 0x101:	/* BCT */
		case 0x181:	/* BCT */
			if(fcr31 & FPCOND)
				return targ;
			return next;
		case 0x100:	/* BCF */
		case 0x180:	/* BCF */
			if(!(fcr31 & FPCOND))
				return targ;
			return next;
		}
	/* shouldn't get here */
	return 0;
}
