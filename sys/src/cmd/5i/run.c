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

void	Iswap(ulong);
void	Imem1(ulong);
void	Imem2(ulong);
void	Ilsm(ulong inst);

void	Ib(ulong);
void	Ibl(ulong);

void	Ssyscall(ulong);

Inst itab[89] =
{
	{ Idp0,		"AND" },	/* 00 - r,r,r */
	{ Idp0,		"EOR" },	/* 01 */
	{ Idp0,		"SUB" },	/* 02 */
	{ Idp0,		"RSB" },	/* 03 */
	{ Idp0,		"ADD" },	/* 04 */
	{ Idp0,		"ADC" },	/* 05 */
	{ Idp0,		"SBC" },	/* 06 */
	{ Idp0,		"RSC" },	/* 07 */
	{ Idp0,		"TST" },	/* 08 */
	{ Idp0,		"TEQ" },	/* 09 */

	{ Idp0,		"CMP" },	/* 10 */
	{ Idp0,		"CMN" },	/* 11 */
	{ Idp0,		"ORR" },	/* 12 */
	{ Idp0,		"MOV" },	/* 13 */
	{ Idp0,		"BIC" },	/* 14 */
	{ Idp0,		"MVN" },	/* 15 */
	{ Idp1,		"AND" },	/* 16 */
	{ Idp1,		"EOR" },	/* 17 */
	{ Idp1,		"SUB" },	/* 18 */
	{ Idp1,		"RSB" },	/* 19 */

	{ Idp1,		"ADD" },	/* 20 */
	{ Idp1,		"ADC" },	/* 21 */
	{ Idp1,		"SBC" },	/* 22 */
	{ Idp1,		"RSC" },	/* 23 */
	{ Idp1,		"TST" },	/* 24 */
	{ Idp1,		"TEQ" },	/* 25 */
	{ Idp1,		"CMP" },	/* 26 */
	{ Idp1,		"CMN" },	/* 27 */
	{ Idp1,		"ORR" },	/* 28 */
	{ Idp1,		"MOV" },	/* 29 */

	{ Idp1,		"BIC" },	/* 30 */
	{ Idp1,		"MVN" },	/* 31 */
	{ Idp2,		"AND" },	/* 32 */
	{ Idp2,		"EOR" },	/* 33 */
	{ Idp2,		"SUB" },	/* 34 */
	{ Idp2,		"RSB" },	/* 35 */
	{ Idp2,		"ADD" },	/* 36 */
	{ Idp2,		"ADC" },	/* 37 */
	{ Idp2,		"SBC" },	/* 38 */
	{ Idp2,		"RSC" },	/* 39 */

	{ Idp2,		"TST" },	/* 40 */
	{ Idp2,		"TEQ" },	/* 41 */
	{ Idp2,		"CMP" },	/* 42 */
	{ Idp2,		"CMN" },	/* 43 */
	{ Idp2,		"ORR" },	/* 44 */
	{ Idp2,		"MOV" },	/* 45 */
	{ Idp2,		"BIC" },	/* 46 */
	{ Idp2,		"MVN" },	/* 47 */
	{ Idp3,		"AND" },	/* 48 - i,r,r */
	{ Idp3,		"EOR" },	/* 49 */

	{ Idp3,		"SUB" },	/* 50 */
	{ Idp3,		"RSB" },	/* 51 */
	{ Idp3,		"ADD" },	/* 52 */
	{ Idp3,		"ADC" },	/* 53 */
	{ Idp3,		"SBC" },	/* 54 */
	{ Idp3,		"RSC" },	/* 55 */
	{ Idp3,		"TST" },	/* 56 */
	{ Idp3,		"TEQ" },	/* 57 */
	{ Idp3,		"CMP" },	/* 58 */
	{ Idp3,		"CMN" },	/* 59 */

	{ Idp3,		"ORR" },	/* 60 */
	{ Idp3,		"MOV" },	/* 61 */
	{ Idp3,		"BIC" },	/* 62 */
	{ Idp3,		"MVN" },	/* 63 */
	{ Imul,		"MUL" },	/* 64 */
	{ Imula,	"MULA" },	/* 65 */

	{ Iswap,	"SWPW" },	/* 66 */
	{ Iswap,	"SWPBU" },	/* 67 */

	{ Imem2,	"MOV" },	/* 68 load/store h/sb */
	{ Imem2,	"MOV" },	/* 69 */
	{ Imem2,	"MOV" },	/* 70 */
	{ Imem2,	"MOV" },	/* 71 */

	{ Imem1,	"MOVW" },	/* 72 load/store w/ub i,r */
	{ Imem1,	"MOVB" },	/* 73 */
	{ Imem1,	"MOVW" },	/* 74 */
	{ Imem1,	"MOVB" },	/* 75 */
	{ Imem1,	"MOVW" },	/* 76 load/store r,r */
	{ Imem1,	"MOVB" },	/* 77 */
	{ Imem1,	"MOVW" },	/* 78 */
	{ Imem1,	"MOVB" },	/* 79 */

	{ Ilsm,		"LDM" },	/* 80 block move r,r */
	{ Ilsm,		"STM" },	/* 81 */
	{ Ib,		"B" },		/* 82 branch */
	{ Ibl,		"BL" },		/* 83 */
	{ Ssyscall,	"SWI" },	/* 84 co processor */
	{ undef },			/* 85 */
	{ undef },			/* 86 */
	{ undef },			/* 87 */
	{ undef },			/* 88 */
};

int
runcmp(void)
{
	switch(reg.cond) {
	case 0x0:	/* eq */	return (reg.cc1 == reg.cc2);
	case 0x1:	/* ne */	return (reg.cc1 != reg.cc2);
	case 0x2:	/* hs */	return ((ulong)reg.cc1 >= (ulong)reg.cc2);
	case 0x3:	/* lo */	return ((ulong)reg.cc1 < (ulong)reg.cc2);
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

void
dpex(long inst, long o1, long o2, int rd)
{

	switch((inst>>21) & 0xf) {
	case  0:	/* and */
		reg.r[rd] = o1 & o2;
		break;
	case  1:	/* eor */
		reg.r[rd] = o1 ^ o2;
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
		reg.r[rd] = o1 + o2;
		if(inst & Sbit) {
			reg.cc1 = -o2;
			reg.cc2 = o1;
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
		break;
	case 13:	/* mov */
		reg.r[rd] = o2;
		break;
	case 14:	/* bic */
		reg.r[rd] = o1 & ~o2;
		break;
	case 15:	/* mvn */
		reg.r[rd] = ~o2;
		break;
	}
	if(inst & Sbit) {
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

	switch(st) {
	case 0:	/* logical left */
		o2 = o2 << sc;
		break;
	case 1:	/* logical right */
		o2 = (ulong)o2 >> sc;
		break;
	case 2:	/* arith right */
		o2 = (long)o2 >> sc;
		break;
	case 3:	/* rotate right */
		o2 = (o2 << (32-sc)) | ((ulong)o2 >> sc);
		break;
	}

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
	o3 &= 0x1f; /* Not the architecture spec */

	switch(st) {
	case 0:	/* logical left */
		o2 = o2 << o3;
		break;
	case 1:	/* logical right */
		o2 = (ulong)o2 >> o3;
		break;
	case 2:	/* arith right */
		o2 = (long)o2 >> o3;
		break;
	case 3:	/* rotate right */
		o2 = (o2 << (32-o3)) | ((ulong)o2 >> o3);
		break;
	}

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

		switch(st) {
		case 0:	/* logical left */
			off = off << sc;
			break;
		case 1:	/* logical right */
			off = (ulong)off >> sc;
			break;
		case 2:	/* arith right */
			off = (long)off >> sc;
			break;
		case 3:	/* rotate right */
			off = (off << (32-sc)) | ((ulong)off >> sc);
			break;
		}
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
