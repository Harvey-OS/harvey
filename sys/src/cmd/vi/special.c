/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>
#define Extern extern
#include "mips.h"

void Snor(ulong);
void Ssll(ulong);
void Ssra(ulong);
void Sslt(ulong);
void Ssltu(ulong);
void Sand(ulong);
void Saddu(ulong);
void Sadd(ulong);
void Sjr(ulong);
void Sor(ulong);
void Ssubu(ulong);
void Sjalr(ulong);
void Sdivu(ulong);
void Smfhi(ulong);
void Smflo(ulong);
void Sxor(ulong);
void Smult(ulong);
void Smultu(ulong);
void Sdiv(ulong);
void Ssrl(ulong);
void Ssllv(ulong);
void Ssrlv(ulong);
void Ssrav(ulong);

Inst ispec[] =
    {
     {Ssll, "sll", Iarith},
     {undef, ""},
     {Ssrl, "srl", Iarith},
     {Ssra, "sra", Iarith},
     {Ssllv, "sllv", Iarith},
     {undef, ""},
     {Ssrlv, "srlv", Iarith},
     {Ssrav, "srav", Iarith},
     {Sjr, "jr", Ibranch},
     {Sjalr, "jalr", Ibranch},
     {undef, ""},
     {undef, ""},
     {Ssyscall, "sysc", Isyscall},
     {undef, ""},
     {undef, ""},
     {undef, ""},
     {Smfhi, "mfhi", Ireg},
     {undef, ""},
     {Smflo, "mflo", Ireg},
     {undef, ""},
     {undef, ""},
     {undef, ""},
     {undef, ""},
     {undef, ""},
     {Smult, "mult", Iarith},
     {Smultu, "multu"},
     {Sdiv, "div", Iarith},
     {Sdivu, "divu", Iarith},
     {undef, ""},
     {undef, ""},
     {undef, ""},
     {undef, ""},
     {Sadd, "add", Iarith},
     {Saddu, "addu", Iarith},
     {undef, ""},
     {Ssubu, "subu", Iarith},
     {Sand, "and", Iarith},
     {Sor, "or", Iarith},
     {Sxor, "xor", Iarith},
     {Snor, "nor", Iarith},
     {undef, ""},
     {undef, ""},
     {Sslt, "slt", Iarith},
     {Ssltu, "sltu", Iarith},
     {undef, ""},
     {undef, ""},
     {undef, ""},
     {undef, ""},
     {undef, ""},
     {undef, ""},
     {undef, ""},
     {undef, ""},
     {undef, ""},
     {undef, ""},
     {undef, ""},
     {undef, ""},
     {undef, ""},
     {undef, ""},
     {undef, ""},
     {undef, ""},
     {undef, ""},
     {undef, ""},
     {undef, ""},
     {undef, ""},
     {0}};

void
Ispecial(uint32_t inst)
{
	Inst *i;

	i = &ispec[inst & 0x3f];
	reg.ip = i;
	i->count++;
	(*i->func)(inst);
}

void
Snor(uint32_t inst)
{
	int rs, rt, rd;

	Get3(rs, rt, rd, inst);
	if(trace)
		itrace("nor\tr%d,r%d,r%d", rd, rs, rt);

	if(inst == INOP)
		nopcount++;
	else
		reg.r[rd] = ~(reg.r[rt] | reg.r[rs]);
}

void
Ssll(uint32_t inst)
{
	int rd, rt, shamt;

	SpecialGetrtrd(rt, rd, inst);
	shamt = (inst >> 6) & 0x1f;
	if(trace)
		itrace("sll\tr%d,r%d,%d", rd, rt, shamt);

	reg.r[rd] = reg.r[rt] << shamt;
}

void
Ssllv(uint32_t inst)
{
	int rd, rt, rs;

	Get3(rs, rt, rd, inst);
	if(trace)
		itrace("sllv\tr%d,r%d,r%d", rd, rt, rs);

	reg.r[rd] = reg.r[rt] << (reg.r[rs] & 0x1f);
}

void
Ssrlv(uint32_t inst)
{
	int rd, rt, rs;

	Get3(rs, rt, rd, inst);
	if(trace)
		itrace("srlv\tr%d,r%d,r%d", rd, rt, rs);

	reg.r[rd] = (uint32_t)reg.r[rt] >> (reg.r[rs] & 0x1f);
}

void
Ssrav(uint32_t inst)
{
	int rd, rt, rs, shamt;

	Get3(rs, rt, rd, inst);
	if(trace)
		itrace("srav\tr%d,r%d,r%d", rd, rt, rs);

	shamt = reg.r[rs] & 0x1f;
	if(shamt != 0 && (reg.r[rt] & SIGNBIT))
		reg.r[rd] = reg.r[rt] >> shamt | ~((1 << (32 - shamt)) - 1);
	else
		reg.r[rd] = reg.r[rt] >> shamt;
}

void
Ssrl(uint32_t inst)
{
	int rd, rt, shamt;

	SpecialGetrtrd(rt, rd, inst);
	shamt = (inst >> 6) & 0x1f;
	if(trace)
		itrace("srl\tr%d,r%d,%d", rd, rt, shamt);

	reg.r[rd] = (uint32_t)reg.r[rt] >> shamt;
}

void
Ssra(uint32_t inst)
{
	int rd, rt, shamt;

	SpecialGetrtrd(rt, rd, inst);
	shamt = (inst >> 6) & 0x1f;
	if(trace)
		itrace("sra\tr%d,r%d,%d", rd, rt, shamt);

	if(shamt != 0 && (reg.r[rt] & SIGNBIT))
		reg.r[rd] = reg.r[rt] >> shamt | ~((1 << (32 - shamt)) - 1);
	else
		reg.r[rd] = reg.r[rt] >> shamt;
}

void
Sslt(uint32_t inst)
{
	int rs, rt, rd;

	Get3(rs, rt, rd, inst);
	if(trace)
		itrace("slt\tr%d,r%d,r%d", rd, rs, rt);

	reg.r[rd] = reg.r[rs] < reg.r[rt] ? 1 : 0;
}

void
Ssltu(uint32_t inst)
{
	int rs, rt, rd;

	Get3(rs, rt, rd, inst);
	if(trace)
		itrace("sltu\tr%d,r%d,r%d", rd, rs, rt);

	reg.r[rd] = (unsigned)reg.r[rs] < (unsigned)reg.r[rt] ? 1 : 0;
}

void
Sand(uint32_t inst)
{
	int rs, rt, rd;

	Get3(rs, rt, rd, inst);
	if(trace)
		itrace("and\tr%d,r%d,r%d", rd, rs, rt);

	reg.r[rd] = reg.r[rs] & reg.r[rt];
}

void
Saddu(uint32_t inst)
{
	int rs, rt, rd;

	Get3(rs, rt, rd, inst);
	if(trace)
		itrace("addu\tr%d,r%d,r%d", rd, rs, rt);

	reg.r[rd] = reg.r[rs] + reg.r[rt];
}

void
Sadd(uint32_t inst)
{
	int rs, rt, rd;

	Get3(rs, rt, rd, inst);
	if(trace)
		itrace("add\tr%d,r%d,r%d", rd, rs, rt);

	reg.r[rd] = reg.r[rs] + reg.r[rt];
}

void
Ssubu(uint32_t inst)
{
	int rs, rt, rd;

	Get3(rs, rt, rd, inst);
	if(trace)
		itrace("subu\tr%d,r%d,r%d", rd, rs, rt);

	reg.r[rd] = reg.r[rs] - reg.r[rt];
}

void
Sor(uint32_t inst)
{
	int rs, rt, rd;

	Get3(rs, rt, rd, inst);
	if(trace)
		itrace("or\tr%d,r%d,r%d", rd, rs, rt);

	reg.r[rd] = reg.r[rs] | reg.r[rt];
}

void
Sxor(uint32_t inst)
{
	int rs, rt, rd;

	Get3(rs, rt, rd, inst);
	if(trace)
		itrace("or\tr%d,r%d,r%d", rd, rs, rt);

	reg.r[rd] = reg.r[rs] ^ reg.r[rt];
}

void
Sjr(uint32_t inst)
{
	uint32_t npc;
	int rs;
	Symbol s;

	rs = (inst >> 21) & 0x1f;
	npc = reg.r[rs];

	if(trace)
		itrace("jr\t0x%lux", npc);

	/* Do the delay slot */
	reg.ir = ifetch(reg.pc + 4);
	Statbra();
	Iexec(reg.ir);
	if(calltree) {
		if(rs == 31 || rs == 2) {
			findsym(npc, CTEXT, &s);
			Bprint(bioout, "%8lux return to %lux %s r1=%lux\n",
			       reg.pc, npc, s.name, reg.r[1]);
		}
	}
	reg.pc = npc - 4;
}

void
Sjalr(uint32_t inst)
{
	uint32_t npc;
	int rs, rd;
	Symbol s;

	rs = (inst >> 21) & 0x1f;
	rd = (inst >> 11) & 0x1f;
	npc = reg.r[rs];

	if(trace)
		itrace("jalr\tr%d,r%d", rd, rs);

	reg.r[rd] = reg.pc + 8;
	/* Do the delay slot */
	reg.ir = ifetch(reg.pc + 4);
	Statbra();
	Iexec(reg.ir);

	if(calltree) {
		findsym(npc, CTEXT, &s);
		if(rs == 31)
			Bprint(bioout, "%8lux return to %8lux %s\n",
			       reg.pc, npc, s.name);
		else {
			printparams(&s, reg.r[29]);
			Bputc(bioout, '\n');
		}
	}

	reg.pc = npc - 4;
}

void
Sdivu(uint32_t inst)
{
	int rs, rt;

	Getrsrt(rs, rt, inst);
	if(trace)
		itrace("divu\tr%d,r%d", rs, rt);

	reg.mlo = (uint32_t)reg.r[rs] / (uint32_t)reg.r[rt];
	reg.mhi = (uint32_t)reg.r[rs] % (uint32_t)reg.r[rt];
}

void
Sdiv(uint32_t inst)
{
	int rs, rt;

	Getrsrt(rs, rt, inst);
	if(trace)
		itrace("div\tr%d,r%d", rs, rt);

	reg.mlo = reg.r[rs] / reg.r[rt];
	reg.mhi = reg.r[rs] % reg.r[rt];
}

void
Smfhi(uint32_t inst)
{
	int rd;

	rd = (inst >> 11) & 0x1ff;
	if(trace)
		itrace("mfhi\tr%d", rd);

	reg.r[rd] = reg.mhi;
}

void
Smflo(uint32_t inst)
{
	int rd;

	rd = (inst >> 11) & 0x1ff;
	if(trace)
		itrace("mflo\tr%d", rd);

	reg.r[rd] = reg.mlo;
}

void
Smult(uint32_t inst)
{
	int rs, rt;
	Mul m;

	Getrsrt(rs, rt, inst);
	if(trace)
		itrace("mult\tr%d,r%d", rs, rt);

	m = mul(reg.r[rs], reg.r[rt]);
	reg.mlo = m.lo;
	reg.mhi = m.hi;
}

void
Smultu(uint32_t inst)
{
	int rs, rt;
	Mulu m;

	Getrsrt(rs, rt, inst);
	if(trace)
		itrace("multu\tr%d,r%d", rs, rt);

	m = mulu(reg.r[rs], reg.r[rt]);
	reg.mlo = m.lo;
	reg.mhi = m.hi;
}
