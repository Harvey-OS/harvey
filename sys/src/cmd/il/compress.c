#include	"l.h"

#define COMPREG(r) ((r & ~0x7) == 0x8)
#define COP_CR(op,rs,rd)\
	(op | (rs)<<2 | (rd)<<7)
#define COP_CI(op,rd,i)\
	(op | (rd)<<7 | ((i)&0x20)<<7 | ((i)&0x1F)<<2)
#define COP_CIW(op,rd,i)\
	(op | (rd)<<2 | (i)<<5)
#define COP_CJ(op,i)\
	(op | (i)<<2)
#define COP_CB(op,rs,i)\
	(op | (rs)<<7 | ((i)&0x1f)<<2 | ((i)&0xE0)<<5)
#define COP_CL(op,rs,rd,i)\
	(op | (rs)<<7 | (rd)<<2 | ((i)&0x7)<<4 | ((i)&0x38)<<7)
#define COP_CSS(op,rs,i)\
	(op | (rs)<<2 | ((i)&0x3F)<<7)
#define COP_CS(op,rs2,rs1,i)\
	(op | (rs2)<<2 | (rs1)<<7 | ((i)&0x7)<<4 | ((i)&0x38)<<7)

static int
immbits(int v, char *permute)
{
	int r, i, c;

	r = 0;
	for(i = 0; (c = *permute++) != 0; i++)
		r |= ((v>>c) & 0x01) << i;
	return r;
}

long
asmcjmp(int a, Prog *p, int first)
{
	long v;

	if(p->cond == P)
		v = 0;
	else
		v = (p->cond->pc - p->pc);
	if(first || ((v&0x01) == 0 && v >= -0x800 && v < 0x800))
		return COP_CJ(a, immbits(v, "\5\1\2\3\7\6\12\10\11\4\13"));
	return 0;
}

int
asmcbz(int a, Prog *p, int first)
{
	long v;
	int r;

	r = p->reg;
	if(r == REGZERO || r == NREG)
		r = p->from.reg;
	else if(p->from.reg != REGZERO)
		return 0;
	if(!COMPREG(r))
		return 0;
	if(p->cond == P)
		v = 0;
	else
		v = (p->cond->pc - p->pc);
	if(first || ((v&0x01) == 0 && v >= -0x100 && v < 0x100))
		return COP_CB(a, r&0x7, immbits(v, "\5\1\2\6\7\3\4\10"));
	return 0;
}

int
asmcload(Prog *p, int a, uint len, uint maxoff)
{
	int v;
	int r;

	v = regoff(&p->from);
	if((v & (len-1)) != 0 || v < 0)
		return 0;
	r = classreg(&p->from);
	if(COMPREG(r) && COMPREG(p->to.reg)){
		if(v < maxoff){
			v |= (v>>5) & ~0x1;
			return COP_CL(a, r&0x7, p->to.reg&0x7, v);
		}
	}else if(r == REGSP){
		if(v < 2*maxoff){
			v |= (v>>6);
			return COP_CI(a | 0x2, p->to.reg, v);
		}
	}
	return 0;
}

int
asmcstore(Prog *p, int a, uint len, uint maxoff)
{
	int v;
	int r;

	v = regoff(&p->to);
	if((v & (len-1)) != 0 || v < 0)
		return 0;
	r = classreg(&p->to);
	if(COMPREG(r) && COMPREG(p->from.reg)){
		if(v < maxoff){
			v |= (v>>5) & ~0x1;
			return COP_CS(a, p->from.reg&0x7, r&0x7, v);
		}
	}else if(r == REGSP){
		if(v < 2*maxoff){
			v |= (v>>6);
			return COP_CSS(a | 0x2, (p->from.reg&0x1F), v);
		}
	}
	return 0;
}

int
asmcompressed(Prog *p, Optab *o, int r, int first)
{
	long v;
	int a;

	switch(o->ctype){
	case 1: /* C.ADD */
		if(p->from.reg != REGZERO && p->to.reg != REGZERO && r == p->to.reg)
			return COP_CR(0x9002, p->from.reg, r);
		break;
	case 2:	/* C.MV */
		if(p->from.type != D_REG)
			diag("compress MOVW R,R doesn't apply\n%P", p);
		if(p->to.reg != REGZERO){
			if(p->from.type == D_REG && p->from.reg != REGZERO)
				return COP_CR(0x8002, p->from.reg, p->to.reg);
			else
				return COP_CI(0x4001, p->to.reg, 0);
		}
		break;
	case 3:	/* C.JALR */
		if(r == REGLINK && p->to.reg != REGZERO && p->to.offset == 0)
			return COP_CR(0x9002, 0, p->to.reg);
		break;
	case 4:	/* C.JR */
		if(p->to.reg != REGZERO &&p->to.offset == 0)
			return COP_CR(0x8002, 0, p->to.reg);
		break;
	case 5:	/* C.AND C.OR C.SUB C.XOR */
		if(r == p->to.reg && COMPREG(p->from.reg) && COMPREG(p->to.reg)){
			v = 0x8C01;
			switch(o->as){
			case AXOR:
				v |= 1<<5;
				break;
			case AOR:
				v |= 2<<5;
				break;
			case AAND:
				v |= 3<<5;
				break;
			}
			return COP_CR(v, p->from.reg&0x7, p->to.reg&0x7);
		}
		break;
	case 6:	/* C.LI */
		v = p->from.offset;
		if(p->to.reg != REGZERO && v >= -0x20 && v < 0x20){
			return COP_CI(0x4001, p->to.reg, v);
		}
		break;
	case 7: /* C.LUI */
		v = p->from.offset;
		if((v&0xFFF) != 0)
			return 0;
		v >>= 12;
		if(p->to.reg != REGZERO && p->to.reg != REGSP && v >= -0x20 && v < 0x20)
			return COP_CI(0x6001, p->to.reg, v);
		break;
	case 8: /* C.SLLI */
		v = p->from.offset;
		if((v & 0x20) != 0 && thechar == 'i')
			break;
		if(r == p->to.reg && r != REGZERO && v != 0 && (v & ~0x3F) == 0)
			return COP_CI(0x0002, p->to.reg, v);
		break;
	case 9:	/* C.SRAI C.SRLI */
		v = p->from.offset;
		if((v & 0x20) != 0 && thechar == 'i')
			break;
		a = (o->as == ASRA) << 10;
		if(r == p->to.reg && COMPREG(r) && v != 0 && (v & ~0x3F) == 0)
			return COP_CI(0x8001 | a, r&0x7, v);
		break;
	case 10: /* C.ADDI C.ADDI16SP C.ADDI4SPN C.NOP */
		v = p->from.offset;
		if(r == p->to.reg && r != REGZERO && v != 0 && v >= -0x20 && v < 0x20)
			return COP_CI(0x0001, p->to.reg, v);
		if(r == p->to.reg && r == REGSP && v != 0 && (v&0xF) == 0 && v >= -0x200 && v < 0x200)
			return COP_CI(0x6001, REGSP, immbits(v, "\5\7\10\6\4\11"));
		if(r == REGSP && COMPREG(p->to.reg) && (v&0x3) == 0 && v > 0 && v < 0x400)
			return COP_CIW(0x0000, p->to.reg&0x7, immbits(v, "\3\2\6\7\10\11\4\5"));
		if(r == p->to.reg && r == REGZERO && v == 0)
			return COP_CI(0x0001, 0, 0);
		break;
	case 11: /* C.JAL (rv32) */
		if(thechar != 'i')
			break;
		if(r == REGLINK)
			return asmcjmp(0x2001, p, first);
		break;
	case 12: /* C.J */
		return asmcjmp(0xA001, p, first);
		break;
	case 13: /* C.ANDI */
		v = p->from.offset;
		if(r == p->to.reg && COMPREG(r) && v >= -0x20 && v < 0x20)
			return COP_CI(0x8801, r&0x7, v);
		break;
	case 14: /* C.BEQZ */
		return asmcbz(0xC001, p, first);
	case 15: /* C.BNEZ */
		return asmcbz(0xE001, p, first);
	case 16: /* C.LW C.LWSP */
		return asmcload(p, 0x4000, 4, 0x80);
	case 17: /* C.FLW, C.FLWSP (rv32) */
		if(thechar != 'i')
			break;
		return asmcload(p, 0x6000, 4, 0x80);
	case 18: /* C.FLD, C.FLDSP */
		return asmcload(p, 0x2000, 8, 0x100);
	case 19: /* C.SW C.SWSP */
		return asmcstore(p, 0xC000, 4, 0x80);
	case 20: /* C.FSW, C.FSWSP (rv32) */
		if(thechar != 'i')
			break;
		return asmcstore(p, 0xE000, 4, 0x80);
	case 21: /* C.FSD, C.FSDSP */
		return asmcstore(p, 0xA000, 8, 0x100);
	case 22: /* C.ADDW C.SUBW (rv64) */
		if(thechar != 'j')
			break;
		if(r == p->to.reg && COMPREG(p->from.reg) && COMPREG(p->to.reg)){
			v = 0x9C01;
			switch(o->as){
			case AADDW:
				v |= 1<<5;
				break;
			}
			return COP_CR(v, p->from.reg&0x7, p->to.reg&0x7);
		}
		break;
	case 23: /* C.ADDIW (rv64) */
		if(thechar != 'j')
			break;
		v = p->from.offset;
		if(p->as == AMOVW){
			v = 0;
			r = p->from.reg;
		}
		if(r == p->to.reg && r != REGZERO && v >= -0x20 && v < 0x20)
			return COP_CI(0x2001, p->to.reg, v);
		break;
	case 24: /* C.LD (rv64) */
		if(thechar != 'j')
			break;
		return asmcload(p, 0x6000, 8, 0x100);
	case 25: /* C.SD (rv64) */
		if(thechar != 'j')
			break;
		return asmcstore(p, 0xE000, 8, 0x100);
	}
	return 0;
}
