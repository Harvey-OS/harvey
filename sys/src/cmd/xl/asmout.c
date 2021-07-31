#include "l.h"

char ccmap[CCEND+1] =
{
	[CCXXX] 	0x01,		/* true */
	[CCTRUE]	0x01,
	[CCFALSE]	0x00,
	[CCEQ]		0x05,
	[CCNE]		0x04,
	[CCGT]		0x0c,
	[CCLE]		0x0d,
	[CCLT]		0x0b,
	[CCGE]		0x0a,
	[CCHI]		0x0e,
	[CCLS]		0x0f,
	[CCCC]		0x08,
	[CCCS]		0x09,
	[CCMI]		0x03,
	[CCPL]		0x02,
	[CCOC]		0x06,
	[CCOS]		0x07,
	[CCFNE]		0x14,
	[CCFEQ]		0x15,
	[CCFGE]		0x12,
	[CCFLT]		0x13,
	[CCFOC]		0x16,
	[CCFOS]		0x17,
	[CCFUC]		0x10,
	[CCFUS]		0x11,
	[CCFGT]		0x18,
	[CCFLE]		0x19,
	[CCIBE]		0x20,
	[CCIBF]		0x21,
	[CCOBE]		0x23,
	[CCOBF]		0x22,
	[CCSYC]		0x28,
	[CCSYS]		0x29,
	[CCFBC]		0x2a,
	[CCFBS]		0x2b,
	[CCIR0C]	0x2c,
	[CCIR0S]	0x2d,
	[CCIR1C]	0x2e,
	[CCIR1S]	0x2f,
	[CCEND]		0x1f,		/* reserved */
};

char regmap[NREG+1] =
{
	[0]		0,
	[1]		1,
	[2]		2,
	[3]		3,
	[4]		4,
	[5]		5,
	[6]		6,
	[7]		7,
	[8]		8,
	[9]		9,
	[10]		10,
	[11]		11,
	[12]		12,
	[13]		13,
	[14]		14,
	[15]		17,
	[16]		18,
	[17]		19,
	[18]		20,
	[19]		21,
	[20]		24,
	[21]		25,
	[22]		26,
	[REGPC]		15,
	[REGPCSH]	30,
	[REGPOS]	23,
	[REGNEG]	22,
	[NREG]		0,	/* a C_ZCON has NREG for it's register */
};

#define	OP16(x)	(0x00000000|((x)<<21))
#define	OP32(x)	(0x80000000|((x)<<21))
#define OPMV(x)	(0x00000000|((x)<<21))
#define OPSP(x)	(0x78000000|((x)<<23))
#define OPMA(x) (0x00000000|((x)<<23))
#define OPDO(x)	(0x8c000000|((x)<<23))
#define	OPMADD	0x20000000
#define	OPMACC	0x60000000
#define	OPMADDT	0x38000000
#define	OPMACCT	0x40000000
#define FONE		5
#define FZERO		4
#define	FREGZERO	0		/* F0 */
#define	FNOSTORE	((0<<3)|7)
#define TOREG	(0<<24)
#define	TOMEM	(1<<24)

#define	OPADD32		0x94000000
#define	OPADD16		0x14000000
#define	OPCALL		0x10000000
#define	OPSHOR		0x90000000
#define	OPDBRA		0x0c000000
#define	OPI24		0xc0000000
#define	OPCALL24	0xe0000000
#define	OPJMP24		0xa0000000

#define	OP_MVR(op,p,i,d)\
	(0x9c000000 | (op) |\
	(regmap[p] << 11) |\
	(regmap[i] << 0) |\
	(regmap[d] << 16))
#define	OP_MVI(op,i,d)\
	(0x1c000000 | (op) |\
	(((i) & 0xffff) << 0) |\
	(regmap[d] << 16))
#define	OP_MVCM(op,p,i,c)\
	(0x9e000000 | (op) |\
	(regmap[p] << 11) |\
	(regmap[i] << 0) |\
	(((c)&0xf) << 16))
#define	OP_MVCR(op,c,d)\
	(0x9c000400 | (op) |\
	(((c)&0xf) << 0) |\
	(regmap[d] << 16))
#define	OP_CRRR(op,cc,r1,r2,r3)\
	(0x18000000 | (op) |\
	(ccmap[cc] << 5) |\
	(regmap[r1] << 0) |\
	(regmap[r2] << 11) |\
	(regmap[r3] << 16))
#define	OP_RRR(op,r1,r2,r3)\
	(0x18000020 | (op) |\
	(regmap[r1] << 0) |\
	(regmap[r2] << 11) |\
	(regmap[r3] << 16))
#define	OP_IR(op,i,r3) \
	(0x1a000000 | (op) |\
	(((i) & 0xffff) << 0) |\
	(regmap[r3] << 16))
#define	OP_IRR(op,i,r2,r3)\
	((op) |\
	(((i)&0xffffL) << 0) |\
	(regmap[r2] << 16) |\
	(regmap[r3] << 21))
#define	OP_I24R(op,i,r3)\
	((op) |\
	(((i)&0xffff) << 0) |\
	((((i)>>16)&0xff) << 21) |\
	(regmap[r3] << 16))
#define	OP_BRA(op,cc,i,r)\
	((op) |\
	(ccmap[cc] << 21) |\
	(((i)&0xffff) << 0) |\
	(regmap[r] << 16))
#define	OP_FLT(op,x,y,fs,fd,z)\
	((op) |\
	((x) << 14) |\
	((y) << 7) |\
	(((fs)&0x7) << 26) |\
	(((fd)&0x3) << 21) |\
	((z) << 0))
#define OP_DO(op,k,r)\
	((op) | 0x02000000 |\
	(((k)&0x7f) << 11) |\
	(regmap[r] << 0))
#define OP_DOI(op,k,n)\
	((op) |\
	(((k)&0x7f) << 11) |\
	(((n)&0x3ff) << 0))

int
asmout(Prog *p, Optab *o, int aflag)
{
	long o1, o2, o3, o4, o5, v, op;
	Prog *ct;
	int r, s, nop, fr, tr;

	o1 = 0;
	o2 = 0;
	o3 = 0;
	o4 = 0;
	o5 = 0;
	nop = 0;

	switch(o->type) {
	default:
		if(aflag)
			return 0;
		diag("unknown type %d\n", o->type);
		if(!debug['a'])
			prasm(p);
		break;

	case 0:		/* pseudo ops */
		if(aflag) {
			if(p->link) {
				if(p->as == ATEXT) {
					ct = curtext;
					curtext = p;
					o1 = asmout(p->link, oplook(p->link), aflag);
					curtext = ct;
				} else
					o1 = asmout(p->link, oplook(p->link), aflag);
			}
			return o1;
		}
		break;

	case 1:		/* mov ind, r */
		v = increg(&p->from);
		o1 = OP_MVR(opcode(p->as)|TOREG, p->from.reg, v, p->to.reg);
		break;
	case 2:		/* mov sext, r */
		v = regoff(p, &p->from);
		o1 = OP_MVI(opcode(p->as)|TOREG, v, p->to.reg);
		break;
	case 3:		/* mov mext,r */
		o1 = setmcon(p, &p->from, REGTMP);
		o2 = OP_MVR(opcode(p->as)|TOREG, REGTMP, REGZERO, p->to.reg);
		break;
	case 4:		/* mov lext,r */
		v = regoff(p, &p->from);
		o1 = OP_I24R(OPI24, v & 0xffff, REGTMP);
		o2 = OP_IRR(OPSHOR, v >> 16, REGTMP, REGTMP);
		o3 = OP_MVR(opcode(p->as)|TOREG, REGTMP, REGZERO, p->to.reg);
		break;
	case 5:		/* mov r,ind */
		if(p->as == AMOVB)
			p->as = AMOVBU;
		v = increg(&p->to);
		o1 = OP_MVR(opcode(p->as)|TOMEM, p->to.reg, v, p->from.reg);
		break;
	case 6:		/* mov r,sext */
		if(p->as == AMOVB)
			p->as = AMOVBU;
		v = regoff(p, &p->to);
		o1 = OP_MVI(opcode(p->as)|TOMEM, v, p->from.reg);
		break;
	case 7:		/* mov r,mext */
		if(p->as == AMOVB)
			p->as = AMOVBU;
		o1 = setmcon(p, &p->to, REGTMP);
		o2 = OP_MVR(opcode(p->as)|TOMEM, REGTMP, REGZERO, p->from.reg);
		break;
	case 8:		/* mov r,lext */
		if(p->as == AMOVB)
			p->as = AMOVBU;
		v = regoff(p, &p->to);
		o1 = OP_I24R(OPI24, v & 0xffff, REGTMP);
		o2 = OP_IRR(OPSHOR, v >> 16, REGTMP, REGTMP);
		o3 = OP_MVR(opcode(p->as)|TOMEM, REGTMP, REGZERO, p->from.reg);
		break;
	case 9:		/* move ind, creg */
		if(p->as == AMOVB)
			p->as = AMOVBU;
		v = increg(&p->from);
		r = p->from.reg;
		o1 = OP_MVCM(opcode(p->as)|TOREG, r, v, p->to.reg);
		break;
	case 10:	/* move reg, creg */
		if(p->as == AMOVB)
			p->as = AMOVBU;
		o1 = OP_MVCR(opcode(p->as)|TOMEM, p->to.reg, p->from.reg);
		break;
	case 11:	/* move creg, ind */
		v = increg(&p->to);
		r = p->to.reg;
		o1 = OP_MVCM(opcode(p->as)|TOMEM, r, v, p->from.reg);
		break;
	case 12:	/* move creg, reg */
		o1 = OP_MVCR(opcode(p->as)|TOREG, p->from.reg, p->to.reg);
		break;
	case 13:	/* mov r1,r2 ==> add $0,r1,r2 */
		if(p->from.reg != REGPC && p->from.reg != REGPCSH){
			r = AADD;
			if(p->as == AMOVH)
				r = AADDH;
			o1 = OP_RRR(opcode(r), REGZERO, p->from.reg, p->to.reg);
			break;
		}
		r = OPADD32;
		if(p->as == AMOVH)
			r = OPADD16;
		o1 = OP_IRR(r, 0, p->from.reg, p->to.reg);
		break;
	case 14:	/* movsu r1,r2 ==> sras r0,r1,r2 */
		o1 = OP_RRR(opcode(ASRLH), REGZERO, p->from.reg, p->to.reg);
		break;
	case 15:	/* movb r,r */
		o1 = OP_RRR(opcode(AADD), p->from.reg, REGZERO, p->to.reg);
		o2 = OP_IR(opcode(ASLL), 8, p->to.reg);
		o3 = OP_IR(opcode(ASRAH), 8, p->to.reg);
		break;
	case 16:	/* movbu r, r */
		o1 = OP_RRR(opcode(AOR), p->from.reg, REGZERO, p->to.reg);
		if(p->as == AMOVBU)
			o2 = OP_IR(opcode(AAND), 0xff, p->to.reg);
		else	/* AMOVHB */
			o2 = OP_IR(opcode(ASRLH), 8, p->to.reg);
		break;
	case 17:	/* mov $scon,r ==> adds scon,rx,r */
		r = p->from.reg;
		if(r == NREG)
			r = o->param;
		v = regoff(p, &p->from);
		o1 = OP_IRR(OPADD32, v, r, p->to.reg);
		break;
	case 18:	/* mov $mcon,r ==> set24 mcon,r or shiftor scon, r0, r */
		o1 = setmcon(p, &p->from, p->to.reg);
		break;
	case 19:	/* mov $lcon,r */
		v = regoff(p, &p->from);
		r = p->to.reg;
		o1 = OP_I24R(OPI24, v & 0xffff, r);
		o2 = OP_IRR(OPSHOR, v >> 16, r, r);
		break;
	case 20:	/* mov $lacon,r */
		v = regoff(p, &p->from);
		r = p->to.reg;
		o1 = OP_I24R(OPI24, v & 0xffff, r);
		o2 = OP_IRR(OPSHOR, v >> 16, r, r);
		o3 = OP_RRR(opcode(AADD), o->param, r, r);
		break;
	case 21:	/* or $hcon,r1,21 => shiftor hcon,r1,r2 */
		v = regoff(p, &p->from);
		r = p->reg;
		if(r == NREG)
			r = p->to.reg;
		o1 = OP_IRR(OPSHOR, v >> 16, r, p->to.reg);
		break;
	case 30:	/* op r1,r2,r3 */
		fr = p->from.reg;
		tr = p->to.reg;
		r = p->reg;
		if(r == NREG)
			r = tr;
		if(p->as == ASUBR){
			fr = r;
			r = p->from.reg;
			p->from.reg = fr;
			p->reg = r;
			p->as = ASUB;
		}
		o1 = OP_CRRR(opcode(p->as), p->cc, fr, r, tr);
		break;
	case 31:	/* add $scon r1,r2,r3 */
		v = regoff(p, &p->from);
		r = p->reg;
		if(r == NREG)
			r = p->to.reg;
		op = OPADD32;
		if(p->as == AADDH)
			op = OPADD16;
		o1 = OP_IRR(op, v, r, p->to.reg);
		break;
	case 32:	/* op $scon,r3 */
		v = regoff(p, &p->from);
		o1 = OP_IR(opcode(p->as), v, p->to.reg);
		break;
	case 33:	/* op $mcon,r2,r3 */
		o1 = setmcon(p, &p->from, REGTMP);
		r = p->reg;
		if(r == NREG)
			r = p->to.reg;
		o2 = OP_RRR(opcode(p->as), REGTMP, r, p->to.reg);
		break;
	case 34:	/* op $lcon,r2,r3 */
		v = regoff(p, &p->from);
		o1 = OP_I24R(OPI24, v & 0xffff, REGTMP);
		o2 = OP_IRR(OPSHOR, v >> 16, REGTMP, REGTMP);
		r = p->reg;
		if(r == NREG)
			r = p->to.reg;
		o3 = OP_RRR(opcode(p->as), REGTMP, r, p->to.reg);
		break;
	case 35:	/* add $mcon,r2,r3 */
		o1 = setmcon(p, &p->from, REGTMP);
		r = p->reg;
		if(r == NREG)
			r = p->to.reg;
		o2 = OP_RRR(opcode(p->as), REGTMP, r, p->to.reg);
		break;
	case 36:	/* add $lcon, regpc, r3 */
		v = regoff(p, &p->from);
		r = p->reg;
		if(r == NREG)
			r = p->to.reg;
		s = p->to.reg;
		o1 = OP_IRR(OPADD32, v & 0xffff, r, s);
		o2 = OP_IRR(OPSHOR, topcon(v), REGZERO, REGTMP);
		o3 = OP_RRR(opcode(p->as), REGTMP, s, s);
		break;
	case 37:	/* cmp r1,r2 */
		o1 = OP_CRRR(opcode(p->as), p->cc, p->to.reg, p->from.reg, REGZERO);
		break;
	case 38:	/* cmp r,$scon */
		v = regoff(p, &p->to);
		o1 = OP_IR(opcode(p->as), v, p->from.reg);
		break;
	case 40:	/* cmp r,$mcon */
		o1 = setmcon(p, &p->to, REGTMP);
		o2 = OP_RRR(opcode(p->as), REGTMP, p->from.reg, REGZERO);
		break;
	case 41:	/* cmp $mcon,r */
		o1 = setmcon(p, &p->from, REGTMP);
		o2 = OP_RRR(opcode(p->as), p->to.reg, REGTMP, REGZERO);
		break;
	case 42:	/* cmp r,$lcon ==> sub $lcon,r,r0 */
		v = regoff(p, &p->to);
		o1 = OP_I24R(OPI24, v & 0xffff, REGTMP);
		o2 = OP_IRR(OPSHOR, v >> 16, REGTMP, REGTMP);
		o3 = OP_RRR(opcode(p->as), REGTMP, p->from.reg, REGZERO);
		break;
	case 43:	/* cmp $lcon,r ==> sub r,$lcon,r0 */
		v = regoff(p, &p->from);
		o1 = OP_I24R(OPI24, v & 0xffff, REGTMP);
		o2 = OP_IRR(OPSHOR, v >> 16, REGTMP, REGTMP);
		o3 = OP_RRR(opcode(p->as), p->to.reg, REGTMP, REGZERO);
		break;
	case 44:	/* ror r1, r2 */
		r = p->from.reg;
		o1 = OP_CRRR(opcode(p->as), p->cc, r, r, p->to.reg);
		break;
	case 50:	/* bra cc,scon(reg) or bra cc,sbra */
		if(aflag)
			return 0;
		v = regoff(p, &p->to);
		r = p->to.reg;
		if(r == NREG)
			r = REGPC;
		if(p->as == AJMP){
			if(v <= BIG-4 && (o2 = grabnop(p))){
				nop = 4;
				v += 4;
			}
		}else if(v <= BIG-4 && (o2 = grabccnop(p, p->cc))){
			nop = 4;
			v += 4;
		}
		o1 = OP_BRA(opcode(p->as), p->cc, v, r);
		break;
	case 51:	/* jmp mcon(reg) or jmp mbra */
		if(aflag)
			return 0;
		v = regoff(p, &p->to);
		r = p->to.reg;
		if(r == NREG)
			r = REGZERO;
		if(v <= BIGM-4 && (o2 = grabnop(p))){
			nop = 4;
			v += 4;
		}
		o1 = OP_I24R(OPJMP24, v, r);
		break;
	case 52:	/* bra cc,lcon(reg) */
		if(aflag)
			return 0;
		v = regoff(p, &p->to);
		o1 = OP_IRR(OPSHOR, topcon(v), REGZERO, REGTMP);
		o2 = OP_RRR(opcode(AADD), p->to.reg, REGTMP, REGTMP);
		o3 = OP_BRA(opcode(p->as), p->cc, v & 0xffff, REGTMP);
		break;
	case 53:	/* bra cc,mbra */
		if(aflag)
			return 0;
		v = regoff(p, &p->to);
		if(v <= BIGM-4 && (o4 = grabnop(p))){
			nop = 4;
			v += 4;
		}
		r = 4;
		if(o2 = grabccnop(p, relinv(p->cc)))
			r += 4;
		else
			o2 = OP_BRA(opcode(ABRA), CCFALSE, 0, REGZERO);
		o1 = OP_BRA(opcode(p->as), relinv(p->cc), r, REGPC);
		o3 = OP_I24R(OPJMP24, v, REGZERO);
		break;
	case 54:	/* bra cc,lbra */
		/*
		 * be careful here.  the compiler assumes
		 * branches don't set condition codes
		 * can probably push them to stack
		 */
		diag("cannot do a 32 bit branch\n");
		break;

	case 55:	/* call r,scon(reg) or call r,sbra */
		if(aflag)
			return 0;
		v = regoff(p, &p->to);
		s = p->reg;
		if(s == NREG)
			s = o->param;
		r = p->to.reg;
		if(r == NREG)
			r = REGPC;
		if(v <= BIG-4 && (o2 = grabnop(p))){
			nop = 4;
			v += 4;
		}
		o1 = OP_IRR(OPCALL, v, r, s);
		break;
	case 57:	/* call r,mbra */
		if(aflag)
			return 0;
		v = regoff(p, &p->to);
		r = p->to.reg;
		if(r == NREG)
			r = o->param;
		if(v <= BIGM && (o2 = grabnop(p))){
			nop = 4;
			v += 4;
		}
		o1 = OP_I24R(OPCALL24, v, r);
		break;
	case 58:	/* call r,lcon(reg) */
		if(aflag)
			return 0;
		v = regoff(p, &p->to);
		r = p->reg;
		if(r == NREG)
			r = o->param;
		o1 = OP_IRR(OPSHOR, topcon(v), REGZERO, REGTMP);
		o2 = OP_RRR(opcode(AADD), p->to.reg, REGTMP, REGTMP);
		o3 = OP_IRR(OPCALL, v & 0xffff, REGTMP, r);
		break;
	case 59:	/* call r,lbra */
		if(aflag)
			return 0;
		v = regoff(p, &p->to);
		r = p->reg;
		if(r == NREG)
			r = o->param;
		/*
		 * use r as a temp
		 * REGTMP is to pass an argument mul & div
		 */
		if(o3 = grabnop(p)){
			nop = 4;
			v += 4;
		}
		o1 = OP_IRR(OPSHOR, topcon(v), REGZERO, r);
		o2 = OP_IRR(OPCALL, v & 0xffff, r, r);
		break;
	case 60:	/* dbra reg,scon(reg) */
		if(aflag)
			return 0;
		v = regoff(p, &p->to);
		o1 = OP_IRR(OPDBRA, v, p->to.reg, p->from.reg);
		break;
	case 61:	/* dbra reg,sbra */
		if(aflag)
			return 0;
		v = regoff(p, &p->to);
		o1 = OP_IRR(OPDBRA, v, REGPC, p->from.reg);
		break;
	case 62:	/* dbra reg,lcon(reg) */
		if(aflag)
			return 0;
		v = regoff(p, &p->to);
		o1 = OP_IRR(OPSHOR, topcon(v), REGZERO, REGTMP);
		o2 = OP_RRR(opcode(AADD), p->to.reg, REGTMP, REGTMP);
		o3 = OP_IRR(OPDBRA, v & 0xffff, REGTMP, p->from.reg);
		break;
	case 64:	/* do r, $c */
		if(aflag)
			return 0;
		v = regoff(p, &p->to);
		if(v > 127 || v < 0)
			diag("illegal do loop length for %P: %d\n", p, v);
		o1 = OP_DO(opcode(p->as), v, p->from.reg);
		break;
	case 65:	/* do $i, $c */
		if(aflag)
			return 0;
		v = regoff(p, &p->to);
		if(v > 127 || v < 0)
			diag("illegal do loop length for %P: %d\n", p, v);
		r = regoff(p, &p->from);
		o1 = OP_DOI(opcode(p->as), v, r);
		break;
	case 66:	/* do r, sbra */
		if(aflag)
			return 0;
		if(!p->cond)
			v = 0;
		else
			v = (p->cond->pc - p->pc) / 4 - 2;
		if(v > 127 || v < 0)
			diag("illegal do loop length for %P: %d\n", p, v);
		o1 = OP_DO(opcode(p->as), v, p->from.reg);
		break;
	case 67:	/* do $i, sbra */
		if(aflag)
			return 0;
		if(!p->cond)
			v = 0;
		else
			v = (p->cond->pc - p->pc) / 4 - 2;
		if(v > 127 || v < 0)
			diag("illegal do loop length for %P: %d\n", p, v);
		v = regoff(p, &p->to);
		r = regoff(p, &p->from);
		o1 = OP_DOI(opcode(p->as), v, r);
		break;
	case 70:	/* sftrst ==> MOVB r0,ior10 or waiti ==> MOVW r0,ior10 */
		if(aflag)
			return 0;
		o1 = OP_MVCR(opcode(p->as)|TOMEM, 10, REGZERO);
		break;
	case 71:	/* iret ==> BRA TRUE,0(pcsh) */
		if(aflag)
			return 0;
		o1 = OP_BRA(opcode(ABRA), CCTRUE, 0, REGPCSH);
		break;
	case 72:	/* word */
		if(aflag)
			return 0;
		o1 = regoff(p, &p->from);
		break;
	case 80:	/* rnd y,f,z */
		o1 = OP_FLT(opcode(p->as),
			0, fltarg(&p->from, 0),
			0, p->reg, fltarg(&p->to, 1));
		break;
	case 81:	/* add x,y,f,z => madd x,y,1,f,z */
		o1 = OP_FLT(opcode(p->as)|OPMADD,
			fltarg(&p->from, 0), fltarg(&p->from1, 0),
			FONE, p->reg, fltarg(&p->to, 1));
		break;
	case 82:	/* addt x,y,f,z => maddt x,y,1,f,z */
		o1 = OP_FLT(opcode(p->as)|OPMADDT,
			fltarg(&p->from, 0), fltarg(&p->from1, 0),
			0, p->reg, fltarg(&p->to, 1));
		break;
	case 83:	/* mull x,y,f,z => macc x,y,0,f,z */
		o1 = OP_FLT(opcode(p->as)|OPMACC,
			fltarg(&p->from, 0), fltarg(&p->from1, 0),
			FZERO, p->reg, fltarg(&p->to, 1));
		break;
	case 84:	/* mult x,y,f,z => macct x,y,0,f,z */
		o1 = OP_FLT(opcode(p->as)|OPMACCT,
			fltarg(&p->from, 0), fltarg(&p->from1, 0),
			FZERO, p->reg, fltarg(&p->to, 1));
		break;
	case 85:	/* madd x,y,f1,f2,z => macc x,y,f1,f2,z */
		o1 = OP_FLT(opcode(p->as)|OPMACC,
			fltarg(&p->from, 0), fltarg(&p->from1, 0),
			p->from2.reg, p->reg, fltarg(&p->to, 1));
		break;
	case 86:	/* madd x,f1,y,f2,z => madd x,y,f1,f2,z */
		o1 = OP_FLT(opcode(p->as)|OPMADD,
			fltarg(&p->from, 0), fltarg(&p->from2, 0),
			p->from1.reg, p->reg, fltarg(&p->to, 1));
		break;
	case 87:	/* madd f1,x,y,f2,z => madd x,y,f1,f2,z */
		o1 = OP_FLT(opcode(p->as)|OPMADD,
			fltarg(&p->from1, 0), fltarg(&p->from2, 0),
			p->from.reg, p->reg, fltarg(&p->to, 1));
		break;
	case 88:	/* maddt x,y,f1,f2,z => macct x,y,f1,f2,z */
		o1 = OP_FLT(opcode(p->as)|OPMACCT,
			fltarg(&p->from, 0), fltarg(&p->from1, 0),
			p->from2.reg, p->reg, fltarg(&p->to, 1));
		break;
	case 95:	/* fmov x,f,z => madd f0,0,x,f,z */
		o1 = OP_FLT(opcode(p->as)|OPMADD,
			0, fltarg(&p->from, 0),
			FZERO, p->reg, fltarg(&p->to, 1));
		break;
	case 98:	/* bmovw $c,a,b => doblock $c; mult f0,a,f0,b */
		v = regoff(p, &p->from);
		o1 = OP_DOI(opcode(p->as), 0, v);
		o2 = OP_FLT(opcode(AFMULT)|OPMACCT,
			FREGZERO, fltarg(&p->from1, 0),
			FZERO, 0, fltarg(&p->to, 1));
		break;
	case 99:	/* bmovw r,a,b => doblock r; mult f0,a,f0,b */
		o1 = OP_DO(opcode(p->as), 0, p->from.reg);
		o2 = OP_FLT(opcode(AFMULT)|OPMACCT,
			FREGZERO, fltarg(&p->from1, 0),
			FZERO, 0, fltarg(&p->to, 1));
		break;
	}
	if(aflag)
		return o1;
	v = p->pc;
	switch(o->size + nop) {
	default:
		if(debug['a'])
			Bprint(&bso, " %.8lux:\t\t%P\n", v, p);
		break;
	case 4:
		if(debug['a'])
			Bprint(&bso, " %.8lux: %.8lux\t%P\n", v, o1, p);
		codeput(o1);
		break;
	case 8:
		if(debug['a'])
			Bprint(&bso, " %.8lux: %.8lux %.8lux%P\n", v, o1, o2, p);
		codeput(o1);
		codeput(o2);
		break;
	case 12:
		if(debug['a'])
			Bprint(&bso, " %.8lux: %.8lux %.8lux %.8lux%P\n", v, o1, o2, o3, p);
		codeput(o1);
		codeput(o2);
		codeput(o3);
		break;
	case 16:
		if(debug['a'])
			Bprint(&bso, " %.8lux: %.8lux %.8lux %.8lux %.8lux%P\n",
				v, o1, o2, o3, o4, p);
		codeput(o1);
		codeput(o2);
		codeput(o3);
		codeput(o4);
		break;
	case 20:
		if(debug['a'])
			Bprint(&bso, " %.8lux: %.8lux %.8lux %.8lux %.8lux %.8lux%P\n",
				v, o1, o2, o3, o4, o5, p);
		codeput(o1);
		codeput(o2);
		codeput(o3);
		codeput(o4);
		codeput(o5);
		break;
	}
	return nop;
}

/*
 * see if a nop following a BRA can be replaced with a conditional instruction
 * could try conditional jumps as well
 */
long
grabccnop(Prog *p, int cc)
{
	long op;

	if(!p->link || !p->cond || p->link->as != ABRA || p->link->cc != CCFALSE)
		return 0;
	op = asmout(p->cond, oplook(p->cond), 1);
	if((op & 0x7e000000) != 0x18000000)
		return 0;
	if(((op >> 5) & 0x3f) != 1)
		return 0;
	op &= ~(1 << 5);
	op |= ccmap[cc] << 5;
	return op;
}

/*
 * try to replace a nop following a JMP with the first instruction of the destination
 */
long
grabnop(Prog *p)
{
	if(!p->link || !p->cond || p->link->as != ABRA || p->link->cc != CCFALSE)
		return 0;

	return asmout(p->cond, oplook(p->cond), 1);
}

int
increg(Adr *a)
{
	int r;

	r = a->type;
	if(r == D_INDREG)
		return REGZERO;
	if(r == D_INC)
		return REGPOS;
	if(r == D_DEC)
		return REGNEG;
	return a->increg;
}

int
fltarg(Adr *a, int isz)
{
	switch(a->type){
	default:
		diag("unknown type %D in fltarg\n", a);
		return 0;
	case D_NONE:
		if(isz)
			return (0 << 3) | 7;
		return 0;
	case D_FREG:
		return (0 << 3) | a->reg;
	case D_INDREG:
		return (a->reg << 3) | 0;
	case D_INC:
		return (a->reg << 3) | 7;
	case D_DEC:
		return (a->reg << 3) | 6;
	case D_INCREG:
		return (a->reg << 3) | (a->increg - 14);
	}
}

/*
 * return the correct upper 16 bits of v
 * such the when added to the sign extended
 * lower 16 bits, v is produced
 */
long
topcon(long v)
{
	ulong x;

	x = v & 0xffff0000;
	if(v & 0x8000)		/* adjust for sign extend */
		x -= 0xffff0000;
	return x >> 16;
}

long
setmcon(Prog *p, Adr *a, int reg)
{
	long v;

	v = regoff(p, a);
	if(v > 0 && v <= 0xffffff)
		return OP_I24R(OPI24, v, reg);
	if(v & 0xffff)
		diag("bad const 0x%.8lux in setmcon\n", v);
	return OP_IRR(OPSHOR, v >> 16, REGZERO, reg);
}

long
opcode(int a)
{
	switch(a) {
	case AADD:
		return OP32(0x0);
	case AADDH:
		return OP16(0x0);
	case AADDCR:
		return OP32(0x3);
	case AADDCRH:
		return OP16(0x3);
	case AAND:
		return OP32(0xe);
	case AANDH:
		return OP16(0xe);
	case AANDN:
		return OP32(0x6);
	case AANDNH:
		return OP16(0x6);
	case ABIT:
		return OP32(0xf);
	case ABITH:
		return OP16(0xf);
	case ACMP:
		return OP32(0x7);
	case ACMPH:
		return OP16(0x7);
	case AOR:
		return OP32(0xa);
	case AORH:
		return OP16(0xa);
	case AROL:
		return OP32(0xb);
	case AROLH:
		return OP16(0xb);
	case AROR:
		return OP32(0x9);
	case ARORH:
		return OP16(0x9);
	case ASLL:
		return OP32(0x1);
	case ASLLH:
		return OP16(0x1);
	case ASRA:
		return OP32(0xd);
	case ASRAH:
		return OP16(0xd);
	case ASRL:
		return OP32(0xc);
	case ASRLH:
		return OP16(0xc);
	case ASUB:
		return OP32(0x4);
	case ASUBH:
		return OP16(0x4);
	case ASUBR:
		return OP32(0x2);
	case ASUBRH:
		return OP16(0x2);
	case AXOR:
		return OP32(0x8);
	case AXORH:
		return OP16(0x8);

	case AMOVB:
	case ASFTRST:		/* funny encoding */
		return OPMV(1);
	case AMOVBU:
		return OPMV(0);
	case AMOVHB:
		return OPMV(4);
	case AMOVW:
	case AWAITI:		/* funny encoding */
		return OPMV(7);
	case AMOVH:
		return OPMV(3);
	case AMOVHU:
		return OPMV(2);

	case ABRA:
	case AJMP:		/* short conditional jump */
		return 0x80000000;

	case AFIFEQ:
		return OPSP(0x6);
	case AFIFGT:
		return OPSP(0x7);
	case AFIFLT:
		return OPSP(0x5);
	case AFDSP:
		return OPSP(0xd);
	case AFIEEE:
		return OPSP(0xc);
	case AFRND:
		return OPSP(0x4);
	case AFSEED:
		return OPSP(0xe);
	case AFMOVFB:
		return OPSP(0x1);
	case AFMOVFW:
		return OPSP(0x9);
	case AFMOVFH:
		return OPSP(0x3);
	case AFMOVBF:
		return OPSP(0x0);
	case AFMOVWF:
		return OPSP(0x8);
	case AFMOVHF:
		return OPSP(0x2);
	case AFMOVF:
	case AFADD:
	case AFADDT:
	case AFMADD:
	case AFMADDT:
	case AFMUL:
	case AFMULT:
		return OPMA(0);
	case AFMOVFN:
	case AFADDN:
	case AFADDTN:
	case AFMADDN:
	case AFMADDTN:
		return OPMA(2);
	case AFSUB:
	case AFSUBT:
	case AFMSUB:
	case AFMSUBT:
	case AFMULN:
	case AFMULTN:
		return OPMA(1);
	case AFSUBN:
	case AFSUBTN:
	case AFMSUBN:
	case AFMSUBTN:
		return OPMA(3);

	case ABMOVW:
		return OPDO(1);
	case ADO:
		return OPDO(0);
	case ADOLOCK:
		return OPDO(2);
	}
	diag("bad opcode %A\n", a);
	return 0;
}
