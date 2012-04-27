#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>
#define Extern extern
#include "power.h"

void	mcrf(ulong);
void	bclr(ulong);
void	crop(ulong);
void	bcctr(ulong);
void	call(ulong);
void	ret(ulong);
void isync(ulong);

Inst	op19[] = {
[0] {mcrf, "mcrf", Ibranch},
[16] {bclr, "bclr", Ibranch},
[33] {crop, "crnor", Ibranch},
[15] {0, "rfi", Ibranch},
[129] {crop, "crandc", Ibranch},
[150] {isync, "isync", Ibranch},
[193] {crop, "crxor", Ibranch},
[225] {crop, "crnand", Ibranch},
[257] {crop, "crand", Ibranch},
[289] {crop, "creqv", Ibranch},
[417] {crop, "crorc", Ibranch},
[449] {crop, "cror", Ibranch},
[528] {bcctr, "bcctr", Ibranch},
	{0, 0, 0}
};

Inset	ops19 = {op19, nelem(op19)-1};

static char *
boname(int bo)
{
	static char buf[8];

	switch(bo>>1){
	case 0:	return "dnzf";
	case 1:	return "dzf";
	case 2:	return "f";
	case 4:	return "dnzt";
	case 5:	return "dzt";
	case 6:	return "t";
	case 8:	return "dnz";
	case 9:	return "dz";
	case 10:	return "a";
	default:
		sprint(buf, "%d?", bo);
		return buf;
	}
}

static char *
cname(int bo, int bi)
{
	int f;
	char *p;
	static char buf[20];
	static char *f0[] = {"lt", "gt", "eq", "so/un"};

	if(bo == 0x14){	/* branch always */
		sprint(buf,"%d", bi);
		return buf;
	}
	for(f = 0; bi >= 4; bi -= 4)
		f++;
	p = buf;
	p += sprint(buf, "%d[", bi);
	if(f)
		p += sprint(buf, "cr%d+", f);
	strcpy(p, f0[bi&3]);
	strcat(p, "]");
	return buf;
}

static int
condok(ulong ir, int ctr)
{
	int bo, bi, xx;

	getbobi(ir);
	if(xx)
		undef(ir);
	if((bo & 0x4) == 0) {
		if(!ctr)
			undef(ir);
		reg.ctr--;
	}
	if(bo & 0x4 || (reg.ctr!=0)^((bo>>1)&1)) {
		if(bo & 0x10 || (((reg.cr & bits[bi])!=0)==((bo>>3)&1)))
			return 1;
	}
	return 0;
}

static void
dobranch(ulong ir, ulong *r, int ctr)
{
	int bo, bi, xx;
	ulong nia;

	getbobi(ir);
	USED(xx);
	if(condok(ir, ctr)) {
		ci->taken++;
		nia = *r & ~3;
		if(bo & 4)	/* assume counting branches aren't returns */
			ret(nia);
	} else
		nia = reg.pc + 4;
	if(trace)
		itrace("%s%s\t%s,%s,#%.8lux", ci->name, ir&1? "l": "", boname(bo), cname(bo, bi), nia);
	if(ir & 1) {
		call(nia);
		reg.lr = reg.pc + 4;
	}
	reg.pc = nia-4;
	/* branch delays? */
}

void
bcctr(ulong ir)
{
	dobranch(ir, &reg.ctr, 1);
}

void
bclr(ulong ir)
{
	dobranch(ir, &reg.lr, 0);
}

void
bcx(ulong ir)
{
	int bo, bi, xx;
	ulong ea;
	long imm;
	static char *opc[] = {"bc", "bcl", "bca", "bcla"};

	getbobi(ir);
	USED(xx);
	imm = ir & 0xFFFC;
	if(ir & 0x08000)
		imm |= 0xFFFF0000;
	if((ir & 2) == 0) {	/* not absolute address */
		ea = reg.pc + imm;
		if(trace)
			itrace("%s\t%s,%s,.%s%ld\tea = #%.8lux", opc[ir&3], boname(bo), cname(bo, bi), imm<0?"":"+", imm, ea);
	} else {
		ea = imm;
		if(trace)
			itrace("%s\t%s,%s,#%.8lux", opc[ir&3], boname(bo), cname(bo, bi), ea);
	}
	if(condok(ir&0xFFFF0000, 1))
		ci->taken++;
	else
		ea = reg.pc + 4;
	if(ir & 1) {
		call(ea);
		reg.lr = reg.pc+4;
	}
	reg.pc = ea-4;
	/* branch delay? */
}

void
crop(ulong ir)
{
	int rd, ra, rb, d;

	getarrr(ir);
	if(trace)
		itrace("%s\tcrb%d,crb%d,crb%d", ci->name, rd, ra, rb);
	ra = (reg.cr & bits[ra]) != 0;
	rb = (reg.cr & bits[rb]) != 0;
	d = 0;
	switch(getxo(ir)) {
	case 257:	d = ra & rb; break;
	case 129:	d = ra & !rb; break;
	case 289:	d = ra == rb; break;
	case 225:	d = !(ra & rb); break;
	case 33:	d = !(ra | rb); break;
	case 449:	d = ra | rb; break;
	case 417:	d = ra | !rb; break;
	case 193:	d = ra ^ rb; break;
	default:	undef(ir); break;
	}
	if(d)
		reg.cr |= bits[rd];
}

void
mcrf(ulong ir)
{
	int rd, ra, rb;

	getarrr(ir);
	if(ir & 1 || rd & 3 || ra & 3 || rb)
		undef(ir);
	ra >>= 2;
	rd >>= 2;
	reg.cr = (reg.cr & ~mkCR(rd, 0xF)) | mkCR(rd, getCR(ra, reg.cr));
	if(trace)
		itrace("mcrf\tcrf%d,crf%d", rd, ra);
}

void
call(ulong npc)
{
	Symbol s;

	if(calltree) {
		findsym(npc, CTEXT, &s);
		Bprint(bioout, "%8lux %s(", reg.pc, s.name);
		printparams(&s, reg.r[1]);
		Bprint(bioout, "from ");
		printsource(reg.pc);
		Bputc(bioout, '\n');
	}
}

void
ret(ulong npc)
{
	Symbol s;

	if(calltree) {
		findsym(npc, CTEXT, &s);
		Bprint(bioout, "%8lux return to #%lux %s r3=#%lux (%ld)\n",
					reg.pc, npc, s.name, reg.r[3], reg.r[3]);
	}
}

void
bx(ulong ir)
{
	ulong ea;
	long imm;
	static char *opc[] = {"b", "bl", "ba", "bla"};

	imm = ir & 0x03FFFFFC;
	if(ir & 0x02000000)
		imm |= 0xFC000000;
	if((ir & 2) == 0) {	/* not absolute address */
		ea = reg.pc + imm;
		if(trace)
			itrace("%s\t.%s%ld\tea = #%.8lux", opc[ir&3], imm<0?"":"+", imm, ea);
	} else {
		ea = imm;
		if(trace)
			itrace("%s\t#%.8lux", opc[ir&3], ea);
	}
	ci->taken++;
	if(ir & 1) {
		call(ea);
		reg.lr = reg.pc+4;
	}
	reg.pc = ea-4;
	/* branch delay? */
}

void
isync(ulong ir)
{
	USED(ir);
	if(trace)
		itrace("isync");
}
