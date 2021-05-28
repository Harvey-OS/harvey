/*
 * this doesn't attempt to implement Power architecture floating-point properties
 * that aren't visible in the Inferno environment.
 * all arithmetic is done in double precision.
 * the FP trap status isn't updated.
 */
#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"

#include	"ureg.h"

#include	"fpi.h"

#define	REG(x) (*(long*)(((char*)em->ur)+roff[(x)]))
#define	FR(x) (*(Internal*)em->fr[(x)&0x1F])
#define	REGSP	0	/* in Plan 9, the user and kernel stacks are different */

#define	getulong(a) (*(ulong*)(a))

enum {
	CRLT = 1<<31,
	CRGT = 1<<30,
	CREQ = 1<<29,
	CRSO = 1<<28,
	CRFU = CRSO,

	CRFX = 1<<27,
	CRFEX = 1<<26,
	CRVX = 1<<25,
	CROX = 1<<24,
};

#define getCR(x,w) (((w)>>(28-(x*4)))&0xF)
#define mkCR(x,v) (((v)&0xF)<<(28-(x*4)))

#define simm(xx, ii)	xx = (short)(ii&0xFFFF);
#define getairr(i)	rd = (i>>21)&0x1f; ra = (i>>16)&0x1f; simm(imm,i)
#define getarrr(i)	rd = (i>>21)&0x1f; ra = (i>>16)&0x1f; rb = (i>>11)&0x1f;
#define getop(i) ((i>>26)&0x3F)
#define getxo(i) ((i>>1)&0x3FF)

#define	FPS_FX	(1<<31)	/* exception summary (sticky) */
#define	FPS_EX	(1<<30)	/* enabled exception summary */
#define	FPS_VX	(1<<29)	/* invalid operation exception summary */
#define	FPS_OX	(1<<28)	/* overflow exception OX (sticky) */
#define	FPS_UX	(1<<27)	/* underflow exception UX (sticky) */
#define	FPS_ZX	(1<<26)	/* zero divide exception ZX (sticky) */
#define	FPS_XX	(1<<25)	/* inexact exception XX (sticky) */
#define	FPS_VXSNAN (1<<24)	/* invalid operation exception for SNaN (sticky) */
#define	FPS_VXISI	(1<<23)	/* invalid operation exception for ∞-∞ (sticky) */
#define	FPS_VXIDI	(1<<22)	/* invalid operation exception for ∞/∞ (sticky) */
#define	FPS_VXZDZ (1<<21)	/* invalid operation exception for 0/0 (sticky) */
#define	FPS_VXIMZ	(1<<20)	/* invalid operation exception for ∞*0 (sticky) */
#define	FPS_VXVC	(1<<19)	/* invalid operation exception for invalid compare (sticky) */
#define	FPS_FR	(1<<18)	/* fraction rounded */
#define	FPS_FI	(1<<17)	/* fraction inexact */
#define	FPS_FPRF	(1<<16)	/* floating point result class */
#define	FPS_FPCC	(0xF<<12)	/* <, >, =, unordered */
#define	FPS_VXCVI	(1<<8)	/* enable exception for invalid integer convert (sticky) */
#define	FPS_VE	(1<<7)	/* invalid operation exception enable */
#define	FPS_OE	(1<<6)	/* enable overflow exceptions */
#define	FPS_UE	(1<<5)	/* enable underflow */
#define	FPS_ZE	(1<<4)	/* enable zero divide */
#define	FPS_XE	(1<<3)	/* enable inexact exceptions */
#define	FPS_RN	(3<<0)	/* rounding mode */

typedef struct Emreg Emreg;

struct Emreg {
	Ureg*	ur;
	ulong	(*fr)[3];
	FPsave*	ufp;
	ulong	ir;
	char*	name;
};

int	fpemudebug = 0;

#undef OFR
#define	OFR(X)	((ulong)&((Ureg*)0)->X)

static	int	roff[] = {
	OFR(r0), OFR(r1), OFR(r2), OFR(r3),
	OFR(r4), OFR(r5), OFR(r6), OFR(r7),
	OFR(r8), OFR(r9), OFR(r10), OFR(r11),
	OFR(r12), OFR(r13), OFR(r14), OFR(r15),
	OFR(r16), OFR(r17), OFR(r18), OFR(r19),
	OFR(r20), OFR(r21), OFR(r22), OFR(r23),
	OFR(r24), OFR(r25), OFR(r26), OFR(r27),
	OFR(r28), OFR(r29), OFR(r30), OFR(r31),
};

/*
 * initial FP register values assumed by qc's code
 */
static Internal fpreginit[] = {
	/* s, e, l, h */
	{0, 0x400, 0x00000000, 0x08000000},	/* F31=2.0 */
	{0, 0x3FF, 0x00000000, 0x08000000},	/* F30=1.0 */
	{0, 0x3FE, 0x00000000, 0x08000000},	/* F29=0.5 */
	{0, 0x1, 0x00000000, 0x00000000}, /* F28=0.0 */
	{0, 0x433, 0x00000000, 0x08000040},	/* F27=FREGCVI */
};

static void
fadd(Emreg *em, Internal *d, int ra, int rb)
{
	Internal a, b;

	a = FR(ra);
	b = FR(rb);
	(a.s == b.s? fpiadd: fpisub)(&b, &a, d);
}

static void
fsub(Emreg *em, Internal *d, int ra, int rb)
{
	Internal a, b;

	a = FR(ra);
	b = FR(rb);
	b.s ^= 1;
	(b.s == a.s? fpiadd: fpisub)(&b, &a, d);
}

static void
fmul(Emreg *em, Internal *d, int ra, int rb)
{
	Internal a, b;

	a = FR(ra);
	b = FR(rb);
	fpimul(&b, &a, d);
}

static void
fdiv(Emreg *em, Internal *d, int ra, int rb)
{
	Internal a, b;

	a = FR(ra);
	b = FR(rb);
	fpidiv(&b, &a, d);
}

static void
fmsub(Emreg *em, Internal *d, int ra, int rc, int rb)
{
	Internal a, c, b, t;

	a = FR(ra);
	c = FR(rc);
	b = FR(rb);
	fpimul(&a, &c, &t);
	b.s ^= 1;
	(b.s == t.s? fpiadd: fpisub)(&b, &t, d);
}

static void
fmadd(Emreg *em, Internal *d, int ra, int rc, int rb)
{
	Internal a, c, b, t;

	a = FR(ra);
	c = FR(rc);
	b = FR(rb);
	fpimul(&a, &c, &t);
	(t.s == b.s? fpiadd: fpisub)(&b, &t, d);
}

static ulong	setfpscr(Emreg*);
static void	setfpcc(Emreg*, int);

static void
unimp(Emreg *em, ulong op)
{
	char buf[60];

	snprint(buf, sizeof(buf), "sys: fp: pc=%lux unimp fp 0x%.8lux", em->ur->pc, op);
	if(fpemudebug)
		print("FPE: %s\n", buf);
	error(buf);
	/* no return */
}

/*
 * floating load/store
 */

static void
fpeairr(Emreg *em, ulong ir, void **eap, int *rdp)
{
	ulong ea;
	long imm;
	int ra, rd, upd;

	getairr(ir);
	ea = imm;
	upd = (ir&(1L<<26))!=0;
	if(ra) {
		ea += REG(ra);
		if(upd){
			if(REGSP && ra == REGSP)
				panic("fpemu: r1 update");	/* can't do it because we're running on the same stack */
			REG(ra) = ea;
		}
	} else {
		if(upd)
			unimp(em, ir);
	}
	*rdp = rd;
	*eap = (void*)ea;
	if(fpemudebug)
		print("%8.8lux %s\tf%d,%ld(r%d) ea=%lux upd=%d\n", em->ur->pc, em->name, rd, imm, ra, ea, upd);
}

static void
fpearrr(Emreg *em, ulong ir, int upd, void **eap, int *rdp)
{
	ulong ea;
	int ra, rb, rd;

	getarrr(ir);
	ea = REG(rb);
	if(ra){
		ea += REG(ra);
		if(upd){
			if(REGSP && ra == REGSP)
				panic("fpemu: r1 update");
			REG(ra) = ea;
		}
		if(fpemudebug)
			print("%8.8lux %s\tf%d,(r%d+r%d) ea=%lux upd=%d\n", em->ur->pc, em->name, rd, ra, rb, ea, upd);
	} else {
		if(upd)
			unimp(em, ir);
		if(fpemudebug)
			print("%8.8lux %s\tf%d,(r%d) ea=%lux\n", em->ur->pc, em->name, rd, rb, ea);
	}
	*eap = (void*)ea;
	*rdp = rd;
}

static void
lfs(Emreg *em, ulong ir)
{
	void *ea;
	int rd;

	em->name = "lfs";
	fpeairr(em, ir, &ea, &rd);
	fpis2i(&FR(rd), (void*)ea);
}

static void
lfsx(Emreg *em, ulong ir)
{
	void *ea;
	int rd;

	em->name = "lfsx";
	fpearrr(em, ir, ((ir>>1)&0x3FF)==567, &ea, &rd);
	fpis2i(&FR(rd), (void*)ea);
}

static void
lfd(Emreg *em, ulong ir)
{
	void *ea;
	int rd;

	em->name = "lfd";
	fpeairr(em, ir, &ea, &rd);
	fpid2i(&FR(rd), (void*)ea);
}

static void
lfdx(Emreg *em, ulong ir)
{
	void *ea;
	int rd;

	em->name = "lfdx";
	fpearrr(em, ir, ((ir>>1)&0x3FF)==631, &ea, &rd);
	fpid2i(&FR(rd), (void*)ea);
}

static void
stfs(Emreg *em, ulong ir)
{
	void *ea;
	int rd;
	Internal tmp;

	em->name = "stfs";
	fpeairr(em, ir, &ea, &rd);
	tmp = FR(rd);
	fpii2s(ea, &tmp);
}

static void
stfsx(Emreg *em, ulong ir)
{
	void *ea;
	int rd;
	Internal tmp;

	em->name = "stfsx";
	fpearrr(em, ir, getxo(ir)==695, &ea, &rd);
	tmp = FR(rd);
	fpii2s(ea, &tmp);
}

static void
stfd(Emreg *em, ulong ir)
{
	void *ea;
	int rd;
	Internal tmp;

	em->name = "stfd";
	fpeairr(em, ir, &ea, &rd);
	tmp = FR(rd);
	fpii2d(ea, &tmp);
}

static void
stfdx(Emreg *em, ulong ir)
{
	void *ea;
	int rd;
	Internal tmp;

	em->name = "stfdx";
	fpearrr(em, ir, ((ir>>1)&0x3FF)==759, &ea, &rd);
	tmp = FR(rd);
	fpii2d(ea, &tmp);
}

static void
mcrfs(Emreg *em, ulong ir)
{
	int rd, ra, rb;
	static ulong fpscr0[] ={
		FPS_FX|FPS_OX,
		FPS_UX|FPS_ZX|FPS_XX|FPS_VXSNAN,
		FPS_VXISI|FPS_VXIDI|FPS_VXZDZ|FPS_VXIMZ,
		FPS_VXVC,
		0,
		FPS_VXCVI,
	};

	getarrr(ir);
	if(rb || ra&3 || rd&3)
		unimp(em, ir);
	ra >>= 2;
	rd >>= 2;
	em->ur->cr = (em->ur->cr & ~mkCR(rd, 0xF)) | mkCR(rd, getCR(ra, em->ufp->fpscr));
	em->ufp->fpscr &= ~fpscr0[ra];
	if(fpemudebug)
		print("%8.8lux mcrfs\tcrf%d,crf%d\n", em->ur->pc, rd, ra);
}

static void
mffs(Emreg *em, ulong ir)
{
	int rd, ra, rb;
	Double dw;

	getarrr(ir);
	if(ra || rb)
		unimp(em, ir);
	dw.h = 0;
	dw.l = ((uvlong)0xFFF8000L<<16)|em->ufp->fpscr;
	fpid2i(&FR(rd), &dw);
	/* it's anyone's guess how CR1 should be set when ir&1 */
	em->ur->cr &= ~mkCR(1, 0xE);	/* leave SO, reset others */
	if(fpemudebug)
		print("%8.8lux mffs%s\tfr%d\n", em->ur->pc, ir&1?".":"", rd);
}

static void
mtfsb1(Emreg *em, ulong ir)
{
	int rd, ra, rb;

	getarrr(ir);
	if(ra || rb)
		unimp(em, ir);
	em->ufp->fpscr |= (1L << (31-rd));
	/* BUG: should set summary bits */
	if(ir & 1)
		em->ur->cr &= ~mkCR(1, 0xE);	/* BUG: manual unclear: leave SO, reset others? */
	if(fpemudebug)
		print("%8.8lux mtfsb1%s\tfr%d\n", em->ur->pc, ir&1?".":"", rd);
}

static void
mtfsb0(Emreg *em, ulong ir)
{
	int rd, ra, rb;

	getarrr(ir);
	if(ra || rb)
		unimp(em, ir);
	em->ufp->fpscr &= ~(1L << (31-rd));
	if(ir & 1)
		em->ur->cr &= ~mkCR(1, 0xE);		/* BUG: manual unclear: leave SO, reset others? */
	if(fpemudebug)
		print("%8.8lux mtfsb0%s\tfr%d\n", em->ur->pc, ir&1?".":"", rd);
}

static void
mtfsf(Emreg *em, ulong ir)
{
	int fm, rb, i;
	ulong v;
	Internal b;
	Double db;

	if(ir & ((1L << 25)|(1L << 16)))
		unimp(em, ir);
	rb = (ir >> 11) & 0x1F;
	fm = (ir >> 17) & 0xFF;
	b = FR(rb);
	fpii2d(&db, &b);	/* reconstruct hi/lo format to recover low word */
	v = db.l;
	for(i=0; i<8; i++)
		if(fm & (1 << (7-i)))
			em->ufp->fpscr = (em->ufp->fpscr & ~mkCR(i, 0xF)) | mkCR(i, getCR(i, v));
	/* BUG: should set FEX and VX `according to the usual rule' */
	if(ir & 1)
		em->ur->cr &= ~mkCR(1, 0xE);		/* BUG: manual unclear: leave SO, reset others? */
	if(fpemudebug)
		print("%8.8lux mtfsf%s\t#%.2x,fr%d\n", em->ur->pc, ir&1?".":"", fm, rb);
}

static void
mtfsfi(Emreg *em, ulong ir)
{
	int imm, rd;

	if(ir & ((0x7F << 16)|(1L << 11)))
		unimp(em, ir);
	rd = (ir >> 23) & 0xF;
	imm = (ir >> 12) & 0xF;
	em->ufp->fpscr = (em->ufp->fpscr & ~mkCR(rd, 0xF)) | mkCR(rd, imm);
	/* BUG: should set FEX and VX `according to the usual rule' */
	if(ir & 1)
		em->ur->cr &= ~mkCR(1, 0xE);		/* BUG: manual unclear: leave SO, reset others? */
	if(fpemudebug)
		print("%8.8lux mtfsfi%s\tcrf%d,#%x\n", em->ur->pc, ir&1?".":"", rd, imm);
}

static void
fcmp(Emreg *em, ulong ir)
{
	int fc, rd, ra, rb, sig, i;
	Internal a, b;

	getarrr(ir);
	if(rd & 3)
		unimp(em, ir);
	rd >>= 2;
	sig = 0;
	switch(getxo(ir)) {
	default:
		unimp(em, ir);
	case 32:
		if(fpemudebug)
			print("%8.8lux fcmpo\tcr%d,f%d,f%d\n", em->ur->pc, rd, ra, rb);
		sig = 1;
		break;
	case 0:
		if(fpemudebug)
			print("%8.8lux fcmpu\tcr%d,f%d,f%d\n", em->ur->pc, rd, ra, rb);
		break;
	}
	if(IsWeird(&FR(ra)) || IsWeird(&FR(rb))) {
		if(sig){
			;	/* BUG: should trap if not masked ... */
		}
		fc = CRFU;
	} else {
		a = FR(ra);
		b = FR(rb);
		fpiround(&a);
		fpiround(&b);
		i = fpicmp(&a, &b);
		if(i > 0)
			fc = CRGT;
		else if(i == 0)
			fc = CREQ;
		else
			fc = CRLT;
	}
	fc >>= 28;
	em->ur->cr = (em->ur->cr & ~mkCR(rd,~0)) | mkCR(rd, fc);
	em->ufp->fpscr = (em->ufp->fpscr & ~0xF800) | (fc<<11);
	/* BUG: update FX, VXSNAN, VXVC */
}

static void
fariths(Emreg *em, ulong ir)
{
	int rd, ra, rb, rc, fmt;
	char *cc, *n;
	ulong fpscr;
	Internal *d;

	fmt = 0;
	rc = (ir>>6)&0x1F;
	getarrr(ir);
	d = &FR(rd);
	switch(getxo(ir)&0x1F) {	/* partial XO decode */
	case 22:	/* fsqrts */
	case 24:	/* fres */
	default:
		unimp(em, ir);
		return;
	case 18:
		if(IsZero(&FR(rb))) {
			em->ufp->fpscr |= FPS_ZX | FPS_FX;
			error("sys: fp: zero divide");
		}
		fdiv(em, d, ra, rb);
		n = "fdivs";
		break;
	case 20:
		fsub(em, d, ra, rb);
		n = "fsubs";
		break;
	case 21:
		fadd(em, d, ra, rb);
		n = "fadds";
		break;
	case 25:
		fmul(em, d, ra, rc);
		rb = rc;
		n = "fmuls";
		break;
	case 28:
		fmsub(em, d, ra, rc, rb);
		fmt = 2;
		n = "fmsubs";
		break;
	case 29:
		fmadd(em, d, ra, rc, rb);
		fmt = 2;
		n = "fmadds";
		break;
	case 30:
		fmsub(em, d, ra, rc, rb);
		d->s ^= 1;
		fmt = 2;
		n = "fnmsubs";
		break;
	case 31:
		fmadd(em, d, ra, rc, rb);
		d->s ^= 1;
		fmt = 2;
		n = "fnmadds";
		break;
	}
	if(fmt==1 && ra)
		unimp(em, ir);
	fpscr = setfpscr(em);
	setfpcc(em, rd);
	cc = "";
	if(ir & 1) {
		cc = ".";
		em->ur->cr = (em->ur->cr & ~mkCR(1, ~0)) | mkCR(1, (fpscr>>28));
	}
	if(fpemudebug) {
		switch(fmt) {
		case 0:
			print("%8.8lux %s%s\tfr%d,fr%d,fr%d\n", em->ur->pc, n, cc, rd, ra, rb);
			break;
		case 1:
			print("%8.8lux %s%s\tfr%d,fr%d\n", em->ur->pc, n, cc, rd, rb);
			break;
		case 2:
			print("%8.8lux %s%s\tfr%d,fr%d,fr%d,fr%d\n", em->ur->pc, n, cc, rd, ra, rc, rb);
			break;
		}
	}
}

static void
farith(Emreg *em, ulong ir)
{
	Word w;
	Double dv;
	int rd, ra, rb, rc, fmt;
	char *cc, *n;
	ulong fpscr;
	int nocc;
	Internal *d;

	fmt = 0;
	nocc = 0;
	rc = (ir>>6)&0x1F;
	getarrr(ir);
	d = &FR(rd);
	switch(getxo(ir)&0x1F) { /* partial XO decode */
	case 22:	/* frsqrt */
	case 23:	/* fsel */
	case 26:	/* fsqrte */
	default:
		unimp(em, ir);
		return;
	case 12:	/* frsp */
		*d = FR(rb);	/* BUG: doesn't round to single precision */
		fmt = 1;
		n = "frsp";
		break;
	case 14:	/* fctiw */	/* BUG: ignores rounding mode */
	case 15:	/* fctiwz */
		fpii2w(&w, &FR(rb));
		dv.h = 0;
		dv.l = w;
		fpid2i(d, &dv);
		fmt = 1;
		nocc = 1;
		n = "fctiw";
		break;
	case 18:
		if(IsZero(&FR(rb))) {
			em->ufp->fpscr |= FPS_ZX | FPS_FX;
			error("sys: fp: zero divide");
		}
		fdiv(em, d, ra, rb);
		n = "fdiv";
		break;
	case 20:
		fsub(em, d, ra, rb);
		n = "fsub";
		break;
	case 21:
		fadd(em, d, ra, rb);
		n = "fadd";
		break;
	case 25:
		fmul(em, d, ra, rc);
		rb = rc;
		n = "fmul";
		break;
	case 28:
		fmsub(em, d, ra, rc, rb);
		fmt = 2;
		n = "fmsub";
		break;
	case 29:
		fmadd(em, d, ra, rc, rb);
		fmt = 2;
		n = "fmadd";
		break;
	case 30:
		fmsub(em, d, ra, rc, rb);
		d->s ^= 1;
		fmt = 2;
		n = "fnmsub";
		break;
	case 31:
		fmadd(em, d, ra, rc, rb);
		d->s ^= 1;
		fmt = 2;
		n = "fnmadd";
		break;
	}
	if(fmt==1 && ra)
		unimp(em, ir);
	fpscr = setfpscr(em);
	if(nocc == 0)
		setfpcc(em, rd);
	cc = "";
	if(ir & 1) {
		cc = ".";
		em->ur->cr = (em->ur->cr & ~mkCR(1, ~0)) | mkCR(1, (fpscr>>28));
	}
	if(fpemudebug) {
		switch(fmt) {
		case 0:
			print("%8.8lux %s%s\tfr%d,fr%d,fr%d\n", em->ur->pc, n, cc, rd, ra, rb);
			break;
		case 1:
			print("%8.8lux %s%s\tfr%d,fr%d\n", em->ur->pc, n, cc, rd, rb);
			break;
		case 2:
			print("%8.8lux %s%s\tfr%d,fr%d,fr%d,fr%d\n", em->ur->pc, n, cc, rd, ra, rc, rb);
			break;
		}
	}
}

static void
farith2(Emreg *em, ulong ir)
{
	int rd, ra, rb;
	char *cc, *n;
	ulong fpscr;
	Internal *d, *b;

	getarrr(ir);
	if(ra)
		unimp(em, ir);
	d = &FR(rd);
	b = &FR(rb);
	switch(getxo(ir)) { /* full XO decode */
	default:
		unimp(em, ir);
	case 40:
		*d = *b;
		d->s ^= 1;
		n = "fneg";
		break;
	case 72:
		*d = *b;
		n = "fmr";
		break;
	case 136:
		*d = *b;
		d->s = 1;
		n = "fnabs";
		break;
	case 264:
		*d = *b;
		d->s = 0;
		n = "fabs";
		break;
	}
	fpscr = setfpscr(em);
	setfpcc(em, rd);
	cc = "";
	if(ir & 1) {
		cc = ".";
		em->ur->cr = (em->ur->cr & ~mkCR(1, ~0)) | mkCR(1, (fpscr>>28));
	}
	if(fpemudebug)
		print("%8.8lux %s%s\tfr%d,fr%d\n", em->ur->pc, n, cc, rd, rb);
}

static ulong
setfpscr(Emreg *em)
{
	ulong fps, fpscr;

	fps = 0;	/* BUG: getfsr() */
	fpscr = em->ufp->fpscr;
	if(fps & FPAOVFL)
		fpscr |= FPS_OX;
	if(fps & FPAINEX)
		fpscr |= FPS_XX;
	if(fps & FPAUNFL)
		fpscr |= FPS_UX;
	if(fps & FPAZDIV)
		fpscr |= FPS_ZX;
	if(fpscr != em->ufp->fpscr) {
		fpscr |= FPS_FX;
		em->ufp->fpscr = fpscr;
	}
	return fpscr;
}

static void
setfpcc(Emreg *em, int r)
{
	int c;
	Internal *d;

	d = &FR(r);
	c = 0;
	if(IsZero(d))
		c |= 2;
	else if(d->s == 1)
		c |= 4;
	else
		c |= 8;
	if(IsNaN(d))
		c |= 1;
	em->ufp->fpscr = (em->ufp->fpscr & ~0xF800) | (0<<15) | (c<<11); /* unsure about class bit */
}

static	uchar	op63flag[32] = {
[12] 1, [14] 1, [15] 1, [18] 1, [20] 1, [21] 1, [22] 1,
[23] 1, [25] 1, [26] 1, [28] 1, [29] 1, [30] 1, [31] 1,
};

/*
 * returns the number of FP instructions emulated
 */
int
fpipower(Ureg *ur)
{
	ulong op;
	int xo;
	Emreg emreg, *em;
	FPsave *ufp;
	int n;

	ufp = &up->fpsave; /* because all the state is in FPsave, it need not be saved/restored */
	em = &emreg;
	em->ur = ur;
	em->fr = ufp->emreg;
	em->ufp = ufp;
	em->name = nil;
	if(em->ufp->fpistate != FPactive) {
		em->ufp->fpistate = FPactive;
		em->ufp->fpscr = 0;	/* TO DO */
		for(n = 0; n < nelem(fpreginit); n++)
			FR(31-n) = fpreginit[n];
	}
	for(n=0;;n++){
		validaddr(ur->pc, 4, 0);
		op = getulong(ur->pc);
		em->ir = op;
		if(fpemudebug > 1)
			print("%8.8lux %8.8lux: ", ur->pc, op);
		switch(op>>26){
		default:
			return n;
		case 48:	/* lfs */
		case 49:	/* lfsu */
			lfs(em, op);
			break;
		case 50:	/* lfd */
		case 51:	/* lfdu */
			lfd(em, op);
			break;
		case 52:	/* stfs */
		case 53:	/* stfsu */
			stfs(em, op);
			break;
		case 54:	/* stfd */
		case 55:	/* stfdu */
			stfd(em, op);
			break;
		case 31:	/* indexed load/store */
			xo = getxo(op);
			if((xo & 0x300) != 0x200)
				return n;
			switch(xo){
			default:
				return n;
			case 535:	/* lfsx */
			case 567:	/* lfsux */
				lfsx(em, op);
				break;
			case 599:	/* lfdx */
			case 631:	/* lfdux */
				lfdx(em, op);
				break;
			case 663:	/* stfsx */
			case 695:	/* stfsux */
				stfsx(em, op);
				break;
			case 727:	/* stfdx */
			case 759:	/* stfdux */
				stfdx(em, op);
				break;
			}
			break;
		case 63:	/* double precision */
			xo = getxo(op);
			if(op63flag[xo & 0x1F]){
				farith(em, op);
				break;
			}
			switch(xo){
			default:
				return n;
			case 0:	/* fcmpu */
			case 32:	/* fcmpo */
				fcmp(em, op);
				break;
			case 40:	/* fneg */
			case 72:	/* fmr */
			case 136:	/* fnabs */
			case 264:	/* fabs */
				farith2(em, op);
				break;
			case 38:
				mtfsb1(em, op);
				break;
			case 64:
				mcrfs(em, op);
				break;
			case 70:
				mtfsb0(em, op);
				break;
			case 134:
				mtfsfi(em, op);
				break;
			case 583:
				mffs(em, op);
				break;
			case 711:
				mtfsf(em, op);
				break;
			}
			break;
		case 59:	/* single precision */
			fariths(em, op);
			break;
		}
		ur->pc += 4;
		if(anyhigher())
			sched();
	}
}

/*
50:	lfd	frD,d(rA)
51:	lfdu	frD,d(rA)
31,631:	lfdux	frD,rA,rB
31,599:	lfdx	frD,rA,rB
48:	lfs	frD,d(rA)
49:	lfsu	frD,d(rA)
31,567:	lfsux	frD,rA,rB
31,535:	lfsx	frD,rA,rB

54:	stfd	frS,d(rA)
55:	stfdu	frS,d(rA)
31,759:	stfdux	frS,rA,rB
31,727:	stfdx	frS,rA,rB
52:	stfs	frS,d(rA)
53:	stfsu	frS,d(rA)
31,695:	stfsux	frS,rA,rB
31,663:	stfsx	frS,rA,rB

63,64:	mcrfs	crfD,crfS
63,583:	mffs[.]	frD
63,70:	mtfsb0[.]	crbD
63,38:	mtfsb1[.]	crbD
63,711:	mtfsf[.]	FM,frB
63,134:	mtfsfi[.]	crfD,IMM
*/

/*
float to int:
	FMOVD	g+0(SB),F1
	FCTIWZ	F1,F4
	FMOVD	F4,.rathole+0(SB)
	MOVW	.rathole+4(SB),R7
	MOVW	R7,l+0(SB)
*/

/*
int to float:
	MOVW	$1127219200,R9
	MOVW	l+0(SB),R7
	MOVW	R9,.rathole+0(SB)
	XOR	$-2147483648,R7,R6
	MOVW	R6,.rathole+4(SB)
	FMOVD	.rathole+0(SB),F0
	FSUB	F27,F0

unsigned to float:
	MOVW	ul+0(SB),R5
	MOVW	R9,.rathole+0(SB)
	XOR	$-2147483648,R5,R4
	MOVW	R4,.rathole+4(SB)
	FMOVD	.rathole+0(SB),F3
	FSUB	F27,F3
	FCMPU	F3,F28
	BGE	,3(PC)
	FMOVD	$4.29496729600000000e+09,F2
	FADD	F2,F3
	FMOVD	F3,g+0(SB)
*/
