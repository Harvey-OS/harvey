#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>
#define Extern extern
#include "sparc.h"

void add(ulong);
void and(ulong);
void or(ulong);
void xor(ulong);
void sub(ulong);
void andn(ulong);
void xnor(ulong);
void subcc(ulong);
void sll(ulong);
void srl(ulong);
void sra(ulong);
void jmpl(ulong);
void andcc(ulong);
void xorcc(ulong);
void andncc(ulong);
void wry(ulong);
void rdy(ulong);
void mulscc(ulong);
void fcmp(ulong);
void farith(ulong);
void addcc(ulong);
void addx(ulong);
void addxcc(ulong);
void orcc(ulong);
void orncc(ulong);
void xnorcc(ulong);
void orn(ulong);

Inst op2[] = {
	{ add,		"add",	Iarith },
	{ and,		"and",	Iarith },
	{ or,		"or",	Iarith },
	{ xor,		"xor",	Iarith },
	{ sub,		"sub",	Iarith },
	{ andn,		"andn", Iarith },
	{ orn,		"orn",	Inop },
	{ xnor,		"xnor", Iarith },
	{ addx,		"addx", Iarith },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ addcc, 	"addcc", Iarith },
	{ andcc, 	"andcc", Iarith },
	{ orcc,	 	"orcc",  Iarith },
	{ xorcc, 	"xorcc", Iarith },
	{ subcc, 	"subcc", Iarith },
	{ andncc,	"andncc",Iarith },
	{ orncc, 	"orncc", Iarith },
	{ xnorcc,	"xnorcc",Iarith },
	{ addxcc,	"addxcc",Iarith },
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
	{ mulscc,	"mulscc", Iarith },
	{ sll,		"sll",	Iarith },
	{ srl,		"srl",	Iarith },
	{ sra,		"sra",	Iarith },
	{ rdy,		"rdy",	Ireg },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ wry,		"wry",	Ireg },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ farith,	"farith", Ifloat },
	{ fcmp,		"fcmp", Ifloat },
	{ undef,	"" },
	{ undef,	"" },
	{ jmpl,		"jmpl", Ibranch },
	{ undef,	"" },
	{ ta,		"ta",	Isyscall },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ undef,	"" },
	{ 0 }
};

void st(ulong);
void stb(ulong);
void sth(ulong);
void ld(ulong);
void ldub(ulong);
void ldsb(ulong);
void lduh(ulong);
void stf(ulong);
void ldf(ulong);
void ldsh(ulong);
void std(ulong);
void ldd(ulong);
void ldstub(ulong);
void swap(ulong);
void lddf(ulong);
void stdf(ulong);

Inst op3[] = {
	{ ld,		"ld",	Iload },
	{ ldub,		"ldub",	Iload },
	{ lduh,		"lduh",	Iload },
	{ ldd,		"ldd",	Iload },
	{ st,		"st",	Istore },
	{ stb,		"stb",	Istore },
	{ sth,		"sth",	Istore },
	{ std,		"std",	Istore },
	{ undef,	"" },
	{ ldsb,		"ldsb",	Iload },
	{ ldsh,		"ldsh", Iload },
	{ undef,	"" },
	{ undef,	"" },
	{ ldstub,	"ldstub", Iload },
	{ undef,	"" },
	{ swap,		"swap",	Iload },
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
	{ undef,	"" },
	{ ldf,		"ldf",	Ifloat },
	{ undef,	"" },
	{ undef,	"" },
	{ lddf,		"lddf", Ifloat },
	{ stf,		"stf",	Ifloat },
	{ undef,	"" },
	{ undef,	"" },
	{ stdf,		"stdf",	Ifloat },
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

void sethi(ulong);
void bicc(ulong);
void fbcc(ulong);
void call(ulong);

Inst op0[] = {
	{ undef,	"" },
	{ undef,	"" },
	{ bicc,		"bicc",	Ibranch },
	{ undef,	"" },
	{ sethi,	"sethi",Iarith },
	{ undef,	"" },
	{ fbcc,		"fbcc",	Ibranch },
	{ undef,	"" },
	/* This is a fake and connot be reached by op0 decode */
	{ call,		"call", Ibranch },
	{ 0 }
};

void call(ulong);

void
run(void)
{
	do {
		reg.r[0] = 0;
		reg.ir = ifetch(reg.pc);
		switch(reg.ir>>30) {
		case 0:
			ci = &op0[(reg.ir>>22)&0x07];
			ci->count++;
			(*ci->func)(reg.ir);
			break;
		case 1:
			ci = &op0[8];
			ci->count++;
			call(reg.ir);
			break;
		case 2:
			ci = &op2[(reg.ir>>19)&0x3f];
			ci->count++;
			(*ci->func)(reg.ir);
			break;
		case 3:
			ci = &op3[(reg.ir>>19)&0x3f];
			ci->count++;
			(*ci->func)(reg.ir);
			break;
		}
		reg.pc += 4;
		if(bplist)
			brkchk(reg.pc, Instruction);
	}while(--count);
}

void
ilock(int rd)
{
	ulong ir;

	ir = getmem_4(reg.pc+4);
	switch(ir>>30) {
	case 0:
	case 1:
		break;
	case 2:
		if(((ir>>20)&0x1f) == 0x1a)		/* floating point */
			break;
	case 3:
		if(rd == ((ir>>14)&0x1f)) {
			loadlock++;
			break;
		}
		if(ir&IMMBIT)
			break;
		if(rd == (ir&0x1f))
			loadlock++;
		break;
	}
}

void
delay(ulong npc)
{
	ulong opc;

	reg.r[0] = 0;
	if(reg.ir != NOP)
		ci->useddelay++;
	switch(reg.ir>>30) {
	case 0:
		ci = &op0[(reg.ir>>22)&0x07];
		ci->count++;
		(*ci->func)(reg.ir);
		break;
	case 1:
		ci = &op0[8];
		ci->count++;
		call(reg.ir);
		break;
	case 2:
		ci = &op2[(reg.ir>>19)&0x3f];
		ci->count++;
		opc = reg.pc;
		reg.pc = npc-4;
		(*ci->func)(reg.ir);
		reg.pc = opc;
		break;
	case 3:
		ci = &op3[(reg.ir>>19)&0x3f];
		ci->count++;
		opc = reg.pc;
		reg.pc = npc-4;
		(*ci->func)(reg.ir);
		reg.pc = opc;
		break;
	}
}

void
undef(ulong ir)
{
/*	Bprint(bioout, "op=%d op2=%d op3=%d\n", ir>>30, (ir>>21)&0x7, (ir>>19)&0x3f); */
	Bprint(bioout, "illegal_instruction IR #%.8lux\n", ir);
	longjmp(errjmp, 0);
}

void
sub(ulong ir)
{
	long v;
	int rd, rs1, rs2;

	getrop23(ir);
	if(ir&IMMBIT) {
		ximm(v, ir);
		if(trace)
			itrace("sub\tr%d,#0x%x,r%d", rs1, v, rd);
	}
	else {
		v = reg.r[rs2];
		if(trace)
			itrace("sub\tr%d,r%d,r%d", rs1, rs2, rd);
	}
	reg.r[rd] = reg.r[rs1] - v;
}

void
sll(ulong ir)
{
	long v;
	int rd, rs1, rs2;

	getrop23(ir);
	if(ir&IMMBIT) {
		ximm(v, ir);
		if(trace)
			itrace("sll\tr%d,#0x%x,r%d", rs1, v, rd);
	}
	else {
		v = reg.r[rs2]&0x1F;
		if(trace)
			itrace("sll\tr%d,r%d,r%d", rs1, rs2, rd);
	}
	reg.r[rd] = reg.r[rs1] << v;
}

void
srl(ulong ir)
{
	long v;
	int rd, rs1, rs2;

	getrop23(ir);
	if(ir&IMMBIT) {
		ximm(v, ir);
		if(trace)
			itrace("srl\tr%d,#0x%x,r%d", rs1, v, rd);
	}
	else {
		v = reg.r[rs2];
		if(trace)
			itrace("srl\tr%d,r%d,r%d", rs1, rs2, rd);
	}
	reg.r[rd] = (ulong)reg.r[rs1] >> v;
}

void
sra(ulong ir)
{
	long v;
	int rd, rs1, rs2;

	getrop23(ir);
	if(ir&IMMBIT) {
		ximm(v, ir);
		if(trace)
			itrace("sra\tr%d,#0x%x,r%d", rs1, v, rd);
	}
	else {
		v = reg.r[rs2];
		if(trace)
			itrace("sra\tr%d,r%d,r%d", rs1, rs2, rd);
	}
	if(reg.r[rs1]&SIGNBIT)
		reg.r[rd] = reg.r[rs1]>>v | ~((1<<(32-v))-1);
	else
		reg.r[rd] = reg.r[rs1]>>v;
}

void
subcc(ulong ir)
{
	long v;
	int b31rs1, b31op2, b31res, r, rd, rs1, rs2;

	getrop23(ir);
	if(ir&IMMBIT) {
		ximm(v, ir);
		if(trace)
			itrace("subcc\tr%d,#0x%x,r%d", rs1, v, rd);
	}
	else {
		v = reg.r[rs2];
		if(trace)
			itrace("subcc\tr%d,r%d,r%d", rs1, rs2, rd);
	}
	r = reg.r[rs1] - v;
	reg.psr &= ~(PSR_z|PSR_n|PSR_c|PSR_v);
	if(r == 0)
		reg.psr |= PSR_z;
	if(r < 0)
		reg.psr |= PSR_n;

	b31rs1 = reg.r[rs1]>>31;
	b31op2 = v>>31;
	b31res = r>>31;

	if((b31rs1 & ~b31op2 & ~b31res)|(~b31rs1 & b31op2 & b31res))
		reg.psr |= PSR_v;

	if((~b31rs1 & b31op2)|(b31res & (~b31rs1|b31op2)))
		reg.psr |= PSR_c;

	reg.r[rd] = r;

}

void
add(ulong ir)
{
	long v;
	int rd, rs1, rs2;

	getrop23(ir);
	if(ir&IMMBIT) {
		ximm(v, ir);
		if(trace)
			itrace("add\tr%d,#0x%x,r%d", rs1, v, rd);
	}
	else {
		v = reg.r[rs2];
		if(trace)
			itrace("add\tr%d,r%d,r%d", rs1, rs2, rd);
	}
	reg.r[rd] = reg.r[rs1] + v;
}

void
addcc(ulong ir)
{
	long v, r;
	int rd, rs1, rs2, b31rs1, b31op2, b31r;

	getrop23(ir);
	if(ir&IMMBIT) {
		ximm(v, ir);
		if(trace)
			itrace("addcc\tr%d,#0x%x,r%d", rs1, v, rd);
	}
	else {
		v = reg.r[rs2];
		if(trace)
			itrace("addcc\tr%d,r%d,r%d", rs1, rs2, rd);
	}
	r = reg.r[rs1] + v;
	reg.psr &= ~(PSR_z|PSR_n|PSR_c|PSR_v);
	if(r == 0)
		reg.psr |= PSR_z;
	if(r < 0)
		reg.psr |= PSR_n;

	b31rs1 = reg.r[rs1]>>31;
	b31op2 = v>>31;
	b31r = r>>31;
	if((b31rs1 & b31op2 & ~b31r)|(~b31rs1 & ~b31op2 & b31r))
		reg.psr |= PSR_v;
	if((b31rs1 & b31op2) | (~b31r & (b31rs1 | b31op2)))
		reg.psr |= PSR_c;

	reg.r[rd] = r;
}

void
addx(ulong ir)
{
	long v;
	int rd, rs1, rs2;

	getrop23(ir);
	if(ir&IMMBIT) {
		ximm(v, ir);
		if(trace)
			itrace("addx\tr%d,#0x%x,r%d", rs1, v, rd);
	}
	else {
		v = reg.r[rs2];
		if(trace)
			itrace("addx\tr%d,r%d,r%d", rs1, rs2, rd);
	}
	if(reg.psr&PSR_c)
		v++;
	reg.r[rd] = reg.r[rs1] + v;
}

void
addxcc(ulong ir)
{
	long r, v;
	int rd, rs1, rs2, b31rs1, b31op2, b31r;

	getrop23(ir);
	if(ir&IMMBIT) {
		ximm(v, ir);
		if(trace)
			itrace("addxcc\tr%d,#0x%x,r%d", rs1, v, rd);
	}
	else {
		v = reg.r[rs2];
		if(trace)
			itrace("addxcc\tr%d,r%d,r%d", rs1, rs2, rd);
	}
	if(reg.psr&PSR_c)
		v++;

	r = reg.r[rs1] + v;
	reg.psr &= ~(PSR_z|PSR_n|PSR_c|PSR_v);
	if(r == 0)
		reg.psr |= PSR_z;
	if(r < 0)
		reg.psr |= PSR_n;

	b31rs1 = reg.r[rs1]>>31;
	b31op2 = v>>31;
	b31r = r>>31;
	if((b31rs1 & b31op2 & ~b31r)|(~b31rs1 & ~b31op2 & b31r))
		reg.psr |= PSR_v;
	if((b31rs1 & b31op2) | (~b31r & (b31rs1 | b31op2)))
		reg.psr |= PSR_c;

	reg.r[rd] = r;
}

void
wry(ulong ir)
{
	long v;
	int rd, rs1, rs2;

	getrop23(ir);
	if(rd != 0)
		undef(ir);
	if(ir&IMMBIT) {
		ximm(v, ir);
		if(trace)
			itrace("wry\tr%d,#0x%x,Y", rs1, v);
	}
	else {
		v = reg.r[rs2];
		if(trace)
			itrace("wry\tr%d,r%d,Y", rs1, rs2);
	}
	reg.Y = reg.r[rs1] + v;
}

void
rdy(ulong ir)
{
	int rd, rs1, rs2;

	getrop23(ir);
	USED(rs2);
	if(rs1 != 0)
		undef(ir);
	
	if(trace)
		itrace("rdy\tY,r%d", rd);

	reg.r[rd] = reg.Y;
}

void
and(ulong ir)
{
	long v;
	int rd, rs1, rs2;

	getrop23(ir);
	if(ir&IMMBIT) {
		ximm(v, ir);
		if(trace)
			itrace("and\tr%d,#0x%x,r%d", rs1, v, rd);
	}
	else {
		v = reg.r[rs2];
		if(trace)
			itrace("and\tr%d,r%d,r%d", rs1, rs2, rd);
	}
	reg.r[rd] = reg.r[rs1] & v;
}

void
andcc(ulong ir)
{
	long v, r;
	int rd, rs1, rs2;

	getrop23(ir);
	if(ir&IMMBIT) {
		ximm(v, ir);
		if(trace)
			itrace("andcc\tr%d,#0x%x,r%d", rs1, v, rd);
	}
	else {
		v = reg.r[rs2];
		if(trace)
			itrace("andcc\tr%d,r%d,r%d", rs1, rs2, rd);
	}
	r = reg.r[rs1] & v;
	reg.psr &= ~(PSR_z|PSR_n|PSR_c|PSR_v);
	if(r == 0)
		reg.psr |= PSR_z;
	if(r < 0)
		reg.psr |= PSR_n;

	reg.r[rd] = r;
}

void
orcc(ulong ir)
{
	long v, r;
	int rd, rs1, rs2;

	getrop23(ir);
	if(ir&IMMBIT) {
		ximm(v, ir);
		if(trace)
			itrace("orcc\tr%d,#0x%x,r%d", rs1, v, rd);
	}
	else {
		v = reg.r[rs2];
		if(trace)
			itrace("orcc\tr%d,r%d,r%d", rs1, rs2, rd);
	}
	r = reg.r[rs1] | v;
	reg.psr &= ~(PSR_z|PSR_n|PSR_c|PSR_v);
	if(r == 0)
		reg.psr |= PSR_z;
	if(r < 0)
		reg.psr |= PSR_n;

	reg.r[rd] = r;
}

void
mulscc(ulong ir)
{
	int b, n, v, rd, rs1, rs2;
	long o1, o2, r, b31o1, b31o2, b31r;

	getrop23(ir);
	if(ir&IMMBIT) {
		ximm(o2, ir);
		if(trace)
			itrace("mulscc\tr%d,#0x%x,r%d", rs1, o2, rd);
	}
	else {
		o2 = reg.r[rs2];
		if(trace)
			itrace("mulscc\tr%d,r%d,r%d", rs1, rs2, rd);
	}
	o1 = reg.r[rs1]>>1;
	n = reg.psr&PSR_n ? 1 : 0;
	v = reg.psr&PSR_v ? 1 : 0;

	o1 |= (n ^ v)<<31;
	if((reg.Y&1) == 0)
		o2 = 0;

	r = o1 + o2;
	reg.psr &= ~(PSR_z|PSR_n|PSR_c|PSR_v);
	if(r == 0)
		reg.psr |= PSR_z;
	if(r < 0)
		reg.psr |= PSR_n;

	b31o1 = o1>>31;
	b31o2 = o2>>31;
	b31r = r>>31;
	if((b31o1 & b31o2 & ~b31r) | (~b31o1 & ~b31o2 & b31r))
		reg.psr |= PSR_v;
	if((b31o1 & b31o2) | (~b31r & (b31o1 | b31o2)))		
		reg.psr |= PSR_c;

	b = reg.r[rs1]&1;
	reg.Y = (reg.Y>>1)|(b<<31);
	reg.r[rd] = r;
}

void
or(ulong ir)
{
	long v;
	int rd, rs1, rs2;

	getrop23(ir);
	if(ir&IMMBIT) {
		ximm(v, ir);
		if(trace)
			itrace("or\tr%d,#0x%x,r%d", rs1, v, rd);
	}
	else {
		v = reg.r[rs2];
		if(trace)
			itrace("or\tr%d,r%d,r%d", rs1, rs2, rd);
	}
	reg.r[rd] = reg.r[rs1] | v;
}

void
xor(ulong ir)
{
	long v;
	int rd, rs1, rs2;

	getrop23(ir);
	if(ir&IMMBIT) {
		ximm(v, ir);
		if(trace)
			itrace("xor\tr%d,#0x%x,r%d", rs1, v, rd);
	}
	else {
		v = reg.r[rs2];
		if(trace)
			itrace("xor\tr%d,r%d,r%d", rs1, rs2, rd);
	}
	reg.r[rd] = reg.r[rs1] ^ v;
}

void
xorcc(ulong ir)
{
	long v, r;
	int rd, rs1, rs2;

	getrop23(ir);
	if(ir&IMMBIT) {
		ximm(v, ir);
		if(trace)
			itrace("xorcc\tr%d,#0x%x,r%d", rs1, v, rd);
	}
	else {
		v = reg.r[rs2];
		if(trace)
			itrace("xorcc\tr%d,r%d,r%d", rs1, rs2, rd);
	}
	r = reg.r[rs1] ^ v;
	reg.psr &= ~(PSR_z|PSR_n|PSR_c|PSR_v);
	if(r == 0)
		reg.psr |= PSR_z;
	if(r < 0)
		reg.psr |= PSR_n;

	reg.r[rd] = r;
}

void
andn(ulong ir)
{
	long v;
	int rd, rs1, rs2;

	getrop23(ir);
	if(ir&IMMBIT) {
		ximm(v, ir);
		if(trace)
			itrace("andn\tr%d,#0x%x,r%d", rs1, v, rd);
	}
	else {
		v = reg.r[rs2];
		if(trace)
			itrace("andn\tr%d,r%d,r%d", rs1, rs2, rd);
	}
	reg.r[rd] = reg.r[rs1] & ~v;
}

void
andncc(ulong ir)
{
	long v, r;
	int rd, rs1, rs2;

	getrop23(ir);
	if(ir&IMMBIT) {
		ximm(v, ir);
		if(trace)
			itrace("andncc\tr%d,#0x%x,r%d", rs1, v, rd);
	}
	else {
		v = reg.r[rs2];
		if(trace)
			itrace("andncc\tr%d,r%d,r%d", rs1, rs2, rd);
	}
	r = reg.r[rs1] & ~v;
	reg.psr &= ~(PSR_z|PSR_n|PSR_c|PSR_v);
	if(r == 0)
		reg.psr |= PSR_z;
	if(r < 0)
		reg.psr |= PSR_n;

	reg.r[rd] = r;
}

void
orn(ulong ir)
{
	long v;
	int rd, rs1, rs2;

	getrop23(ir);
	if(rd == 0 && rs1 == 0)	/* ken used orn r0,r0,r0 as nop */
		nopcount++;

	if(ir&IMMBIT) {
		ximm(v, ir);
		if(trace)
			itrace("orn\tr%d,#0x%x,r%d", rs1, v, rd);
	}
	else {
		v = reg.r[rs2];
		if(trace)
			itrace("orn\tr%d,r%d,r%d", rs1, rs2, rd);
	}
	reg.r[rd] = reg.r[rs1] | ~v;
}

void
orncc(ulong ir)
{
	long r, v;
	int rd, rs1, rs2;

	getrop23(ir);
	if(ir&IMMBIT) {
		ximm(v, ir);
		if(trace)
			itrace("orncc\tr%d,#0x%x,r%d", rs1, v, rd);
	}
	else {
		v = reg.r[rs2];
		if(trace)
			itrace("orncc\tr%d,r%d,r%d", rs1, rs2, rd);
	}
	r = reg.r[rs1] | ~v;
	reg.psr &= ~(PSR_z|PSR_n|PSR_c|PSR_v);
	if(r == 0)
		reg.psr |= PSR_z;
	if(r < 0)
		reg.psr |= PSR_n;

	reg.r[rd] = r;
}

void
xnor(ulong ir)
{
	long v;
	int rd, rs1, rs2;

	getrop23(ir);
	if(ir&IMMBIT) {
		ximm(v, ir);
		if(trace)
			itrace("xnor\tr%d,#0x%x,r%d", rs1, v, rd);
	}
	else {
		v = reg.r[rs2];
		if(trace)
			itrace("xnor\tr%d,r%d,r%d", rs1, rs2, rd);
	}
	reg.r[rd] = reg.r[rs1] ^ ~v;
}

void
xnorcc(ulong ir)
{
	long v, r;
	int rd, rs1, rs2;

	getrop23(ir);
	if(ir&IMMBIT) {
		ximm(v, ir);
		if(trace)
			itrace("xnorcc\tr%d,#0x%x,r%d", rs1, v, rd);
	}
	else {
		v = reg.r[rs2];
		if(trace)
			itrace("xnorcc\tr%d,r%d,r%d", rs1, rs2, rd);
	}
	r = reg.r[rs1] ^ ~v;
	reg.psr &= ~(PSR_z|PSR_n|PSR_c|PSR_v);
	if(r == 0)
		reg.psr |= PSR_z;
	if(r < 0)
		reg.psr |= PSR_n;

	reg.r[rd] = r;
}

void
st(ulong ir)
{
	ulong ea;
	int rd, rs1, rs2;

	getrop23(ir);
	if(ir&IMMBIT) {
		ximm(ea, ir);
		if(trace)
			itrace("st\tr%d,0x%lux(r%d) %lux=%lux",
					rd, ea, rs1, ea+reg.r[rs1], reg.r[rd]);
		ea += reg.r[rs1];
	}
	else {
		ea = reg.r[rs1] + reg.r[rs2];
		if(trace)
			itrace("st\tr%d,[r%d+r%d] %lux=%lux",
					rd, rs1, rs2, ea, reg.r[rd]);
	}

	putmem_w(ea, reg.r[rd]);
}

void
std(ulong ir)
{
	ulong ea;
	int rd, rs1, rs2;

	getrop23(ir);
	if(ir&IMMBIT) {
		ximm(ea, ir);
		if(trace)
			itrace("std\tr%d,0x%lux(r%d) %lux=%lux",
					rd, ea, rs1, ea+reg.r[rs1], reg.r[rd]);
		ea += reg.r[rs1];
	}
	else {
		ea = reg.r[rs1] + reg.r[rs2];
		if(trace)
			itrace("std\tr%d,[r%d+r%d] %lux=%lux",
					rd, rs1, rs2, ea, reg.r[rd]);
	}

	putmem_w(ea, reg.r[rd]);
	putmem_w(ea+4, reg.r[rd+1]);
}

void
stb(ulong ir)
{
	ulong ea;
	int rd, rs1, rs2;

	getrop23(ir);
	if(ir&IMMBIT) {
		ximm(ea, ir);
		if(trace)
			itrace("stb\tr%d,0x%lux(r%d) %lux=%lux",
					rd, ea, rs1, ea+reg.r[rs1], reg.r[rd]&0xff);
		ea += reg.r[rs1];
	}
	else {
		ea = reg.r[rs1] + reg.r[rs2];
		if(trace)
			itrace("stb\tr%d,[r%d+r%d] %lux=%lux",
					rd, rs1, rs2, ea, reg.r[rd]&0xff);
	}

	putmem_b(ea, reg.r[rd]);
}

void
sth(ulong ir)
{
	ulong ea;
	int rd, rs1, rs2;

	getrop23(ir);
	if(ir&IMMBIT) {
		ximm(ea, ir);
		if(trace)
			itrace("sth\tr%d,0x%lux(r%d) %lux=%lux",
					rd, ea, rs1, ea+reg.r[rs1], reg.r[rd]&0xffff);
		ea += reg.r[rs1];
	}
	else {
		ea = reg.r[rs1] + reg.r[rs2];
		if(trace)
			itrace("sth\tr%d,[r%d+r%d] %lux=%lux",
					rd, rs1, rs2, ea, reg.r[rd]&0xffff);
	}

	putmem_h(ea, reg.r[rd]);
}

void
ld(ulong ir)
{
	ulong ea;
	int rd, rs1, rs2;

	getrop23(ir);
	if(ir&IMMBIT) {
		ximm(ea, ir);
		if(trace)
			itrace("ld\tr%d,0x%lux(r%d) ea=%lux",rd, ea, rs1, ea+reg.r[rs1]);
		ea += reg.r[rs1];
	}
	else {
		ea = reg.r[rs1] + reg.r[rs2];
		if(trace)
			itrace("ld\tr%d,[r%d+r%d] ea=%lux", rd, rs1, rs2, ea);
	}

	reg.r[rd] = getmem_w(ea);
	ilock(rd);
}

void
swap(ulong ir)
{
	ulong t, ea;
	int rd, rs1, rs2;

	getrop23(ir);
	if(ir&IMMBIT) {
		ximm(ea, ir);
		if(trace)
			itrace("swap\tr%d,0x%lux(r%d) ea=%lux",
						rd, ea, rs1, ea+reg.r[rs1]);
		ea += reg.r[rs1];
	}
	else {
		ea = reg.r[rs1] + reg.r[rs2];
		if(trace)
			itrace("swap\tr%d,[r%d+r%d] ea=%lux", rd, rs1, rs2, ea);
	}

	t = reg.r[rd];
	reg.r[rd] = getmem_w(ea);
	putmem_w(ea, t);
}

void
ldd(ulong ir)
{
	ulong ea;
	int rd, rs1, rs2;

	getrop23(ir);
	if(ir&IMMBIT) {
		ximm(ea, ir);
		if(trace)
			itrace("ldd\tr%d,0x%lux(r%d) ea=%lux",rd, ea, rs1, ea+reg.r[rs1]);
		ea += reg.r[rs1];
	}
	else {
		ea = reg.r[rs1] + reg.r[rs2];
		if(trace)
			itrace("ldd\tr%d,[r%d+r%d] ea=%lux", rd, rs1, rs2, ea);
	}

	reg.r[rd] = getmem_w(ea);
	reg.r[rd+1] = getmem_w(ea+4);
}

void
ldub(ulong ir)
{
	ulong ea;
	int rd, rs1, rs2;

	getrop23(ir);
	if(ir&IMMBIT) {
		ximm(ea, ir);
		if(trace)
			itrace("ldub\tr%d,0x%lux(r%d) ea=%lux",
						rd, ea, rs1, ea+reg.r[rs1]);
		ea += reg.r[rs1];
	}
	else {
		ea = reg.r[rs1] + reg.r[rs2];
		if(trace)
			itrace("ldub\tr%d,[r%d+r%d] ea=%lux", rd, rs1, rs2, ea);
	}

	reg.r[rd] = getmem_b(ea) & 0xff;
	ilock(rd);
}

void
ldstub(ulong ir)
{
	ulong ea;
	int rd, rs1, rs2;

	getrop23(ir);
	if(ir&IMMBIT) {
		ximm(ea, ir);
		if(trace)
			itrace("ldstub\tr%d,0x%lux(r%d) ea=%lux",
						rd, ea, rs1, ea+reg.r[rs1]);
		ea += reg.r[rs1];
	}
	else {
		ea = reg.r[rs1] + reg.r[rs2];
		if(trace)
			itrace("ldstub\tr%d,[r%d+r%d] ea=%lux", rd, rs1, rs2, ea);
	}

	reg.r[rd] = getmem_b(ea) & 0xff;
	putmem_b(ea, 0xff);
}

void
ldsb(ulong ir)
{
	ulong ea;
	int rd, rs1, rs2;

	getrop23(ir);
	if(ir&IMMBIT) {
		ximm(ea, ir);
		if(trace)
			itrace("ldsb\tr%d,0x%lux(r%d) ea=%lux",
						rd, ea, rs1, ea+reg.r[rs1]);
		ea += reg.r[rs1];
	}
	else {
		ea = reg.r[rs1] + reg.r[rs2];
		if(trace)
			itrace("ldsb\tr%d,[r%d+r%d] ea=%lux", rd, rs1, rs2, ea);
	}

	reg.r[rd] = (schar)getmem_b(ea);
	ilock(rd);
}

void
lduh(ulong ir)
{
	ulong ea;
	int rd, rs1, rs2;

	getrop23(ir);
	if(ir&IMMBIT) {
		ximm(ea, ir);
		if(trace)
			itrace("lduh\tr%d,0x%lux(r%d) ea=%lux",
						rd, ea, rs1, ea+reg.r[rs1]);
		ea += reg.r[rs1];
	}
	else {
		ea = reg.r[rs1] + reg.r[rs2];
		if(trace)
			itrace("lduh\tr%d,[r%d+r%d] ea=%lux", rd, rs1, rs2, ea);
	}

	reg.r[rd] = getmem_h(ea) & 0xffff;
	ilock(rd);
}

void
ldsh(ulong ir)
{
	ulong ea;
	int rd, rs1, rs2;

	getrop23(ir);
	if(ir&IMMBIT) {
		ximm(ea, ir);
		if(trace)
			itrace("ldsh\tr%d,0x%lux(r%d) ea=%lux",
						rd, ea, rs1, ea+reg.r[rs1]);
		ea += reg.r[rs1];
	}
	else {
		ea = reg.r[rs1] + reg.r[rs2];
		if(trace)
			itrace("ldsh\tr%d,[r%d+r%d] ea=%lux", rd, rs1, rs2, ea);
	}

	reg.r[rd] = (short)getmem_h(ea);
	ilock(rd);
}

void
sethi(ulong ir)
{
	int rd;
	ulong v;

	rd = (ir>>25)&0x1f;
	v = (ir&0x3FFFFF)<<10;

	if(rd == 0)
		nopcount++;

	if(trace)
		itrace("sethi\t0x%lux,r%d", v, rd);

	reg.r[rd] = v;
}

void
call(ulong ir)
{
	Symbol s;
	ulong npc;

	npc = (ir<<2) + reg.pc;
	if(trace)
		itrace("call\t%lux", npc);

	ci->taken++;
	reg.r[15] = reg.pc;
	reg.ir = ifetch(reg.pc+4);
	delay(npc);

	if(calltree) {
		findsym(npc, CTEXT, &s);
		Bprint(bioout, "%8lux %s(", reg.pc, s.name);
		printparams(&s, reg.r[1]);
		Bprint(bioout, "from ");
		printsource(reg.pc);
		Bputc(bioout, '\n');
	}
	npc -= 4;
	reg.pc = npc;
}

void
jmpl(ulong ir)
{
	ulong ea, o;
	Symbol s;
	int rd, rs1, rs2;

	getrop23(ir);
	if(ir&IMMBIT) {
		ximm(ea, ir);
		o = ea;
		if(trace)
			itrace("jmpl\t0x%lux(r%d),r%d", ea, rs1, rd);

		ea += reg.r[rs1];
		if(calltree && rd == 0 && o == 8) {
			findsym(ea-4, CTEXT, &s);
			Bprint(bioout, "%8lux return to %lux %s r7=%lux\n",
						reg.pc, ea-4, s.name, reg.r[7]);
		}
	}
	else {
		ea = reg.r[rs1] + reg.r[rs2];
		if(trace)
			itrace("jmpl\t[r%d+r%d],r%d", rs1, rs2, rd);
	}

	ci->taken++;
	reg.r[rd] = reg.pc;
	reg.ir = ifetch(reg.pc+4);
	delay(ea);
	reg.pc = ea-4;
}

void
bicc(ulong ir)
{
	char *op;
	ulong npc, anul, ba;
	int takeit, z, v, n, c;

	SET(op, takeit);
	ba = 0;
	switch((ir>>25)&0x0F) {
	case 0:
		op = "bn";
		takeit = 0;
		break;
	case 1:
		op = "be";
		takeit = reg.psr&PSR_z;
		break;
	case 2:
		op = "ble";
		z = reg.psr&PSR_z ? 1 : 0;
		v = reg.psr&PSR_v ? 1 : 0;
		n = reg.psr&PSR_n ? 1 : 0;
		takeit = z | (n ^ v);
		break;
	case 3:
		op = "bl";
		v = reg.psr&PSR_v ? 1 : 0;
		n = reg.psr&PSR_n ? 1 : 0;
		takeit = n ^ v;
		break;
	case 4:
		op = "bleu";
		z = reg.psr&PSR_z ? 1 : 0;
		c = reg.psr&PSR_c ? 1 : 0;
		takeit = c | z;
		break;
	case 5:
		op = "bcs";
		takeit = reg.psr&PSR_c;
		break;
	case 6:
		op = "bneg";
		takeit = reg.psr&PSR_n;
		break;
	case 7:
		op = "bvs";
		takeit = reg.psr&PSR_v;
		break;
	case 8:
		op = "ba";
		ba = 1;
		takeit = 1;
		break;
	case 9:
		op = "bne";
		takeit = !(reg.psr&PSR_z);
		break;
	case 10:
		op = "bg";
		z = reg.psr&PSR_z ? 1 : 0;
		v = reg.psr&PSR_v ? 1 : 0;
		n = reg.psr&PSR_n ? 1 : 0;
		takeit = !(z | (n ^ v));
		break;
	case 11:
		op = "bge";
		v = reg.psr&PSR_v ? 1 : 0;
		n = reg.psr&PSR_n ? 1 : 0;
		takeit = !(n ^ v);
		break;
	case 12:
		op = "bgu";
		z = reg.psr&PSR_z ? 1 : 0;
		c = reg.psr&PSR_c ? 1 : 0;
		takeit = !(c | z);
		break;
	case 13:
		op = "bcc";
		takeit = !(reg.psr&PSR_c);
		break;
	case 14:
		op = "bpos";
		takeit = !(reg.psr&PSR_n);
		break;
	case 15:
		op = "bvc";
		takeit = !(reg.psr&PSR_v);
		break;
	}

	npc = ir & 0x3FFFFF;
	if(npc & (1<<21))
		npc |= ~((1<<22)-1);
	npc = (npc<<2) + reg.pc;

	anul = ir&ANUL;
	if(trace) {
		if(anul)
			itrace("%s,a\t%lux", op, npc);
		else
			itrace("%s\t%lux", op, npc);
	}

	if(takeit == 0) {
		reg.pc += 4;
		if(anul == 0) {
			reg.ir = ifetch(reg.pc);
			delay(reg.pc+4);
		}
		else
			anulled++;
		return;
	}

	ci->taken++;
	if(ba && anul) {
		anulled++;
		reg.pc = npc-4;
		return;	
	}
	reg.ir = ifetch(reg.pc+4);
	delay(npc);
	reg.pc = npc-4;
}
