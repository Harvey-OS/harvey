#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>
#define Extern extern
#include "sim.h"

void	Iconstn(ulong);
void	Iconsth(ulong);
void	Iconst(ulong);
void	Istor(ulong);
void	Istori(ulong);
void	Ilod(ulong);
void	Ilodi(ulong);
void	Imfsr(ulong);
void	Imtsr(ulong);
void	Isetip(ulong);

void	Iadd(ulong);
void	Iaddi(ulong);
void	Isub(ulong);
void	Isubi(ulong);
void	Iisub(ulong);
void	Iisubi(ulong);
void	Iand(ulong);
void	Iandi(ulong);
void	Ior(ulong);
void	Iori(ulong);
void	Ixor(ulong);
void	Ixori(ulong);
void	Ixnor(ulong);
void	Ixnori(ulong);
void	Isll(ulong);
void	Islli(ulong);
void	Isrl(ulong);
void	Isrli(ulong);
void	Isra(ulong);
void	Israi(ulong);

void	Icpeq(ulong);
void	Icpeqi(ulong);
void	Icpge(ulong);
void	Icpgei(ulong);
void	Icpgeu(ulong);
void	Icpgeui(ulong);
void	Icpgt(ulong);
void	Icpgti(ulong);
void	Icpgtu(ulong);
void	Icpgtui(ulong);
void	Icple(ulong);
void	Icplei(ulong);
void	Icpleu(ulong);
void	Icpleui(ulong);
void	Icplt(ulong);
void	Icplti(ulong);
void	Icpltu(ulong);
void	Icpltui(ulong);
void	Icpneq(ulong);
void	Icpneqi(ulong);

void	Iaseq(ulong);
void	Iaseqi(ulong);

void	Imstep(ulong);
void	Imstepi(ulong);
void	Imstepl(ulong);
void	Imstepli(ulong);
void	Imstepul(ulong);
void	Imstepuli(ulong);

void	Idstep0(ulong);
void	Idstep0i(ulong);
void	Idstep(ulong);
void	Idstepi(ulong);
void	Idstepl(ulong);
void	Idstepli(ulong);
void	Idstepr(ulong);
void	Idstepri(ulong);

void	Icall(ulong);
void	Icalli(ulong);
void	Icalla(ulong);
void	Ijmp(ulong);
void	Ijmpi(ulong);
void	Ijmpa(ulong);
void	Ijmpt(ulong);
void	Ijmpti(ulong);
void	Ijmpta(ulong);
void	Ijmpf(ulong);
void	Ijmpfi(ulong);
void	Ijmpfa(ulong);
void	Ijmpfdec(ulong);
void	Ijmpfdeca(ulong);

Inst itab[] = {
	{ undef,	"" },	/* 0x00 */
	{ Iconstn,	"constn" },
	{ Iconsth,	"consth" },
	{ Iconst,	"const" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },

	{ undef,	"" },	/* 0x10 */
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ Iadd,		"add" },
	{ Iaddi,	"addi" },
	{ Ilod,		"load" },
	{ Ilodi,	"loadi" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ Istor,	"store" },
	{ Istori,	"storei" },

	{ undef,	"" },	/* 0x20 */
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ Isub,		"sub" },
	{ Isubi,	"subi" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },

	{ undef,	"" },	/* 0x30 */
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ Iisub,	"isub" },
	{ Iisubi,	"isubi" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },

	{ Icplt,	"cplt" },	/* 0x40 */
	{ Icplti,	"cplti" },
	{ Icpltu,	"cpltu" },
	{ Icpltui,	"cpltui" },
	{ Icple,	"cple" },
	{ Icplei,	"cplei" },
	{ Icpleu,	"cpleu" },
	{ Icpleui,	"cpleui" },
	{ Icpgt,	"cpgt" },
	{ Icpgti,	"cpgti" },
	{ Icpgtu,	"cpgtu" },
	{ Icpgtui,	"cpgtui" },
	{ Icpge,	"cpge" },
	{ Icpgei,	"cpgei" },
	{ Icpgeu,	"cpgeu" },
	{ Icpgeui,	"cpgeui" },

	{ undef,	"" },	/* 0x50 */
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },

	{ Icpeq,	"cpeq" },	/* 0x60 */
	{ Icpeqi,	"cpeqi" },
	{ Icpneq,	"cpneq" },
	{ Icpneqi,	"cpneqi" },
	{ Imstep,	"mstep" },
	{ Imstepi,	"mstepi" },
	{ Imstepl,	"mstepl" },
	{ Imstepli,	"mstepli" },
	{ Idstep0,	"dstepi" },
	{ Idstep0i,	"dstepii" },
	{ Idstep,	"dstep" },
	{ Idstepi,	"dstepi" },
	{ Idstepl,	"dstepl" },
	{ Idstepli,	"dstepli" },
	{ Idstepr,	"dstepr" },
	{ Idstepri,	"dstepri" },

	{ Iaseq,	"aseq" },	/* 0x70 */
	{ Iaseqi,	"aseqi" },
	{ undef,	"" },
	{ undef,	"" },
	{ Imstepul,	"mstepul" },
	{ Imstepuli,	"mstepuli" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },

	{ Isll,		"sll" },	/* 0x80 */
	{ Islli,	"slli" },
	{ Isrl,		"srl" },
	{ Isrli,	"srli" },
	{ undef,	"" },
	{ undef,	"" },
	{ Isra,		"sra" },
	{ Israi,	"srai" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },

	{ Iand,		"and" },	/* 0x90 */
	{ Iandi,	"andi" },
	{ Ior,		"or" },
	{ Iori,		"ori" },
	{ Ixor,		"xor" },
	{ Ixori,	"xori" },
	{ Ixnor,	"xnor" },
	{ Ixnori,	"xnori" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ Isetip,	"setip" },
	{ undef,	"" },

	{ Ijmp,		"jmp" },	/* 0xa0 */
	{ Ijmpa,	"jmpa" },
	{ undef,	"" },
	{ undef,	"" },
	{ Ijmpf,	"jmpf" },
	{ Ijmpfa,	"jmpfa" },
	{ undef,	"" },
	{ undef,	"" },
	{ Icall,	"call" },
	{ Icalla,	"calla" },
	{ undef,	"" },
	{ undef,	"" },
	{ Ijmpt,	"jmpt" },
	{ Ijmpta,	"jmpta" },
	{ undef,	"" },
	{ undef,	"" },

	{ undef,	"" },	/* 0xb0 */
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },

	{ Ijmpi,	"jmpi" },	/* 0xc0 */
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ Ijmpfi,	"jmpfi" },
	{ undef,	"" },
	{ Imfsr,	"mfsr" },
	{ undef,	"" },
	{ Icalli,	"calli" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ Ijmpti,	"jmpti" },
	{ undef,	"" },
	{ Imtsr,	"mtsr" },
	{ undef,	"" },

	{ undef,	"" },	/* 0xd0 */
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ Ssyscall,	"syscall" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },

	{ undef,	"" },	/* 0xe0 */
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },

	{ undef,	"" },	/* 0xf0 */
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },

	{ 0 }
};

ulong
run(void)
{
	do {
		reg.ir = ifetch(reg.pc);
		Iexec(reg.ir);
		reg.pc += 4;
		if(bplist)
			brkchk(reg.pc, Instruction);
	}while(--count);
	return reg.pc;
}

void
undef(ulong inst)
{
/*
	if((reg.ir>>26) == 0)
		Bprint(bioout, "special=%d,%d table=%d\n",
		(reg.ir>>3)&0x7, reg.ir&0x7, reg.ir&0x3f);
	else
		Bprint(bioout, "code=%d,%d table=%d\n",
		reg.ir>>29, (reg.ir>>26)&0x7, reg.ir>>26);
*/
	Bprint(bioout, "Undefined Instruction Trap IR %.8lux\n", inst);
	longjmp(errjmp, 0);
}

void
Ilod(ulong inst)
{
	int rb, ra, rc;
	long a, v;

	Getrrr(rc, ra, rb, inst);
	GetIab();
	a = reg.r[rb];
	if(trace)
		itrace("load\t0,%d,r%d,r%d [#%lux]", rc, ra, rb, a);
	switch(rc) {
	default:
		Bprint(bioout, "Undefined load cntl field #%ux\n", rc);
		longjmp(errjmp, 0);
	case 0x00:	/* long */
		v = getmem_w(a);
		break;
	case 0x01:	/* uchar */
		v = getmem_b(a);
		break;
	case 0x11:	/* schar */
		v = getmem_b(a);
		if(v & 0x80)
			v |= ~0xffL;
		break;
	case 0x02:	/* ushort */
		v = getmem_h(a);
		break;
	case 0x12:	/* sshort */
		v = getmem_h(a);
		if(v & 0x8000)
			v |= ~0xffffL;
		break;
	}
	reg.r[ra] = v;
}

void
Ilodi(ulong inst)
{
	int rb, ra, rc;
	long a, v;

	Getrrr(rc, ra, rb, inst);
	GetIa();
	a = rb;
	if(trace)
		itrace("loadi\t0,%d,r%d,r%d [#%lux]", rc, ra, rb, a);
	switch(rc) {
	default:
		Bprint(bioout, "Undefined load cntl field #%ux\n", rc);
		longjmp(errjmp, 0);
	case 0x00:	/* long */
		v = getmem_w(a);
		break;
	case 0x01:	/* uchar */
		v = getmem_b(a);
		break;
	case 0x11:	/* schar */
		v = getmem_b(a);
		if(v & 0x80)
			v |= ~0xffL;
		break;
	case 0x02:	/* ushort */
		v = getmem_h(a);
		break;
	case 0x12:	/* sshort */
		v = getmem_h(a);
		if(v & 0x8000)
			v |= ~0xffffL;
		break;
	}
	reg.r[ra] = v;
}

void
Istor(ulong inst)
{
	int rb, ra, rc;
	long a, v;

	Getrrr(rc, ra, rb, inst);
	GetIab();
	a = reg.r[rb];
	v = reg.r[ra];
	if(trace)
		itrace("store\t0,%d,r%d,r%d ([#%lux]<-#%lux)", rc, ra, rb, a, v);
	switch(rc) {
	default:
		Bprint(bioout, "Undefined store cntl field #%ux\n", rc);
		longjmp(errjmp, 0);
	case 0x00:	/* long */
		putmem_w(a, v);
		break;
	case 0x01:	/* uchar */
	case 0x11:	/* schar */
		putmem_b(a, v);
		break;
	case 0x02:	/* ushort */
	case 0x12:	/* sshort */
		putmem_h(a, v);
		break;
	}
}

void
Istori(ulong inst)
{
	int rb, ra, rc;
	long a, v;

	Getrrr(rc, ra, rb, inst);
	GetIa();
	a = rb;
	v = reg.r[ra];
	if(trace)
		itrace("storei\t0,%d,r%d,r%d ([#%lux]<-#%lux)", rc, ra, rb, a, v);
	switch(rc) {
	default:
		Bprint(bioout, "Undefined store cntl field #%ux\n", rc);
		longjmp(errjmp, 0);
	case 0x00:	/* long */
		putmem_w(a, v);
		break;
	case 0x01:	/* uchar */
	case 0x11:	/* schar */
		putmem_b(a, v);
		break;
	case 0x02:	/* ushort */
	case 0x12:	/* sshort */
		putmem_h(a, v);
		break;
	}
}

void
Isetip(ulong inst)
{
	int rb, ra, rc;

	Getrrr(rc, ra, rb, inst);
	reg.s[Sipa] = ra<<2;
	reg.s[Sipb] = rb<<2;
	reg.s[Sipc] = rc<<2;
	if(trace)
		itrace("setip\tr%d,r%d,r%d", rc, ra, rb);
}

void
Imtsr(ulong inst)
{
	int rb, ra, rc;

	Getrrr(rc, ra, rb, inst);
	GetIb();
	reg.s[ra] = reg.r[rb];
	if(trace)
		itrace("mtsr\ts%d,r%d (#%lux)", ra, rb, reg.s[ra]);
	USED(rc);
}

void
Imfsr(ulong inst)
{
	int rb, ra, rc;

	Getrrr(rc, ra, rb, inst);
	GetIc();
	reg.r[rc] = reg.s[ra];
	if(trace)
		itrace("mfsr\tr%d,s%d (#%lux)", rc, ra, reg.s[ra]);
	USED(rb);
}

void
Iconstn(ulong inst)
{
	int imm, ra;

	Getir(imm, ra, inst);
	GetIa();
	reg.r[ra] = imm | (~0 << 16);
	if(trace)
		itrace("constn\tr%d,#%lux (#%lux)", ra, imm, reg.r[ra]);
}

void
Iconsth(ulong inst)
{
	int imm, ra;

	Getir(imm, ra, inst);
	GetIa();
	reg.r[ra] = (reg.r[ra] & 0xffff) | (imm << 16);
	if(trace)
		itrace("consth\tr%d,#%lux (#%lux)", ra, imm, reg.r[ra]);
}

void
Iconst(ulong inst)
{
	int imm, ra;

	Getir(imm, ra, inst);
	GetIa();
	reg.r[ra] = imm;
	if(trace)
		itrace("const\tr%d,#%lux (#%lux)", ra, imm, reg.r[ra]);
}

void
Iadd(ulong inst)
{
	int rb, ra, rc;

	Getrrr(rc, ra, rb, inst);
	GetIabc();
	reg.r[rc] = reg.r[ra] + reg.r[rb];
	if(trace)
		itrace("add\tr%d,r%d,r%d (#%lux)", rc, ra, rb, reg.r[rc]);
}

void
Iaddi(ulong inst)
{
	int rb, ra, rc;

	Getrrr(rc, ra, rb, inst);
	GetIac();
	reg.r[rc] = reg.r[ra] + rb;
	if(trace)
		itrace("addi\tr%d,r%d,#%lux (#%lux)", rc, ra, rb, reg.r[rc]);
}

void
Isub(ulong inst)
{
	int rb, ra, rc;

	Getrrr(rc, ra, rb, inst);
	GetIabc();
	reg.r[rc] = reg.r[ra] - reg.r[rb];
	if(trace)
		itrace("sub\tr%d,r%d,r%d (#%lux)", rc, ra, rb, reg.r[rc]);
}

void
Isubi(ulong inst)
{
	int rb, ra, rc;

	Getrrr(rc, ra, rb, inst);
	GetIac();
	reg.r[rc] = reg.r[ra] - rb;
	if(trace)
		itrace("subi\tr%d,r%d,#%lux (#%lux)", rc, ra, rb, reg.r[rc]);
}

void
Iisub(ulong inst)
{
	int rb, ra, rc;

	Getrrr(rc, ra, rb, inst);
	GetIabc();
	reg.r[rc] = reg.r[rb] - reg.r[ra];
	if(trace)
		itrace("isub\tr%d,r%d,r%d (#%lux)", rc, ra, rb, reg.r[rc]);
}

void
Iisubi(ulong inst)
{
	int rb, ra, rc;

	Getrrr(rc, ra, rb, inst);
	GetIac();
	reg.r[rc] = rb - reg.r[ra];
	if(trace)
		itrace("isubi\tr%d,r%d,#%lux (#%lux)", rc, ra, rb, reg.r[rc]);
}

void
Iand(ulong inst)
{
	int rb, ra, rc;

	Getrrr(rc, ra, rb, inst);
	GetIabc();
	reg.r[rc] = reg.r[ra] & reg.r[rb];
	if(trace)
		itrace("and\tr%d,r%d,r%d (#%lux)", rc, ra, rb, reg.r[rc]);
}

void
Iandi(ulong inst)
{
	int rb, ra, rc;

	Getrrr(rc, ra, rb, inst);
	GetIac();
	reg.r[rc] = reg.r[ra] & rb;
	if(trace)
		itrace("andi\tr%d,r%d,#%lux (#%lux)", rc, ra, rb, reg.r[rc]);
}

void
Ior(ulong inst)
{
	int rb, ra, rc;

	Getrrr(rc, ra, rb, inst);
	GetIabc();
	reg.r[rc] = reg.r[ra] | reg.r[rb];
	if(trace)
		itrace("or\tr%d,r%d,r%d (#%lux)", rc, ra, rb, reg.r[rc]);
}

void
Iori(ulong inst)
{
	int rb, ra, rc;

	Getrrr(rc, ra, rb, inst);
	GetIac();
	reg.r[rc] = reg.r[ra] | rb;
	if(trace)
		itrace("ori\tr%d,r%d,#%lux (#%lux)", rc, ra, rb, reg.r[rc]);
}

void
Ixor(ulong inst)
{
	int rb, ra, rc;

	Getrrr(rc, ra, rb, inst);
	GetIabc();
	reg.r[rc] = reg.r[ra] ^ reg.r[rb];
	if(trace)
		itrace("xor\tr%d,r%d,r%d (#%lux)", rc, ra, rb, reg.r[rc]);
}

void
Ixori(ulong inst)
{
	int rb, ra, rc;

	Getrrr(rc, ra, rb, inst);
	GetIac();
	reg.r[rc] = reg.r[ra] ^ rb;
	if(trace)
		itrace("xori\tr%d,r%d,#%lux (#%lux)", rc, ra, rb, reg.r[rc]);
}

void
Ixnor(ulong inst)
{
	int rb, ra, rc;

	Getrrr(rc, ra, rb, inst);
	GetIabc();
	reg.r[rc] = ~(reg.r[ra] ^ reg.r[rb]);
	if(trace)
		itrace("xnor\tr%d,r%d,r%d (#%lux)", rc, ra, rb, reg.r[rc]);
}

void
Ixnori(ulong inst)
{
	int rb, ra, rc;

	Getrrr(rc, ra, rb, inst);
	GetIac();
	reg.r[rc] = ~(reg.r[ra] ^ rb);
	if(trace)
		itrace("xnori\tr%d,r%d,#%lux (#%lux)", rc, ra, rb, reg.r[rc]);
}

void
Isll(ulong inst)
{
	int rb, ra, rc;

	Getrrr(rc, ra, rb, inst);
	GetIabc();
	reg.r[rc] = reg.r[ra] << (reg.r[rb] & 31);
	if(trace)
		itrace("sll\tr%d,r%d,r%d (#%lux)", rc, ra, rb, reg.r[rc]);
}

void
Islli(ulong inst)
{
	int rb, ra, rc;

	Getrrr(rc, ra, rb, inst);
	GetIac();
	reg.r[rc] = reg.r[ra] << (rb & 31);
	if(trace)
		itrace("slli\tr%d,r%d,#%lux (#%lux)", rc, ra, rb, reg.r[rc]);
}

void
Isrl(ulong inst)
{
	int rb, ra, rc;

	Getrrr(rc, ra, rb, inst);
	GetIabc();
	reg.r[rc] = (ulong)reg.r[ra] >> (reg.r[rb] & 31);
	if(trace)
		itrace("srl\tr%d,r%d,r%d (#%lux)", rc, ra, rb, reg.r[rc]);
}

void
Isrli(ulong inst)
{
	int rb, ra, rc;

	Getrrr(rc, ra, rb, inst);
	GetIac();
	reg.r[rc] = (ulong)reg.r[ra] >> (rb & 31);
	if(trace)
		itrace("srli\tr%d,r%d,#%lux (#%lux)", rc, ra, rb, reg.r[rc]);
}

void
Isra(ulong inst)
{
	int rb, ra, rc;

	Getrrr(rc, ra, rb, inst);
	GetIabc();
	reg.r[rc] = (long)reg.r[ra] >> (reg.r[rb] & 31);
	if(trace)
		itrace("sra\tr%d,r%d,r%d (#%lux)", rc, ra, rb, reg.r[rc]);
}

void
Israi(ulong inst)
{
	int rb, ra, rc;

	Getrrr(rc, ra, rb, inst);
	GetIac();
	reg.r[rc] = (long)reg.r[ra] >> (rb & 31);
	if(trace)
		itrace("srai\tr%d,r%d,#%lux (#%lux)", rc, ra, rb, reg.r[rc]);
}

void
Icpeq(ulong inst)
{
	int rb, ra, rc;

	Getrrr(rc, ra, rb, inst);
	GetIabc();
	if((long)reg.r[ra] == (long)reg.r[rb])
		reg.r[rc] |= True;
	else
		reg.r[rc] &= ~True;
	if(trace)
		itrace("cpeq\tr%d,r%d,r%d (#%lux)", rc, ra, rb, reg.r[rc]);
}

void
Iaseq(ulong inst)
{
	int rb, ra, rc;

	Getrrr(rc, ra, rb, inst);
	GetIabc();
	if((long)reg.r[ra] != (long)reg.r[rb])
		error("aseq failed: r%d (#%lux) != r%d (#%lux)", ra, reg.r[ra], rb, reg.r[rb]);
	if(trace)
		itrace("aseq\t%d,r%d,r%d", rc, ra, rb);
}

void
Icpeqi(ulong inst)
{
	int rb, ra, rc;

	Getrrr(rc, ra, rb, inst);
	GetIac();
	if((long)reg.r[ra] == (long)rb)
		reg.r[rc] |= True;
	else
		reg.r[rc] &= ~True;
	if(trace)
		itrace("cpeqi\tr%d,r%d,#%lux (#%lux)", rc, ra, rb, reg.r[rc]);
}

void
Iaseqi(ulong inst)
{
	int rb, ra, rc;

	Getrrr(rc, ra, rb, inst);
	GetIac();
	if((long)reg.r[ra] != (long)rb)
		error("aseqi failed: r%d (#%lux) != #%lux", ra, reg.r[ra], rb);
	if(trace)
		itrace("aseqi\t%d,r%d,#%lux", rc, ra, rb);
}

void
Icpge(ulong inst)
{
	int rb, ra, rc;

	Getrrr(rc, ra, rb, inst);
	GetIabc();
	if((long)reg.r[ra] >= (long)reg.r[rb])
		reg.r[rc] |= True;
	else
		reg.r[rc] &= ~True;
	if(trace)
		itrace("cpge\tr%d,r%d,r%d (#%lux)", rc, ra, rb, reg.r[rc]);
}

void
Icpgei(ulong inst)
{
	int rb, ra, rc;

	Getrrr(rc, ra, rb, inst);
	GetIac();
	if((long)reg.r[ra] >= (long)rb)
		reg.r[rc] |= True;
	else
		reg.r[rc] &= ~True;
	if(trace)
		itrace("cpgei\tr%d,r%d,#%lux (#%lux)", rc, ra, rb, reg.r[rc]);
}

void
Icpgeu(ulong inst)
{
	int rb, ra, rc;

	Getrrr(rc, ra, rb, inst);
	GetIabc();
	if((ulong)reg.r[ra] >= (ulong)reg.r[rb])
		reg.r[rc] |= True;
	else
		reg.r[rc] &= ~True;
	if(trace)
		itrace("cpgeu\tr%d,r%d,r%d (#%lux)", rc, ra, rb, reg.r[rc]);
}

void
Icpgeui(ulong inst)
{
	int rb, ra, rc;

	Getrrr(rc, ra, rb, inst);
	GetIac();
	if((ulong)reg.r[ra] >= (ulong)rb)
		reg.r[rc] |= True;
	else
		reg.r[rc] &= ~True;
	if(trace)
		itrace("cpgeui\tr%d,r%d,#%lux (#%lux)", rc, ra, rb, reg.r[rc]);
}

void
Icpgt(ulong inst)
{
	int rb, ra, rc;

	Getrrr(rc, ra, rb, inst);
	GetIabc();
	if((long)reg.r[ra] > (long)reg.r[rb])
		reg.r[rc] |= True;
	else
		reg.r[rc] &= ~True;
	if(trace)
		itrace("cpgt\tr%d,r%d,r%d (#%lux)", rc, ra, rb, reg.r[rc]);
}

void
Icpgti(ulong inst)
{
	int rb, ra, rc;

	Getrrr(rc, ra, rb, inst);
	GetIac();
	if((long)reg.r[ra] > (long)rb)
		reg.r[rc] |= True;
	else
		reg.r[rc] &= ~True;
	if(trace)
		itrace("cpgti\tr%d,r%d,#%lux (#%lux)", rc, ra, rb, reg.r[rc]);
}

void
Icpgtu(ulong inst)
{
	int rb, ra, rc;

	Getrrr(rc, ra, rb, inst);
	GetIabc();
	if((ulong)reg.r[ra] > (ulong)reg.r[rb])
		reg.r[rc] |= True;
	else
		reg.r[rc] &= ~True;
	if(trace)
		itrace("cpgtu\tr%d,r%d,r%d (#%lux)", rc, ra, rb, reg.r[rc]);
}

void
Icpgtui(ulong inst)
{
	int rb, ra, rc;

	Getrrr(rc, ra, rb, inst);
	GetIac();
	if((ulong)reg.r[ra] > (ulong)rb)
		reg.r[rc] |= True;
	else
		reg.r[rc] &= ~True;
	if(trace)
		itrace("cpgtui\tr%d,r%d,#%lux (#%lux)", rc, ra, rb, reg.r[rc]);
}

void
Icple(ulong inst)
{
	int rb, ra, rc;

	Getrrr(rc, ra, rb, inst);
	GetIabc();
	if((long)reg.r[ra] <= (long)reg.r[rb])
		reg.r[rc] |= True;
	else
		reg.r[rc] &= ~True;
	if(trace)
		itrace("cple\tr%d,r%d,r%d (#%lux)", rc, ra, rb, reg.r[rc]);
}

void
Icplei(ulong inst)
{
	int rb, ra, rc;

	Getrrr(rc, ra, rb, inst);
	GetIac();
	if((long)reg.r[ra] <= (long)rb)
		reg.r[rc] |= True;
	else
		reg.r[rc] &= ~True;
	if(trace)
		itrace("cplei\tr%d,r%d,#%lux (#%lux)", rc, ra, rb, reg.r[rc]);
}

void
Icpleu(ulong inst)
{
	int rb, ra, rc;

	Getrrr(rc, ra, rb, inst);
	GetIabc();
	if((ulong)reg.r[ra] <= (ulong)reg.r[rb])
		reg.r[rc] |= True;
	else
		reg.r[rc] &= ~True;
	if(trace)
		itrace("cpleu\tr%d,r%d,r%d (#%lux)", rc, ra, rb, reg.r[rc]);
}

void
Icpleui(ulong inst)
{
	int rb, ra, rc;

	Getrrr(rc, ra, rb, inst);
	GetIac();
	if((ulong)reg.r[ra] <= (ulong)rb)
		reg.r[rc] |= True;
	else
		reg.r[rc] &= ~True;
	if(trace)
		itrace("cpleui\tr%d,r%d,#%lux (#%lux)", rc, ra, rb, reg.r[rc]);
}

void
Icplt(ulong inst)
{
	int rb, ra, rc;

	Getrrr(rc, ra, rb, inst);
	GetIabc();
	if((long)reg.r[ra] < (long)reg.r[rb])
		reg.r[rc] |= True;
	else
		reg.r[rc] &= ~True;
	if(trace)
		itrace("cplt\tr%d,r%d,r%d (#%lux)", rc, ra, rb, reg.r[rc]);
}

void
Icplti(ulong inst)
{
	int rb, ra, rc;

	Getrrr(rc, ra, rb, inst);
	GetIac();
	if((long)reg.r[ra] < (long)rb)
		reg.r[rc] |= True;
	else
		reg.r[rc] &= ~True;
	if(trace)
		itrace("cplti\tr%d,r%d,#%lux (#%lux)", rc, ra, rb, reg.r[rc]);
}

void
Icpltu(ulong inst)
{
	int rb, ra, rc;

	Getrrr(rc, ra, rb, inst);
	GetIabc();
	if((ulong)reg.r[ra] < (ulong)reg.r[rb])
		reg.r[rc] |= True;
	else
		reg.r[rc] &= ~True;
	if(trace)
		itrace("cpltu\tr%d,r%d,r%d (#%lux)", rc, ra, rb, reg.r[rc]);
}

void
Icpltui(ulong inst)
{
	int rb, ra, rc;

	Getrrr(rc, ra, rb, inst);
	GetIac();
	if((ulong)reg.r[ra] < (ulong)rb)
		reg.r[rc] |= True;
	else
		reg.r[rc] &= ~True;
	if(trace)
		itrace("cpltui\tr%d,r%d,#%lux (#%lux)", rc, ra, rb, reg.r[rc]);
}

void
Icpneq(ulong inst)
{
	int rb, ra, rc;

	Getrrr(rc, ra, rb, inst);
	GetIabc();
	if((long)reg.r[ra] != (long)reg.r[rb])
		reg.r[rc] |= True;
	else
		reg.r[rc] &= ~True;
	if(trace)
		itrace("cpneq\tr%d,r%d,r%d (#%lux)", rc, ra, rb, reg.r[rc]);
}

void
Icpneqi(ulong inst)
{
	int rb, ra, rc;

	Getrrr(rc, ra, rb, inst);
	GetIac();
	if((long)reg.r[ra] != (long)rb)
		reg.r[rc] |= True;
	else
		reg.r[rc] &= ~True;
	if(trace)
		itrace("cpneqi\tr%d,r%d,#%lux (#%lux)", rc, ra, rb, reg.r[rc]);
}

static ulong
mstepu(ulong a, ulong b)
{
	ulong t1, t2;

	t1 = b;		/* sum */
	if(reg.s[Sq] & 1)
		t1 += a;
	reg.s[Sq] = (ulong)reg.s[Sq] >> 1;
	if(t1 & 1)
		reg.s[Sq] |= SIGNBIT;
	t2 = ~(a ^ b);		/* sign bits different */
	t2 &= (a ^ t1);		/* and result not same sign */
	return (t1>>1) | (t2 & SIGNBIT);
}

static ulong
mstep(ulong a, ulong b)
{
	ulong t1;

	t1 = mstepu(a, b);
	t1 ^= (t1 << 1) & SIGNBIT;
	return t1;
}

void
Imstepul(ulong inst)
{
	int rb, ra, rc;

	Getrrr(rc, ra, rb, inst);
	GetIabc();
	reg.r[rc] = mstepu(reg.r[ra], reg.r[rb]);
	if(trace)
		itrace("mstepul\tr%d,r%d,#%lux (#%lux #%lux)", rc, ra, rb, reg.r[rc], reg.s[Sq]);
}

void
Imstepuli(ulong inst)
{
	int rb, ra, rc;

	Getrrr(rc, ra, rb, inst);
	GetIac();
	reg.r[rc] = mstepu(reg.r[ra], rb);
	if(trace)
		itrace("mstepuli\tr%d,r%d,r%d (#%lux #%lux)", rc, ra, rb, reg.r[rc], reg.s[Sq]);
}

void
Imstep(ulong inst)
{
	int rb, ra, rc;

	Getrrr(rc, ra, rb, inst);
	GetIabc();
	reg.r[rc] = mstep(reg.r[ra], reg.r[rb]);
	if(trace)
		itrace("mstep\tr%d,r%d,#%lux (#%lux #%lux)", rc, ra, rb, reg.r[rc], reg.s[Sq]);
}

void
Imstepi(ulong inst)
{
	int rb, ra, rc;

	Getrrr(rc, ra, rb, inst);
	GetIac();
	reg.r[rc] = mstep(reg.r[ra], rb);
	if(trace)
		itrace("mstepi\tr%d,r%d,r%d (#%lux #%lux)", rc, ra, rb, reg.r[rc], reg.s[Sq]);
}

void
Imstepl(ulong inst)
{
	int rb, ra, rc;

	Getrrr(rc, ra, rb, inst);
	GetIabc();
	reg.r[rc] = mstep(-reg.r[ra], reg.r[rb]);
	if(trace)
		itrace("mstepl\tr%d,r%d,#%lux (#%lux #%lux)", rc, ra, rb, reg.r[rc], reg.s[Sq]);
}

void
Imstepli(ulong inst)
{
	int rb, ra, rc;

	Getrrr(rc, ra, rb, inst);
	GetIac();
	reg.r[rc] = mstep(-reg.r[ra], rb);
	if(trace)
		itrace("mstepli\tr%d,r%d,r%d (#%lux)", rc, ra, rb, reg.r[rc]);
}

void
Idstep0(ulong inst)
{
	int rb, ra, rc;

	Getrrr(rc, ra, rb, inst);
	GetIbc();
	reg.s[Salusr] |= DF;
	if(reg.r[rb] & SIGNBIT)
		reg.s[Salusr] |= N;
	else
		reg.s[Salusr] &= ~N;
	reg.r[rc] = reg.r[rb] << 1;
	if(reg.s[Sq] & SIGNBIT)
		reg.r[rc] |= 1;
	reg.s[Sq] <<= 1;
	if(trace)
		itrace("dstep0\tr%d,r%d (#%lux #%lux)", rc, rb, reg.r[rc], reg.s[Sq]);
	USED(ra);
}

void
Idstep0i(ulong inst)
{
	int rb, ra, rc;

	Getrrr(rc, ra, rb, inst);
	GetIc();
	reg.s[Salusr] |= DF;
	reg.s[Salusr] &= ~N;
	reg.r[rc] = rb << 1;
	if(reg.s[Sq] & SIGNBIT)
		reg.r[rc] |= 1;
	reg.s[Sq] <<= 1;
	if(trace)
		itrace("dstep0i\tr%d,#%lux (#%lux #%lux)", rc, rb, reg.r[rc], reg.s[Sq]);
	USED(ra);
}

static ulong
dstep(ulong a, ulong b)
{
	ulong t1, t2;

	t1 = a + b;
	t2 = (a & b) | ((a | b) & ~t1);	/* carry out */
	if(!(t2 & SIGNBIT))
		reg.s[Salusr] ^= DF;
	if(reg.s[Salusr] & N)
		reg.s[Salusr] ^= DF;
	if(t1 & SIGNBIT)
		reg.s[Salusr] |= N;
	else
		reg.s[Salusr] &= ~N;
	t1 <<= 1;
	if(reg.s[Sq] & SIGNBIT)
		t1 |= 1;
	reg.s[Sq] <<= 1;
	if(reg.s[Salusr] & DF)
		reg.s[Sq] |= 1;
	return t1;
}

void
Idstep(ulong inst)
{
	int rb, ra, rc;
	int t;

	Getrrr(rc, ra, rb, inst);
	GetIabc();
	t = reg.r[rb];
	if(reg.s[Salusr] & DF)
		t = -t;
	reg.r[rc] = dstep(reg.r[ra], t);
	if(trace)
		itrace("dstep\tr%d,r%d,r%d (#%lux #%lux)", rc, ra, rb, reg.r[rc], reg.s[Sq]);
}

void
Idstepi(ulong inst)
{
	int rb, ra, rc;

	Getrrr(rc, ra, rb, inst);
	GetIac();
	reg.r[rc] = 0;
	if(trace)
		itrace("dstepi\tr%d,r%d,#%lux (#%lux)", rc, ra, rb, reg.r[rc]);
}

void
Idstepl(ulong inst)
{
	int rb, ra, rc;
	int t;

	Getrrr(rc, ra, rb, inst);
	GetIabc();
	t = reg.r[rb];
	if(reg.s[Salusr] & DF)
		t = -t;
	dstep(reg.r[ra], t);
	reg.r[rc] = reg.r[ra] + t;
	if(trace)
		itrace("dstepl\tr%d,r%d,r%d (#%lux #%lux)", rc, ra, rb, reg.r[rc], reg.s[Sq]);
}

void
Idstepli(ulong inst)
{
	int rb, ra, rc;

	Getrrr(rc, ra, rb, inst);
	GetIac();
	reg.r[rc] = 0;
	if(trace)
		itrace("dstepli\tr%d,r%d,#%lux (#%lux)", rc, ra, rb, reg.r[rc]);
}

void
Idstepr(ulong inst)
{
	int rb, ra, rc;

	Getrrr(rc, ra, rb, inst);
	GetIabc();
	if(reg.s[Salusr] & DF)
		reg.r[rc] = reg.r[ra];
	else
		reg.r[rc] = reg.r[ra] + reg.r[rb];
	if(trace)
		itrace("dstepr\tr%d,r%d,r%d (#%lux #%lux)", rc, ra, rb, reg.r[rc], reg.s[Sq]);
}

void
Idstepri(ulong inst)
{
	int rb, ra, rc;

	Getrrr(rc, ra, rb, inst);
	GetIac();
	reg.r[rc] = 0;
	if(trace)
		itrace("dstepri\tr%d,r%d,#%lux (#%lux)", rc, ra, rb, reg.r[rc]);
}


void
Icalli(ulong inst)
{
	int rb, ra, rc;
	long npc;

	Getrrr(rc, ra, rb, inst);
	GetIab();
	if(trace)
		itrace("calli\tr%d,r%d", ra, rb);
	if(1) {
		/* Do the delay slot */
		reg.r[ra] = reg.pc+8;
		npc = reg.r[rb];
		reg.pc += 4;
		reg.ir = ifetch(reg.pc);
		Statbra();
		Iexec(reg.ir);
		reg.pc = npc - 4;
	}
	USED(ra,rc);
}

void
Icall(ulong inst)
{
	int ra;
	long imm;

	Getir(imm, ra, inst);
	GetIa();
	if(imm & 0x8000)
		imm |= ~0 << 16;
	imm = reg.pc + (imm<<2);
	if(trace)
		itrace("call\tr%d,#%lux", ra, imm);
	if(1) {
		/* Do the delay slot */
		reg.r[ra] = reg.pc+8;
		reg.pc += 4;
		reg.ir = ifetch(reg.pc);
		Statbra();
		Iexec(reg.ir);
		reg.pc = imm - 4;
	}
}

void
Icalla(ulong inst)
{
	int ra;
	long imm;

	Getir(imm, ra, inst);
	GetIa();
	imm <<= 2;
	if(trace)
		itrace("calla\tr%d,#%lux", ra, imm);
	if(1) {
		/* Do the delay slot */
		reg.r[ra] = reg.pc+8;
		reg.pc += 4;
		reg.ir = ifetch(reg.pc);
		Statbra();
		Iexec(reg.ir);
		reg.pc = imm - 4;
	}
}

void
Ijmpi(ulong inst)
{
	int rb, ra, rc;
	long npc;

	Getrrr(rc, ra, rb, inst);
	GetIb();
	if(trace)
		itrace("jmpi\tr%d", rb);
	if(1) {
		/* Do the delay slot */
		npc = reg.r[rb];
		reg.pc += 4;
		reg.ir = ifetch(reg.pc);
		Statbra();
		Iexec(reg.ir);
		reg.pc = npc - 4;
	}
	USED(ra,rc);
}

void
Ijmp(ulong inst)
{
	int ra;
	long imm;

	Getir(imm, ra, inst);
	if(imm & 0x8000)
		imm |= ~0 << 16;
	imm = reg.pc + (imm<<2);
	if(trace)
		itrace("jmp\t#%lux", imm);
	if(1) {
		/* Do the delay slot */
		reg.pc += 4;
		reg.ir = ifetch(reg.pc);
		Statbra();
		Iexec(reg.ir);
		reg.pc = imm - 4;
	}
	USED(ra);
}

void
Ijmpa(ulong inst)
{
	int ra;
	long imm;

	Getir(imm, ra, inst);
	imm <<= 2;
	if(trace)
		itrace("jmpa\t#%lux", imm);
	if(1) {
		/* Do the delay slot */
		reg.pc += 4;
		reg.ir = ifetch(reg.pc);
		Statbra();
		Iexec(reg.ir);
		reg.pc = imm - 4;
	}
	USED(ra);
}

void
Ijmpti(ulong inst)
{
	int rb, ra, rc;
	long npc;

	Getrrr(rc, ra, rb, inst);
	GetIab();
	if(trace)
		itrace("jmpi\tr%d", rb);
	if(reg.r[ra] & True) {
		/* Do the delay slot */
		npc = reg.r[rb];
		reg.pc += 4;
		reg.ir = ifetch(reg.pc);
		Statbra();
		Iexec(reg.ir);
		reg.pc = npc - 4;
	}
	USED(rc);
}

void
Ijmpt(ulong inst)
{
	int ra;
	long imm;

	Getir(imm, ra, inst);
	GetIa();
	if(imm & 0x8000)
		imm |= ~0 << 16;
	imm = reg.pc + (imm<<2);
	if(trace)
		itrace("jmpt\tr%d,#%lux", ra, imm);
	if(reg.r[ra] & True) {
		/* Do the delay slot */
		reg.pc += 4;
		reg.ir = ifetch(reg.pc);
		Statbra();
		Iexec(reg.ir);
		reg.pc = imm - 4;
	}
}

void
Ijmpta(ulong inst)
{
	int ra;
	long imm;

	Getir(imm, ra, inst);
	GetIa();
	imm <<= 2;
	if(trace)
		itrace("jmpta\tr%d,#%lux", ra, imm);
	if(reg.r[ra] & True) {
		/* Do the delay slot */
		reg.pc += 4;
		reg.ir = ifetch(reg.pc);
		Statbra();
		Iexec(reg.ir);
		reg.pc = imm - 4;
	}
}

void
Ijmpfi(ulong inst)
{
	int rb, ra, rc;
	long npc;

	Getrrr(rc, ra, rb, inst);
	GetIab();
	if(trace)
		itrace("jmpi\tr%d", rb);
	if(!(reg.r[ra] & True)) {
		/* Do the delay slot */
		npc = reg.r[rb];
		reg.pc += 4;
		reg.ir = ifetch(reg.pc);
		Statbra();
		Iexec(reg.ir);
		reg.pc = npc - 4;
	}
	USED(rc);
}

void
Ijmpf(ulong inst)
{
	int ra;
	long imm;

	Getir(imm, ra, inst);
	GetIa();
	if(imm & 0x8000)
		imm |= ~0 << 16;
	imm = reg.pc + (imm<<2);
	if(trace)
		itrace("jmpf\tr%d,#%lux", ra, imm);
	if(!(reg.r[ra] & True)) {
		/* Do the delay slot */
		reg.pc += 4;
		reg.ir = ifetch(reg.pc);
		Statbra();
		Iexec(reg.ir);
		reg.pc = imm - 4;
	}
}

void
Ijmpfa(ulong inst)
{
	int ra;
	long imm;

	Getir(imm, ra, inst);
	GetIa();
	imm <<= 2;
	if(trace)
		itrace("jmpfa\tr%d,#%lux", ra, imm);
	if(!(reg.r[ra] & True)) {
		/* Do the delay slot */
		reg.pc += 4;
		reg.ir = ifetch(reg.pc);
		Statbra();
		Iexec(reg.ir);
		reg.pc = imm - 4;
	}
}

void
Ijmpfdec(ulong inst)
{
	USED(inst);
	abort();
}

void
Ijmpfdeca(ulong inst)
{
	USED(inst);
	abort();
}

