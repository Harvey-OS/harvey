#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>
#define Extern extern
#include "3210.h"

enum{
	FPU	= 7,			/* number of fpu cycles kept; safely large */
	FMAT	= 6,			/* strange source accum for add-tap */
	FNEG	= 1 << 24,		/* negate addend */
	FSUB	= 1 << 23,		/* subtract */
	NOSTORE	= 7,			/* don't store z */
};

#define	ASRC(i)		(((i)>>26)&7)
#define	ADEST(i)	(((i)>>21)&3)
#define	XARG(i)		(((i)>>14)&0x7f)
#define	YARG(i)		(((i)>>7)&0x7f)
#define	ZARG(i)		(((i)>>0)&0x7f)
#define	FSPEC(i)	(((i)>>23)&0xf)

Fpu	fpus[FPU+1];
Freg	fzero;
Freg	fone;
int	cycle;			/* current index in fpus */

Fspec fputab[16] = {
	{0,		0,	1,	4,	"ic"},
	{0,		0,	4,	1,	"oc"},
	{Ffloat16,	sfetch,	2,	4,	"float16"},
	{Fint16,	mfetch,	4,	2,	"int16"},
	{Fround,	mfetch,	4,	4,	"round"},
	{0,		mfetch,	4,	4,	"ifalt"},
	{0,		mfetch,	4,	4,	"ifaeq"},
	{0,		mfetch,	4,	4,	"ifagt"},
	{Ffloat32,	mfetch,	4,	4,	"float32"},
	{Fint32,	mfetch,	4,	4,	"int32"},
	{0,		0,	0,	0,	"fop10"},
	{0,		0,	0,	0,	"fop11"},
	{0,		mfetch,	4,	4,	"ieee"},
	{0,		mfetch,	4,	4,	"dsp"},
	{Fseed,		mfetch,	4,	4,	"seed"},
	{0,		0,	0,	0,	"fop15"},
};

void
resetfpu(void)
{
	fone.val = 0x00000080;
	memset(fpus, 0, sizeof(fpus));
	cycle = 0;
}

/*
 * perform fpu calculations at end of i-cycle
 */
void
fpuop(void)
{
	Fpu *fpu;

	fpu = &fpus[(cycle - 3) & FPU];
	if(fpu->op)
		fpu->op(fpu);
	fpu = &fpus[(cycle - 4) & FPU];
	if(fpu->op && ZARG(fpu->inst) != NOSTORE){
		switch(fpu->zinc){
		case 4:
			putmem_w(fpu->z.a, fpu->z.v.val);
			break;
		case 2:
			putmem_h(fpu->z.a, fpu->z.v.val);
			break;
		case 1:
			putmem_b(fpu->z.a, fpu->z.v.val);
			break;
		}
	}
	cycle = (cycle + 1) & FPU;
	fpus[cycle].op = 0;
}

/*
 * increment the z reg
 */
void
fpuupdate(int s, int t, int u, int v)
{
	Fpu *fpu;
	int r;

	fpu = &fpus[(cycle - 1) & FPU];
	if(fpu->op){
		r = update(ZARG(fpu->inst), fpu->z.a, fpu->zinc);
		if(r && (r == s || r == t || r == u || r == v)){
			if((ZARG(fpu->inst)&7) == 6)
				warn("r%d was decremented in the previous instruction", r);
			else
				warn("r%d was incremented in the previous instruction", r);
		}
	}
}

/*
 * fetch last cycles fpu mem refs
 */
int
fpufetch(void)
{
	Fpu *fpu;

	fpu = &fpus[(cycle - 1) & FPU];
	if(fpu->op && fpu->fetch)
		return (*fpu->fetch)(fpu);
	return 0;
}

void
Dfmadd(char *buf, int n, ulong inst, ulong pc, int rvalid)
{
	char x[16], y[16], z[16];
	char *neg, *add, *d, *a;

	USED(pc, rvalid);
	fargname(x, XARG(inst), 0);
	fargname(y, YARG(inst), 0);
	fargname(z, ZARG(inst), 1);
	neg = "";
	if(inst & FNEG)
		neg = "-";
	add = "+";
	if(inst & FSUB)
		add = "-";
	d = fregname[ADEST(inst)];
	a = fregname[ASRC(inst)];

	switch(inst >> 29){
	case 1:
		/*
		 * strange instruction encoding
		 * for f-add-tap
		 */
		if(ASRC(inst) == FMAT){
			if(*z)
				snprint(buf, n, "%s = %s(%s = %s) %s %s", d, neg, z, y, add, x);
			else
				snprint(buf, n, "%s = %s%s %s %s", d, neg, y, add, x);
		}else{
			if(*z)
				snprint(buf, n, "%s = %s = %s%s %s %s * %s", z, d, neg, y, add, a, x);
			else
				snprint(buf, n, "%s = %s%s %s %s * %s", d, neg, y, add, a, x);
		}
		break;
	case 2:
		if(*z)
			snprint(buf, n, "%s = %s%s %s (%s = %s) * %s", d, neg, a, add, z, y, x);
		else
			snprint(buf, n, "%s = %s%s %s %s * %s", d, neg, a, add, y, x);
		break;
	case 3:
		if(*z)
			snprint(buf, n, "%s = %s = %s%s %s %s * %s", z, d, neg, a, add, y, x);
		else
			snprint(buf, n, "%s = %s%s %s %s * %s", d, neg, a, add, y, x);
		break;
	default:
		snprint(buf, n, "bad fmadd instruction %.8lux\n", inst);
	}
}

/*
 * 1st instruction cycle for
 * all multiply-add instructions
 */
void
Ifmadd(ulong inst)
{
	Fpu *fpu;
	int x, y, s, t, u, v;

	fpu = &fpus[cycle & FPU];
	x = XARG(inst);
	y = YARG(inst);
	fpu->inst = inst;
	fpu->zinc = 4;
	fpu->x.a = crack(x, 0);
	s = update(x, fpu->x.a, 4);
	t = increg(x);
	fpu->y.a = crack(y, 0);
	u = update(y, fpu->y.a, 4);
	v = increg(y);
	fpuupdate(s, t, u, v);
	ldlast(s, t, u, v);
	fpu->z.a = crack(ZARG(inst), 1);

	switch(fpu->inst >> 29){
	case 1:
		/*
		 * strange instruction encoding
		 * for f-add-tap
		 */
		if(ASRC(fpu->inst) == FMAT){
			fpu->fetch = xyfetch;
			fpu->op = addtap;
		}else{
			fpu->fetch = axyfetch;
			fpu->op = multaddstore;
		}
		break;
	case 2:
		fpu->fetch = xyfetch;
		fpu->op = multacctap;
		break;
	case 3:
		fpu->fetch = xyfetch;
		fpu->op = multaccstore;
		break;
	default:
		error("bad fpu instruction %.8lux", inst);
	}
}

void
Dfspecial(char *buf, int n, ulong inst, ulong pc, int rvalid)
{
	Fspec *f;
	char y[16], z[16], *d;

	USED(pc, rvalid);
	fargname(y, YARG(inst), 0);
	fargname(z, ZARG(inst), 1);
	d = fregname[ADEST(inst)];
	f = &fputab[FSPEC(inst)];
	if(*z)
		snprint(buf, n, "%s = %s = %s(%s)", z, d, f->name, y);
	else
		snprint(buf, n, "%s = %s(%s)", d, f->name, y);
}

/*
 * first cycles of floating special functions
 */
void
Ifspecial(ulong inst)
{
	Fpu *fpu;
	Fspec *f;
	int y, s, t;

	f = &fputab[FSPEC(inst)];
	if(!f->op)
		error("illegal fpu special instruction %.8lux", inst);
	fpu = &fpus[cycle & FPU];
	y = YARG(inst);
	fpu->inst = inst;
	fpu->zinc = f->zinc;
	fpu->y.a = crack(y, 0);
	s = update(y, fpu->y.a, f->inc);
	t = increg(y);
	fpuupdate(s, t, 0, 0);
	ldlast(s, t, 0, 0);
	fpu->z.a = crack(ZARG(inst), 1);
	fpu->fetch = f->fetch;
	fpu->op = f->op;
}

void
Ffloat16(Fpu *fpu)
{
	Freg f;
	long v;
	int r;

	yfetch(fpu);
	v = fpu->y.v.val >> 16;
	if(v & 0x8000)
		v |= ~0xffff;
	f.val = dtodsp(v);
	f.guard = 0;
	f = fpuflags(f);
	r = ADEST(fpu->inst);
	reg.f[r] = f;
	fpu->z.v = f;
}

void
Ffloat32(Fpu *fpu)
{
	Freg f;
	long v;
	int r;

	yfetch(fpu);
	v = fpu->y.v.val;
	f.val = dtodsp(v);
	f.guard = 0;
	f = fpuflags(f);
	r = ADEST(fpu->inst);
	reg.f[r] = f;
	fpu->z.v = f;
}

void
Fint16(Fpu *fpu)
{
	Freg f;
	ulong v;
	int r;

	yfetch(fpu);
	v = fpu->y.v.val;
	f.val = dtoshort(dsptod(v)) << 16;
	f.guard = 0;
	r = ADEST(fpu->inst);
	reg.f[r] = f;

	f.val >>= 16;
	fpu->z.v = f;
}

void
Fint32(Fpu *fpu)
{
	Freg f;
	int r;

	yfetch(fpu);
	f.val = dtolong(dsptod(fpu->y.v.val));
	f.guard = 0;
	r = ADEST(fpu->inst);
	reg.f[r] = f;
	fpu->z.v = f;
}

void
Fround(Fpu *fpu)
{
	Freg f;
	int r;

	yfetch(fpu);
	r = ADEST(fpu->inst);
	f = fround(fpu->y.v);
	reg.f[r] = f;
	fpu->z.v = f;
}

/*
 * strange approximation to inverse of y by
 * complementing all bits other than sign bit
 */
void
Fseed(Fpu *fpu)
{
	Freg f;
	int r;

	yfetch(fpu);
	r = ADEST(fpu->inst);
	f.val = fpu->y.v.val ^ 0x7fffffff;
	f.guard = fpu->y.v.guard & 0xff;
	f = fpuflags(f);
	reg.f[r] = f;
	fpu->z.v = f;
}

/*
 * astore = (z = y) + 1.0 * x
 */
void
addtap(Fpu *fpu)
{
	Freg f;
	int r;

	yfetch(fpu);
	f = fmuladd(fpu->inst & FNEG, fpu->inst & FSUB, fpu->y.v, fone, fpu->x.v);
	r = ADEST(fpu->inst);
	reg.f[r] = f;
	fpu->z.v = fpu->y.v;
}

/*
 * z = astore = y + afetch * x
 */
void
multaddstore(Fpu *fpu)
{
	Freg f;
	int r;

	yfetch(fpu);
	f = fmuladd(fpu->inst & FNEG, fpu->inst & FSUB, fpu->y.v, fpu->accum, fpu->x.v);
	r = ADEST(fpu->inst);
	reg.f[r] = f;
	fpu->z.v = f;
}

/*
 * z = astore = afetch + y * x
 */
void
multaccstore(Fpu *fpu)
{
	Freg f;
	int r;

	fpu->accum = faccum(fpu->inst);
	f = fmuladd(fpu->inst & FNEG, fpu->inst & FSUB, fpu->accum, fpu->y.v, fpu->x.v);
	r = ADEST(fpu->inst);
	reg.f[r] = f;
	fpu->z.v = f;
}

/*
 * astore = afetch + (z=y) * x
 */
void
multacctap(Fpu *fpu)
{
	Freg f;
	int r;

	fpu->accum = faccum(fpu->inst);
	f = fmuladd(fpu->inst & FNEG, fpu->inst & FSUB, fpu->accum, fpu->y.v, fpu->x.v);
	r = ADEST(fpu->inst);
	reg.f[r] = f;
	fpu->z.v = fpu->y.v;
}

/*
 * fetch y if a memory operand
 * return whether y fetched memory
 */
int
mfetch(Fpu *fpu)
{
	int y;

	y = YARG(fpu->inst);
	if(y >> 3){
		fpu->y.v.val = getmem_w(fpu->y.a);
		fpu->y.v.guard = 0;
		return 1;
	}
	return 0;
}

/*
 * fetch y if not a memory operand
 */
void
yfetch(Fpu *fpu)
{
	int y;

	y = YARG(fpu->inst);
	if(y >> 3)
		return;
	if(y & 4)
		error("illegal floating point operand");
	fpu->y.v = reg.f[y & 3];
}

/*
 * fetch a 16 bit integer y if a memory operand
 * return whether memory was fetched
 */
int
sfetch(Fpu *fpu)
{
	int y;

	y = YARG(fpu->inst);
	if(y >> 3){
		fpu->y.v.val = getmem_h(fpu->y.a) << 16;
		return 1;
	}
	return 0;
}

/*
 * fetch x and y
 * return whether y fetched memory
 */
int
xyfetch(Fpu *fpu)
{
	int y;

	fpu->x.v = ffetch(XARG(fpu->inst), fpu->x.a);
	y = YARG(fpu->inst);
	fpu->y.v = ffetch(y, fpu->y.a);
	if(y >> 3)
		return 1;
	return 0;
}

/*
 * fetch x, y, and accumlator
 */
int
axyfetch(Fpu *fpu)
{
	fpu->accum = faccum(fpu->inst);
	return xyfetch(fpu);
}

/*
 * look at operand and get value of any mem ref reg
 */
ulong
crack(int operand, int isz)
{
	int r;

	r = operand >> 3;
	if(!r){
		if(isz && operand != NOSTORE)
			error("illegal Z operand");
		return 0;
	}
	if(r == 15)
		error("illegal register r15 in dau mem ref");
	return reg.r[r];
}

int
increg(int op)
{
	op &= 7;
	if(op > 0 && op < 6)
		return op + 14;
	return 0;
}

/*
 * update any mem ref register
 */
int
update(int operand, long old, int inc)
{
	int v;

	v = operand >> 3;
	if(!v)
		return v;
	if(v == 15)
		error("register r15 cannot be mem source in fpu op");
	/*
	 * do the increment
	 */
	operand &= 7;
	switch(operand){
	case 0:
		return 0;
	case 6:
		reg.r[v] = old - inc;
		break;
	case 7:
		reg.r[v] = old + inc;
		break;
	default:
		reg.r[v] = old + reg.r[14 + operand];
		break;
	}
	return v;
}

/*
 * fetch an x or y operand
 */
Freg
ffetch(int operand, ulong v)
{
	Freg f;

	if(operand >> 3){
		f.val = getmem_w(v);
		f.guard = 0;
		return f;
	}
	if(operand & 4)
		error("illegal floating point operand");
	return reg.f[operand & 3];
}

/*
 * fetch an accumulator only operand
 */
Freg
faccum(ulong inst)
{
	int v;

	v = ASRC(inst);
	if(v <= 3)
		return reg.f[v];
	if(v == 4)
		return fzero;
	if(v == 5)
		return fone;
	error("illegal floating point operand");
	return fzero;
}

void
fargname(char *buf, int x, int isz)
{
	int p;

	p = x >> 3;
	x &= 7;
	if(p == 0){
		if(x <= 3)
			sprint(buf, "a%d", x);
		else if(isz && x == 7)
			sprint(buf, "");
		else
			sprint(buf, "a?");
		return;
	}
	if(p == 15){
		sprint(buf, "*r?");
		return;
	}
	buf += sprint(buf, "*r%d", p);
	if(x == 0)
		return;
	if(x == 6)
		sprint(buf, "--");
	else if(x == 7)
		sprint(buf, "++");
	else
		sprint(buf, "+r%d", x + 14);
}

char *fregname[] =
{
	"a0",
	"a1",
	"a2",
	"a3",
	"0.0",
	"1.0",
	"a6?",
	"a7?",
};

/*
 * conversions
 */
long
dtolong(double d)
{
	if(d > 2147483647.)
		return 2147483647;
	if(d < -2147483648.)
		return -2147483648;
	switch(Round()){
	case 0: case 2:		/* round to nearest */
		return floor(d + 0.5);
	case 1:			/* trunc to -infinty */
		return floor(d);
	case 3:			/* trunc to 0 */
		return d;
	}
	return 0;
}

int
dtouchar(double d)
{
	int c;

	c = dtolong(d);
	if(c < 0)
		return 0;
	if(c > 255)
		return 255;
	return c;
}

int
dtoshort(double d)
{
	int c;

	c = dtolong(d);
	if(c < -32768)
		return -32768;
	if(c > 32767)
		return 32767;
	return c;
}

double
shorttod(int i)
{
	return i;
}

double
longtod(long i)
{
	return i;
}

double
uchartod(uchar c)
{
	return c;
}
