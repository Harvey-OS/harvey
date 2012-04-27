#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>
#define Extern extern
#include "power.h"

ulong	setfpscr(void);
void	setfpcc(double);
void	farith(ulong);
void	farith2(ulong);
void	fariths(ulong);
void	fcmp(ulong);
void	mtfsb1(ulong);
void	mcrfs(ulong);
void	mtfsb0(ulong);
void	mtfsf(ulong);
void	mtfsfi(ulong);
void	mffs(ulong);
void	mtfsf(ulong);

Inst	op59[] = {
[18] {fariths, "fdivs", Ifloat},
[20] {fariths, "fsubs", Ifloat},
[21] {fariths, "fadds", Ifloat},
[22] {unimp, "fsqrts", Ifloat},
[24] {unimp, "fres", Ifloat},
[25] {fariths, "fmuls", Ifloat},
[28] {fariths, "fmsubs", Ifloat},
[29] {fariths, "fmadds", Ifloat},
[30] {fariths, "fnmsubs", Ifloat},
[31] {fariths, "fnmadds", Ifloat},
};

Inset	ops59 = {op59, nelem(op59)};

Inst	op63a[] = {
[12] {farith, "frsp", Ifloat},
[14] {farith, "fctiw", Ifloat},
[15] {farith, "fctiwz", Ifloat},
[18] {farith, "fdiv", Ifloat},
[20] {farith, "fsub", Ifloat},
[21] {farith, "fadd", Ifloat},
[22] {unimp, "frsqrt", Ifloat},
[23] {unimp, "fsel", Ifloat},
[25] {farith, "fmul", Ifloat},
[26] {unimp, "frsqrte", Ifloat},
[28] {farith, "fmsub", Ifloat},
[29] {farith, "fmadd", Ifloat},
[30] {farith, "fnmsub", Ifloat},
[31] {farith, "fnmadd", Ifloat},
};

Inset	ops63a= {op63a, nelem(op63a)};

Inst	op63b[] = {
[0] {fcmp, "fcmpu", Ifloat},
[32] {fcmp, "fcmpo", Ifloat},
[38] {mtfsb1, "mtfsb1", Ifloat},
[40] {farith2, "fneg", Ifloat},
[64] {mcrfs, "mcrfs", Ifloat},
[70] {mtfsb0, "mtfsb0", Ifloat},
[72] {farith2, "fmr", Ifloat},
[134] {mtfsfi, "mtfsfi", Ifloat},
[136] {farith2, "fnabs", Ifloat},
[264] {farith2, "fabs", Ifloat},
[583] {mffs, "mffs", Ifloat},
[711] {mtfsf, "mtfsf", Ifloat},
};

Inset	ops63b = {op63b, nelem(op63b)};

void
fpreginit(void)
{
	int i;

	/* Normally initialised by the kernel */
	reg.fd[27] = 4503601774854144.0;
	reg.fd[29] = 0.5;
	reg.fd[28] = 0.0;
	reg.fd[30] = 1.0;
	reg.fd[31] = 2.0;
	for(i = 0; i < 27; i++)
		reg.fd[i] = reg.fd[28];
}

static double
v2fp(uvlong v)
{
	FPdbleword f;

	f.hi = v>>32;
	f.lo = v;
	return f.x;
}

static uvlong
fp2v(double d)
{
	FPdbleword f;

	f.x = d;
	return ((uvlong)f.hi<<32) | f.lo;
}

void
lfs(ulong ir)
{
	ulong ea;
	int imm, ra, rd, upd;
	union {
		ulong	i;
		float	f;
	} u;

	getairr(ir);
	ea = imm;
	upd = (ir&(1L<<26))!=0;
	if(ra) {
		ea += reg.r[ra];
		if(upd)
			reg.r[ra] = ea;
	} else {
		if(upd)
			undef(ir);
	}
	if(trace)
		itrace("%s\tf%d,%ld(r%d) ea=%lux", ci->name, rd, imm, ra, ea);

	u.i = getmem_w(ea);
	reg.fd[rd] = u.f;
}

void
lfsx(ulong ir)
{
	ulong ea;
	int rd, ra, rb, upd;
	union {
		ulong	i;
		float	f;
	} u;

	getarrr(ir);
	ea = reg.r[rb];
	upd = ((ir>>1)&0x3FF)==567;
	if(ra){
		ea += reg.r[ra];
		if(upd)
			reg.r[ra] = ea;
		if(trace)
			itrace("%s\tf%d,(r%d+r%d) ea=%lux", ci->name, rd, ra, rb, ea);
	} else {
		if(upd)
			undef(ir);
		if(trace)
			itrace("%s\tf%d,(r%d) ea=%lux", ci->name, rd, rb, ea);
	}

	u.i = getmem_w(ea);
	reg.fd[rd] = u.f;
}

void
lfd(ulong ir)
{
	ulong ea;
	int imm, ra, rd, upd;

	getairr(ir);
	ea = imm;
	upd = (ir&(1L<<26))!=0;
	if(ra) {
		ea += reg.r[ra];
		if(upd)
			reg.r[ra] = ea;
	} else {
		if(upd)
			undef(ir);
	}
	if(trace)
		itrace("%s\tf%d,%ld(r%d) ea=%lux", ci->name, rd, imm, ra, ea);

	reg.fd[rd] = v2fp(getmem_v(ea));
}

void
lfdx(ulong ir)
{
	ulong ea;
	int rd, ra, rb, upd;

	getarrr(ir);
	ea = reg.r[rb];
	upd = ((ir>>1)&0x3FF)==631;
	if(ra){
		ea += reg.r[ra];
		if(upd)
			reg.r[ra] = ea;
		if(trace)
			itrace("%s\tf%d,(r%d+r%d) ea=%lux", ci->name, rd, ra, rb, ea);
	} else {
		if(upd)
			undef(ir);
		if(trace)
			itrace("%s\tf%d,(r%d) ea=%lux", ci->name, rd, rb, ea);
	}

	reg.fd[rd] = v2fp(getmem_v(ea));
}

void
stfs(ulong ir)
{
	ulong ea;
	int imm, ra, rd, upd;
	union {
		float f;
		ulong w;
	} u;

	getairr(ir);
	ea = imm;
	upd = (ir&(1L<<26))!=0;
	if(ra) {
		ea += reg.r[ra];
		if(upd)
			reg.r[ra] = ea;
	} else {
		if(upd)
			undef(ir);
	}
	if(trace)
		itrace("%s\tf%d,%ld(r%d) %lux=%g",
					ci->name, rd, imm, ra, ea, reg.fd[rd]);
	u.f = reg.fd[rd];	/* BUG: actual PPC conversion is more subtle than this */
	putmem_w(ea, u.w);
}

void
stfsx(ulong ir)
{
	ulong ea;
	int rd, ra, rb, upd;
	union {
		float	f;
		ulong	w;
	} u;

	getarrr(ir);
	ea = reg.r[rb];
	upd = getxo(ir)==695;
	if(ra){
		ea += reg.r[ra];
		if(upd)
			reg.r[ra] = ea;
		if(trace)
			itrace("%s\tf%d,(r%d+r%d) %lux=%g", ci->name, rd, ra, rb, ea, (float)reg.fd[rd]);
	} else {
		if(upd)
			undef(ir);
		if(trace)
			itrace("%s\tf%d,(r%d) %lux=%g", ci->name, rd, rb, ea, (float)reg.fd[rd]);
	}

	u.f = reg.fd[rd];	/* BUG: actual PPC conversion is more subtle than this */
	putmem_w(ea, u.w);
}

void
stfd(ulong ir)
{
	ulong ea;
	int imm, ra, rd, upd;

	getairr(ir);
	ea = imm;
	upd = (ir&(1L<<26))!=0;
	if(ra) {
		ea += reg.r[ra];
		if(upd)
			reg.r[ra] = ea;
	} else {
		if(upd)
			undef(ir);
	}
	if(trace)
		itrace("%s\tf%d,%ld(r%d) %lux=%g",
					ci->name, rd, imm, ra, ea, reg.fd[rd]);

	putmem_v(ea, fp2v(reg.fd[rd]));
}

void
stfdx(ulong ir)
{
	ulong ea;
	int rd, ra, rb, upd;

	getarrr(ir);
	ea = reg.r[rb];
	upd = ((ir>>1)&0x3FF)==759;
	if(ra){
		ea += reg.r[ra];
		if(upd)
			reg.r[ra] = ea;
		if(trace)
			itrace("%s\tf%d,(r%d+r%d) %lux=%g", ci->name, rd, ra, rb, ea, reg.fd[rd]);
	} else {
		if(upd)
			undef(ir);
		if(trace)
			itrace("%s\tf%d,(r%d) %lux=%g", ci->name, rd, rb, ea, reg.fd[rd]);
	}

	putmem_v(ea, fp2v(reg.fd[rd]));
}

void
mcrfs(ulong ir)
{
	ulong rd, ra, rb;
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
		undef(ir);
	ra >>= 2;
	rd >>= 2;
	reg.cr = (reg.cr & ~mkCR(rd, 0xF)) | mkCR(rd, getCR(ra, reg.fpscr));
	reg.fpscr &= ~fpscr0[ra];
	if(trace)
		itrace("mcrfs\tcrf%d,crf%d\n", rd, ra);
}

void
mffs(ulong ir)
{
	int rd, ra, rb;
	FPdbleword d;

	getarrr(ir);
	if(ra || rb)
		undef(ir);
	d.hi = 0xFFF80000UL;
	d.lo = reg.fpscr;
	reg.fd[rd] = d.x;
	/* it's anyone's guess how CR1 should be set when ir&1 */
	reg.cr &= ~mkCR(1, 0xE);	/* leave SO, reset others */
	if(trace)
		itrace("mffs%s\tfr%d\n", ir&1?".":"", rd);
}

void
mtfsb1(ulong ir)
{
	int rd, ra, rb;

	getarrr(ir);
	if(ra || rb)
		undef(ir);
	reg.fpscr |= (1L << (31-rd));
	/* BUG: should set summary bits */
	if(ir & 1)
		reg.cr &= ~mkCR(1, 0xE);	/* BUG: manual unclear: leave SO, reset others? */
	if(trace)
		itrace("mtfsb1%s\tfr%d\n", ir&1?".":"", rd);
}

void
mtfsb0(ulong ir)
{
	int rd, ra, rb;

	getarrr(ir);
	if(ra || rb)
		undef(ir);
	reg.fpscr &= ~(1L << (31-rd));
	if(ir & 1)
		reg.cr &= ~mkCR(1, 0xE);		/* BUG: manual unclear: leave SO, reset others? */
	if(trace)
		itrace("mtfsb0%s\tfr%d\n", ir&1?".":"", rd);
}

void
mtfsf(ulong ir)
{
	int fm, rb, i;
	FPdbleword d;
	ulong v;

	if(ir & ((1L << 25)|(1L << 16)))
		undef(ir);
	rb = (ir >> 11) & 0x1F;
	fm = (ir >> 17) & 0xFF;
	d.x = reg.fd[rb];
	v = d.lo;
	for(i=0; i<8; i++)
		if(fm & (1 << (7-i)))
			reg.fpscr = (reg.fpscr & ~mkCR(i, 0xF)) | mkCR(i, getCR(i, v));
	/* BUG: should set FEX and VX `according to the usual rule' */
	if(ir & 1)
		reg.cr &= ~mkCR(1, 0xE);		/* BUG: manual unclear: leave SO, reset others? */
	if(trace)
		itrace("mtfsf%s\t#%.2x,fr%d", ir&1?".":"", fm, rb);
}

void
mtfsfi(ulong ir)
{
	int imm, rd;

	if(ir & ((0x7F << 16)|(1L << 11)))
		undef(ir);
	rd = (ir >> 23) & 0xF;
	imm = (ir >> 12) & 0xF;
	reg.fpscr = (reg.fpscr & ~mkCR(rd, 0xF)) | mkCR(rd, imm);
	/* BUG: should set FEX and VX `according to the usual rule' */
	if(ir & 1)
		reg.cr &= ~mkCR(1, 0xE);		/* BUG: manual unclear: leave SO, reset others? */
	if(trace)
		itrace("mtfsfi%s\tcrf%d,#%x", ir&1?".":"", rd, imm);
}

void
fcmp(ulong ir)
{
	int fc, rd, ra, rb;

	getarrr(ir);
	if(rd & 3)
		undef(ir);
	rd >>= 2;
	SET(fc);
	switch(getxo(ir)) {
	default:
		undef(ir);
	case 0:
		if(trace)
			itrace("fcmpu\tcr%d,f%d,f%d", rd, ra, rb);
		if(isNaN(reg.fd[ra]) || isNaN(reg.fd[rb])) {
			fc = CRFU;
			break;
		}
		if(reg.fd[ra] == reg.fd[rb]) {
			fc = CREQ;
			break;
		}
		if(reg.fd[ra] < reg.fd[rb]) {
			fc = CRLT;
			break;
		}
		if(reg.fd[ra] > reg.fd[rb]) {
			fc = CRGT;
			break;
		}
		print("qi: fcmp error\n");
		break;
	case 32:
		if(trace)
			itrace("fcmpo\tcr%d,f%d,f%d", rd, ra, rb);
		if(isNaN(reg.fd[ra]) || isNaN(reg.fd[rb])) {	/* BUG: depends whether quiet or signalling ... */
			fc = CRFU;
			Bprint(bioout, "invalid_fp_register\n");
			longjmp(errjmp, 0);
		}
		if(reg.fd[ra] == reg.fd[rb]) {
			fc = CREQ;
			break;
		}
		if(reg.fd[ra] < reg.fd[rb]) {
			fc = CRLT;
			break;
		}
		if(reg.fd[ra] > reg.fd[rb]) {
			fc = CRGT;
			break;
		}
		print("qi: fcmp error\n");
		break;

	}
	fc >>= 28;
	reg.cr = (reg.cr & ~mkCR(rd,~0)) | mkCR(rd, fc);
	reg.fpscr = (reg.fpscr & ~0xF800) | (fc<<11);
	/* BUG: update FX, VXSNAN, VXVC */
}

/*
 * the farith functions probably don't produce the right results
 * in the presence of NaNs, Infs, etc., esp. wrt exception handling, 
 */
void
fariths(ulong ir)
{
	int rd, ra, rb, rc, fmt;
	char *cc;
	ulong fpscr;

	fmt = 0;
	rc = (ir>>6)&0x1F;
	getarrr(ir);
	switch(getxo(ir)&0x1F) {	/* partial XO decode */
	default:
		undef(ir);
	case 18:
		if((float)reg.fd[rb] == 0.0) {
			Bprint(bioout, "fp_exception ZX\n");
			reg.fpscr |= FPS_ZX | FPS_FX;
			longjmp(errjmp, 0);
		}
		reg.fd[rd] = (float)(reg.fd[ra] / reg.fd[rb]);
		break;
	case 20:
		reg.fd[rd] = (float)(reg.fd[ra] - reg.fd[rb]);
		break;
	case 21:
		reg.fd[rd] = (float)(reg.fd[ra] + reg.fd[rb]);
		break;
	case 25:
		reg.fd[rd] = (float)(reg.fd[ra] * reg.fd[rc]);
		rb = rc;
		break;
	case 28:
		reg.fd[rd] = (float)((reg.fd[ra] * reg.fd[rc]) - reg.fd[rb]);
		fmt = 2;
		break;
	case 29:
		reg.fd[rd] = (float)((reg.fd[ra] * reg.fd[rc]) + reg.fd[rb]);
		fmt = 2;
		break;
	case 30:
		reg.fd[rd] = (float)-((reg.fd[ra] * reg.fd[rc]) - reg.fd[rb]);
		fmt = 2;
		break;
	case 31:
		reg.fd[rd] = (float)-((reg.fd[ra] * reg.fd[rc]) + reg.fd[rb]);
		fmt = 2;
		break;
	}
	if(fmt==1 && ra)
		undef(ir);
	fpscr = setfpscr();
	setfpcc(reg.fd[rd]);
	cc = "";
	if(ir & 1) {
		cc = ".";
		reg.cr = (reg.cr & ~mkCR(1, ~0)) | mkCR(1, (fpscr>>28));
	}
	if(trace) {
		switch(fmt) {
		case 0:
			itrace("%s%s\tfr%d,fr%d,fr%d", ci->name, cc, rd, ra, rb);
			break;
		case 1:
			itrace("%s%s\tfr%d,fr%d", ci->name, cc, rd, rb);
			break;
		case 2:
			itrace("%s%s\tfr%d,fr%d,fr%d,fr%d", ci->name, cc, rd, ra, rc, rb);
			break;
		}
	}
}

void
farith(ulong ir)
{
	vlong vl;
	int rd, ra, rb, rc, fmt;
	char *cc;
	ulong fpscr;
	int nocc;
	double d;

	fmt = 0;
	nocc = 0;
	rc = (ir>>6)&0x1F;
	getarrr(ir);
	switch(getxo(ir)&0x1F) { /* partial XO decode */
	default:
		undef(ir);
	case 12:	/* frsp */
		reg.fd[rd] = (float)reg.fd[rb];
		fmt = 1;
		break;
	case 14:	/* fctiw */	/* BUG: ignores rounding mode */
	case 15:	/* fctiwz */
		d = reg.fd[rb];
		if(d >= 0x7fffffff)
			vl = 0x7fffffff;
		else if(d < 0x80000000)
			vl = 0x80000000;
		else
			vl = d;
		reg.fd[rd] = v2fp(vl);
		fmt = 1;
		nocc = 1;
		break;
	case 18:
		if(reg.fd[rb] == 0.0) {
			Bprint(bioout, "fp_exception ZX\n");
			reg.fpscr |= FPS_ZX | FPS_FX;
			longjmp(errjmp, 0);
		}
		reg.fd[rd] = reg.fd[ra] / reg.fd[rb];
		break;
	case 20:
		reg.fd[rd] = reg.fd[ra] - reg.fd[rb];
		break;
	case 21:
		reg.fd[rd] = reg.fd[ra] + reg.fd[rb];
		break;
	case 25:
		reg.fd[rd] = reg.fd[ra] * reg.fd[rc];
		rb = rc;
		break;
	case 28:
		reg.fd[rd] = (reg.fd[ra] * reg.fd[rc]) - reg.fd[rb];
		fmt = 2;
		break;
	case 29:
		reg.fd[rd] = (reg.fd[ra] * reg.fd[rc]) + reg.fd[rb];
		fmt = 2;
		break;
	case 30:
		reg.fd[rd] = -((reg.fd[ra] * reg.fd[rc]) - reg.fd[rb]);
		fmt = 2;
		break;
	case 31:
		reg.fd[rd] = -((reg.fd[ra] * reg.fd[rc]) + reg.fd[rb]);
		fmt = 2;
		break;
	}
	if(fmt==1 && ra)
		undef(ir);
	fpscr = setfpscr();
	if(nocc == 0)
		setfpcc(reg.fd[rd]);
	cc = "";
	if(ir & 1) {
		cc = ".";
		reg.cr = (reg.cr & ~mkCR(1, ~0)) | mkCR(1, (fpscr>>28));
	}
	if(trace) {
		switch(fmt) {
		case 0:
			itrace("%s%s\tfr%d,fr%d,fr%d", ci->name, cc, rd, ra, rb);
			break;
		case 1:
			itrace("%s%s\tfr%d,fr%d", ci->name, cc, rd, rb);
			break;
		case 2:
			itrace("%s%s\tfr%d,fr%d,fr%d,fr%d", ci->name, cc, rd, ra, rc, rb);
			break;
		}
	}
}

void
farith2(ulong ir)
{
	int rd, ra, rb;
	char *cc;
	ulong fpscr;

	getarrr(ir);
	switch(getxo(ir)) { /* full XO decode */
	default:
		undef(ir);
	case 40:
		reg.fd[rd] = -reg.fd[rb];
		break;
	case 72:
		reg.fd[rd] = reg.fd[rb];
		break;
	case 136:
		reg.fd[rd] = -fabs(reg.fd[rb]);
		break;
	case 264:
		reg.fd[rd] = fabs(reg.fd[rb]);
		break;
	}
	if(ra)
		undef(ir);
	fpscr = setfpscr();
	setfpcc(reg.fd[rd]);
	cc = "";
	if(ir & 1) {
		cc = ".";
		reg.cr = (reg.cr & ~mkCR(1, ~0)) | mkCR(1, (fpscr>>28));
	}
	if(trace)
		itrace("%s%s\tfr%d,fr%d", ci->name, cc, rd, rb);
}

ulong
setfpscr(void)
{
	ulong fps, fpscr;

	fps = getfsr();
	fpscr = reg.fpscr;
	if(fps & FPAOVFL)
		fpscr |= FPS_OX;
	if(fps & FPAINEX)
		fpscr |= FPS_XX;
	if(fps & FPAUNFL)
		fpscr |= FPS_UX;
	if(fps & FPAZDIV)
		fpscr |= FPS_ZX;
	if(fpscr != reg.fpscr) {
		fpscr |= FPS_FX;
		reg.fpscr = fpscr;
	}
	return fpscr;
}

void
setfpcc(double r)
{
	int c;

	c = 0;
	if(r == 0)
		c |= 2;
	else if(r < 0)
		c |= 4;
	else
		c |= 8;
	if(isNaN(r))
		c |= 1;
	reg.fpscr = (reg.fpscr & ~0xF800) | (0<<15) | (c<<11); /* unsure about class bit */
}
