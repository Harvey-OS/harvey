#include "l.h"

#define	OPVCC(o,xo,oe,rc) (((o)<<26)|((xo)<<1)|((oe)<<10)|((rc)&1))
#define	OPCC(o,xo,rc) OPVCC((o),(xo),0,(rc))
#define	OP(o,xo) OPVCC((o),(xo),0,0)

/* the order is dest, a/s, b/imm for both arithmetic and logical operations */
#define	AOP_RRR(op,d,a,b) ((op)|(((d)&31L)<<21)|(((a)&31L)<<16)|(((b)&31L)<<11))
#define	AOP_IRR(op,d,a,simm) ((op)|(((d)&31L)<<21)|(((a)&31L)<<16)|((simm)&0xFFFF))
#define	LOP_RRR(op,a,s,b) ((op)|(((s)&31L)<<21)|(((a)&31L)<<16)|(((b)&31L)<<11))
#define	LOP_IRR(op,a,s,uimm) ((op)|(((s)&31L)<<21)|(((a)&31L)<<16)|((uimm)&0xFFFF))
#define	OP_BR(op,li,aa) ((op)|((li)&0x03FFFFFC)|((aa)<<1))
#define	OP_BC(op,bo,bi,bd,aa) ((op)|(((bo)&0x1F)<<21)|(((bi)&0x1F)<<16)|((bd)&0xFFFC)|((aa)<<1))
#define	OP_BCR(op,bo,bi) ((op)|(((bo)&0x1F)<<21)|(((bi)&0x1F)<<16))
#define	OP_RLW(op,a,s,sh,mb,me) ((op)|(((s)&31L)<<21)|(((a)&31L)<<16)|(((sh)&31L)<<11)|\
					(((mb)&31L)<<6)|(((me)&31L)<<1))

#define	OP_ADD	OPVCC(31,266,0,0)
#define	OP_ADDI	OPVCC(14,0,0,0)
#define	OP_ADDIS OPVCC(15,0,0,0)
#define	OP_ANDI	OPVCC(28,0,0,0)
#define	OP_EXTSB	OPVCC(31,954,0,0)
#define	OP_EXTSH OPVCC(31,922,0,0)
#define	OP_MCRF	OPVCC(19,0,0,0)
#define	OP_MCRFS OPVCC(63,64,0,0)
#define	OP_MCRXR OPVCC(31,512,0,0)
#define	OP_MFCR	OPVCC(31,19,0,0)
#define	OP_MFFS	OPVCC(63,583,0,0)
#define	OP_MFMSR OPVCC(31,83,0,0)
#define	OP_MFSPR OPVCC(31,339,0,0)
#define	OP_MFSR	OPVCC(31,595,0,0)
#define	OP_MFSRIN	OPVCC(31,659,0,0)
#define	OP_MTCRF OPVCC(31,144,0,0)
#define	OP_MTFSF OPVCC(63,711,0,0)
#define	OP_MTFSFI OPVCC(63,134,0,0)
#define	OP_MTMSR OPVCC(31,146,0,0)
#define	OP_MTSPR OPVCC(31,467,0,0)
#define	OP_MTSR	OPVCC(31,210,0,0)
#define	OP_MTSRIN	OPVCC(31,242,0,0)
#define	OP_MULLW OPVCC(31,235,0,0)
#define	OP_OR	OPVCC(31,444,0,0)
#define	OP_ORI	OPVCC(24,0,0,0)
#define	OP_RLWINM	OPVCC(21,0,0,0)
#define	OP_SUBF	OPVCC(31,40,0,0)

#define	oclass(v)	((v).class-1)

long	oprrr(int), opirr(int), opload(int), opstore(int), oploadx(int), opstorex(int);

int
getmask(uchar *m, ulong v)
{
	int i;

	m[0] = m[1] = 0;
	if(v != ~0L && v & (1<<31) && v & 1){	/* MB > ME */
		if(getmask(m, ~v)){
			i = m[0]; m[0] = m[1]+1; m[1] = i-1;
			return 1;
		}
		return 0;
	}
	for(i=0; i<32; i++)
		if(v & (1<<(31-i))){
			m[0] = i;
			do {
				m[1] = i;
			} while(++i<32 && (v & (1<<(31-i))) != 0);
			for(; i<32; i++)
				if(v & (1<<(31-i)))
					return 0;
			return 1;
		}
	return 0;
}

void
maskgen(Prog *p, uchar *m, ulong v)
{
	if(!getmask(m, v))
		diag("cannot generate mask #%lux\n%P", v, p);
}

static void
reloc(Adr *a, long pc, int sext)
{
	if(a->name == D_EXTERN || a->name == D_STATIC)
		dynreloc(a->sym, pc, 1, 1, sext);
}
	
int
asmout(Prog *p, Optab *o, int aflag)
{
	long o1, o2, o3, o4, o5, v, t;
	Prog *ct;
	int r, a;
	uchar mask[2];

	o1 = 0;
	o2 = 0;
	o3 = 0;
	o4 = 0;
	o5 = 0;
	switch(o->type) {
	default:
		if(aflag)
			return 0;
		diag("unknown type %d", o->type);
		if(!debug['a'])
			prasm(p);
		break;

	case 0:		/* pseudo ops */
		if(aflag) {
			if(p->link) {
				if(p->as == ATEXT) {
					ct = curtext;
					o2 = autosize;
					curtext = p;
					autosize = p->to.offset + 4;
					o1 = asmout(p->link, oplook(p->link), aflag);
					curtext = ct;
					autosize = o2;
				} else
					o1 = asmout(p->link, oplook(p->link), aflag);
			}
			return o1;
		}
		break;

	case 1:		/* mov r1,r2 ==> OR Rs,Rs,Ra */
		if(p->to.reg == REGZERO && p->from.type == D_CONST) {
			v = regoff(&p->from);
			if(r0iszero && v != 0) {
				nerrors--;
				diag("literal operation on R0\n%P", p);
			}
			o1 = LOP_IRR(OP_ADDI, REGZERO, REGZERO, v);
			break;
		}
		o1 = LOP_RRR(OP_OR, p->to.reg, p->from.reg, p->from.reg);
		break;

	case 2:		/* int/cr/fp op Rb,[Ra],Rd */
		r = p->reg;
		if(r == NREG)
			r = p->to.reg;
		o1 = AOP_RRR(oprrr(p->as), p->to.reg, r, p->from.reg);
		break;

	case 3:		/* mov $soreg/addcon/ucon, r ==> addis/addi $i,reg',r */
		v = regoff(&p->from);
		r = p->from.reg;
		if(r == NREG)
			r = o->param;
		a = OP_ADDI;
		if(o->a1 == C_UCON) {
			a = OP_ADDIS;
			v >>= 16;
		}
		if(r0iszero && p->to.reg == 0 && (r != 0 || v != 0))
			diag("literal operation on R0\n%P", p);
		o1 = AOP_IRR(a, p->to.reg, r, v);
		break;

	case 4:		/* add/mul $scon,[r1],r2 */
		v = regoff(&p->from);
		r = p->reg;
		if(r == NREG)
			r = p->to.reg;
		if(r0iszero && p->to.reg == 0)
			diag("literal operation on R0\n%P", p);
		o1 = AOP_IRR(opirr(p->as), p->to.reg, r, v);
		break;

	case 5:		/* syscall */
		if(aflag)
			return 0;
		o1 = oprrr(p->as);
		break;

	case 6:		/* logical op Rb,[Rs,]Ra; no literal */
		r = p->reg;
		if(r == NREG)
			r = p->to.reg;
		o1 = LOP_RRR(oprrr(p->as), p->to.reg, r, p->from.reg);
		break;

	case 7:		/* mov r, soreg ==> stw o(r) */
		r = p->to.reg;
		if(r == NREG)
			r = o->param;
		v = regoff(&p->to);
		if(p->to.type == D_OREG && p->reg != NREG) {
			if(v)
				diag("illegal indexed instruction\n%P", p);
			o1 = AOP_RRR(opstorex(p->as), p->from.reg, p->reg, r);
		} else
			o1 = AOP_IRR(opstore(p->as), p->from.reg, r, v);
		break;

	case 8:		/* mov soreg, r ==> lbz/lhz/lwz o(r) */
		r = p->from.reg;
		if(r == NREG)
			r = o->param;
		v = regoff(&p->from);
		if(p->from.type == D_OREG && p->reg != NREG) {
			if(v)
				diag("illegal indexed instruction\n%P", p);
			o1 = AOP_RRR(oploadx(p->as), p->to.reg, p->reg, r);
		} else
			o1 = AOP_IRR(opload(p->as), p->to.reg, r, v);
		break;

	case 9:		/* movb soreg, r ==> lbz o(r),r2; extsb r2,r2 */
		r = p->from.reg;
		if(r == NREG)
			r = o->param;
		v = regoff(&p->from);
		if(p->from.type == D_OREG && p->reg != NREG) {
			if(v)
				diag("illegal indexed instruction\n%P", p);
			o1 = AOP_RRR(oploadx(p->as), p->to.reg, p->reg, r);
		} else
			o1 = AOP_IRR(opload(p->as), p->to.reg, r, v);
		o2 = LOP_RRR(OP_EXTSB, p->to.reg, p->to.reg, 0);
		break;

	case 10:		/* sub Ra,[Rb],Rd => subf Rd,Ra,Rb */
		r = p->reg;
		if(r == NREG)
			r = p->to.reg;
		o1 = AOP_RRR(oprrr(p->as), p->to.reg, p->from.reg, r);
		break;

	case 11:	/* br/bl lbra */
		if(aflag)
			return 0;
		v = 0;
		if(p->cond == UP){
			if(p->to.sym->type != SUNDEF)
				diag("bad branch sym type");
			v = (ulong)p->to.sym->value >> (Roffset-2);
			dynreloc(p->to.sym, p->pc, 0, 0, 0);
		}
		else if(p->cond)
			v = p->cond->pc - p->pc;
		if(v & 03) {
			diag("odd branch target address\n%P", p);
			v &= ~03;
		}
		if(v < -(1L<<25) || v >= (1L<<25))
			diag("branch too far\n%P", p);
		o1 = OP_BR(opirr(p->as), v, 0);
		break;

	case 12:	/* movb r,r (signed); extsb is on PowerPC but not POWER */
		o1 = LOP_RRR(OP_EXTSB, p->to.reg, p->from.reg, 0);
		break;

	case 13:	/* mov[bh]z r,r; uses rlwinm not andi. to avoid changing CC */
		if(p->as == AMOVBZ)
			o1 = OP_RLW(OP_RLWINM, p->to.reg, p->from.reg, 0, 24, 31);
		else if(p->as == AMOVH)
			o1 = LOP_RRR(OP_EXTSH, p->to.reg, p->from.reg, 0);
		else if(p->as == AMOVHZ)
			o1 = OP_RLW(OP_RLWINM, p->to.reg, p->from.reg, 0, 16, 31);
		else
			diag("internal: bad mov[bh]z\n%P", p);
		break;

/*14 */

	case 17:	/* bc bo,bi,lbra (same for now) */
	case 16:	/* bc bo,bi,sbra */
		if(aflag)
			return 0;
		a = 0;
		if(p->from.type == D_CONST)
			a = regoff(&p->from);
		r = p->reg;
		if(r == NREG)
			r = 0;
		v = 0;
		if(p->cond)
			v = p->cond->pc - p->pc;
		if(v & 03) {
			diag("odd branch target address\n%P", p);
			v &= ~03;
		}
		if(v < -(1L<<16) || v >= (1L<<16))
			diag("branch too far\n%P", p);
		o1 = OP_BC(opirr(p->as), a, r, v, 0);
		break;

	case 15:	/* br/bl (r) => mov r,lr; br/bl (lr) */
		if(aflag)
			return 0;
		if(p->as == ABC || p->as == ABCL)
			v = regoff(&p->to)&31L;
		else
			v = 20;	/* unconditional */
		r = p->reg;
		if(r == NREG)
			r = 0;
		o1 = AOP_RRR(OP_MTSPR, p->to.reg, 0, 0) | ((D_LR&0x1f)<<16) | (((D_LR>>5)&0x1f)<<11);
		o2 = OPVCC(19, 16, 0, 0);
		if(p->as == ABL || p->as == ABCL)
			o2 |= 1;
		o2 = OP_BCR(o2, v, r);
		break;

	case 18:	/* br/bl (lr/ctr); bc/bcl bo,bi,(lr/ctr) */
		if(aflag)
			return 0;
		if(p->as == ABC || p->as == ABCL)
			v = regoff(&p->from)&31L;
		else
			v = 20;	/* unconditional */
		r = p->reg;
		if(r == NREG)
			r = 0;
		switch(oclass(p->to)) {
		case C_CTR:
			o1 = OPVCC(19, 528, 0, 0);
			break;
		case C_LR:
			o1 = OPVCC(19, 16, 0, 0);
			break;
		default:
			diag("bad optab entry (18): %d\n%P", p->to.class, p);
			v = 0;
		}
		if(p->as == ABL || p->as == ABCL)
			o1 |= 1;
		o1 = OP_BCR(o1, v, r);
		break;

	case 19:	/* mov $lcon,r ==> cau+or */
		v = regoff(&p->from);
		o1 = AOP_IRR(OP_ADDIS, p->to.reg, REGZERO, v>>16);
		o2 = LOP_IRR(OP_ORI, p->to.reg, p->to.reg, v);
		if(dlm)
			reloc(&p->from, p->pc, 0);
		break;

	case 20:	/* add $ucon,,r */
		v = regoff(&p->from);
		r = p->reg;
		if(r == NREG)
			r = p->to.reg;
		if(p->as == AADD && (!r0iszero && p->reg == 0 || r0iszero && p->to.reg == 0))
			diag("literal operation on R0\n%P", p);
		o1 = AOP_IRR(opirr(p->as+AEND), p->to.reg, r, v>>16);
		break;

	case 22:	/* add $lcon,r1,r2 ==> cau+or+add */	/* could do add/sub more efficiently */
		v = regoff(&p->from);
		if(p->to.reg == REGTMP || p->reg == REGTMP)
			diag("cant synthesize large constant\n%P", p);
		o1 = AOP_IRR(OP_ADDIS, REGTMP, REGZERO, v>>16);
		o2 = LOP_IRR(OP_ORI, REGTMP, REGTMP, v);
		r = p->reg;
		if(r == NREG)
			r = p->to.reg;
		o3 = AOP_RRR(oprrr(p->as), p->to.reg, REGTMP, r);
		if(dlm)
			reloc(&p->from, p->pc, 0);
		break;

	case 23:	/* and $lcon,r1,r2 ==> cau+or+and */	/* masks could be done using rlnm etc. */
		v = regoff(&p->from);
		if(p->to.reg == REGTMP || p->reg == REGTMP)
			diag("cant synthesize large constant\n%P", p);
		o1 = AOP_IRR(OP_ADDIS, REGTMP, REGZERO, v>>16);
		o2 = LOP_IRR(OP_ORI, REGTMP, REGTMP, v);
		r = p->reg;
		if(r == NREG)
			r = p->to.reg;
		o3 = LOP_RRR(oprrr(p->as), p->to.reg, REGTMP, r);
		if(dlm)
			reloc(&p->from, p->pc, 0);
		break;
/*24*/

	case 26:	/* mov $lsext/auto/oreg,,r2 ==> cau+add */
		v = regoff(&p->from);
		if(v & 0x8000L)
			v += 0x10000L;
		if(p->to.reg == REGTMP)
			diag("can't synthesize large constant\n%P", p);
		r = p->from.reg;
		if(r == NREG)
			r = o->param;
		o1 = AOP_IRR(OP_ADDIS, REGTMP, r, v>>16);
		o2 = AOP_IRR(OP_ADDI, p->to.reg, REGTMP, v);
		break;

	case 27:		/* subc ra,$simm,rd => subfic rd,ra,$simm */
		v = regoff(&p->from3);
		r = p->from.reg;
		o1 = AOP_IRR(opirr(p->as), p->to.reg, r, v);
		break;

	case 28:	/* subc r1,$lcon,r2 ==> cau+or+subfc */
		v = regoff(&p->from3);
		if(p->to.reg == REGTMP || p->from.reg == REGTMP)
			diag("can't synthesize large constant\n%P", p);
		o1 = AOP_IRR(OP_ADDIS, REGTMP, REGZERO, v>>16);
		o2 = LOP_IRR(OP_ORI, REGTMP, REGTMP, v);
		o3 = AOP_RRR(oprrr(p->as), p->to.reg, p->from.reg, REGTMP);
		if(dlm)
			reloc(&p->from3, p->pc, 0);
		break;

/*29, 30, 31 */

	case 32:	/* fmul frc,fra,frd */
		r = p->reg;
		if(r == NREG)
			r = p->to.reg;
		o1 = AOP_RRR(oprrr(p->as), p->to.reg, r, 0)|((p->from.reg&31L)<<6);
		break;

	case 33:	/* fabs [frb,]frd; fmr. frb,frd */
		r = p->from.reg;
		if(oclass(p->from) == C_NONE)
			r = p->to.reg;
		o1 = AOP_RRR(oprrr(p->as), p->to.reg, 0, r);
		break;

	case 34:	/* FMADDx fra,frb,frc,frd (d=a*b+c) */
		o1 = AOP_RRR(oprrr(p->as), p->to.reg, p->from.reg, p->reg)|((p->from3.reg&31L)<<6);
		break;

	case 35:	/* mov r,lext/lauto/loreg ==> cau $(v>>16),sb,r'; store o(r') */
		v = regoff(&p->to);
		if(v & 0x8000L)
			v += 0x10000L;
		r = p->to.reg;
		if(r == NREG)
			r = o->param;
		o1 = AOP_IRR(OP_ADDIS, REGTMP, r, v>>16);
		o2 = AOP_IRR(opstore(p->as), p->from.reg, REGTMP, v);
		break;

	case 36:	/* mov bz/h/hz lext/lauto/lreg,r ==> lbz/lha/lhz etc */
		v = regoff(&p->from);
		if(v & 0x8000L)
			v += 0x10000L;
		r = p->from.reg;
		if(r == NREG)
			r = o->param;
		o1 = AOP_IRR(OP_ADDIS, REGTMP, r, v>>16);
		o2 = AOP_IRR(opload(p->as), p->to.reg, REGTMP, v);
		break;

	case 37:	/* movb lext/lauto/lreg,r ==> lbz o(reg),r; extsb r */
		v = regoff(&p->from);
		if(v & 0x8000L)
			v += 0x10000L;
		r = p->from.reg;
		if(r == NREG)
			r = o->param;
		o1 = AOP_IRR(OP_ADDIS, REGTMP, r, v>>16);
		o2 = AOP_IRR(opload(p->as), p->to.reg, REGTMP, v);
		o3 = LOP_RRR(OP_EXTSB, p->to.reg, p->to.reg, 0);
		break;

	case 40:	/* word */
		if(aflag)
			return 0;
		o1 = regoff(&p->from);
		break;

	case 41:	/* stswi */
		o1 = AOP_RRR(opirr(p->as), p->from.reg, p->to.reg, 0) | ((regoff(&p->from3)&0x7F)<<11);
		break;

	case 42:	/* lswi */
		o1 = AOP_RRR(opirr(p->as), p->to.reg, p->from.reg, 0) | ((regoff(&p->from3)&0x7F)<<11);
		break;

	case 43:	/* unary indexed source: dcbf (b); dcbf (a+b) */
		r = p->reg;
		if(r == NREG)
			r = 0;
		o1 = AOP_RRR(oprrr(p->as), 0, r, p->from.reg);
		break;

	case 44:	/* indexed store */
		r = p->reg;
		if(r == NREG)
			r = 0;
		o1 = AOP_RRR(opstorex(p->as), p->from.reg, r, p->to.reg);
		break;
	case 45:	/* indexed load */
		r = p->reg;
		if(r == NREG)
			r = 0;
		o1 = AOP_RRR(oploadx(p->as), p->to.reg, r, p->from.reg);
		break;

	case 46:	/* plain op */
		o1 = oprrr(p->as);
		break;

	case 47:	/* op Ra, Rd; also op [Ra,] Rd */
		r = p->from.reg;
		if(r == NREG)
			r = p->to.reg;
		o1 = AOP_RRR(oprrr(p->as), p->to.reg, r, 0);
		break;

	case 48:	/* op Rs, Ra */
		r = p->from.reg;
		if(r == NREG)
			r = p->to.reg;
		o1 = LOP_RRR(oprrr(p->as), p->to.reg, r, 0);
		break;

	case 49:	/* op Rb */
		o1 = AOP_RRR(oprrr(p->as), 0, 0, p->from.reg);
		break;

/*50*/

	case 51:	/* rem[u] r1[,r2],r3 */
		r = p->reg;
		if(r == NREG)
			r = p->to.reg;
		v = oprrr(p->as);
		t = v & ((1<<10)|1);	/* OE|Rc */
		o1 = AOP_RRR(v&~t, REGTMP, r, p->from.reg);
		o2 = AOP_RRR(OP_MULLW, REGTMP, REGTMP, p->from.reg);
		o3 = AOP_RRR(OP_SUBF|t, p->to.reg, REGTMP, r);
		break;

	case 52:	/* mtfsbNx cr(n) */
		v = regoff(&p->from)&31L;
		o1 = AOP_RRR(oprrr(p->as), v, 0, 0);
		break;

	case 53:	/* mffsX ,fr1 */
		o1 = AOP_RRR(OP_MFFS, p->to.reg, 0, 0);
		break;

	case 54:	/* mov msr,r1; mov r1, msr*/
		if(oclass(p->from) == C_REG)
			o1 = AOP_RRR(OP_MTMSR, p->from.reg, 0, 0);
		else
			o1 = AOP_RRR(OP_MFMSR, p->to.reg, 0, 0);
		break;

	case 55:	/* mov sreg,r1; mov r1,sreg */
		v = 0;
		if(p->from.type == D_SREG) {
			r = p->from.reg;
			o1 = OP_MFSR;
			if(r == NREG && p->reg != NREG) {
				r = 0;
				v = p->reg;
				o1 = OP_MFSRIN;
			}
			o1 = AOP_RRR(o1, p->to.reg, r&15L, v);
		} else {
			r = p->to.reg;
			o1 = OP_MTSR;
			if(r == NREG && p->reg != NREG) {
				r = 0;
				v = p->reg;
				o1 = OP_MTSRIN;
			}
			o1 = AOP_RRR(o1, p->from.reg, r&15L, v);
		}
		if(r == NREG)
			diag("illegal move indirect to/from segment register\n%P", p);
		break;

	case 56:	/* sra $sh,[s,]a */
		v = regoff(&p->from);
		r = p->reg;
		if(r == NREG)
			r = p->to.reg;
		o1 = AOP_RRR(opirr(p->as), r, p->to.reg, v&31L);
		break;

	case 57:	/* slw $sh,[s,]a -> rlwinm ... */
		v = regoff(&p->from);
		r = p->reg;
		if(r == NREG)
			r = p->to.reg;
		/*
		 * Let user (gs) shoot himself in the foot. 
		 * qc has already complained.
		 *
		if(v < 0 || v > 31)
			diag("illegal shift %ld\n%P", v, p);
		 */
		if(v < 0)
			v = 0;
		else if(v > 32)
			v = 32;
		if(p->as == ASRW || p->as == ASRWCC) {	/* shift right */
			mask[0] = v;
			mask[1] = 31;
			v = 32-v;
		} else {
			mask[0] = 0;
			mask[1] = 31-v;
		}
		o1 = OP_RLW(OP_RLWINM, p->to.reg, r, v, mask[0], mask[1]);
		if(p->as == ASLWCC || p->as == ASRWCC)
			o1 |= 1;	/* Rc */
		break;

	case 58:		/* logical $andcon,[s],a */
		v = regoff(&p->from);
		r = p->reg;
		if(r == NREG)
			r = p->to.reg;
		o1 = LOP_IRR(opirr(p->as), p->to.reg, r, v);
		break;

	case 59:	/* or/and $ucon,,r */
		v = regoff(&p->from);
		r = p->reg;
		if(r == NREG)
			r = p->to.reg;
		o1 = LOP_IRR(opirr(p->as+AEND), p->to.reg, r, v>>16);	/* oris, xoris, andis */
		break;

	case 60:	/* tw to,a,b */
		r = regoff(&p->from)&31L;
		o1 = AOP_RRR(oprrr(p->as), r, p->reg, p->to.reg);
		break;

	case 61:	/* tw to,a,$simm */
		r = regoff(&p->from)&31L;
		v = regoff(&p->to);
		o1 = AOP_IRR(opirr(p->as), r, p->reg, v);
		break;

	case 62:	/* rlwmi $sh,s,$mask,a */
		v = regoff(&p->from);
		maskgen(p, mask, regoff(&p->from3));
		o1 = AOP_RRR(opirr(p->as), p->reg, p->to.reg, v);
		o1 |= ((mask[0]&31L)<<6)|((mask[1]&31L)<<1);
		break;

	case 63:	/* rlwmi b,s,$mask,a */
		maskgen(p, mask, regoff(&p->from3));
		o1 = AOP_RRR(opirr(p->as), p->reg, p->to.reg, p->from.reg);
		o1 |= ((mask[0]&31L)<<6)|((mask[1]&31L)<<1);
		break;

	case 64:	/* mtfsf fr[, $m] {,fpcsr} */
		if(p->from3.type != D_NONE)
			v = regoff(&p->from3)&255L;
		else
			v = 255;
		o1 = OP_MTFSF | (v<<17) | (p->from.reg<<11);
		break;

	case 65:	/* MOVFL $imm,FPSCR(n) => mtfsfi crfd,imm */
		if(p->to.reg == NREG)
			diag("must specify FPSCR(n)\n%P", p);
		o1 = OP_MTFSFI | ((p->to.reg&15L)<<23) | ((regoff(&p->from)&31L)<<12);
		break;

	case 66:	/* mov spr,r1; mov r1,spr, also dcr */
		if(p->from.type == D_REG) {
			r = p->from.reg;
			v = p->to.offset;
			if(p->to.type == D_DCR) {
				if(p->to.reg != NREG) {
					o1 = OPVCC(31,387,0,0);	/* mtdcrx */
					v = p->to.reg;
				}else
					o1 = OPVCC(31,451,0,0);	/* mtdcr */
			}else
				o1 = OPVCC(31,467,0,0); /* mtspr */
		} else {
			r = p->to.reg;
			v = p->from.offset;
			if(p->from.type == D_DCR) {
				if(p->from.reg != NREG) {
					o1 = OPVCC(31,259,0,0);	/* mfdcrx */
					v = p->from.reg;
				}else
					o1 = OPVCC(31,323,0,0);	/* mfdcr */
			}else
				o1 = OPVCC(31,339,0,0);	/* mfspr */
		}
		o1 = AOP_RRR(o1, r, 0, 0) | ((v&0x1f)<<16) | (((v>>5)&0x1f)<<11);
		break;

	case 67:	/* mcrf crfD,crfS */
		if(p->from.type != D_CREG || p->from.reg == NREG ||
		   p->to.type != D_CREG || p->to.reg == NREG)
			diag("illegal CR field number\n%P", p);
		o1 = AOP_RRR(OP_MCRF, ((p->to.reg&7L)<<2), ((p->from.reg&7)<<2), 0);
		break;

	case 68:	/* mfcr rD */
		if(p->from.type == D_CREG && p->from.reg != NREG)
			diag("must move whole CR to register\n%P", p);
		o1 = AOP_RRR(OP_MFCR, p->to.reg, 0, 0);
		break;

	case 69:	/* mtcrf CRM,rS */
		if(p->from3.type != D_NONE) {
			if(p->to.reg != NREG)
				diag("can't use both mask and CR(n)\n%P", p);
			v = regoff(&p->from3) & 0xff;
		} else {
			if(p->to.reg == NREG)
				v = 0xff;	/* CR */
			else
				v = 1<<(7-(p->to.reg&7));	/* CR(n) */
		}
		o1 = AOP_RRR(OP_MTCRF, p->from.reg, 0, 0) | (v<<12);
		break;

	case 70:	/* [f]cmp r,r,cr*/
		if(p->reg == NREG)
			r = 0;
		else
			r = (p->reg&7)<<2;
		o1 = AOP_RRR(oprrr(p->as), r, p->from.reg, p->to.reg);
		break;

	case 71:	/* cmp[l] r,i,cr*/
		if(p->reg == NREG)
			r = 0;
		else
			r = (p->reg&7)<<2;
		o1 = AOP_RRR(opirr(p->as), r, p->from.reg, 0) | (regoff(&p->to)&0xffff);
		break;

	case 72:	/* mcrxr crfD */
		if(p->to.reg == NREG)
			diag("must move XER to CR(n)\n%P", p);
		o1 = AOP_RRR(OP_MCRXR, ((p->to.reg&7L)<<2), 0, 0);
		break;

	case 73:	/* mcrfs crfD,crfS */
		if(p->from.type != D_FPSCR || p->from.reg == NREG ||
		   p->to.type != D_CREG || p->to.reg == NREG)
			diag("illegal FPSCR/CR field number\n%P", p);
		o1 = AOP_RRR(OP_MCRFS, ((p->to.reg&7L)<<2), ((p->from.reg&7)<<2), 0);
		break;

	/* relocation operations */

	case 74:
		v = regoff(&p->to);
		o1 = AOP_IRR(OP_ADDIS, REGTMP, REGZERO, v>>16);
		o2 = AOP_IRR(opstore(p->as), p->from.reg, REGTMP, v);
		if(dlm)
			reloc(&p->to, p->pc, 1);
		break;

	case 75:
		v = regoff(&p->from);
		o1 = AOP_IRR(OP_ADDIS, REGTMP, REGZERO, v>>16);
		o2 = AOP_IRR(opload(p->as), p->to.reg, REGTMP, v);
		if(dlm)
			reloc(&p->from, p->pc, 1);
		break;

	case 76:
		v = regoff(&p->from);
		o1 = AOP_IRR(OP_ADDIS, REGTMP, REGZERO, v>>16);
		o2 = AOP_IRR(opload(p->as), p->to.reg, REGTMP, v);
		o3 = LOP_RRR(OP_EXTSB, p->to.reg, p->to.reg, 0);
		if(dlm)
			reloc(&p->from, p->pc, 1);
		break;

	}
	if(aflag)
		return o1;
	v = p->pc;
	switch(o->size) {
	default:
		if(debug['a'])
			Bprint(&bso, " %.8lux:\t\t%P\n", v, p);
		break;
	case 4:
		if(debug['a'])
			Bprint(&bso, " %.8lux: %.8lux\t%P\n", v, o1, p);
		lput(o1);
		break;
	case 8:
		if(debug['a'])
			Bprint(&bso, " %.8lux: %.8lux %.8lux%P\n", v, o1, o2, p);
		lput(o1);
		lput(o2);
		break;
	case 12:
		if(debug['a'])
			Bprint(&bso, " %.8lux: %.8lux %.8lux %.8lux%P\n", v, o1, o2, o3, p);
		lput(o1);
		lput(o2);
		lput(o3);
		break;
	case 16:
		if(debug['a'])
			Bprint(&bso, " %.8lux: %.8lux %.8lux %.8lux %.8lux%P\n",
				v, o1, o2, o3, o4, p);
		lput(o1);
		lput(o2);
		lput(o3);
		lput(o4);
		break;
	case 20:
		if(debug['a'])
			Bprint(&bso, " %.8lux: %.8lux %.8lux %.8lux %.8lux %.8lux%P\n",
				v, o1, o2, o3, o4, o5, p);
		lput(o1);
		lput(o2);
		lput(o3);
		lput(o4);
		lput(o5);
		break;
	}
	return 0;
}

long
oprrr(int a)
{
	switch(a) {
	case AADD:	return OPVCC(31,266,0,0);
	case AADDCC:	return OPVCC(31,266,0,1);
	case AADDV:	return OPVCC(31,266,1,0);
	case AADDVCC:	return OPVCC(31,266,1,1);
	case AADDC:	return OPVCC(31,10,0,0);
	case AADDCCC:	return OPVCC(31,10,0,1);
	case AADDCV:	return OPVCC(31,10,1,0);
	case AADDCVCC:	return OPVCC(31,10,1,1);
	case AADDE:	return OPVCC(31,138,0,0);
	case AADDECC:	return OPVCC(31,138,0,1);
	case AADDEV:	return OPVCC(31,138,1,0);
	case AADDEVCC:	return OPVCC(31,138,1,1);
	case AADDME:	return OPVCC(31,234,0,0);
	case AADDMECC:	return OPVCC(31,234,0,1);
	case AADDMEV:	return OPVCC(31,234,1,0);
	case AADDMEVCC:	return OPVCC(31,234,1,1);
	case AADDZE:	return OPVCC(31,202,0,0);
	case AADDZECC:	return OPVCC(31,202,0,1);
	case AADDZEV:	return OPVCC(31,202,1,0);
	case AADDZEVCC:	return OPVCC(31,202,1,1);

	case AAND:	return OPVCC(31,28,0,0);
	case AANDCC:	return OPVCC(31,28,0,1);
	case AANDN:	return OPVCC(31,60,0,0);
	case AANDNCC:	return OPVCC(31,60,0,1);

	case ACMP:	return OPVCC(31,0,0,0);
	case ACMPU:	return OPVCC(31,32,0,0);

	case ACNTLZW:	return OPVCC(31,26,0,0);
	case ACNTLZWCC:	return OPVCC(31,26,0,1);

	case ACRAND:	return OPVCC(19,257,0,0);
	case ACRANDN:	return OPVCC(19,129,0,0);
	case ACREQV:	return OPVCC(19,289,0,0);
	case ACRNAND:	return OPVCC(19,225,0,0);
	case ACRNOR:	return OPVCC(19,33,0,0);
	case ACROR:	return OPVCC(19,449,0,0);
	case ACRORN:	return OPVCC(19,417,0,0);
	case ACRXOR:	return OPVCC(19,193,0,0);

	case ADCBF:	return OPVCC(31,86,0,0);
	case ADCBI:	return OPVCC(31,470,0,0);
	case ADCBST:	return OPVCC(31,54,0,0);
	case ADCBT:	return OPVCC(31,278,0,0);
	case ADCBTST:	return OPVCC(31,246,0,0);
	case ADCBZ:	return OPVCC(31,1014,0,0);

	case AREM:
	case ADIVW:	return OPVCC(31,491,0,0);
	case AREMCC:
	case ADIVWCC:	return OPVCC(31,491,0,1);
	case AREMV:
	case ADIVWV:	return OPVCC(31,491,1,0);
	case AREMVCC:
	case ADIVWVCC:	return OPVCC(31,491,1,1);
	case AREMU:
	case ADIVWU:	return OPVCC(31,459,0,0);
	case AREMUCC:
	case ADIVWUCC:	return OPVCC(31,459,0,1);
	case AREMUV:
	case ADIVWUV:	return OPVCC(31,459,1,0);
	case AREMUVCC:
	case ADIVWUVCC:	return OPVCC(31,459,1,1);

	case AEIEIO:	return OPVCC(31,854,0,0);

	case AEQV:	return OPVCC(31,284,0,0);
	case AEQVCC:	return OPVCC(31,284,0,1);

	case AEXTSB:	return OPVCC(31,954,0,0);
	case AEXTSBCC:	return OPVCC(31,954,0,1);
	case AEXTSH:	return OPVCC(31,922,0,0);
	case AEXTSHCC:	return OPVCC(31,922,0,1);

	case AFABS:	return OPVCC(63,264,0,0);
	case AFABSCC:	return OPVCC(63,264,0,1);
	case AFADD:	return OPVCC(63,21,0,0);
	case AFADDCC:	return OPVCC(63,21,0,1);
	case AFADDS:	return OPVCC(59,21,0,0);
	case AFADDSCC:	return OPVCC(59,21,0,1);
	case AFCMPO:	return OPVCC(63,32,0,0);
	case AFCMPU:	return OPVCC(63,0,0,0);
	case AFCTIW:	return OPVCC(63,14,0,0);
	case AFCTIWCC:	return OPVCC(63,14,0,1);
	case AFCTIWZ:	return OPVCC(63,15,0,0);
	case AFCTIWZCC:	return OPVCC(63,15,0,1);
	case AFDIV:	return OPVCC(63,18,0,0);
	case AFDIVCC:	return OPVCC(63,18,0,1);
	case AFDIVS:	return OPVCC(59,18,0,0);
	case AFDIVSCC:	return OPVCC(59,18,0,1);
	case AFMADD:	return OPVCC(63,29,0,0);
	case AFMADDCC:	return OPVCC(63,29,0,1);
	case AFMADDS:	return OPVCC(59,29,0,0);
	case AFMADDSCC:	return OPVCC(59,29,0,1);
	case AFMOVS:
	case AFMOVD:	return OPVCC(63,72,0,0);	/* load */
	case AFMOVDCC:	return OPVCC(63,72,0,1);
	case AFMSUB:	return OPVCC(63,28,0,0);
	case AFMSUBCC:	return OPVCC(63,28,0,1);
	case AFMSUBS:	return OPVCC(59,28,0,0);
	case AFMSUBSCC:	return OPVCC(59,28,0,1);
	case AFMUL:	return OPVCC(63,25,0,0);
	case AFMULCC:	return OPVCC(63,25,0,1);
	case AFMULS:	return OPVCC(59,25,0,0);
	case AFMULSCC:	return OPVCC(59,25,0,1);
	case AFNABS:	return OPVCC(63,136,0,0);
	case AFNABSCC:	return OPVCC(63,136,0,1);
	case AFNEG:	return OPVCC(63,40,0,0);
	case AFNEGCC:	return OPVCC(63,40,0,1);
	case AFNMADD:	return OPVCC(63,31,0,0);
	case AFNMADDCC:	return OPVCC(63,31,0,1);
	case AFNMADDS:	return OPVCC(59,31,0,0);
	case AFNMADDSCC:	return OPVCC(59,31,0,1);
	case AFNMSUB:	return OPVCC(63,30,0,0);
	case AFNMSUBCC:	return OPVCC(63,30,0,1);
	case AFNMSUBS:	return OPVCC(59,30,0,0);
	case AFNMSUBSCC:	return OPVCC(59,30,0,1);
	case AFRES:	return OPVCC(59,24,0,0);
	case AFRESCC:	return OPVCC(59,24,0,1);
	case AFRSP:	return OPVCC(63,12,0,0);
	case AFRSPCC:	return OPVCC(63,12,0,1);
	case AFRSQRTE:	return OPVCC(63,26,0,0);
	case AFRSQRTECC:	return OPVCC(63,26,0,1);
	case AFSEL:	return OPVCC(63,23,0,0);
	case AFSELCC:	return OPVCC(63,23,0,1);
	case AFSQRT:	return OPVCC(63,22,0,0);
	case AFSQRTCC:	return OPVCC(63,22,0,1);
	case AFSQRTS:	return OPVCC(59,22,0,0);
	case AFSQRTSCC:	return OPVCC(59,22,0,1);
	case AFSUB:	return OPVCC(63,20,0,0);
	case AFSUBCC:	return OPVCC(63,20,0,1);
	case AFSUBS:	return OPVCC(59,20,0,0);
	case AFSUBSCC:	return OPVCC(59,20,0,1);

	/* fp2 */
	case AFPMUL:	return OPVCC(0,8,0,0);
	case AFXMUL:	return OPVCC(0,9,0,0);
	case AFXPMUL:	return OPVCC(0,10,0,0);
	case AFXSMUL:	return OPVCC(0,11,0,0);
	case AFPADD:	return OPVCC(0,12,0,0);
	case AFPSUB:	return OPVCC(0,13,0,0);
	case AFPRE:	return OPVCC(0,14,0,0);
	case AFPRSQRTE:	return OPVCC(0,15,0,0);
	case AFPMADD:	return OPVCC(0,16,0,0);
	case AFXMADD:	return OPVCC(0,17,0,0);
	case AFXCPMADD:	return OPVCC(0,18,0,0);
	case AFXCSMADD:	return OPVCC(0,19,0,0);
	case AFPNMADD:	return OPVCC(0,20,0,0);
	case AFXNMADD:	return OPVCC(0,21,0,0);
	case AFXCPNMADD:	return OPVCC(0,22,0,0);
	case AFXCSNMADD:	return OPVCC(0,23,0,0);
	case AFPMSUB:	return OPVCC(0,24,0,0);
	case AFXMSUB:	return OPVCC(0,25,0,0);
	case AFXCPMSUB:	return OPVCC(0,26,0,0);
	case AFXCSMSUB:	return OPVCC(0,27,0,0);
	case AFPNMSUB:	return OPVCC(0,28,0,0);
	case AFXNMSUB:	return OPVCC(0,29,0,0);
	case AFXCPNMSUB:	return OPVCC(0,30,0,0);
	case AFXCSNMSUB:	return OPVCC(0,31,0,0);
	case AFPABS:	return OPVCC(0,96,0,0);
	case AFPNEG:	return OPVCC(0,160,0,0);
	case AFPRSP:	return OPVCC(0,192,0,0);
	case AFPNABS:	return OPVCC(0,224,0,0);
	case AFSCMP:	return OPVCC(0,320,0,0)|(3<<21);
	case AFSABS:	return OPVCC(0,352,0,0);
	case AFSNEG:	return OPVCC(0,416,0,0);
	case AFSNABS:	return OPVCC(0,480,0,0);
	case AFPCTIW:	return OPVCC(0,576,0,0);
	case AFPCTIWZ:	return OPVCC(0,704,0,0);

	case AFPMOVD:	return OPVCC(0,32,0,0);	/* fpmr */
	case AFSMOVD:	return OPVCC(0,288,0,0);	/* fsmr */
	case AFXMOVD:	return OPVCC(0,544,0,0);	/* fxmr */
	case AFMOVSPD:		return OPVCC(0,800,0,0);	/* fsmtp */
	case AFMOVPSD:		return OPVCC(0,928,0,0);	/* fsmfp */

	case AFXCPNPMA:	return OPVCC(4,24,0,0);
	case AFXCSNPMA:	return OPVCC(4,25,0,0);
	case AFXCPNSMA:	return OPVCC(4,26,0,0);
	case AFXCSNSMA:	return OPVCC(4,27,0,0);
	case AFXCXNPMA:	return OPVCC(4,29,0,0);
	case AFXCXNSMA:	return OPVCC(4,30,0,0);
	case AFXCXMA:	return OPVCC(4,28,0,0);
	case AFXCXNMS:	return OPVCC(4,31,0,0);

	case AICBI:	return OPVCC(31,982,0,0);
	case AISYNC:	return OPVCC(19,150,0,0);

	case AMTFSB0:	return OPVCC(63,70,0,0);
	case AMTFSB0CC:	return OPVCC(63,70,0,1);
	case AMTFSB1:	return OPVCC(63,38,0,0);
	case AMTFSB1CC:	return OPVCC(63,38,0,1);

	case AMULHW:	return OPVCC(31,75,0,0);
	case AMULHWCC:	return OPVCC(31,75,0,1);
	case AMULHWU:	return OPVCC(31,11,0,0);
	case AMULHWUCC:	return OPVCC(31,11,0,1);
	case AMULLW:	return OPVCC(31,235,0,0);
	case AMULLWCC:	return OPVCC(31,235,0,1);
	case AMULLWV:	return OPVCC(31,235,1,0);
	case AMULLWVCC:	return OPVCC(31,235,1,1);

	/* the following group is only available on IBM embedded powerpc */
	case AMACCHW:	return OPVCC(4,172,0,0);
	case AMACCHWCC:	return OPVCC(4,172,0,1);
	case AMACCHWS:	return OPVCC(4,236,0,0);
	case AMACCHWSCC:	return OPVCC(4,236,0,1);
	case AMACCHWSU:	return OPVCC(4,204,0,0);
	case AMACCHWSUCC:	return OPVCC(4,204,0,1);
	case AMACCHWSUV:	return OPVCC(4,204,1,0);
	case AMACCHWSUVCC:	return OPVCC(4,204,1,1);
	case AMACCHWSV:	return OPVCC(4,236,1,0);
	case AMACCHWSVCC:	return OPVCC(4,236,1,1);
	case AMACCHWU:	return OPVCC(4,140,0,0);
	case AMACCHWUCC:	return OPVCC(4,140,0,1);
	case AMACCHWUV:	return OPVCC(4,140,1,0);
	case AMACCHWUVCC:	return OPVCC(4,140,1,1);
	case AMACCHWV:	return OPVCC(4,172,1,0);
	case AMACCHWVCC:	return OPVCC(4,172,1,1);
	case AMACHHW:	return OPVCC(4,44,0,0);
	case AMACHHWCC:	return OPVCC(4,44,0,1);
	case AMACHHWS:	return OPVCC(4,108,0,0);
	case AMACHHWSCC:	return OPVCC(4,108,0,1);
	case AMACHHWSU:	return OPVCC(4,76,0,0);
	case AMACHHWSUCC:	return OPVCC(4,76,0,1);
	case AMACHHWSUV:	return OPVCC(4,76,1,0);
	case AMACHHWSUVCC:	return OPVCC(4,76,1,1);
	case AMACHHWSV:	return OPVCC(4,108,1,0);
	case AMACHHWSVCC:	return OPVCC(4,108,1,1);
	case AMACHHWU:	return OPVCC(4,12,0,0);
	case AMACHHWUCC:	return OPVCC(4,12,0,1);
	case AMACHHWUV:	return OPVCC(4,12,1,0);
	case AMACHHWUVCC:	return OPVCC(4,12,1,1);
	case AMACHHWV:	return OPVCC(4,44,1,0);
	case AMACHHWVCC:	return OPVCC(4,44,1,1);
	case AMACLHW:	return OPVCC(4,428,0,0);
	case AMACLHWCC:	return OPVCC(4,428,0,1);
	case AMACLHWS:	return OPVCC(4,492,0,0);
	case AMACLHWSCC:	return OPVCC(4,492,0,1);
	case AMACLHWSU:	return OPVCC(4,460,0,0);
	case AMACLHWSUCC:	return OPVCC(4,460,0,1);
	case AMACLHWSUV:	return OPVCC(4,460,1,0);
	case AMACLHWSUVCC:	return OPVCC(4,460,1,1);
	case AMACLHWSV:	return OPVCC(4,492,1,0);
	case AMACLHWSVCC:	return OPVCC(4,492,1,1);
	case AMACLHWU:	return OPVCC(4,396,0,0);
	case AMACLHWUCC:	return OPVCC(4,396,0,1);
	case AMACLHWUV:	return OPVCC(4,396,1,0);
	case AMACLHWUVCC:	return OPVCC(4,396,1,1);
	case AMACLHWV:	return OPVCC(4,428,1,0);
	case AMACLHWVCC:	return OPVCC(4,428,1,1);
	case AMULCHW:	return OPVCC(4,168,0,0);
	case AMULCHWCC:	return OPVCC(4,168,0,1);
	case AMULCHWU:	return OPVCC(4,136,0,0);
	case AMULCHWUCC:	return OPVCC(4,136,0,1);
	case AMULHHW:	return OPVCC(4,40,0,0);
	case AMULHHWCC:	return OPVCC(4,40,0,1);
	case AMULHHWU:	return OPVCC(4,8,0,0);
	case AMULHHWUCC:	return OPVCC(4,8,0,1);
	case AMULLHW:	return OPVCC(4,424,0,0);
	case AMULLHWCC:	return OPVCC(4,424,0,1);
	case AMULLHWU:	return OPVCC(4,392,0,0);
	case AMULLHWUCC:	return OPVCC(4,392,0,1);
	case ANMACCHW:	return OPVCC(4,174,0,0);
	case ANMACCHWCC:	return OPVCC(4,174,0,1);
	case ANMACCHWS:	return OPVCC(4,238,0,0);
	case ANMACCHWSCC:	return OPVCC(4,238,0,1);
	case ANMACCHWSV:	return OPVCC(4,238,1,0);
	case ANMACCHWSVCC:	return OPVCC(4,238,1,1);
	case ANMACCHWV:	return OPVCC(4,174,1,0);
	case ANMACCHWVCC:	return OPVCC(4,174,1,1);
	case ANMACHHW:	return OPVCC(4,46,0,0);
	case ANMACHHWCC:	return OPVCC(4,46,0,1);
	case ANMACHHWS:	return OPVCC(4,110,0,0);
	case ANMACHHWSCC:	return OPVCC(4,110,0,1);
	case ANMACHHWSV:	return OPVCC(4,110,1,0);
	case ANMACHHWSVCC:	return OPVCC(4,110,1,1);
	case ANMACHHWV:	return OPVCC(4,46,1,0);
	case ANMACHHWVCC:	return OPVCC(4,46,1,1);
	case ANMACLHW:	return OPVCC(4,430,0,0);
	case ANMACLHWCC:	return OPVCC(4,430,0,1);
	case ANMACLHWS:	return OPVCC(4,494,0,0);
	case ANMACLHWSCC:	return OPVCC(4,494,0,1);
	case ANMACLHWSV:	return OPVCC(4,494,1,0);
	case ANMACLHWSVCC:	return OPVCC(4,494,1,1);
	case ANMACLHWV:	return OPVCC(4,430,1,0);
	case ANMACLHWVCC:	return OPVCC(4,430,1,1);

	case ANAND:	return OPVCC(31,476,0,0);
	case ANANDCC:	return OPVCC(31,476,0,1);
	case ANEG:	return OPVCC(31,104,0,0);
	case ANEGCC:	return OPVCC(31,104,0,1);
	case ANEGV:	return OPVCC(31,104,1,0);
	case ANEGVCC:	return OPVCC(31,104,1,1);
	case ANOR:	return OPVCC(31,124,0,0);
	case ANORCC:	return OPVCC(31,124,0,1);
	case AOR:	return OPVCC(31,444,0,0);
	case AORCC:	return OPVCC(31,444,0,1);
	case AORN:	return OPVCC(31,412,0,0);
	case AORNCC:	return OPVCC(31,412,0,1);

	case ARFI:	return OPVCC(19,50,0,0);
	case ARFCI:	return OPVCC(19,51,0,0);

	case ARLWMI:	return OPVCC(20,0,0,0);
	case ARLWMICC: return OPVCC(20,0,0,1);
	case ARLWNM:	return OPVCC(23,0,0,0);
	case ARLWNMCC:	return OPVCC(23,0,0,1);

	case ASYSCALL:	return OPVCC(17,1,0,0);

	case ASLW:	return OPVCC(31,24,0,0);
	case ASLWCC:	return OPVCC(31,24,0,1);

	case ASRAW:	return OPVCC(31,792,0,0);
	case ASRAWCC:	return OPVCC(31,792,0,1);

	case ASRW:	return OPVCC(31,536,0,0);
	case ASRWCC:	return OPVCC(31,536,0,1);

	case ASUB:	return OPVCC(31,40,0,0);
	case ASUBCC:	return OPVCC(31,40,0,1);
	case ASUBV:	return OPVCC(31,40,1,0);
	case ASUBVCC:	return OPVCC(31,40,1,1);
	case ASUBC:	return OPVCC(31,8,0,0);
	case ASUBCCC:	return OPVCC(31,8,0,1);
	case ASUBCV:	return OPVCC(31,8,1,0);
	case ASUBCVCC:	return OPVCC(31,8,1,1);
	case ASUBE:	return OPVCC(31,136,0,0);
	case ASUBECC:	return OPVCC(31,136,0,1);
	case ASUBEV:	return OPVCC(31,136,1,0);
	case ASUBEVCC:	return OPVCC(31,136,1,1);
	case ASUBME:	return OPVCC(31,232,0,0);
	case ASUBMECC:	return OPVCC(31,232,0,1);
	case ASUBMEV:	return OPVCC(31,232,1,0);
	case ASUBMEVCC:	return OPVCC(31,232,1,1);
	case ASUBZE:	return OPVCC(31,200,0,0);
	case ASUBZECC:	return OPVCC(31,200,0,1);
	case ASUBZEV:	return OPVCC(31,200,1,0);
	case ASUBZEVCC:	return OPVCC(31,200,1,1);

	case ASYNC:	return OPVCC(31,598,0,0);
	case ATLBIE:	return OPVCC(31,306,0,0);
	case ATW:	return OPVCC(31,4,0,0);

	case AXOR:	return OPVCC(31,316,0,0);
	case AXORCC:	return OPVCC(31,316,0,1);
	}
	diag("bad r/r opcode %A", a);
	return 0;
}

long
opirr(int a)
{
	switch(a) {
	case AADD:	return OPVCC(14,0,0,0);
	case AADDC:	return OPVCC(12,0,0,0);
	case AADDCCC:	return OPVCC(13,0,0,0);
	case AADD+AEND:	return OPVCC(15,0,0,0);		/* ADDIS/CAU */

	case AANDCC:	return OPVCC(28,0,0,0);
	case AANDCC+AEND:	return OPVCC(29,0,0,0);		/* ANDIS./ANDIU. */

	case ABR:	return OPVCC(18,0,0,0);
	case ABL:	return OPVCC(18,0,0,0) | 1;
	case ABC:	return OPVCC(16,0,0,0);
	case ABCL:	return OPVCC(16,0,0,0) | 1;

	case ABEQ:	return AOP_RRR(16<<26,12,2,0);
	case ABGE:	return AOP_RRR(16<<26,4,0,0);
	case ABGT:	return AOP_RRR(16<<26,12,1,0);
	case ABLE:	return AOP_RRR(16<<26,4,1,0);
	case ABLT:	return AOP_RRR(16<<26,12,0,0);
	case ABNE:	return AOP_RRR(16<<26,4,2,0);
	case ABVC:	return AOP_RRR(16<<26,4,3,0);
	case ABVS:	return AOP_RRR(16<<26,12,3,0);

	case ACMP:	return OPVCC(11,0,0,0);
	case ACMPU:	return OPVCC(10,0,0,0);
	case ALSW:	return OPVCC(31,597,0,0);

	case AMULLW:	return OPVCC(7,0,0,0);

	case AOR:	return OPVCC(24,0,0,0);
	case AOR+AEND:	return OPVCC(25,0,0,0);		/* ORIS/ORIU */

	case ARLWMI:	return OPVCC(20,0,0,0);		/* rlwimi */
	case ARLWMICC:	return OPVCC(20,0,0,1);

	case ARLWNM:	return OPVCC(21,0,0,0);		/* rlwinm */
	case ARLWNMCC:	return OPVCC(21,0,0,1);

	case ASRAW:	return OPVCC(31,824,0,0);
	case ASRAWCC:	return OPVCC(31,824,0,1);

	case ASTSW:	return OPVCC(31,725,0,0);

	case ASUBC:	return OPVCC(8,0,0,0);

	case ATW:	return OPVCC(3,0,0,0);

	case AXOR:	return OPVCC(26,0,0,0);		/* XORIL */
	case AXOR+AEND:	return OPVCC(27,0,0,0);		/* XORIU */
	}
	diag("bad opcode i/r %A", a);
	return 0;
}

/*
 * load o(a),d
 */
long
opload(int a)
{
	switch(a) {
	case AMOVW:	return OPVCC(32,0,0,0);		/* lwz */
	case AMOVWU:	return OPVCC(33,0,0,0);		/* lwzu */
	case AMOVB:
	case AMOVBZ:	return OPVCC(34,0,0,0);		/* load */
	case AMOVBU:
	case AMOVBZU:	return OPVCC(35,0,0,0);
	case AFMOVD:	return OPVCC(50,0,0,0);
	case AFMOVDU:	return OPVCC(51,0,0,0);
	case AFMOVS:	return OPVCC(48,0,0,0);
	case AFMOVSU:	return OPVCC(49,0,0,0);
	case AMOVH:	return OPVCC(42,0,0,0);
	case AMOVHU:	return OPVCC(43,0,0,0);
	case AMOVHZ:	return OPVCC(40,0,0,0);
	case AMOVHZU:	return OPVCC(41,0,0,0);
	case AMOVMW:	return OPVCC(46,0,0,0);	/* lmw */
	}
	diag("bad load opcode %A", a);
	return 0;
}

/*
 * indexed load a(b),d
 */
long
oploadx(int a)
{
	switch(a) {
	case AMOVW: return OPVCC(31,23,0,0);	/* lwzx */
	case AMOVWU:	return OPVCC(31,55,0,0); /* lwzux */
	case AMOVB:
	case AMOVBZ: return OPVCC(31,87,0,0);	/* lbzx */
	case AMOVBU:
	case AMOVBZU: return OPVCC(31,119,0,0);	/* lbzux */
	case AFMOVD:	return OPVCC(31,599,0,0);	/* lfdx */
	case AFMOVDU:	return OPVCC(31,631,0,0);	/*  lfdux */
	case AFMOVS:	return OPVCC(31,535,0,0);	/* lfsx */
	case AFMOVSU:	return OPVCC(31,567,0,0);	/* lfsux */
	case AMOVH:	return OPVCC(31,343,0,0);	/* lhax */
	case AMOVHU:	return OPVCC(31,375,0,0);	/* lhaux */
	case AMOVHBR:	return OPVCC(31,790,0,0);	/* lhbrx */
	case AMOVWBR:	return OPVCC(31,534,0,0);	/* lwbrx */
	case AMOVHZ:	return OPVCC(31,279,0,0);	/* lhzx */
	case AMOVHZU:	return OPVCC(31,311,0,0);	/* lhzux */
	case AECIWX:	return OPVCC(31,310,0,0);	/* eciwx */
	case ALWAR:	return OPVCC(31,20,0,0);	/* lwarx */
	case ALSW:	return OPVCC(31,533,0,0);	/* lswx */
	case AFSMOVS:	return OPVCC(31,142,0,0);	/* lfssx */
	case AFSMOVSU:	return OPVCC(31,174,0,0);	/* lfssux */
	case AFSMOVD:	return OPVCC(31,206,0,0);	/* lfsdx */
	case AFSMOVDU:	return OPVCC(31,238,0,0);	/* lfsdux */
	case AFXMOVS:	return OPVCC(31,270,0,0);	/* lfxsx */
	case AFXMOVSU:	return OPVCC(31,302,0,0);	/* lfxsux */
	case AFXMOVD:	return OPVCC(31,334,0,0);	/* lfxdx */
	case AFXMOVDU:	return OPVCC(31,366,0,0);	/* lfxdux */
	case AFPMOVS:	return OPVCC(31,398,0,0);	/* lfpsx */
	case AFPMOVSU:	return OPVCC(31,430,0,0);	/* lfpsux */
	case AFPMOVD:	return OPVCC(31,462,0,0);	/* lfpdx */
	case AFPMOVDU:	return OPVCC(31,494,0,0);	/* lfpdux */
	}
	diag("bad loadx opcode %A", a);
	return 0;
}

/*
 * store s,o(d)
 */
long
opstore(int a)
{
	switch(a) {
	case AMOVB:
	case AMOVBZ:	return OPVCC(38,0,0,0);	/* stb */
	case AMOVBU:
	case AMOVBZU:	return OPVCC(39,0,0,0);	/* stbu */
	case AFMOVD:	return OPVCC(54,0,0,0);	/* stfd */
	case AFMOVDU:	return OPVCC(55,0,0,0);	/* stfdu */
	case AFMOVS:	return OPVCC(52,0,0,0);	/* stfs */
	case AFMOVSU:	return OPVCC(53,0,0,0);	/* stfsu */
	case AMOVHZ:
	case AMOVH:	return OPVCC(44,0,0,0);	/* sth */
	case AMOVHZU:
	case AMOVHU:	return OPVCC(45,0,0,0);	/* sthu */
	case AMOVMW:	return OPVCC(47,0,0,0);	/* stmw */
	case ASTSW:	return OPVCC(31,725,0,0);	/* stswi */
	case AMOVW:	return OPVCC(36,0,0,0);	/* stw */
	case AMOVWU:	return OPVCC(37,0,0,0);	/* stwu */
	}
	diag("unknown store opcode %A", a);
	return 0;
}

/*
 * indexed store s,a(b)
 */
long
opstorex(int a)
{
	switch(a) {
	case AMOVB:
	case AMOVBZ:	return OPVCC(31,215,0,0);	/* stbx */
	case AMOVBU:
	case AMOVBZU:	return OPVCC(31,247,0,0);	/* stbux */
	case AFMOVD:	return OPVCC(31,727,0,0);	/* stfdx */
	case AFMOVDU:	return OPVCC(31,759,0,0);	/* stfdux */
	case AFMOVS:	return OPVCC(31,663,0,0);	/* stfsx */
	case AFMOVSU:	return OPVCC(31,695,0,0);	/* stfsux */
	case AMOVHZ:
	case AMOVH:	return OPVCC(31,407,0,0);	/* sthx */
	case AMOVHBR:	return OPVCC(31,918,0,0);	/* sthbrx */
	case AMOVHZU:
	case AMOVHU:	return OPVCC(31,439,0,0);	/* sthux */
	case AMOVW:	return OPVCC(31,151,0,0);	/* stwx */
	case AMOVWU:	return OPVCC(31,183,0,0);	/* stwux */
	case ASTSW:	return OPVCC(31,661,0,0);	/* stswx */
	case AMOVWBR:	return OPVCC(31,662,0,0);	/* stwbrx */
	case ASTWCCC:	return OPVCC(31,150,0,1);	/* stwcx. */
	case AECOWX:	return OPVCC(31,438,0,0);	/* ecowx */
	case AFSMOVS:	return OPVCC(31,654,0,0);	/* stfssx */
/*	case AFSMOVSU:	return OPVCC(31,yy,0,0); */	/* stfssux not known */
/*	case AFSMOVD:	return OPVCC(31,yy,0,0); */ /* stfsdx not known */
	case AFSMOVDU:	return OPVCC(31,750,0,0);	/* stfsdux */
	case AFXMOVS:	return OPVCC(31,782,0,0);	/* stfxsx */
	case AFXMOVSU:	return OPVCC(31,814,0,0);	/* stfxsux */
	case AFXMOVD:	return OPVCC(31,846,0,0);	/* stfxdx */
	case AFXMOVDU:	return OPVCC(31,878,0,0);	/* stfxdux */
	case AFPMOVS:	return OPVCC(31,910,0,0);	/* stfpsx */
	case AFPMOVSU:	return OPVCC(31,942,0,0);	/* stfpsux */
	case AFPMOVD:	return OPVCC(31,974,0,0);	/* stfpdx */
	case AFPMOVDU:	return OPVCC(31,1006,0,0);	/* stfpdux */
	case AFPMOVIW:	return OPVCC(31,526,0,0);	/* stfpiwx */
	}
	diag("unknown storex opcode %A", a);
	return 0;
}
