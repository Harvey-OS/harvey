#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"ureg.h"
#include	"io.h"
#include	"../port/error.h"

enum	/* op */
{
	ABS =	5,
	ADD =	0,
	CVTD = 	33,
	CVTS =	32,
	CVTW =	36,
	DIV =	3,
	MOV =	6,
	MUL =	2,
	NEG =	7,
	SUB =	1,
};

static int	fpunimp(ulong);
static ulong	branch(Ureg*, ulong);

void
fptrap(Ureg *ur)
{
	ulong iw, npc;

	if((u->fpsave.fpstatus&(1<<17)) == 0)
		return;

	if(ur->cause & (1<<31))
		iw = *(ulong*)(ur->pc+4);
	else
		iw = *(ulong*)ur->pc;

	if(fpunimp(iw) == 0)
		return;

	if(ur->cause & (1<<31)){
		npc = branch(ur, u->fpsave.fpstatus);
		if(npc == 0)
			return;
		ur->pc = npc;
	}
	else
		ur->pc += 4;

	u->fpsave.fpstatus = u->fpsave.fpstatus & ~(1<<17);
}

static void
unpack(FPsave *f, int fmt, int reg, int *sign, int *exp)
{
	*sign = 1;
	if(f->fpreg[reg] & 0x80000000)
		*sign = -1;

	switch(fmt){
	case 0:
		*exp = ((f->fpreg[reg]>>23)&0xFF) - ((1<<7)-2);
		break;
	case 1:
		if(reg & 1)	/* shouldn't happen */
			reg &= ~1;
		*exp = ((f->fpreg[reg]>>20)&0x7FF) - ((1<<10)-2);
		break;
	}
}

static void
zeroreg(FPsave *f, int fmt, int reg, int sign)
{
	int size;

	size = 0;
	switch(fmt){
	case 0:
		size = 4;
		break;
	case 1:
		if(reg & 1)
			reg &= ~1;
		size = 8;
		break;
	}
	memset(&f->fpreg[reg], 0, size);
	if(sign < 0)
		f->fpreg[reg] |= 0x80000000;
}

static int
fpunimp(ulong iw)
{
	int ss, st, sd;
	int es, et, ed;
	int maxe, maxm;
	ulong op, fmt, ft, fs, fd;

	if((iw>>25) != 0x23)
		return 0;
	op = iw & ((1<<6)-1);
	fmt = (iw>>21) & ((1<<4)-1);
	ft = (iw>>16) & ((1<<5)-1);
	fs = (iw>>11) & ((1<<5)-1);
	fd = (iw>>6) & ((1<<5)-1);
	unpack(&u->fpsave, fmt, fs, &ss, &es);
	unpack(&u->fpsave, fmt, ft, &st, &et);
	ed = 0;
	maxe = 0;
	maxm = 0;
	switch(fmt){
	case 0:
		maxe = 1<<7;
		maxm = 24;
		break;
	case 1:
		maxe = 1<<10;
		maxm = 53;
		break;
	}
	switch(op){
	case ABS:
		u->fpsave.fpreg[fd] &= ~0x80000000;
		return 1;

	case NEG:
		u->fpsave.fpreg[fd] ^= 0x80000000;
		return 1;

	case SUB:
		st = -st;
	case ADD:
		if(es<-(maxe-maxm) && et<-(maxe-maxm))
			ed = -maxe;
		if(es > et)
			sd = es;
		else
			sd = et;
		break;

	case DIV:
		et = -et;
	case MUL:
		sd = 1;
		if(ss != st)
			sd = -1;
		ed = es + et;
		break;

	case CVTS:
		if(fmt != 1)
			return 0;
		fmt = 0;	/* convert FROM double TO single */
		maxe = 1<<7;
		ed = es;
		sd = ss;
		break;

	default:	/* probably a compare */
		return 0;
	}
	if(ed <= -(maxe-5)){	/* guess: underflow */
		zeroreg(&u->fpsave, fmt, fd, sd);
		/* Set underflow exception and sticky */
		u->fpsave.fpstatus |= (1<<3)|(1<<13);
		return 1;
	}
	return 0;
}

static ulong*
reg(Ureg *ur, int regno)
{
	/* regs go from R31 down in ureg, R29 is missing */
	if(regno == 31)
		return &ur->r31;
	if(regno == 30)
		return &ur->r30;
	if(regno == 29)
		return &ur->sp;
	return (&ur->r28) + (28-regno);
}

static ulong
branch(Ureg *ur, ulong fcr31)
{
	ulong iw, npc, rs, rt, rd, offset;

	iw = *(ulong*)ur->pc;
	rs = (iw>>21) & 0x1F;
	if(rs)
		rs = *reg(ur, rs);
	rt = (iw>>16) & 0x1F;
	if(rt)
		rt = *reg(ur, rt);
	offset = iw & ((1<<16)-1);
	if(offset & (1<<15))	/* sign extend */
		offset |= ~((1<<16)-1);
	offset <<= 2;
	/*
	 * Integer unit jumps first
	 */
	switch(iw>>26){
	case 0:			/* SPECIAL: JR or JALR */
		switch(iw&0x3F){
		case 0x09:	/* JALR */
			rd = (iw>>11) & 0x1F;
			if(rd)
				*reg(ur, rd) = ur->pc+8;
			/* fall through */
		case 0x08:	/* JR */
			return rs;
		default:
			return 0;
		}
	case 1:			/* BCOND */
		switch((iw>>16) & 0x1F){
		case 0x10:	/* BLTZAL */
			ur->r31 = ur->pc + 8;
			/* fall through */
		case 0x00:	/* BLTZ */
			if((long)rs < 0)
				return ur->pc+4 + offset;
			return ur->pc + 8;
		case 0x11:	/* BGEZAL */
			ur->r31 = ur->pc + 8;
			/* fall through */
		case 0x01:	/* BGEZ */
			if((long)rs >= 0)
				return ur->pc+4 + offset;
			return ur->pc + 8;
		default:
			return 0;
		}
	case 3:			/* JAL */
		ur->r31 = ur->pc+8;
		/* fall through */
	case 2:			/* JMP */
		npc = iw & ((1<<26)-1);
		npc <<= 2;
		return npc | (ur->pc&0xF0000000);
	case 4:			/* BEQ */
		if(rs == rt)
			return ur->pc+4 + offset;
		return ur->pc + 8;
	case 5:			/* BNE */
		if(rs != rt)
			return ur->pc+4 + offset;
		return ur->pc + 8;
	case 6:			/* BLEZ */
		if((long)rs <= 0)
			return ur->pc+4 + offset;
		return ur->pc + 8;
	case 7:			/* BGTZ */
		if((long)rs > 0)
			return ur->pc+4 + offset;
		return ur->pc + 8;
	}
	/*
	 * Floating point unit jumps
	 */
	if((iw>>26) == 0x11)	/* COP1 */
		switch((iw>>16) & 0x3C1){
		case 0x101:	/* BCT */
		case 0x181:	/* BCT */
			if(fcr31 & (1<<23))
				return ur->pc+4 + offset;
			return ur->pc + 8;
		case 0x100:	/* BCF */
		case 0x180:	/* BCF */
			if(!(fcr31 & (1<<23)))
				return ur->pc+4 + offset;
			return ur->pc + 8;
		}
	/* shouldn't get here */
	return 0;
}
