#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>
#define Extern extern
#include "mips.h"

void	Iaddi(ulong);
void	Isw(ulong);
void	Ilui(ulong);
void	Iori(ulong);
void	Ixori(ulong);
void	Ilw(ulong);
void	Ijal(ulong);
void	Ispecial(ulong);
void	Ibeq(ulong);
void	Ibeql(ulong);
void	Iaddiu(ulong);
void	Ilb(ulong);
void	Iandi(ulong);
void	Ij(ulong);
void	Ibne(ulong);
void	Ibnel(ulong);
void	Isb(ulong);
void	Islti(ulong);
void	Ibcond(ulong);
void	Ibgtz(ulong);
void	Ibgtzl(ulong);
void	Ilbu(ulong);
void	Ilhu(ulong);
void	Ish(ulong);
void	Ilh(ulong);
void	Iblez(ulong);
void	Iblezl(ulong);
void	Isltiu(ulong);
void	Iswc1(ulong);
void	Ilwc1(ulong);
void	Icop1(ulong);
void	Ilwl(ulong);
void	Ilwr(ulong);
void	Ill(ulong);
void	Isc(ulong);

Inst itab[] = {
	{ Ispecial,	0 },
	{ Ibcond,	"bcond",	Ibranch },
	{ Ij,		"j",		Ibranch },
	{ Ijal,		"jal",		Ibranch },
	{ Ibeq,		"beq",		Ibranch },
	{ Ibne,		"bne",		Ibranch },
	{ Iblez,	"blez",		Ibranch },
	{ Ibgtz,	"bgtz",		Ibranch },
	{ Iaddi,	"addi",		Iarith },	/* 8 */
	{ Iaddiu,	"addiu",	Iarith },
	{ Islti,	"slti",		Iarith },
	{ Isltiu,	"sltiu",	Iarith },
	{ Iandi,	"andi",		Iarith },
	{ Iori,		"ori",		Iarith },
	{ Ixori,	"xori",		Iarith },
	{ Ilui,		"lui",		Iload },	/* 15 */
	{ undef,	"" },
	{ Icop1,	"cop1",		Ifloat },
	{ undef,	"" },
	{ undef,	"" },
	{ Ibeql,	"beql" },
	{ Ibnel,	"bnel" },
	{ Iblezl,	"blezl" },
	{ Ibgtzl,	"bgtzl" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ Ilb,		"lb",		Iload },
	{ Ilh,		"lh",		Iload },
	{ Ilwl,		"lwl", 		Iload },
	{ Ilw,		"lw",		Iload },
	{ Ilbu,		"lbu",		Iload },
	{ Ilhu,		"lhu",		Iload },
	{ Ilwr,		"lwr",		Iload },
	{ undef,	"" },
	{ Isb,		"sb",		Istore },
	{ Ish,		"sh",		Istore },
	{ undef,	"" },
	{ Isw,		"sw",		Istore },	/* 43 */
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ Ill,			"ll",			Iload},
	{ Ilwc1,		"lwc1",		Ifloat },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ Isc,		"sc",		Istore },
	{ Iswc1,	"swc1",		Ifloat },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ 0 }
};

void
dortrace(void)
{
	int i;

	for(i = 0; i < 32; i++)
		if(rtrace & (1<<i))
			Bprint(bioout, "R%.2d %.8lux\n", i, reg.r[i]);
}

void
run(void)
{
	do {
		reg.r[0] = 0;
		reg.ir = ifetch(reg.pc);
		Iexec(reg.ir);
		reg.pc += 4;
		if(bplist)
			brkchk(reg.pc, Instruction);
		if(rtrace)
			dortrace();
Bflush(bioout);
	}while(--count);
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
Iaddi(ulong inst)
{
	int rs, rt;
	int imm;

	Getrsrt(rs, rt, inst);
	imm = (short)(inst&0xffff);

	if(trace)
		itrace("addi\tr%d,r%d,#0x%x", rt, rs, imm);

	reg.r[rt] = reg.r[rs] + imm;
}

void
Iandi(ulong inst)
{
	int rs, rt;
	int imm;

	Getrsrt(rs, rt, inst);
	imm = inst&0xffff;

	if(trace)
		itrace("andi\tr%d,r%d,#0x%x", rt, rs, imm);

	reg.r[rt] = reg.r[rs] & imm;
}

void
Isw(ulong inst)
{
	int rt, rb;
	int off;
	ulong v;

	Getrbrt(rb, rt, inst);
	off = (short)(inst&0xffff);

	v = reg.r[rt];
	if(trace)
		itrace("sw\tr%d,0x%x(r%d) %lux=%lux",
				rt, off, rb, reg.r[rb]+off, v);

	putmem_w(reg.r[rb]+off, v);
}

void
Isb(ulong inst)
{
	int rt, rb;
	int off;
	uchar value;

	Getrbrt(rb, rt, inst);
	off = (short)(inst&0xffff);

	value = reg.r[rt];
	if(trace)
		itrace("sb\tr%d,0x%x(r%d) %lux=%lux", rt, off, rb, reg.r[rb]+off, value);

	putmem_b(reg.r[rb]+off, value);
}

void
Ish(ulong inst)
{
	int rt, rb;
	int off;
	ushort value;

	Getrbrt(rb, rt, inst);
	off = (short)(inst&0xffff);

	value = reg.r[rt];
	if(trace)
		itrace("sh\tr%d,0x%x(r%d) %lux=%lux",
				rt, off, rb, reg.r[rb]+off, value&0xffff);

	putmem_h(reg.r[rb]+off, value);
}

void
Ilui(ulong inst)
{
	int rs, rt;
	int imm;

	Getrsrt(rs, rt, inst);
	USED(rs);
	imm = inst<<16;

	if(trace)
		itrace("lui\tr%d,#0x%x", rt, imm);

	reg.r[rt] = imm;
}

void
Iori(ulong inst)
{
	int rs, rt;
	int imm;

	Getrsrt(rs, rt, inst);
	imm = inst&0xffff;

	if(trace)
		itrace("ori\tr%d,r%d,#0x%x", rt, rs, imm);

	reg.r[rt] = reg.r[rs] | imm;
}

void
Ixori(ulong inst)
{
	int rs, rt;
	int imm;

	Getrsrt(rs, rt, inst);
	imm = inst&0xffff;

	if(trace)
		itrace("xori\tr%d,r%d,#0x%x", rt, rs, imm);

	reg.r[rt] = reg.r[rs] ^ imm;
}

void
Ilw(ulong inst)
{
	int rt, rb;
	int off;
	ulong v, va;

	Getrbrt(rb, rt, inst);
	off = (short)(inst&0xffff);

	va = reg.r[rb]+off;

	if(trace) {
		v = 0;
		if(!badvaddr(va, 4))
			v = getmem_w(va);
		itrace("lw\tr%d,0x%x(r%d) %lux=%lux", rt, off, rb, va, v);
	}

	reg.r[rt] = getmem_w(va);
}

void
Ilwl(ulong inst)
{
	int rt, rb;
	int off;
	ulong v, va;

	Getrbrt(rb, rt, inst);
	off = (short)(inst&0xffff);

	va = reg.r[rb]+off;

	if(trace) {
		v = 0;
		if(!badvaddr(va, 4))
			v = getmem_w(va & ~3) << ((va & 3) << 3);
		itrace("lwl\tr%d,0x%x(r%d) %lux=%lux", rt, off, rb, va, v);
	}

	v = getmem_w(va & ~3);
	switch(va & 3) {
	case 0:
		reg.r[rt] = v;
		break;
	case 1:
		reg.r[rt] = (v<<8) | (reg.r[rt] & 0xff);
		break;
	case 2:
		reg.r[rt] = (v<<16) | (reg.r[rt] & 0xffff);
		break;
	case 3:
		reg.r[rt] = (v<<24) | (reg.r[rt] & 0xffffff);
		break;
	}
}

void
Ilwr(ulong inst)
{
	int rt, rb;
	int off;
	ulong v, va;

	Getrbrt(rb, rt, inst);
	off = (short)(inst&0xffff);

	va = reg.r[rb]+off;

	if(trace) {
		v = 0;
		if(!badvaddr(va, 4))
			v = getmem_w(va & ~3) << ((va & 3) << 3);
		itrace("lwr\tr%d,0x%x(r%d) %lux=%lux", rt, off, rb, va, v);
	}

	v = getmem_w(va & ~3);
	switch(va & 3) {
	case 0:
		break;
	case 1:
		reg.r[rt] = (v>>24) | (reg.r[rt] & 0xffffff00);
		break;
	case 2:
		reg.r[rt] = (v>>16) | (reg.r[rt] & 0xffff0000);
		break;
	case 3:
		reg.r[rt] = (v>>8) | (reg.r[rt] & 0xff000000);
		break;
	}
}

void
Ilh(ulong inst)
{
	int rt, rb;
	int off;
	ulong v, va;

	Getrbrt(rb, rt, inst);
	off = (short)(inst&0xffff);

	va = reg.r[rb]+off;

	if(trace) {
		v = 0;
		if(!badvaddr(va, 2))
			v = (short)getmem_h(va);
		itrace("lw\tr%d,0x%x(r%d) %lux=%lux", rt, off, rb, va, v);
	}

	reg.r[rt] = (short)getmem_h(va);
}

void
Ilhu(ulong inst)
{
	int rt, rb;
	int off;
	ulong v, va;

	Getrbrt(rb, rt, inst);
	off = (short)(inst&0xffff);

	va = reg.r[rb]+off;

	if(trace) {
		v = 0;
		if(!badvaddr(va, 2))
			v = getmem_h(va) & 0xffff;
		itrace("lhu\tr%d,0x%x(r%d) %lux=%lux", rt, off, rb, va, v);
	}

	reg.r[rt] = getmem_h(va) & 0xffff;
}

void
Ilb(ulong inst)
{
	int rt, rb;
	int off;
	ulong v, va;

	Getrbrt(rb, rt, inst);
	off = (short)(inst&0xffff);

	va = reg.r[rb]+off;

	if(trace) {
		v = 0;
		if(!badvaddr(va, 1))
			v = (schar)getmem_b(va);
		itrace("lb\tr%d,0x%x(r%d) %lux=%lux", rt, off, rb, va, v);
	}

	reg.r[rt] = (schar)getmem_b(va);
}

void
Ilbu(ulong inst)
{
	int rt, rb;
	int off;
	ulong v, va;

	Getrbrt(rb, rt, inst);
	off = (short)(inst&0xffff);

	va = reg.r[rb]+off;

	if(trace) {
		v = 0;
		if(!badvaddr(va, 1))
			v = getmem_b(va) & 0xff;
		itrace("lbu\tr%d,0x%x(r%d) %lux=%lux", rt, off, rb, va, v);
	}

	reg.r[rt] = getmem_b(va) & 0xff;
}

void
Ijal(ulong inst)
{
	ulong npc;
	Symbol s;

	npc = (reg.pc&0xF0000000)|((inst&0x3FFFFFF)<<2);
	if(trace)
		itrace("jal\t0x%lux", npc);

	reg.r[31] = reg.pc+8;
	/* Do the delay slot */
	reg.ir = ifetch(reg.pc+4);
	Statbra();
	Iexec(reg.ir);

	if(calltree) {
		findsym(npc, CTEXT, &s);
		Bprint(bioout, "%8lux %s(", reg.pc, s.name);
		printparams(&s, reg.r[29]);
		Bprint(bioout, "from ");
		printsource(reg.pc);
		Bputc(bioout, '\n');
	}

	reg.pc = npc-4;
}

void
Ij(ulong inst)
{
	ulong npc;

	npc = (reg.pc&0xF0000000)|((inst&0x3FFFFFF)<<2);
	if(trace)
		itrace("j\t0x%lux", npc);

	/* Do the delay slot */
	reg.ir = ifetch(reg.pc+4);
	Statbra();
	Iexec(reg.ir);
	reg.pc = npc-4;
}

void
Ibeq(ulong inst)
{
	int rt, rs;
	int off;
	ulong npc;

	Getrsrt(rs, rt, inst);
	off = (short)(inst&0xffff);

	npc = reg.pc + (off<<2) + 4;
	if(trace)
		itrace("beq\tr%d,r%d,0x%lux", rs, rt, npc);

	if(reg.r[rs] == reg.r[rt]) {
		/* Do the delay slot */
		reg.ir = ifetch(reg.pc+4);
		Statbra();
		Iexec(reg.ir);
		reg.pc = npc-4;
	}
}

void
Ibeql(ulong inst)
{
	int rt, rs;
	int off;
	ulong npc;

	Getrsrt(rs, rt, inst);
	off = (short)(inst&0xffff);

	npc = reg.pc + (off<<2) + 4;
	if(trace)
		itrace("beq\tr%d,r%d,0x%lux", rs, rt, npc);

	if(reg.r[rs] == reg.r[rt]) {
		/* Do the delay slot */
		reg.ir = ifetch(reg.pc+4);
		Statbra();
		Iexec(reg.ir);
		reg.pc = npc-4;
	} else
		reg.pc += 4;
}

void
Ibgtz(ulong inst)
{
	int rs;
	int off;
	ulong npc, r;

	rs = (inst>>21)&0x1f;
	off = (short)(inst&0xffff);

	npc = reg.pc + (off<<2) + 4;
	if(trace)
		itrace("bgtz\tr%d,0x%lux", rs, npc);

	r = reg.r[rs];
	if(!(r&SIGNBIT) && r != 0) {
		/* Do the delay slot */
		reg.ir = ifetch(reg.pc+4);
		Iexec(reg.ir);
		reg.pc = npc-4;
	}
}

void
Ibgtzl(ulong inst)
{
	int rs;
	int off;
	ulong npc, r;

	rs = (inst>>21)&0x1f;
	off = (short)(inst&0xffff);

	npc = reg.pc + (off<<2) + 4;
	if(trace)
		itrace("bgtz\tr%d,0x%lux", rs, npc);

	r = reg.r[rs];
	if(!(r&SIGNBIT) && r != 0) {
		/* Do the delay slot */
		reg.ir = ifetch(reg.pc+4);
		Iexec(reg.ir);
		reg.pc = npc-4;
	} else
		reg.pc += 4;
}

void
Iblez(ulong inst)
{
	int rs;
	int off;
	ulong npc, r;

	rs = (inst>>21)&0x1f;
	off = (short)(inst&0xffff);

	npc = reg.pc + (off<<2) + 4;
	if(trace)
		itrace("blez\tr%d,0x%lux", rs, npc);

	r = reg.r[rs];
	if((r&SIGNBIT) || r == 0) {
		/* Do the delay slot */
		reg.ir = ifetch(reg.pc+4);
		Statbra();
		Iexec(reg.ir);
		reg.pc = npc-4;
	}
}

void
Iblezl(ulong inst)
{
	int rs;
	int off;
	ulong npc, r;

	rs = (inst>>21)&0x1f;
	off = (short)(inst&0xffff);

	npc = reg.pc + (off<<2) + 4;
	if(trace)
		itrace("blez\tr%d,0x%lux", rs, npc);

	r = reg.r[rs];
	if((r&SIGNBIT) || r == 0) {
		/* Do the delay slot */
		reg.ir = ifetch(reg.pc+4);
		Statbra();
		Iexec(reg.ir);
		reg.pc = npc-4;
	} else
		reg.pc += 4;
}

void
Ibne(ulong inst)
{
	int rt, rs;
	int off;
	ulong npc;

	Getrsrt(rs, rt, inst);
	off = (short)(inst&0xffff);

	npc = reg.pc + (off<<2) + 4;
	if(trace)
		itrace("bne\tr%d,r%d,0x%lux", rs, rt, npc);

	if(reg.r[rs] != reg.r[rt]) {
		/* Do the delay slot */
		reg.ir = ifetch(reg.pc+4);
		Statbra();
		Iexec(reg.ir);
		reg.pc = npc-4;
	}
}

void
Ibnel(ulong inst)
{
	int rt, rs;
	int off;
	ulong npc;

	Getrsrt(rs, rt, inst);
	off = (short)(inst&0xffff);

	npc = reg.pc + (off<<2) + 4;
	if(trace)
		itrace("bne\tr%d,r%d,0x%lux", rs, rt, npc);

	if(reg.r[rs] != reg.r[rt]) {
		/* Do the delay slot */
		reg.ir = ifetch(reg.pc+4);
		Statbra();
		Iexec(reg.ir);
		reg.pc = npc-4;
	} else
		reg.pc += 4;
}

void
Iaddiu(ulong inst)
{
	int rs, rt;
	int imm;

	Getrsrt(rs, rt, inst);
	imm = (short)(inst&0xffff);

	if(trace)
		itrace("addiu\tr%d,r%d,#0x%x", rt, rs, imm);

	reg.r[rt] = reg.r[rs]+imm;
}

void
Islti(ulong inst)
{
	int rs, rt;
	int imm;

	Getrsrt(rs, rt, inst);
	imm = (short)(inst&0xffff);

	if(trace)
		itrace("slti\tr%d,r%d,#0x%x", rt, rs, imm);

	reg.r[rt] = reg.r[rs] < imm ? 1 : 0;
}

void
Isltiu(ulong inst)
{
	int rs, rt;
	int imm;

	Getrsrt(rs, rt, inst);
	imm = (short)(inst&0xffff);

	if(trace)
		itrace("sltiu\tr%d,r%d,#0x%x", rt, rs, imm);

	reg.r[rt] = (ulong)reg.r[rs] < (ulong)imm ? 1 : 0;
}

/* ll and sc are implemented as lw and sw, since we simulate a uniprocessor */

void
Ill(ulong inst)
{
	int rt, rb;
	int off;
	ulong v, va;

	Getrbrt(rb, rt, inst);
	off = (short)(inst&0xffff);

	va = reg.r[rb]+off;

	if(trace) {
		v = 0;
		if(!badvaddr(va, 4))
			v = getmem_w(va);
		itrace("ll\tr%d,0x%x(r%d) %lux=%lux", rt, off, rb, va, v);
	}

	reg.r[rt] = getmem_w(va);
}

void
Isc(ulong inst)
{
	int rt, rb;
	int off;
	ulong v;

	Getrbrt(rb, rt, inst);
	off = (short)(inst&0xffff);

	v = reg.r[rt];
	if(trace)
		itrace("sc\tr%d,0x%x(r%d) %lux=%lux",
				rt, off, rb, reg.r[rb]+off, v);

	putmem_w(reg.r[rb]+off, v);
}

enum
{
	Bltz	= 0,
	Bgez	= 1,
	Bltzal	= 0x10,
	Bgezal	= 0x11,
	Bltzl	= 2,
	Bgezl	= 3,
	Bltzall	= 0x12,
	Bgezall	= 0x13,
};

static char *sbcond[] =
{
	[Bltz]		"ltz",
	[Bgez]		"gez",
	[Bltzal]	"ltzal",
	[Bgezal]	"gezal",
	[Bltzl]		"ltzl",
	[Bgezl]		"gezl",
	[Bltzall]	"ltzall",
	[Bgezall]	"gezall",
};

void
Ibcond(ulong inst)
{
	int rs, bran;
	int off, doit, likely;
	ulong npc;

	rs = (inst>>21)&0x1f;
	bran = (inst>>16)&0x1f;
	off = (short)(inst&0xffff);
	doit = 0;
	likely = 0;

	npc = reg.pc + (off<<2) + 4;
	switch(bran) {
	default:
		Bprint(bioout, "bcond=%d\n", bran);
		undef(inst);
	case Bltzl:
		likely = 1;
	case Bltz:
		if(reg.r[rs]&SIGNBIT)
			doit = 1;
		break;
	case Bgezl:
		likely = 1;
	case Bgez:
		if(!(reg.r[rs]&SIGNBIT))
			doit = 1;
		break;
	case Bltzall:
		likely = 1;
	case Bltzal:
		reg.r[31] = reg.pc+8;
		if(reg.r[rs]&SIGNBIT)
			doit = 1;
		break;
	case Bgezall:
		likely = 1;
	case Bgezal:
		reg.r[31] = reg.pc+8;
		if(!(reg.r[rs]&SIGNBIT))
			doit = 1;
		break;
	}

	if(trace)
		itrace("b%s\tr%d,0x%lux", sbcond[bran], rs, npc);

	if(doit) {
		/* Do the delay slot */
		reg.ir = ifetch(reg.pc+4);
		Statbra();
		Iexec(reg.ir);
		reg.pc = npc-4;
	} else
	if(likely)
		reg.pc += 4;

}
