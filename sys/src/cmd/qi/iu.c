#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>
#define Extern extern
#include "power.h"

void	add(ulong);
void	addc(ulong);
void	adde(ulong);
void	addme(ulong);
void	addze(ulong);
void	and(ulong);
void	andc(ulong);
void	cmp(ulong);
void	cmpl(ulong);
void	cntlzw(ulong);
void	dcbf(ulong);
void	dcbi(ulong);
void	dcbst(ulong);
void	dcbt(ulong);
void	dcbtst(ulong);
void	dcbz(ulong);
void	divw(ulong);
void	divwu(ulong);
void	eciwx(ulong);
void	ecowx(ulong);
void	eieio(ulong);
void	eqv(ulong);
void	extsb(ulong);
void	extsh(ulong);
void	icbi(ulong);
void	lbzx(ulong);
void	lfdx(ulong);
void	lfsx(ulong);
void	lhax(ulong);
void	lhbrx(ulong);
void	lhzx(ulong);
void	lswi(ulong);
void	lswx(ulong);
void	lwarx(ulong);
void	lwbrx(ulong);
void	lwzx(ulong);
void	mcrxr(ulong);
void	mfcr(ulong);
void	mfmsr(ulong);
void	mfpmr(ulong);
void	mfspr(ulong);
void	mfsr(ulong);
void	mfsrin(ulong);
void	mftb(ulong);
void	mftbu(ulong);
void	mspr(ulong);
void	mtcrf(ulong);
void	mtmsr(ulong);
void	mtpmr(ulong);
void	mtspr(ulong);
void	mtsr(ulong);
void	mtsrin(ulong);
void	mttb(ulong);
void	mttbu(ulong);
void	mulhw(ulong);
void	mulhwu(ulong);
void	mullw(ulong);
void	nand(ulong);
void	neg(ulong);
void	nor(ulong);
void	or(ulong);
void	orc(ulong);
void	slbia(ulong);
void	slbia(ulong);
void	slw(ulong);
void	sraw(ulong);
void	srawi(ulong);
void	srw(ulong);
void	stbx(ulong);
void	stfdx(ulong);
void	stfiwx(ulong);
void	stfsx(ulong);
void	sthbrx(ulong);
void	sthx(ulong);
void	stswi(ulong);
void	stswx(ulong);
void	stwbrx(ulong);
void	stwcx(ulong);
void	stwx(ulong);
void	subf(ulong);
void	subfc(ulong);
void	subfe(ulong);
void	subfme(ulong);
void	subfze(ulong);
void	sync(ulong);
void	tlbie(ulong);
void	tw(ulong);
void	xor(ulong);

Inst	op31[] = {
[0] {cmp, "cmp", Iarith},
[4] {tw, "tw", Iarith},
[8] {subfc, "subfc", Iarith},
[10] {addc, "addc", Iarith},
[11] {mulhwu, "mulhwu", Iarith},
[19] {mfcr, "mfcr", Iarith},
[20] {lwarx, "lwarx", Iload},
[23] {lwzx, "lwzx", Iload},
[24] {slw, "slw", Ilog},
[26] {cntlzw, "cntlzw", Ilog},
[28] {and, "and", Ilog},
[32] {cmpl, "cmpl", Iarith},
[40] {subf, "subf", Iarith},
[54] {dcbst, "dcbst", Icontrol},
[55] {lwzx, "lwzux", Iload},
[60] {andc, "andc", Ilog},
[75] {mulhw, "mulhw", Iarith},
[83] {0, "mfmsr", Icontrol},
[86] {dcbf, "dcbf", Icontrol},
[87] {lbzx, "lbzx", Iload},
[104] {neg, "neg", Iarith},
[115] {0, "mfpmr", Iarith},
[119] {lbzx, "lbzux", Iload},
[124] {nor, "nor", Iarith},
[136] {subfe, "subfe", Iarith},
[138] {adde, "adde", Iarith},
[144] {mtcrf, "mtcrf", Ireg},
[146] {0, "mtmsr", Icontrol},
[150] {stwcx, "stwcx.", Istore},
[151] {stwx, "stwx", Istore},
[178] {0, "mtpmr", Icontrol},
[183] {stwx, "stwux", Istore},
[200] {subfze, "subfze", Iarith},
[202] {addze, "addze", Iarith},
[210] {0, "mtsr", Ireg},
[215] {stbx, "stbx", Istore},
[232] {subfme, "subfme", Iarith},
[234] {addme, "addme", Iarith},
[235] {mullw, "mullw", Iarith},
[242] {0, "mtsrin", Ireg},
[246] {dcbtst, "dcbtst", Icontrol},
[247] {stbx, "stbux", Istore},
[266] {add, "add", Iarith},
[275] {0, "mftb", Icontrol},
[278] {dcbt, "dcbt", Icontrol},
[279] {lhzx, "lhzx", Iload},
[284] {eqv, "eqv", Ilog},
[306] {0, "tlbie", Icontrol},
[307] {0, "mftbu", Icontrol},
[310] {0, "eciwx", Icontrol},
[311] {lhzx, "lhzux", Iload},
[316] {xor, "xor", Ilog},
[339] {mspr, "mfspr", Ireg},
[343] {lhax, "lhax", Iload},
[375] {lhax, "lhaux", Iload},
[403] {0, "mttb", Icontrol},
[407] {sthx, "sthx", Istore},
[412] {orc, "orc", Ilog},
[434] {0, "slbia", Iarith},
[435] {0, "mttbu", Icontrol},
[438] {0, "ecowx", Icontrol},
[439] {sthx, "sthux", Istore},
[444] {or, "or", Ilog},
[459] {divwu, "divwu", Iarith},
[467] {mspr, "mtspr", Ireg},
[470] {0, "dcbi", Icontrol},
[476] {nand, "nand", Ilog},
[491] {divw, "divw", Iarith},
[498] {0, "slbia", Icontrol},
[512] {mcrxr, "mcrxr", Ireg},
[533] {lswx, "lswx", Iload},
[534] {lwbrx, "lwbrx", Iload},
[535] {lfsx, "lfsx", Ifloat},
[536] {srw, "srw", Ilog},
[567] {lfsx, "lfsux", Ifloat},
[595] {0, "mfsr", Iarith},
[597] {lswi, "lswi", Iarith},
[598] {sync, "sync", Iarith},
[599] {lfdx, "lfdx", Ifloat},
[631] {lfdx, "lfdux", Ifloat},
[659] {0, "mfsrin", Ireg},
[661] {stswx, "stswx", Istore},
[662] {stwbrx, "stwbrx", Istore},
[663] {stfsx, "stfsx", Istore},
[695] {stfsx, "stfsux", Istore},
[725] {stswi, "stswi", Istore},
[727] {stfdx, "stfdx", Istore},
[759] {stfdx, "stfdux", Istore},
[790] {lhbrx, "lhbrx", Iload},
[792] {sraw, "sraw", Ilog},
[824] {srawi, "srawi", Ilog},
[854] {0, "eieio", Icontrol},
[918] {sthbrx, "sthbrx", Istore},
[922] {extsh, "extsh", Iarith},
[954] {extsb, "extsb", Iarith},
[982] {icbi, "icbi", Icontrol},
[983] {unimp, "stfiwx", Istore},
[1014] {dcbz, "dcbz", Icontrol},
};

Inset	ops31 = {op31, nelem(op31)};

void
mspr(ulong ir)
{
	int rd, ra, rb;
	ulong *d;
	char *n;
	char buf[20];

	getarrr(ir);
	switch((rb<<5) | ra) {
	case 0:
		undef(ir);	/* was mq */
		return;
	case 1:
		d = &reg.xer; n = "xer";
		break;
	case 268:
	case 284:
		d = &reg.tbl; n = "tbl";
		break;
	case 269:
	case 285:
		d = &reg.tbu; n = "tbu";
		break;
	case 22:
		d = &reg.dec; n = "dec";
		break;
	case 8:
		d = &reg.lr; n = "lr";
		break;
	case 9:
		d = &reg.ctr; n = "ctr";
		break;
	default:
		d = 0; sprint(n = buf, "spr%d", rd);
		break;
	}
	if(getxo(ir) == 339) {
		if(trace)
			itrace("%s\tr%d,%s", ci->name, rd, n);
		if(d != nil)
			reg.r[rd] = *d;
	} else {
		if(trace)
			itrace("%s\t%s,r%d", ci->name, n, rd);
		if(d != nil)
			*d = reg.r[rd];
	}
}

static void
setcr(int d, long r)
{
	int c;

	c = 0;
	if(reg.xer & XER_SO)
		c |= 1;
	if(r == 0)
		c |= 2;
	else if(r > 0)
		c |= 4;
	else
		c |= 8;
	reg.cr = (reg.cr & ~mkCR(d, 0xF)) | mkCR(d, c);
}

void
addi(ulong ir)
{
	int rd, ra;
	long imm;

	getairr(ir);
	if(trace) {
		if(ra)
			itrace("%s\tr%d,r%d,$0x%lux", ci->name, rd, ra, imm);
		else
			itrace("li\tr%d,$0x%lux", rd, imm);
	}
	if(ra)
		imm += reg.r[ra];
	reg.r[rd] = imm;
}

void
addis(ulong ir)
{
	int rd, ra;
	long imm;

	getairr(ir);
	if(trace) {
		if(ra)
			itrace("%s\tr%d,r%d,$0x%lux", ci->name, rd, ra, imm);
		else
			itrace("lis\tr%d,$0x%lux", rd, imm);
	}
	imm <<= 16;
	if(ra)
		imm += reg.r[ra];
	reg.r[rd] = imm;
}

void
and(ulong ir)
{
	int rs, ra, rb;

	getlrrr(ir);
	reg.r[ra] = reg.r[rs] & reg.r[rb];
	if(trace)
		itrace("%s%s\tr%d,r%d,r%d", ci->name, ir&1?".":"", ra, rs, rb);
	if(ir & 1)
		setcr(0, reg.r[ra]);
}

void
andc(ulong ir)
{
	int rs, ra, rb;

	getlrrr(ir);
	reg.r[ra] = reg.r[rs] & ~reg.r[rb];
	if(trace)
		itrace("%s%s\tr%d,r%d,r%d", ci->name, ir&1?".":"", ra, rs, rb);
	if(ir & 1)
		setcr(0, reg.r[ra]);
}

void
andicc(ulong ir)
{
	int rs, ra;
	ulong imm;

	getlirr(ir);
	reg.r[ra] = reg.r[rs] & imm;
	if(trace)
		itrace("%s\tr%d,r%d,$0x%lx", ci->name, ra, rs, imm);
	setcr(0, reg.r[ra]);
}

void
andiscc(ulong ir)
{
	int rs, ra;
	ulong imm;

	getlirr(ir);
	reg.r[ra] = reg.r[rs] & (imm<<16);
	if(trace)
		itrace("%s\tr%d,r%d,$0x%lx", ci->name, ra, rs, imm);
	setcr(0, reg.r[ra]);
}

void
cmpli(ulong ir)
{
	int rd, ra;
	ulong c;
	ulong imm, v;

	getairr(ir);
	imm &= 0xFFFF;
	if(rd & 3)
		undef(ir);
	rd >>= 2;
	v = reg.r[ra];
	c = 0;
	if(reg.xer & XER_SO)
		c |= CRSO;
	if(v < imm)
		c |= CRLT;
	else if(v == imm)
		c |= CREQ;
	else
		c |= CRGT;
	c >>= 28;
	reg.cr = (reg.cr & ~mkCR(rd, 0xF)) | mkCR(rd, c);
	if(trace)
		itrace("%s\tcrf%d,r%d,0x%lux [cr=#%x]", ci->name, rd, ra, imm, c);
}

void
cmp(ulong ir)
{
	int rd, ra, rb;
	ulong c;
	long va, vb;

	getarrr(ir);
	if(rd & 3)
		undef(ir);
	rd >>= 2;
	c = 0;
	if(reg.xer & XER_SO)
		c |= CRSO;
	va = reg.r[ra];
	vb = reg.r[rb];
	if(va < vb)
		c |= CRLT;
	else if(va == vb)
		c |= CREQ;
	else
		c |= CRGT;
	c >>= 28;
	reg.cr = (reg.cr & ~mkCR(rd, 0xF)) | mkCR(rd, c);
	if(trace)
		itrace("%s\tcrf%d,r%d,r%d [cr=#%x]", ci->name, rd, ra, rb, c);
}

void
cmpi(ulong ir)
{
	int rd, ra;
	ulong c;
	long imm, v;

	getairr(ir);
	if(rd & 3)
		undef(ir);
	rd >>= 2;
	v = reg.r[ra];
	c = 0;
	if(reg.xer & XER_SO)
		c |= CRSO;
	if(v < imm)
		c |= CRLT;
	else if(v == imm)
		c |= CREQ;
	else
		c |= CRGT;
	c >>= 28;
	reg.cr = (reg.cr & ~mkCR(rd, 0xF)) | mkCR(rd, c);
	if(trace)
		itrace("%s\tcrf%d,r%d,0x%lux [cr=#%x]", ci->name, rd, ra, imm, c);
}

void
cmpl(ulong ir)
{
	int rd, ra, rb;
	ulong c;
	ulong va, vb;

	getarrr(ir);
	if(rd & 3)
		undef(ir);
	rd >>= 2;
	c = 0;
	if(reg.xer & XER_SO)
		c |= CRSO;
	va = reg.r[ra];
	vb = reg.r[rb];
	if(va < vb)
		c |= CRLT;
	else if(va == vb)
		c |= CREQ;
	else
		c |= CRGT;
	c >>= 28;
	reg.cr = (reg.cr & ~mkCR(rd, 0xF)) | mkCR(rd, c);
	if(trace)
		itrace("%s\tcrf%d,r%d,r%d [cr=#%x]", ci->name, rd, ra, rb, c);
}

void
cntlzw(ulong ir)
{
	int rs, ra, rb, n;

	getlrrr(ir);
	if(rb)
		undef(ir);
	for(n=0; n<32 && (reg.r[rs] & (1L<<(31-n))) == 0; n++)
		;
	reg.r[ra] = n;
	if(trace)
		itrace("%s%s\tr%d,r%d", ci->name, ir&1?".":"", ra, rs);
	if(ir & 1)
		setcr(0, reg.r[ra]);
}

void
eqv(ulong ir)
{
	int rs, ra, rb;

	getlrrr(ir);
	reg.r[ra] = ~(reg.r[rs] ^ reg.r[rb]);
	if(trace)
		itrace("%s%s\tr%d,r%d,r%d", ci->name, ir&1?".":"", ra, rs, rb);
	if(ir & 1)
		setcr(0, reg.r[ra]);
}

void
extsb(ulong ir)
{
	int rs, ra, rb;

	getlrrr(ir);
	if(rb)
		undef(ir);
	reg.r[ra] = (schar)reg.r[rs];
	if(trace)
		itrace("%s%s\tr%d,r%d", ci->name, ir&1?".":"", ra, rs);
	if(ir & 1)
		setcr(0, reg.r[ra]);
}

void
extsh(ulong ir)
{
	int rs, ra, rb;

	getlrrr(ir);
	if(rb)
		undef(ir);
	reg.r[ra] = (short)reg.r[rs];
	if(trace)
		itrace("%s%s\tr%d,r%d", ci->name, ir&1?".":"", ra, rs);
	if(ir & 1)
		setcr(0, reg.r[ra]);
}

void
add(ulong ir)
{
	int rd, ra, rb;
	uvlong r;

	getarrr(ir);
	r = (uvlong)(ulong)reg.r[ra] + (uvlong)(ulong)reg.r[rb];
	if(ir & OE) {
		reg.xer &= ~XER_OV;
		if(r >> 16)
			reg.xer |= XER_SO | XER_OV;	/* TO DO: rubbish */
	}
	reg.r[rd] = (ulong)r;
	if(ir & Rc)
		setcr(0, reg.r[rd]);
	if(trace)
		itrace("%s%s%s\tr%d,r%d,r%d", ci->name, ir&OE?"o":"", ir&1?".":"", rd, ra, rb);
}

void
addc(ulong ir)
{
	int rd, ra, rb;
	ulong v;
	uvlong r;

	getarrr(ir);
	r = (uvlong)(ulong)reg.r[ra] + (uvlong)(ulong)reg.r[rb];
	v = r>>32;
	reg.xer &= ~XER_CA;
	if(v)
		reg.xer |= XER_CA;
	if(ir & OE) {
		reg.xer &= ~XER_OV;
		if(v>>1)
			reg.xer |= XER_SO | XER_OV;
	}
	reg.r[rd] = (ulong)r;
	if(ir & Rc)
		setcr(0, reg.r[rd]);
	if(trace)
		itrace("%s%s%s\tr%d,r%d,r%d", ci->name, ir&OE?"o":"", ir&1?".":"", rd, ra, rb);
}

void
adde(ulong ir)
{
	int rd, ra, rb;
	ulong v;
	uvlong r;

	getarrr(ir);
	r = (uvlong)(ulong)reg.r[ra] + (uvlong)(ulong)reg.r[rb] + ((reg.xer&XER_CA)!=0);
	v = r>>32;
	reg.xer &= ~XER_CA;
	if(v)
		reg.xer |= XER_CA;
	if(ir & OE) {
		reg.xer &= ~XER_OV;
		if(v>>1)
			reg.xer |= XER_SO | XER_OV;
	}
	reg.r[rd] = (ulong)r;
	if(ir & Rc)
		setcr(0, reg.r[rd]);
	if(trace)
		itrace("%s%s%s\tr%d,r%d,r%d", ci->name, ir&OE?"o":"", ir&1?".":"", rd, ra, rb);
}

void
addic(ulong ir)
{
	int rd, ra;
	long imm;
	ulong v;
	uvlong r;

	getairr(ir);
	r = (uvlong)(ulong)reg.r[ra] + (uvlong)(ulong)imm;
	v = r>>32;
	reg.xer &= ~XER_CA;
	if(v)
		reg.xer |= XER_CA;
	reg.r[rd] = (ulong)r;
	if(trace)
		itrace("%s\tr%d,r%d,$%ld", ci->name, rd, ra, imm);
}

void
addiccc(ulong ir)
{
	int rd, ra;
	long imm;
	ulong v;
	uvlong r;

	getairr(ir);
	r = (uvlong)(ulong)reg.r[ra] + (uvlong)(ulong)imm;
	v = r>>32;
	reg.xer &= ~XER_CA;
	if(v)
		reg.xer |= XER_CA;
	reg.r[rd] = (ulong)r;
	setcr(0, reg.r[rd]);
	if(trace)
		itrace("%s\tr%d,r%d,$%ld", ci->name, rd, ra, imm);
}

void
addme(ulong ir)
{
	int rd, ra, rb;
	ulong v;
	uvlong r;

	getarrr(ir);
	if(rb)
		undef(ir);
	r = (uvlong)(ulong)reg.r[ra] + (uvlong)0xFFFFFFFFU + ((reg.xer&XER_CA)!=0);
	v = r>>32;
	reg.xer &= ~XER_CA;
	if(v)
		reg.xer |= XER_CA;
	if(ir & OE) {
		reg.xer &= ~XER_OV;
		if(v>>1)
			reg.xer |= XER_SO | XER_OV;
	}
	reg.r[rd] = (ulong)r;
	if(ir & Rc)
		setcr(0, reg.r[rd]);
	if(trace)
		itrace("%s%s%s\tr%d,r%d", ci->name, ir&OE?"o":"", ir&1?".":"", rd, ra);
}

void
addze(ulong ir)
{
	int rd, ra, rb;
	ulong v;
	uvlong r;

	getarrr(ir);
	if(rb)
		undef(ir);
	r = (uvlong)(ulong)reg.r[ra] + ((reg.xer&XER_CA)!=0);
	v = r>>32;
	reg.xer &= ~XER_CA;
	if(v)
		reg.xer |= XER_CA;
	if(ir & OE) {
		reg.xer &= ~XER_OV;
		if(v>>1)
			reg.xer |= XER_SO | XER_OV;
	}
	reg.r[rd] = (ulong)r;
	if(ir & Rc)
		setcr(0, reg.r[rd]);
	if(trace)
		itrace("%s%s%s\tr%d,r%d", ci->name, ir&OE?"o":"", ir&1?".":"", rd, ra);
}

void
divw(ulong ir)
{
	int rd, ra, rb;

	getarrr(ir);
	if(reg.r[rb] != 0 && ((ulong)reg.r[ra] != 0x80000000 || reg.r[rb] != -1))
		reg.r[rd] = reg.r[ra]/reg.r[rb];
	else if(ir & OE)
		reg.xer |= XER_SO | XER_OV;
	if(ir & Rc)
		setcr(0, reg.r[rd]);
	if(trace)
		itrace("%s%s%s\tr%d,r%d,r%d", ci->name, ir&OE?"o":"", ir&1?".":"", rd, ra, rb);
}

void
divwu(ulong ir)
{
	int rd, ra, rb;

	getarrr(ir);
	if(reg.r[rb] != 0)
		reg.r[rd] = (ulong)reg.r[ra]/(ulong)reg.r[rb];
	else if(ir & OE)
		reg.xer |= XER_SO | XER_OV;
	if(ir & Rc)
		setcr(0, reg.r[rd]);
	if(trace)
		itrace("%s%s%s\tr%d,r%d,r%d", ci->name, ir&OE?"o":"", ir&1?".":"", rd, ra, rb);
}

void
mcrxr(ulong ir)
{
	int rd, ra, rb;

	getarrr(ir);
	if(rd & 3 || ra != 0 || rb != 0 || ir & Rc)
		undef(ir);
	rd >>= 2;
	reg.cr = (reg.cr & ~mkCR(rd, 0xF)) | mkCR(rd, reg.xer>>28);
	reg.xer &= ~(0xF<<28);
}

void
mtcrf(ulong ir)
{
	int rs, crm, i;
	ulong m;

	if(ir & ((1<<20)|(1<<11)|Rc))
		undef(ir);
	rs = (ir>>21)&0x1F;
	crm = (ir>>12)&0xFF;
	m = 0;
	for(i = 0x80; i; i >>= 1) {
		m <<= 4;
		if(crm & i)
			m |= 0xF;
	}
	reg.cr = (reg.cr & ~m) | (reg.r[rs] & m);
}

void
mfcr(ulong ir)
{
	int rd, ra, rb;

	getarrr(ir);
	if(ra != 0 || rb != 0 || ir & Rc)
		undef(ir);
	reg.r[rd] = reg.cr;
}

void
mulhw(ulong ir)
{
	int rd, ra, rb;

	getarrr(ir);
	reg.r[rd] = ((vlong)(long)reg.r[ra]*(long)reg.r[rb])>>32;
	if(ir & Rc)
		setcr(0, reg.r[rd]);
	if(trace)
		itrace("%s%s%s\tr%d,r%d,r%d", ci->name, ir&OE?"o":"", ir&Rc?".":"", rd, ra, rb);
	/* BUG: doesn't set OV */
}

void
mulhwu(ulong ir)
{
	int rd, ra, rb;

	getarrr(ir);
	reg.r[rd] = ((uvlong)(ulong)reg.r[ra]*(ulong)reg.r[rb])>>32;
	if(ir & Rc)
		setcr(0, reg.r[rd]);	/* not sure whether CR setting is signed or unsigned */
	if(trace)
		itrace("%s%s%s\tr%d,r%d,r%d", ci->name, ir&OE?"o":"", ir&Rc?".":"", rd, ra, rb);
	/* BUG: doesn't set OV */
}

void
mullw(ulong ir)
{
	int rd, ra, rb;

	getarrr(ir);
	reg.r[rd] = (uvlong)(ulong)reg.r[ra]*(ulong)reg.r[rb];
	if(ir & Rc)
		setcr(0, reg.r[rd]);
	if(trace)
		itrace("%s%s%s\tr%d,r%d,r%d", ci->name, ir&OE?"o":"", ir&Rc?".":"", rd, ra, rb);
	/* BUG: doesn't set OV */
}

void
mulli(ulong ir)
{
	int rd, ra;
	long imm;

	getairr(ir);
	reg.r[rd] = (uvlong)(ulong)reg.r[ra]*(ulong)imm;
	if(trace)
		itrace("%s\tr%d,r%d,$%ld", ci->name, rd, ra, imm);
}

void
nand(ulong ir)
{
	int rs, ra, rb;

	getlrrr(ir);
	reg.r[ra] = ~(reg.r[rs] & reg.r[rb]);
	if(ir & Rc)
		setcr(0, reg.r[ra]);
	if(trace)
		itrace("%s%s\tr%d,r%d,r%d", ci->name, ir&1?".":"", ra, rs, rb);
}

void
neg(ulong ir)
{
	int rd, ra, rb;

	getarrr(ir);
	if(rb)
		undef(ir);
	if(ir & OE)
		reg.xer &= ~XER_OV;
	if((ulong)reg.r[ra] == 0x80000000) {
		if(ir & OE)
			reg.xer |= XER_SO | XER_OV;
		reg.r[rd] = reg.r[ra];
	} else
		reg.r[rd] = -reg.r[ra];
	if(ir & Rc)
		setcr(0, reg.r[rd]);
}

void
nor(ulong ir)
{
	int rs, ra, rb;

	getlrrr(ir);
	reg.r[ra] = ~(reg.r[rs] | reg.r[rb]);
	if(ir & Rc)
		setcr(0, reg.r[ra]);
	if(trace)
		itrace("%s%s\tr%d,r%d,r%d", ci->name, ir&1?".":"", ra, rs, rb);
}

void
or(ulong ir)
{
	int rs, ra, rb;

	getlrrr(ir);
	reg.r[ra] = reg.r[rs] | reg.r[rb];
	if(ir & Rc)
		setcr(0, reg.r[ra]);
	if(trace) {
		if(rs == rb)
			itrace("mr%s\tr%d,r%d", ir&1?".":"", ra, rs);
		else
			itrace("%s%s\tr%d,r%d,r%d", ci->name, ir&1?".":"", ra, rs, rb);
	}
}

void
orc(ulong ir)
{
	int rs, ra, rb;

	getlrrr(ir);
	reg.r[ra] = reg.r[rs] | ~reg.r[rb];
	if(ir & Rc)
		setcr(0, reg.r[ra]);
	if(trace)
		itrace("%s%s\tr%d,r%d,r%d", ci->name, ir&1?".":"", ra, rs, rb);
}

void
ori(ulong ir)
{
	int rs, ra;
	ulong imm;

	getlirr(ir);
	reg.r[ra] = reg.r[rs] | imm;
	if(trace)
		itrace("%s\tr%d,r%d,$0x%lx", ci->name, ra, rs, imm);
}

void
oris(ulong ir)
{
	int rs, ra;
	ulong imm;

	getlirr(ir);
	reg.r[ra] = reg.r[rs] | (imm<<16);
	if(trace)
		itrace("%s\tr%d,r%d,$0x%lx", ci->name, ra, rs, imm);
}

static ulong
mkmask(int mb, int me)
{
	int i;
	ulong v;

	if(mb > me)
		return mkmask(0, me) | mkmask(mb, 31);
	v = 0;
	for(i=mb; i<=me; i++)
		v |= 1L << (31-i);	/* don't need a loop, but i'm lazy */
	return v;
}

static ulong
rotl(ulong v, int sh)
{
	if(sh == 0)
		return v;
	return (v<<sh) | (v>>(32-sh));
}

void
rlwimi(ulong ir)
{
	int rs, ra, rb, sh;
	ulong m;

	getlrrr(ir);
	sh = rb;
	m = mkmask((ir>>6)&0x1F, (ir>>1)&0x1F);
	reg.r[ra] = (reg.r[ra] & ~m) | (rotl(reg.r[rs], sh) & m);
	if(trace)
		itrace("%s\tr%d,r%d,%d,#%lux", ci->name, ra, rs, sh, m);
	if(ir & 1)
		setcr(0, reg.r[ra]);
}

void
rlwinm(ulong ir)
{
	int rs, ra, rb, sh;
	ulong m;

	getlrrr(ir);
	sh = rb;
	m = mkmask((ir>>6)&0x1F, (ir>>1)&0x1F);
	reg.r[ra] = rotl(reg.r[rs], sh) & m;
	if(trace)
		itrace("%s%s\tr%d,r%d,%d,#%lux", ci->name, ir&Rc?".":"", ra, rs, sh, m);
	if(ir & Rc)
		setcr(0, reg.r[ra]);
}

void
rlwnm(ulong ir)
{
	int rs, ra, rb, sh;
	ulong m;

	getlrrr(ir);
	sh = reg.r[rb] & 0x1F;
	m = mkmask((ir>>6)&0x1F, (ir>>1)&0x1F);
	reg.r[ra] = rotl(reg.r[rs], sh) & m;
	if(trace)
		itrace("%s\tr%d,r%d,r%d,#%lux", ci->name, ra, rs, rb, m);
	if(ir & 1)
		setcr(0, reg.r[ra]);
}

void
slw(ulong ir)
{
	int rs, ra, rb;
	long v;

	getlrrr(ir);
	v = reg.r[rb];
	if((v & 0x20) == 0) {
		v &= 0x1F;
		reg.r[ra] = (ulong)reg.r[rs] << v;
	} else
		reg.r[ra] = 0;
	if(ir & Rc)
		setcr(0, reg.r[ra]);
	if(trace)
		itrace("%s%s\tr%d,r%d,r%d", ci->name, ir&1?".":"", ra, rs, rb);
}

void
sraw(ulong ir)
{
	int rs, ra, rb;
	long v;

	getlrrr(ir);
	v = reg.r[rb];
	if((v & 0x20) == 0) {
		v &= 0x1F;
		if(reg.r[rs]&SIGNBIT && v)
			reg.r[ra] = reg.r[rs]>>v | ~((1<<(32-v))-1);
		else
			reg.r[ra] = reg.r[rs]>>v;
	} else
		reg.r[ra] = reg.r[rs]&SIGNBIT? ~0: 0;
	if(ir & Rc)
		setcr(0, reg.r[ra]);
	if(trace)
		itrace("%s%s\tr%d,r%d,r%d", ci->name, ir&1?".":"", ra, rs, rb);
}

void
srawi(ulong ir)
{
	int rs, ra, rb;
	long v;

	getlrrr(ir);
	v = rb;
	if((v & 0x20) == 0) {
		v &= 0x1F;
		if(reg.r[rs]&SIGNBIT && v)
			reg.r[ra] = reg.r[rs]>>v | ~((1<<(32-v))-1);
		else
			reg.r[ra] = reg.r[rs]>>v;
	} else
		reg.r[ra] = reg.r[rs]&SIGNBIT? ~0: 0;
	if(ir & Rc)
		setcr(0, reg.r[ra]);
	if(trace)
		itrace("%s%s\tr%d,r%d,$%d", ci->name, ir&1?".":"", ra, rs, v);
}

void
srw(ulong ir)
{
	int rs, ra, rb;
	long v;

	getlrrr(ir);
	v = reg.r[rb];
	if((v & 0x20) == 0)
		reg.r[ra] = (ulong)reg.r[rs] >> (v&0x1F);
	else
		reg.r[ra] = 0;
	if(ir & Rc)
		setcr(0, reg.r[ra]);
	if(trace)
		itrace("%s%s\tr%d,r%d,r%d", ci->name, ir&1?".":"", ra, rs, rb);
}

void
subf(ulong ir)
{
	int rd, ra, rb;
	uvlong r;

	getarrr(ir);
	r = (uvlong)((ulong)~reg.r[ra]) + (uvlong)(ulong)reg.r[rb] + 1;
	if(ir & OE) {
		reg.xer &= ~XER_OV;
		if(r >> 16)
			reg.xer |= XER_SO | XER_OV;
	}
	reg.r[rd] = (ulong)r;
	if(ir & Rc)
		setcr(0, reg.r[rd]);
	if(trace)
		itrace("%s%s%s\tr%d,r%d,r%d", ci->name, ir&OE?"o":"", ir&1?".":"", rd, ra, rb);
}

void
subfc(ulong ir)
{
	int rd, ra, rb;
	ulong v;
	uvlong r;

	getarrr(ir);
	r = (uvlong)((ulong)~reg.r[ra]) + (uvlong)(ulong)reg.r[rb] + 1;
	v = r>>32;
	reg.xer &= ~XER_CA;
	if(v)
		reg.xer |= XER_CA;
	if(ir & OE) {
		reg.xer &= ~XER_OV;
		if(v>>1)
			reg.xer |= XER_SO | XER_OV;
	}
	reg.r[rd] = (ulong)r;
	if(ir & Rc)
		setcr(0, reg.r[rd]);
	if(trace)
		itrace("%s%s%s\tr%d,r%d,r%d", ci->name, ir&OE?"o":"", ir&1?".":"", rd, ra, rb);
}

void
subfe(ulong ir)
{
	int rd, ra, rb;
	ulong v;
	uvlong r;

	getarrr(ir);
	r = (uvlong)((ulong)~reg.r[ra]) + (uvlong)(ulong)reg.r[rb] + ((reg.xer&XER_CA)!=0);
	v = r>>32;
	reg.xer &= ~XER_CA;
	if(v)
		reg.xer |= XER_CA;
	if(ir & OE) {
		reg.xer &= ~XER_OV;
		if(v>>1)
			reg.xer |= XER_SO | XER_OV;
	}
	reg.r[rd] = (ulong)r;
	if(ir & Rc)
		setcr(0, reg.r[rd]);
	if(trace)
		itrace("%s%s%s\tr%d,r%d,r%d", ci->name, ir&OE?"o":"", ir&1?".":"", rd, ra, rb);
}

void
subfic(ulong ir)
{
	int rd, ra;
	long imm;
	ulong v;
	uvlong r;

	getairr(ir);
	r = (uvlong)((ulong)~reg.r[ra]) + (uvlong)(ulong)imm + 1;
	v = r>>32;
	reg.xer &= ~XER_CA;
	if(v)
		reg.xer |= XER_CA;
	reg.r[rd] = (ulong)r;
	if(trace)
		itrace("%s\tr%d,r%d,$%ld", ci->name, rd, ra, imm);
}

void
subfme(ulong ir)
{
	int rd, ra, rb;
	ulong v;
	uvlong r;

	getarrr(ir);
	if(rb)
		undef(ir);
	r = (uvlong)((ulong)~reg.r[ra]) + (uvlong)0xFFFFFFFFU + ((reg.xer&XER_CA)!=0);
	v = r>>32;
	reg.xer &= ~XER_CA;
	if(v)
		reg.xer |= XER_CA;
	if(ir & OE) {
		reg.xer &= ~XER_OV;
		if(v>>1)
			reg.xer |= XER_SO | XER_OV;
	}
	reg.r[rd] = (ulong)r;
	if(ir & Rc)
		setcr(0, reg.r[rd]);
	if(trace)
		itrace("%s%s%s\tr%d,r%d", ci->name, ir&OE?"o":"", ir&1?".":"", rd, ra);
}

void
subfze(ulong ir)
{
	int rd, ra, rb;
	ulong v;
	uvlong r;

	getarrr(ir);
	if(rb)
		undef(ir);
	r = (uvlong)((ulong)~reg.r[ra]) + ((reg.xer&XER_CA)!=0);
	v = r>>32;
	reg.xer &= ~XER_CA;
	if(v)
		reg.xer |= XER_CA;
	if(ir & OE) {
		reg.xer &= ~XER_OV;
		if(v>>1)
			reg.xer |= XER_SO | XER_OV;
	}
	reg.r[rd] = (ulong)r;
	if(ir & Rc)
		setcr(0, reg.r[rd]);
	if(trace)
		itrace("%s%s%s\tr%d,r%d", ci->name, ir&OE?"o":"", ir&1?".":"", rd, ra);
}

void
xor(ulong ir)
{
	int rs, ra, rb;

	getlrrr(ir);
	reg.r[ra] = reg.r[rs] ^ reg.r[rb];
	if(trace)
		itrace("%s\tr%d,r%d,r%d", ci->name, ra, rs, rb);
}

void
xori(ulong ir)
{
	int rs, ra;
	ulong imm;

	getlirr(ir);
	reg.r[ra] = reg.r[rs] ^ imm;
	if(trace)
		itrace("%s\tr%d,r%d,$0x%lx", ci->name, ra, rs, imm);
}

void
xoris(ulong ir)
{
	int rs, ra;
	ulong imm;

	getlirr(ir);
	reg.r[ra] = reg.r[rs] ^ (imm<<16);
	if(trace)
		itrace("%s\tr%d,r%d,$0x%lx", ci->name, ra, rs, imm);
}

void
lwz(ulong ir)
{
	ulong ea;
	int ra, rd, upd;
	long imm;

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
		itrace("%s\tr%d,%ld(r%d) ea=%lux", ci->name, rd, imm, ra, ea);

	reg.r[rd] = getmem_w(ea);
}

void
lwzx(ulong ir)
{
	ulong ea;
	int rb, ra, rd, upd;

	getarrr(ir);
	ea = reg.r[rb];
	upd = getxo(ir)==55;
	if(ra) {
		ea += reg.r[ra];
		if(upd)
			reg.r[ra] = ea;
		if(trace)
			itrace("%s\tr%d,(r%d+r%d) ea=%lux", ci->name, rd, ra, rb, ea);
	} else {
		if(upd)
			undef(ir);
		if(trace)
			itrace("%s\tr%d,(r%d) ea=%lux", ci->name, rd, rb, ea);
	}

	reg.r[rd] = getmem_w(ea);
}

void
lwarx(ulong ir)
{
	lwzx(ir);
}

void
lbz(ulong ir)
{
	ulong ea;
	int ra, rd, upd;
	long imm;

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
		itrace("%s\tr%d,%ld(r%d) ea=%lux", ci->name, rd, imm, ra, ea);

	reg.r[rd] = getmem_b(ea);
}

void
lbzx(ulong ir)
{
	ulong ea;
	int rb, ra, rd, upd;

	getarrr(ir);
	ea = reg.r[rb];
	upd = getxo(ir)==119;
	if(ra) {
		ea += reg.r[ra];
		if(upd)
			reg.r[ra] = ea;
		if(trace)
			itrace("%s\tr%d,(r%d+r%d) ea=%lux", ci->name, rd, ra, rb, ea);
	} else {
		if(upd)
			undef(ir);
		if(trace)
			itrace("%s\tr%d,(r%d) ea=%lux", ci->name, rd, rb, ea);
	}

	reg.r[rd] = getmem_b(ea);
}

void
stw(ulong ir)
{
	ulong ea;
	int ra, rd, upd;
	long imm;

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
		itrace("%s\tr%d,%ld(r%d) #%lux=#%lux (%ld)",
					ci->name, rd, imm, ra, ea, reg.r[rd], reg.r[rd]);
	putmem_w(ea, reg.r[rd]);

}

void
stwx(ulong ir)
{
	ulong ea;
	int ra, rd, upd, rb;

	getarrr(ir);
	ea = reg.r[rb];
	upd = getxo(ir)==183;
	if(ra) {
		ea += reg.r[ra];
		if(upd)
			reg.r[ra] = ea;
		if(trace)
			itrace("%s\tr%d,(r%d+r%d) #%lux=#%lux (%ld)",
					ci->name, rd, ra, rb, ea, reg.r[rd], reg.r[rd]);
	} else {
		if(upd)
			undef(ir);
		if(trace)
			itrace("%s\tr%d,(r%d) #%lux=#%lux (%ld)",
					ci->name, rd, rb, ea, reg.r[rd], reg.r[rd]);
	}
	putmem_w(ea, reg.r[rd]);

}

void
stwcx(ulong ir)
{
	ulong ea;
	int ra, rd, rb;

	if((ir & Rc) == 0)
		undef(ir);
	getarrr(ir);
	ea = reg.r[rb];
	if(ra) {
		ea += reg.r[ra];
		if(trace)
			itrace("%s\tr%d,(r%d+r%d) #%lux=#%lux (%ld)",
					ci->name, rd, ra, rb, ea, reg.r[rd], reg.r[rd]);
	} else {
		if(trace)
			itrace("%s\tr%d,(r%d) #%lux=#%lux (%ld)",
					ci->name, rd, rb, ea, reg.r[rd], reg.r[rd]);
	}
	putmem_w(ea, reg.r[rd]);	/* assume a reservation exists; store succeeded */
	setcr(0, 0);

}

void
stb(ulong ir)
{
	ulong ea;
	int ra, rd, upd, v;
	long imm;

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
	v = reg.r[rd] & 0xFF;
	if(trace)
		itrace("%s\tr%d,%ld(r%d) #%lux=#%lux (%ld)",
					ci->name, rd, imm, ra, ea, v, v);
	putmem_b(ea, v);
}

void
stbx(ulong ir)
{
	ulong ea;
	int ra, rd, upd, rb, v;

	getarrr(ir);
	ea = reg.r[rb];
	upd = getxo(ir)==247;
	v = reg.r[rd] & 0xFF;
	if(ra) {
		ea += reg.r[ra];
		if(upd)
			reg.r[ra] = ea;
		if(trace)
			itrace("%s\tr%d,(r%d+r%d) #%lux=#%lux (%ld)",
					ci->name, rd, ra, rb, ea, v, v);
	} else {
		if(upd)
			undef(ir);
		if(trace)
			itrace("%s\tr%d,(r%d) #%lux=#%lux (%ld)",
					ci->name, rd, rb, ea, v, v);
	}
	putmem_b(ea, v);

}

void
lhz(ulong ir)
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
		itrace("%s\tr%d,%ld(r%d) ea=%lux", ci->name, rd, imm, ra, ea);

	reg.r[rd] = getmem_h(ea);
}

void
lhzx(ulong ir)
{
	ulong ea;
	int rb, ra, rd, upd;

	getarrr(ir);
	ea = reg.r[rb];
	upd = getxo(ir)==311;
	if(ra) {
		ea += reg.r[ra];
		if(upd)
			reg.r[ra] = ea;
		if(trace)
			itrace("%s\tr%d,(r%d+r%d) ea=%lux", ci->name, rd, ra, rb, ea);
	} else {
		if(upd)
			undef(ir);
		if(trace)
			itrace("%s\tr%d,(r%d) ea=%lux", ci->name, rd, rb, ea);
	}

	reg.r[rd] = getmem_h(ea);
}

void
lha(ulong ir)
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
		itrace("%s\tr%d,%ld(r%d) ea=%lux", ci->name, rd, imm, ra, ea);

	reg.r[rd] = (short)getmem_h(ea);
}

void
lhax(ulong ir)
{
	ulong ea;
	int rb, ra, rd, upd;

	getarrr(ir);
	ea = reg.r[rb];
	upd = getxo(ir)==311;
	if(ra) {
		ea += reg.r[ra];
		if(upd)
			reg.r[ra] = ea;
		if(trace)
			itrace("%s\tr%d,(r%d+r%d) ea=%lux", ci->name, rd, ra, rb, ea);
	} else {
		if(upd)
			undef(ir);
		if(trace)
			itrace("%s\tr%d,(r%d) ea=%lux", ci->name, rd, rb, ea);
	}

	reg.r[rd] = (short)getmem_h(ea);
}

void
lhbrx(ulong ir)
{
	ulong ea;
	int rb, ra, rd;
	ulong v;

	getarrr(ir);
	ea = reg.r[rb];
	if(ra) {
		ea += reg.r[ra];
		if(trace)
			itrace("%s\tr%d,(r%d+r%d) ea=%lux", ci->name, rd, ra, rb, ea);
	} else {
		if(trace)
			itrace("%s\tr%d,(r%d) ea=%lux", ci->name, rd, rb, ea);
	}
	v = getmem_h(ea);

	reg.r[rd] = ((v&0xFF)<<8)|(v&0xFF);
}

void
sth(ulong ir)
{
	ulong ea;
	int imm, ra, rd, upd, v;

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
	v = reg.r[rd] & 0xFFFF;
	if(trace)
		itrace("%s\tr%d,%ld(r%d) #%lux=#%lux (%ld)",
					ci->name, rd, imm, ra, ea, v, v);
	putmem_h(ea, v);

}

void
sthx(ulong ir)
{
	ulong ea;
	int ra, rd, upd, rb, v;

	getarrr(ir);
	ea = reg.r[rb];
	upd = getxo(ir)==247;
	v = reg.r[rd] & 0xFFFF;
	if(ra) {
		ea += reg.r[ra];
		if(upd)
			reg.r[ra] = ea;
		if(trace)
			itrace("%s\tr%d,(r%d+r%d) #%lux=#%lux (%ld)",
					ci->name, rd, ra, rb, ea, v, v);
	} else {
		if(upd)
			undef(ir);
		if(trace)
			itrace("%s\tr%d,(r%d) #%lux=#%lux (%ld)",
					ci->name, rd, rb, ea, v, v);
	}
	putmem_h(ea, v);
}

void
sthbrx(ulong ir)
{
	ulong ea;
	int ra, rd, rb;
	ulong v;

	getarrr(ir);
	ea = reg.r[rb];
	v = reg.r[rd];
	v = ((v&0xFF)<<8)|(v&0xFF);
	if(ra) {
		ea += reg.r[ra];
		if(trace)
			itrace("%s\tr%d,(r%d+r%d) #%lux=#%lux (%ld)",
					ci->name, rd, ra, rb, ea, v, v);
	} else {
		if(trace)
			itrace("%s\tr%d,(r%d) #%lux=#%lux (%ld)",
					ci->name, rd, rb, ea, v, v);
	}
	putmem_h(ea, v);
}

void
lwbrx(ulong ir)
{
	ulong ea;
	int rb, ra, rd, i;
	ulong v;

	getarrr(ir);
	if(ir & Rc)
		undef(ir);
	ea = reg.r[rb];
	if(ra) {
		ea += reg.r[ra];
		if(trace)
			itrace("%s\tr%d,(r%d+r%d) ea=%lux", ci->name, rd, ra, rb, ea);
	} else {
		if(trace)
			itrace("%s\tr%d,(r%d) ea=%lux", ci->name, rd, rb, ea);
	}
	v = 0;
	for(i = 0; i < 4; i++)
		v = v>>8 | getmem_b(ea++);	/* assume unaligned load is allowed */
	reg.r[rd] = v;
}

void
stwbrx(ulong ir)
{
	ulong ea;
	int rb, ra, rd, i;
	ulong v;

	getarrr(ir);
	if(ir & Rc)
		undef(ir);
	ea = reg.r[rb];
	if(ra) {
		ea += reg.r[ra];
		if(trace)
			itrace("%s\tr%d,(r%d+r%d) ea=%lux", ci->name, rd, ra, rb, ea);
	} else {
		if(trace)
			itrace("%s\tr%d,(r%d) ea=%lux", ci->name, rd, rb, ea);
	}
	v = 0;
	for(i = 0; i < 4; i++) {
		putmem_b(ea++, v & 0xFF);	/* assume unaligned store is allowed */
		v >>= 8;
	}
}

void
lswi(ulong ir)
{
	ulong ea;
	int rb, ra, rd, n, i, r, b;

	getarrr(ir);
	if(ir & Rc)
		undef(ir);
	n = rb;
	if(n == 0)
		n = 32;
	ea = 0;
	if(ra) {
		ea += reg.r[ra];
		if(trace)
			itrace("%s\tr%d,(r%d),%d ea=%lux", ci->name, rd, ra, n, ea);
	} else {
		if(trace)
			itrace("%s\tr%d,(0),%d ea=0", ci->name, rd, n);
	}
	i = -1;
	r = rd-1;
	while(--n >= 0) {
		if(i < 0) {
			r = (r+1)&0x1F;
			if(ra == 0 || r != ra)
				reg.r[r] = 0;
			i = 24;
		}
		b = getmem_b(ea++);
		if(ra == 0 || r != ra)
			reg.r[r] = (reg.r[r] & ~(0xFF<<i)) | (b << i);
		i -= 8;
	}
}

void
lswx(ulong ir)
{
	ulong ea;
	int rb, ra, rd, n, i, r, b;

	getarrr(ir);
	if(ir & Rc)
		undef(ir);
	n = reg.xer & 0x7F;
	ea = reg.r[rb];
	if(ra) {
		ea += reg.r[ra];
		if(trace)
			itrace("%s\tr%d,(r%d+r%d) ea=%lux n=%d", ci->name, rd, ra, rb, ea, n);
	} else {
		if(trace)
			itrace("%s\tr%d,(r%d) ea=%lux n=%d", ci->name, rd, rb, ea, n);
	}
	i = -1;
	r = rd-1;
	while(--n >= 0) {
		if(i < 0) {
			r = (r+1)&0x1F;
			if((ra == 0 || r != ra) && r != rb)
				reg.r[r] = 0;
			i = 24;
		}
		b = getmem_b(ea++);
		if((ra == 0 || r != ra) && r != rb)
			reg.r[r] = (reg.r[r] & ~(0xFF<<i)) | (b << i);
		i -= 8;
	}
}

void
stswx(ulong ir)
{
	ulong ea;
	int rb, ra, rd, n, i, r;

	getarrr(ir);
	if(ir & Rc)
		undef(ir);
	n = reg.xer & 0x7F;
	ea = reg.r[rb];
	if(ra) {
		ea += reg.r[ra];
		if(trace)
			itrace("%s\tr%d,(r%d+r%d) ea=%lux n=%d", ci->name, rd, ra, rb, ea, n);
	} else {
		if(trace)
			itrace("%s\tr%d,(r%d) ea=%lux n=%d", ci->name, rd, rb, ea, n);
	}
	i = -1;
	r = rd-1;
	while(--n >= 0) {
		if(i < 0) {
			r = (r+1)&0x1F;
			i = 24;
		}
		putmem_b(ea++, (reg.r[r]>>i)&0xFF);
		i -= 8;
	}
}

void
stswi(ulong ir)
{
	ulong ea;
	int rb, ra, rd, n, i, r;

	getarrr(ir);
	if(ir & Rc)
		undef(ir);
	n = rb;
	if(n == 0)
		n = 32;
	ea = 0;
	if(ra) {
		ea += reg.r[ra];
		if(trace)
			itrace("%s\tr%d,(r%d),%d ea=%lux", ci->name, rd, ra, n, ea);
	} else {
		if(trace)
			itrace("%s\tr%d,(0),%d ea=0", ci->name, rd, n);
	}
	i = -1;
	r = rd-1;
	while(--n >= 0) {
		if(i < 0) {
			r = (r+1)&0x1F;
			i = 24;
		}
		putmem_b(ea++, (reg.r[r]>>i)&0xFF);
		i -= 8;
	}
}

void
lmw(ulong ir)
{
	ulong ea;
	int ra, rd, r;
	long imm;

	getairr(ir);
	ea = imm;
	if(ra)
		ea += reg.r[ra];
	if(trace)
		itrace("%s\tr%d,%ld(r%d) ea=%lux", ci->name, rd, imm, ra, ea);

	for(r = rd; r <= 31; r++) {
		if(r != 0 && r != rd)
			reg.r[rd] = getmem_w(ea);
		ea += 4;
	}
}

void
stmw(ulong ir)
{
	ulong ea;
	int ra, rd, r;
	long imm;

	getairr(ir);
	ea = imm;
	if(ra)
		ea += reg.r[ra];
	if(trace)
		itrace("%s\tr%d,%ld(r%d) ea=%lux", ci->name, rd, imm, ra, ea);

	for(r = rd; r <= 31; r++) {
		putmem_w(ea, reg.r[rd]);
		ea += 4;
	}
}

void
twi(ulong ir)
{
	int rd, ra;
	long a, imm;

	getairr(ir);
	a = reg.r[ra];
	if(trace)
		itrace("twi\t#%.2x,r%d,$0x%lux (%ld)", rd, ra, imm, imm);
	if(a < imm && rd&0x10 ||
	   a > imm && rd&0x08 ||
	   a == imm && rd&0x04 ||
	   (ulong)a < imm && rd&0x02 ||
	   (ulong)a > imm && rd&0x01) {
		Bprint(bioout, "program_exception (trap type)\n");
		longjmp(errjmp, 0);
	}
}

void
tw(ulong ir)
{
	int rd, ra, rb;
	long a, b;

	getarrr(ir);
	a = reg.r[ra];
	b = reg.r[rb];
	if(trace)
		itrace("tw\t#%.2x,r%d,r%d", rd, ra, rb);
	if(a < b && rd&0x10 ||
	   a > b && rd&0x08 ||
	   a == b && rd&0x04 ||
	   (ulong)a < b && rd&0x02 ||
	   (ulong)a > b && rd&0x01) {
		Bprint(bioout, "program_exception (trap type)\n");
		longjmp(errjmp, 0);
	}
}

void
sync(ulong ir)
{
	USED(ir);
	if(trace)
		itrace("sync");
}

void
icbi(ulong ir)
{
	int rd, ra, rb;

	if(ir & Rc)
		undef(ir);
	getarrr(ir);
	USED(rd);
	if(trace)
		itrace("%s\tr%d,r%d", ci->name, ra, rb);
}

void
dcbf(ulong ir)
{
	int rd, ra, rb;

	if(ir & Rc)
		undef(ir);
	getarrr(ir);
	USED(rd);
	if(trace)
		itrace("%s\tr%d,r%d", ci->name, ra, rb);
}

void
dcbst(ulong ir)
{
	int rd, ra, rb;

	if(ir & Rc)
		undef(ir);
	getarrr(ir);
	USED(rd);
	if(trace)
		itrace("%s\tr%d,r%d", ci->name, ra, rb);
}

void
dcbt(ulong ir)
{
	int rd, ra, rb;

	if(ir & Rc)
		undef(ir);
	getarrr(ir);
	USED(rd);
	if(trace)
		itrace("%s\tr%d,r%d", ci->name, ra, rb);
}

void
dcbtst(ulong ir)
{
	int rd, ra, rb;

	if(ir & Rc)
		undef(ir);
	getarrr(ir);
	USED(rd);
	if(trace)
		itrace("%s\tr%d,r%d", ci->name, ra, rb);
}

void
dcbz(ulong ir)
{
	int rd, ra, rb;

	if(ir & Rc)
		undef(ir);
	getarrr(ir);
	USED(rd);
	if(trace)
		itrace("%s\tr%d,r%d", ci->name, ra, rb);
}
