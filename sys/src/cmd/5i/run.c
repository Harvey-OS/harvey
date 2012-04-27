#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>
#include "arm.h"

static	int	dummy;
static	char*	shtype[4] =
{
	"<<",
	">>",
	"->",
	"@>",
};
static	char*	cond[16] =
{
	".EQ",	".NE",	".HS",	".LO",
	".MI",	".PL",	".VS",	".VC",
	".HI",	".LS",	".GE",	".LT",
	".GT",	".LE",	"",	".NO",
};

void	Idp0(ulong);
void	Idp1(ulong);
void	Idp2(ulong);
void	Idp3(ulong);

void	Imul(ulong);
void	Imula(ulong);
void	Imull(ulong);

void	Iswap(ulong);
void	Imem1(ulong);
void	Imem2(ulong);
void	Ilsm(ulong inst);

void	Ib(ulong);
void	Ibl(ulong);

void	Ssyscall(ulong);

Inst itab[] =
{
	{ Idp0,		"AND",	Iarith },	/* 00 - r,r,r */
	{ Idp0,		"EOR",	Iarith },	/* 01 */
	{ Idp0,		"SUB",	Iarith },	/* 02 */
	{ Idp0,		"RSB",	Iarith },	/* 03 */
	{ Idp0,		"ADD",	Iarith },	/* 04 */
	{ Idp0,		"ADC",	Iarith },	/* 05 */
	{ Idp0,		"SBC",	Iarith },	/* 06 */
	{ Idp0,		"RSC",	Iarith },	/* 07 */
	{ Idp0,		"TST",	Iarith },	/* 08 */
	{ Idp0,		"TEQ",	Iarith },	/* 09 */

	{ Idp0,		"CMP",	Iarith },	/* 10 */
	{ Idp0,		"CMN",	Iarith },	/* 11 */
	{ Idp0,		"ORR",	Iarith },	/* 12 */
	{ Idp0,		"MOV",	Iarith },	/* 13 */
	{ Idp0,		"BIC",	Iarith },	/* 14 */
	{ Idp0,		"MVN",	Iarith },	/* 15 */
	{ Idp1,		"AND",	Iarith },	/* 16 */
	{ Idp1,		"EOR",	Iarith },	/* 17 */
	{ Idp1,		"SUB",	Iarith },	/* 18 */
	{ Idp1,		"RSB",	Iarith },	/* 19 */

	{ Idp1,		"ADD",	Iarith },	/* 20 */
	{ Idp1,		"ADC",	Iarith },	/* 21 */
	{ Idp1,		"SBC",	Iarith },	/* 22 */
	{ Idp1,		"RSC",	Iarith },	/* 23 */
	{ Idp1,		"TST",	Iarith },	/* 24 */
	{ Idp1,		"TEQ",	Iarith },	/* 25 */
	{ Idp1,		"CMP",	Iarith },	/* 26 */
	{ Idp1,		"CMN",	Iarith },	/* 27 */
	{ Idp1,		"ORR",	Iarith },	/* 28 */
	{ Idp1,		"MOV",	Iarith },	/* 29 */

	{ Idp1,		"BIC",	Iarith },	/* 30 */
	{ Idp1,		"MVN",	Iarith },	/* 31 */
	{ Idp2,		"AND",	Iarith },	/* 32 */
	{ Idp2,		"EOR",	Iarith },	/* 33 */
	{ Idp2,		"SUB",	Iarith },	/* 34 */
	{ Idp2,		"RSB",	Iarith },	/* 35 */
	{ Idp2,		"ADD",	Iarith },	/* 36 */
	{ Idp2,		"ADC",	Iarith },	/* 37 */
	{ Idp2,		"SBC",	Iarith },	/* 38 */
	{ Idp2,		"RSC",	Iarith },	/* 39 */

	{ Idp2,		"TST",	Iarith },	/* 40 */
	{ Idp2,		"TEQ",	Iarith },	/* 41 */
	{ Idp2,		"CMP",	Iarith },	/* 42 */
	{ Idp2,		"CMN",	Iarith },	/* 43 */
	{ Idp2,		"ORR",	Iarith },	/* 44 */
	{ Idp2,		"MOV",	Iarith },	/* 45 */
	{ Idp2,		"BIC",	Iarith },	/* 46 */
	{ Idp2,		"MVN",	Iarith },	/* 47 */
	{ Idp3,		"AND",	Iarith },	/* 48 - i,r,r */
	{ Idp3,		"EOR",	Iarith },	/* 49 */

	{ Idp3,		"SUB",	Iarith },	/* 50 */
	{ Idp3,		"RSB",	Iarith },	/* 51 */
	{ Idp3,		"ADD",	Iarith },	/* 52 */
	{ Idp3,		"ADC",	Iarith },	/* 53 */
	{ Idp3,		"SBC",	Iarith },	/* 54 */
	{ Idp3,		"RSC",	Iarith },	/* 55 */
	{ Idp3,		"TST",	Iarith },	/* 56 */
	{ Idp3,		"TEQ",	Iarith },	/* 57 */
	{ Idp3,		"CMP",	Iarith },	/* 58 */
	{ Idp3,		"CMN",	Iarith },	/* 59 */

	{ Idp3,		"ORR",	Iarith },	/* 60 */
	{ Idp3,		"MOV",	Iarith },	/* 61 */
	{ Idp3,		"BIC",	Iarith },	/* 62 */
	{ Idp3,		"MVN",	Iarith },	/* 63 */
	{ Imul,		"MUL",	Iarith },	/* 64 */
	{ Imula,	"MULA",	Iarith },	/* 65 */

	{ Iswap,	"SWPW",	Imem },	/* 66 */
	{ Iswap,	"SWPBU",	Imem },	/* 67 */

	{ Imem2,	"MOV",	Imem },	/* 68 load/store h/sb */
	{ Imem2,	"MOV",	Imem },	/* 69 */
	{ Imem2,	"MOV",	Imem },	/* 70 */
	{ Imem2,	"MOV",	Imem },	/* 71 */

	{ Imem1,	"MOVW",	Imem },	/* 72 load/store w/ub i,r */
	{ Imem1,	"MOVB",	Imem },	/* 73 */
	{ Imem1,	"MOVW",	Imem },	/* 74 */
	{ Imem1,	"MOVB",	Imem },	/* 75 */
	{ Imem1,	"MOVW",	Imem },	/* 76 load/store r,r */
	{ Imem1,	"MOVB",	Imem },	/* 77 */
	{ Imem1,	"MOVW",	Imem },	/* 78 */
	{ Imem1,	"MOVB",	Imem },	/* 79 */

	{ Ilsm,		"LDM",	Imem },	/* 80 block move r,r */
	{ Ilsm,		"STM",	Imem },	/* 81 */
	{ Ib,		"B",	Ibranch },		/* 82 branch */
	{ Ibl,		"BL",	Ibranch },		/* 83 */
	{ Ssyscall,	"SWI",	Isyscall },	/* 84 co processor */
	{ undef,	"undef" },	/* 85 */
	{ undef,	"undef" },	/* 86 */
	{ undef,	"undef"  },	/* 87 */
	{ Imull,	"MULLU",	Iarith },	/* 88 */
	{ Imull,	"MULALU",	Iarith },	/* 89 */
	{ Imull,	"MULL",	Iarith  },	/* 90 */
	{ Imull,	"MULAL",	Iarith  },	/* 91 */
	{ undef,	"undef"  },	/* 92 */

	{ 0 }
};

int
runcmp(void)
{
	switch(reg.cond) {
	case 0x0:	/* eq */	return (reg.cc1 == reg.cc2);
	case 0x1:	/* ne */	return (reg.cc1 != reg.cc2);
	case 0x2:	/* hs */	return ((ulong)reg.cc1 >= (ulong)reg.cc2);
	case 0x3:	/* lo */	return ((ulong)reg.cc1 < (ulong)reg.cc2);
	case 0x4:	/* mi */	return (reg.cc1 - reg.cc2 < 0);
	case 0x5:	/* pl */	return (reg.cc1 - reg.cc2 >= 0);
	case 0x8:	/* hi */	return ((ulong)reg.cc1 > (ulong)reg.cc2);
	case 0x9:	/* ls */	return ((ulong)reg.cc1 <= (ulong)reg.cc2);
	case 0xa:	/* ge */	return (reg.cc1 >= reg.cc2);
	case 0xb:	/* lt */	return (reg.cc1 < reg.cc2);
	case 0xc:	/* gt */	return (reg.cc1 > reg.cc2);
	case 0xd:	/* le */	return (reg.cc1 <= reg.cc2);
	case 0xe:	/* al */	return 1;
	case 0xf:	/* nv */	return 0;
	default:
		Bprint(bioout, "unimplemented condition prefix %x (%ld %ld)\n",
			reg.cond, reg.cc1, reg.cc2);
		undef(reg.ir);
		return 0;
	}
}

int
runteq(void)
{
	long res = reg.cc1 ^ reg.cc2;
	switch(reg.cond) {
	case 0x0:	/* eq */	return res == 0;
	case 0x1:	/* ne */	return res != 0;
	case 0x4:	/* mi */	return (res & SIGNBIT) != 0;
	case 0x5:	/* pl */	return (res & SIGNBIT) == 0;
	case 0xe:	/* al */	return 1;
	case 0xf:	/* nv */	return 0;
	default:
		Bprint(bioout, "unimplemented condition prefix %x (%ld %ld)\n",
			reg.cond, reg.cc1, reg.cc2);
		undef(reg.ir);
		return 0;
	}
}

int
runtst(void)
{
	long res = reg.cc1 & reg.cc2;
	switch(reg.cond) {
	case 0x0:	/* eq */	return res == 0;
	case 0x1:	/* ne */	return res != 0;
	case 0x4:	/* mi */	return (res & SIGNBIT) != 0;
	case 0x5:	/* pl */	return (res & SIGNBIT) == 0;
	case 0xe:	/* al */	return 1;
	case 0xf:	/* nv */	return 0;
	default:
		Bprint(bioout, "unimplemented condition prefix %x (%ld %ld)\n",
			reg.cond, reg.cc1, reg.cc2);
		undef(reg.ir);
		return 0;
	}
}

void
run(void)
{
	int execute;

	do {
		if(trace)
			Bflush(bioout);
		reg.ar = reg.r[REGPC];
		reg.ir = ifetch(reg.ar);
		reg.class = armclass(reg.ir);
		reg.ip = &itab[reg.class];
		reg.cond = (reg.ir>>28) & 0xf;
		switch(reg.compare_op) {
		case CCcmp:
			execute = runcmp();
			break;
		case CCteq:
			execute = runteq();
			break;
		case CCtst:
			execute = runtst();
			break;
		default:
			Bprint(bioout, "unimplemented compare operation %x\n",
				reg.compare_op);
			return;
		}

		if(execute) {
			reg.ip->count++;
			(*reg.ip->func)(reg.ir);
		} else {
			if(trace)
				itrace("%s%s	IGNORED",
					reg.ip->name, cond[reg.cond]);
		}
		reg.r[REGPC] += 4;
		if(bplist)
			brkchk(reg.r[REGPC], Instruction);
	} while(--count);
}

void
undef(ulong inst)
{
	Bprint(bioout, "undefined instruction trap pc #%lux inst %.8lux class %d\n",
		reg.r[REGPC], inst, reg.class);
	longjmp(errjmp, 0);
}

long
shift(long v, int st, int sc, int isreg)
{
	if(sc == 0) {
		switch(st) {
		case 0:	/* logical left */
			reg.cout = reg.cbit;
			break;
		case 1:	/* logical right */
			reg.cout = (v >> 31) & 1;
			break;
		case 2:	/* arith right */
			reg.cout = reg.cbit;
			break;
		case 3:	/* rotate right */
			if(isreg) {
				reg.cout = reg.cbit;
			}
			else {
				reg.cout = v & 1;
				v = ((ulong)v >> 1) | (reg.cbit << 31);
			}
		}
	}
	else {
		switch(st) {
		case 0:	/* logical left */
			reg.cout = (v >> (32 - sc)) & 1;
			v = v << sc;
			break;
		case 1:	/* logical right */
			reg.cout = (v >> (sc - 1)) & 1;
			v = (ulong)v >> sc;
			break;
		case 2:	/* arith right */
			if(sc >= 32) {
				reg.cout = (v >> 31) & 1;
				if(reg.cout)
					v = 0xFFFFFFFF;
				else
					v = 0;
			}
			else {
				reg.cout = (v >> (sc - 1)) & 1;
				v = (long)v >> sc;
			}
			break;
		case 3:	/* rotate right */
			reg.cout = (v >> (sc - 1)) & 1;
			v = (v << (32-sc)) | ((ulong)v >> sc);
			break;
		}
	}
	return v;
}

void
dpex(long inst, long o1, long o2, int rd)
{
	int cbit;

	cbit = 0;
	switch((inst>>21) & 0xf) {
	case  0:	/* and */
		reg.r[rd] = o1 & o2;
		cbit = 1;
		break;
	case  1:	/* eor */
		reg.r[rd] = o1 ^ o2;
		cbit = 1;
		break;
	case  2:	/* sub */
		reg.r[rd] = o1 - o2;
	case 10:	/* cmp */
		if(inst & Sbit) {
			reg.cc1 = o1;
			reg.cc2 = o2;
			reg.compare_op = CCcmp;
		}
		return;
	case  3:	/* rsb */
		reg.r[rd] = o2 - o1;
		if(inst & Sbit) {
			reg.cc1 = o2;
			reg.cc2 = o1;
			reg.compare_op = CCcmp;
		}
		return;
	case  4:	/* add */
		if(calltree && rd == REGPC && o2 == 0) {
			Symbol s;

			findsym(o1 + o2, CTEXT, &s);
			Bprint(bioout, "%8lux return to %lux %s r0=%lux\n",
						reg.r[REGPC], o1 + o2, s.name, reg.r[REGRET]);
		}
		reg.r[rd] = o1 + o2;
		if(inst & Sbit) {
			if(((uvlong)(ulong)o1 + (uvlong)(ulong)o2) & (1LL << 32))
				reg.cbit = 1;
			else
				reg.cbit = 0;
			reg.cc1 = o2;
			reg.cc2 = -o1;
			reg.compare_op = CCcmp;
		}
		return;
	case  5:	/* adc */
	case  6:	/* sbc */
	case  7:	/* rsc */
		undef(inst);
	case  8:	/* tst */
		if(inst & Sbit) {
			reg.cc1 = o1;
			reg.cc2 = o2;
			reg.compare_op = CCtst;
		}
		return;
	case  9:	/* teq */
		if(inst & Sbit) {
			reg.cc1 = o1;
			reg.cc2 = o2;
			reg.compare_op = CCteq;
		}
		return;
	case 11:	/* cmn */
		if(inst & Sbit) {
			reg.cc1 = o1;
			reg.cc2 = -o2;
			reg.compare_op = CCcmp;
		}
		return;
	case 12:	/* orr */
		reg.r[rd] = o1 | o2;
		cbit = 1;
		break;
	case 13:	/* mov */
		reg.r[rd] = o2;
		cbit = 1;
		break;
	case 14:	/* bic */
		reg.r[rd] = o1 & ~o2;
		cbit = 1;
		break;
	case 15:	/* mvn */
		reg.r[rd] = ~o2;
		cbit = 1;
		break;
	}
	if(inst & Sbit) {
		if(cbit)
			reg.cbit = reg.cout;
		reg.cc1 = reg.r[rd];
		reg.cc2 = 0;
		reg.compare_op = CCcmp;
	}
}

/*
 * data processing instruction R,R,R
 */
void
Idp0(ulong inst)
{
	int rn, rd, rm;
	long o1, o2;

	rn = (inst>>16) & 0xf;
	rd = (inst>>12) & 0xf;
	rm = inst & 0xf;
	o1 = reg.r[rn];
	if(rn == REGPC)
		o1 += 8;
	o2 = reg.r[rm];
	if(rm == REGPC)
		o2 += 8;

	dpex(inst, o1, o2, rd);
	if(trace)
		itrace("%s%s\tR%d,R%d,R%d =#%x",
			reg.ip->name, cond[reg.cond],
			rm, rn, rd,
			reg.r[rd]);
	if(rd == REGPC)
		reg.r[rd] -= 4;
}

/*
 * data processing instruction (R<>#),R,R
 */
void
Idp1(ulong inst)
{
	int rn, rd, rm, st, sc;
	long o1, o2;

	rn = (inst>>16) & 0xf;
	rd = (inst>>12) & 0xf;
	rm = inst & 0xf;
	st = (inst>>5) & 0x3;
	sc = (inst>>7) & 0x1f;
	o1 = reg.r[rn];
	if(rn == REGPC)
		o1 += 8;
	o2 = reg.r[rm];
	if(rm == REGPC)
		o2 += 8;
	o2 = shift(o2, st, sc, 0);
	dpex(inst, o1, o2, rd);
	if(trace)
		itrace("%s%s\tR%d%s%d,R%d,R%d =#%x",
			reg.ip->name, cond[reg.cond], rm, shtype[st], sc, rn, rd,
			reg.r[rd]);
	if(rd == REGPC)
		reg.r[rd] -= 4;
}

/*
 * data processing instruction (R<>R),R,R
 */
void
Idp2(ulong inst)
{
	int rn, rd, rm, rs, st;
	long o1, o2, o3;

	rn = (inst>>16) & 0xf;
	rd = (inst>>12) & 0xf;
	rm = inst & 0xf;
	st = (inst>>5) & 0x3;
	rs = (inst>>8) & 0xf;
	o1 = reg.r[rn];
	if(rn == REGPC)
		o1 += 8;
	o2 = reg.r[rm];
	if(rm == REGPC)
		o2 += 8;
	o3 = reg.r[rs];
	if(rs == REGPC)
		o3 += 8;
	o2 = shift(o2, st, o3, 1);
	dpex(inst, o1, o2, rd);
	if(trace)
		itrace("%s%s\tR%d%sR%d=%d,R%d,R%d =#%x",
			reg.ip->name, cond[reg.cond], rm, shtype[st], rs, o3, rn, rd,
			reg.r[rd]);
	if(rd == REGPC)
		reg.r[rd] -= 4;
}

/*
 * data processing instruction #<>#,R,R
 */
void
Idp3(ulong inst)
{
	int rn, rd, sc;
	long o1, o2;

	rn = (inst>>16) & 0xf;
	rd = (inst>>12) & 0xf;
	o1 = reg.r[rn];
	if(rn == REGPC)
		o1 += 8;
	o2 = inst & 0xff;
	sc = (inst>>7) & 0x1e;
	o2 = (o2 >> sc) | (o2 << (32 - sc));

	dpex(inst, o1, o2, rd);
	if(trace)
		itrace("%s%s\t#%x,R%d,R%d =#%x",
			reg.ip->name, cond[reg.cond], o2, rn, rd,
			reg.r[rd]);
	if(rd == REGPC)
		reg.r[rd] -= 4;
}

void
Imul(ulong inst)
{
	int rs, rd, rm;

	rd = (inst>>16) & 0xf;
	rs = (inst>>8) & 0xf;
	rm = inst & 0xf;

	if(rd == REGPC || rs == REGPC || rm == REGPC || rd == rm)
		undef(inst);

	reg.r[rd] = reg.r[rm]*reg.r[rs];

	if(trace)
		itrace("%s%s\tR%d,R%d,R%d =#%x",
			reg.ip->name, cond[reg.cond], rs, rm, rd,
			reg.r[rd]);
}

void
Imull(ulong inst)
{
	vlong v;
	int rs, rd, rm, rn;

	rd = (inst>>16) & 0xf;
	rn = (inst>>12) & 0xf;
	rs = (inst>>8) & 0xf;
	rm = inst & 0xf;

	if(rd == REGPC || rn == REGPC || rs == REGPC || rm == REGPC
	|| rd == rm || rn == rm || rd == rn)
		undef(inst);

	if(inst & (1<<22)){
		v = (vlong)reg.r[rm] * (vlong)reg.r[rs];
		if(inst & (1 << 21))
			v += reg.r[rn];
	}else{
		v = (uvlong)(ulong)reg.r[rm] * (uvlong)(ulong)reg.r[rs];
		if(inst & (1 << 21))
			v += (ulong)reg.r[rn];
	}
	reg.r[rd] = v >> 32;
	reg.r[rn] = v;

	if(trace)
		itrace("%s%s\tR%d,R%d,(R%d,R%d) =#%llx",
			reg.ip->name, cond[reg.cond], rs, rm, rn, rd,
			v);
}

void
Imula(ulong inst)
{
	int rs, rd, rm, rn;

	rd = (inst>>16) & 0xf;
	rn = (inst>>12) & 0xf;
	rs = (inst>>8) & 0xf;
	rm = inst & 0xf;

	if(rd == REGPC || rn == REGPC || rs == REGPC || rm == REGPC || rd == rm)
		undef(inst);

	reg.r[rd] = reg.r[rm]*reg.r[rs] + reg.r[rn];

	if(trace)
		itrace("%s%s\tR%d,R%d,R%d,R%d =#%x",
			reg.ip->name, cond[reg.cond], rs, rm, rn, rd,
			reg.r[rd]);
}

void
Iswap(ulong inst)
{
	int rn, rd, rm;
	ulong address, value, bbit;

	bbit = inst & (1<<22);
	rn = (inst>>16) & 0xf;
	rd = (inst>>12) & 0xf;
	rm = (inst>>0) & 0xf;

	address = reg.r[rn];
	if(bbit) {
		value = getmem_b(address);
		putmem_b(address, reg.r[rm]);
	} else {
		value = getmem_w(address);
		putmem_w(address, reg.r[rm]);
	}
	reg.r[rd] = value;

	if(trace) {
		char *bw, *dotc;

		bw = "";
		if(bbit)
			bw = "B";
		dotc = cond[reg.cond];

		itrace("SWP%s%s\t#%x(R%d),R%d #%lux=#%x",
			bw, dotc,
			rn, rd,
			address, value);
	}
}

/*
 * load/store word/byte
 */
void
Imem1(ulong inst)
{
	int rn, rd, off, rm, sc, st;
	ulong address, value, pbit, ubit, bbit, wbit, lbit, bit25;

	bit25 = inst & (1<<25);
	pbit = inst & (1<<24);
	ubit = inst & (1<<23);
	bbit = inst & (1<<22);
	wbit = inst & (1<<21);
	lbit = inst & (1<<20);
	rn = (inst>>16) & 0xf;
	rd = (inst>>12) & 0xf;

	SET(st);
	SET(sc);
	SET(rm);
	if(bit25) {
		rm = inst & 0xf;
		st = (inst>>5) & 0x3;
		sc = (inst>>7) & 0x1f;
		off = reg.r[rm];
		if(rm == REGPC)
			off += 8;
		off = shift(off, st, sc, 0);
	} else {
		off = inst & 0xfff;
	}
	if(!ubit)
		off = -off;
	if(rn == REGPC)
		off += 8;

	address = reg.r[rn];
	if(pbit)
		address += off;

	if(lbit) {
		if(bbit)
			value = getmem_b(address);
		else
			value = getmem_w(address);
		if(rd == REGPC)
			value -= 4;
		reg.r[rd] = value;
	} else {
		value = reg.r[rd];
		if(rd == REGPC)
			value -= 4;
		if(bbit)
			putmem_b(address, value);
		else
			putmem_w(address, value);
	}
	if(!(pbit && !wbit))
		reg.r[rn] += off;

	if(trace) {
		char *bw, *dotp, *dotc;

		bw = "W";
		if(bbit)
			bw = "BU";
		dotp = "";
		if(!pbit)
			dotp = ".P";
		dotc = cond[reg.cond];

		if(lbit) {
			if(!bit25)
				itrace("MOV%s%s%s\t#%x(R%d),R%d #%lux=#%x",
					bw, dotp, dotc,
					off, rn, rd,
					address, value);
			else
				itrace("MOV%s%s%s\t(R%d%s%d)(R%d),R%d  #%lux=#%x",
					bw, dotp, dotc,
					rm, shtype[st], sc, rn, rd,
					address, value);
		} else {
			if(!bit25)
				itrace("MOV%s%s%s\tR%d,#%x(R%d) #%lux=#%x",
					bw, dotp, dotc,
					rd, off, rn,
					address, value);
			else
				itrace("MOV%s%s%s\tR%d,(R%d%s%d)(R%d) #%lux=#%x",
					bw, dotp, dotc,
					rd, rm, shtype[st], sc, rn,
					address, value);
		}
	}
}

/*
 * load/store unsigned byte/half word
 */
void
Imem2(ulong inst)
{
	int rn, rd, off, rm;
	ulong address, value, pbit, ubit, hbit, sbit, wbit, lbit, bit22;

	pbit = inst & (1<<24);
	ubit = inst & (1<<23);
	bit22 = inst & (1<<22);
	wbit = inst & (1<<21);
	lbit = inst & (1<<20);
	sbit = inst & (1<<6);
	hbit = inst & (1<<5);
	rn = (inst>>16) & 0xf;
	rd = (inst>>12) & 0xf;

	SET(rm);
	if(bit22) {
		off = ((inst>>4) & 0xf0) | (inst & 0xf);
	} else {
		rm = inst & 0xf;
		off = reg.r[rm];
		if(rm == REGPC)
			off += 8;
	}
	if(!ubit)
		off = -off;
	if(rn == REGPC)
		off += 8;

	address = reg.r[rn];
	if(pbit)
		address += off;

	if(lbit) {
		if(hbit) {
			value = getmem_h(address);
			if(sbit && (value & 0x8000))
				value |= 0xffff0000;
		} else {
			value = getmem_b(address);
			if(value & 0x80)
				value |= 0xffffff00;
		}
		if(rd == REGPC)
			value -= 4;
		reg.r[rd] = value;
	} else {
		value = reg.r[rd];
		if(rd == REGPC)
			value -= 4;
		if(hbit) {
			putmem_h(address, value);
		} else {
			putmem_b(address, value);
		}
	}
	if(!(pbit && !wbit))
		reg.r[rn] += off;

	if(trace) {
		char *hb, *dotp, *dotc;

		hb = "B";
		if(hbit)
			hb = "H";
		dotp = "";
		if(!pbit)
			dotp = ".P";
		dotc = cond[reg.cond];

		if(lbit) {
			if(bit22)
				itrace("MOV%s%s%s\t#%x(R%d),R%d #%lux=#%x",
					hb, dotp, dotc,
					off, rn, rd,
					address, value);
			else
				itrace("MOV%s%s%s\t(R%d)(R%d),R%d  #%lux=#%x",
					hb, dotp, dotc,
					rm, rn, rd,
					address, value);
		} else {
			if(bit22)
				itrace("MOV%s%s%s\tR%d,#%x(R%d) #%lux=#%x",
					hb, dotp, dotc,
					rd, off, rn,
					address, value);
			else
				itrace("MOV%s%s%s\tR%d,(R%d)(R%d) #%lux=#%x",
					hb, dotp, dotc,
					rd, rm, rn,
					address, value);
		}
	}
}

void
Ilsm(ulong inst)
{
	char pbit, ubit, sbit, wbit, lbit;
	int i, rn, reglist;
	ulong address, predelta, postdelta;

	pbit = (inst>>24) & 0x1;
	ubit = (inst>>23) & 0x1;
	sbit = (inst>>22) & 0x1;
	wbit = (inst>>21) & 0x1;
	lbit = (inst>>20) & 0x1;
	rn =   (inst>>16) & 0xf;
	reglist = inst & 0xffff;

	if(reglist & 0x8000)
		undef(reg.ir);
	if(sbit)
		undef(reg.ir);

	address = reg.r[rn];

	if(pbit) {
		predelta = 4;
		postdelta = 0;
	} else {
		predelta = 0;
		postdelta = 4;
	}
	if(ubit) {
		for (i = 0; i < 16; ++i) {
			if(!(reglist & (1 << i)))
				continue;
			address += predelta;
			if(lbit)
				reg.r[i] = getmem_w(address);
			else
				putmem_w(address, reg.r[i]);
			address += postdelta;
		}
	} else {
		for (i = 15; 0 <= i; --i) {
			if(!(reglist & (1 << i)))
				continue;
			address -= predelta;
			if(lbit)
				reg.r[i] = getmem_w(address);
			else
				putmem_w(address, reg.r[i]);
			address -= postdelta;
		}
	}
	if(wbit) {
		reg.r[rn] = address;
	}

	if(trace) {
		itrace("%s.%c%c\tR%d=%lux%s, <%lux>",
			(lbit ? "LDM" : "STM"), (ubit ? 'I' : 'D'), (pbit ? 'B' : 'A'),
			rn, reg.r[rn], (wbit ? "!" : ""), reglist);
	}
}

void
Ib(ulong inst)
{
	long v;

	v = inst & 0xffffff;
	v = reg.r[REGPC] + 8 + ((v << 8) >> 6);
	if(trace)
		itrace("B%s\t#%lux", cond[reg.cond], v);
	reg.r[REGPC] = v - 4;
}

void
Ibl(ulong inst)
{
	long v;
	Symbol s;

	v = inst & 0xffffff;
	v = reg.r[REGPC] + 8 + ((v << 8) >> 6);
	if(trace)
		itrace("BL%s\t#%lux", cond[reg.cond], v);

	if(calltree) {
		findsym(v, CTEXT, &s);
		Bprint(bioout, "%8lux %s(", reg.r[REGPC], s.name);
		printparams(&s, reg.r[13]);
		Bprint(bioout, "from ");
		printsource(reg.r[REGPC]);
		Bputc(bioout, '\n');
	}

	reg.r[REGLINK] = reg.r[REGPC] + 4;
	reg.r[REGPC] = v - 4;
}
