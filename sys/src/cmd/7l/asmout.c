#include	"l.h"

#define	S32	(0U<<31)
#define	S64	(1U<<31)
#define	Rm(X)	(((X)&31)<<16)
#define	Rn(X)	(((X)&31)<<5)
#define	Rd(X)	(((X)&31)<<0)
#define	Sbit	(1U<<29)

#define	OPDP2(x)		(0<<30 | 0 << 29 | 0xd6<<21 | (x)<<10)
#define	OPDP3(sf,op54,op31,o0)	((sf)<<31 | (op54)<<29 | 0x1B<<24 | (op31)<<21 | (o0)<<15)
#define	OPBcc(x)		(0x2A<<25 | 0<<24 | 0<<4 | ((x)&15))
#define	OPBLR(x)		(0x6B<<25 | 0<<23 | (x)<<21 | 0x1F<<16 | 0<<10)	/* x=0, JMP; 1, CALL; 2, RET */
#define	SYSOP(l,op0,op1,crn,crm,op2,rt)	(0x354<<22 | (l)<<21 | (op0)<<19 | (op1)<<16 | (crn)<<12 | (crm)<<8 | (op2)<<5 | (rt))
#define	SYSHINT(x)	SYSOP(0,0,3,2,0,(x),0x1F)

#define	LDSTR12U(sz,v,opc)	((sz)<<30 | 7<<27 | (v)<<26 | 1<<24 | (opc)<<22)
#define	LDSTR9S(sz,v,opc)	((sz)<<30 | 7<<27 | (v)<<26 | 0<<24 | (opc)<<22)
#define	LD2STR(o)	((o) & ~(3<<22))

#define	LDSTX(sz,o2,l,o1,o0)	((sz)<<30 | 0x8<<24 | (o2)<<23 | (l)<<22 | (o1)<<21 | (o0)<<15)

#define	FPCMP(m,s,type,op,op2)	((m)<<31 | (s)<<29 | 0x1E<<24 | (type)<<22 | 1<<21 | (op)<<14 | 8<<10 | (op2))
#define	FPCCMP(m,s,type,op)	((m)<<31 | (s)<<29 | 0x1E<<24 | (type)<<22 | 1<<21 | 1<<10 | (op)<<4)
#define	FPOP1S(m,s,type,op)	((m)<<31 | (s)<<29 | 0x1E<<24 | (type)<<22 | 1<<21 | (op)<<15 | 0x10<<10)
#define	FPOP2S(m,s,type,op)	((m)<<31 | (s)<<29 | 0x1E<<24 | (type)<<22 | 1<<21 | (op)<<12 | 2<<10)
#define	FPCVTI(sf,s,type,rmode,op)	((sf)<<31 | (s)<<29 | 0x1E<<24 | (type)<<22 | 1<<21 | (rmode)<<19 | (op)<<16 | 0<<10)
#define	FPCVTF(sf,s,type,rmode,op,scale)	((sf)<<31 | (s)<<29 | 0x1E<<24 | (type)<<22 | 0<<21 | (rmode)<<19 | (op)<<16 | (scale)<<10)
#define	ADR(p,o,rt)	((p)<<31 | ((o)&3)<<29 | (0x10<<24) | (((o>>2)&0x7FFFF)<<5) | (rt))

#define	LSL0_32	(2<<13)
#define	LSL0_64	(3<<13)

static long	opbrr(int);
static long	opbra(int);
static long	oshrr(int, int, int);
static long	olhrr(int, int, int);
static long	olsr12u(long, long, int, int);
static long	olsr9s(long, long, int, int);
static long	opimm(int);
static vlong	brdist(Prog*, int, int, int);
static long	opbfm(int, int, int, int, int);
static long	opextr(int, long, int, int, int);
static long	opbit(int);
static long	op0(int);
static long	opstr12(int);
static long	opstr9(int);
static long	opldr9(int);
static long	opxrrr(int);
static long	olsxrr(int, int, int, int);
static long	oprrr(int);
static long	opirr(int);
static long	opldr12(int);
static long	opldrpp(int);
static long	opload(int);
static long	opstore(int);
static long	omovlit(int, Prog*, Adr*, int);
static int	movesize(int);
static long	oaddi(long, long, int, int);

/*
 * valid pstate field values, and value to use in instruction
 */
static struct{
	ulong	a;
	ulong	b;
} pstatefield[] = {
D_SPSel,		(0<<16) | (4<<12) | (5<<5),
D_DAIFSet,	(3<<16) | (4<<12) | (6<<5),
D_DAIFClr,	(3<<16) | (4<<12) | (7<<5),
};

void
asmout(Prog *p, Optab *o)
{
	long o1, o2, o3, o4, o5, v, hi;
	ulong u;
	vlong d;
	int r, s, rf, rt, ra, nzcv, cond, i, as;
	Mask *mask;
	static Prog *lastcase;

	o1 = 0;
	o2 = 0;
	o3 = 0;
	o4 = 0;
	o5 = 0;
	switch(o->type) {
	default:
		diag("unknown asm %d", o->type);
		prasm(p);
		break;

	case 0:		/* pseudo ops */
		break;

	case 1:		/* op Rm,[Rn],Rd; default Rn=Rd -> op Rm<<0,[Rn,]Rd (shifted register) */
		o1 = oprrr(p->as);
		rf = p->from.reg;
		rt = p->to.reg;
		r = p->reg;
		if(p->to.type == D_NONE)
			rt = REGZERO;
		if(r == NREG)
			r = rt;
		o1 |= (rf<<16) | (r<<5) | rt;
		break;

	case 2:		/* add/sub $(uimm12|uimm24)[,R],R; cmp $(uimm12|uimm24),R */
		o1 = opirr(p->as);
		rt = p->to.reg;
		if(p->to.type == D_NONE){
			if((o1 & Sbit) == 0)
				diag("ineffective ZR destination\n%P", p);
			rt = REGZERO;
		}
		r = p->reg;
		if(r == NREG)
			r = rt;
		v = regoff(&p->from);
		o1 = oaddi(o1, v, r, rt);
		break;

	case 3:		/* op R<<n[,R],R (shifted register) */
		o1 = oprrr(p->as);
		o1 |= p->from.offset;	/* includes reg, op, etc */
		rt = p->to.reg;
		if(p->to.type == D_NONE)
			rt = REGZERO;
		r = p->reg;
		if(p->as == AMVN || p->as == AMVNW)
			r = REGZERO;
		else if(r == NREG)
			r = rt;
		o1 |= (r<<5) | rt;
		break;

	case 4:		/* mov $addcon, R; mov $recon, R; mov $racon, R */
		o1 = opirr(p->as);
		rt = p->to.reg;
		r = o->param;
		if(r == 0)
			r = REGZERO;
		v = regoff(&p->from);
		if((v & 0xFFF000) != 0){
			v >>= 12;
			o1 |= 1<<22;	/* shift, by 12 */
		}
		o1 |= ((v& 0xFFF) << 10) | (r<<5) | rt;
		break;

	case 5:		/* b s; bl s */
		o1 = opbra(p->as);
		o1 |= brdist(p, 0, 26, 2);
		break;

	case 6:		/* b ,O(R); bl ,O(R) */
		o1 = opbrr(p->as);
		o1 |= p->to.reg << 5;
		break;

	case 7:		/* beq s */
		o1 = opbra(p->as);
		o1 |= brdist(p, 0, 19, 2)<<5;
		break;

	case 8:		/* lsl $c,[R],R -> ubfm $(W-1)-c,$(-c MOD (W-1)),Rn,Rd */
		rt = p->to.reg;
		rf = p->reg;
		if(rf == NREG)
			rf = rt;
		v = p->from.offset;
		switch(p->as){
		case AASR:	o1 = opbfm(ASBFM, v, 63, rf, rt); break;
		case AASRW:	o1 = opbfm(ASBFMW, v, 31, rf, rt); break;
		case ALSL:	o1 = opbfm(AUBFM, (64-v)&63, 63-v, rf, rt); break;
		case ALSLW:	o1 = opbfm(AUBFMW, (32-v)&31, 31-v, rf, rt); break;
		case ALSR:	o1 = opbfm(AUBFM, v, 63, rf, rt); break;
		case ALSRW:	o1 = opbfm(AUBFMW, v, 31, rf, rt); break;
		case AROR:	o1 = opextr(AEXTR, v, rf, rf, rt); break;
		case ARORW:	o1 = opextr(AEXTRW, v, rf, rf, rt); break;
		default:
			diag("bad shift $con\n%P", curp);
			break;
		}
		break;

	case 9:		/* lsl Rm,[Rn],Rd -> lslv Rm, Rn, Rd */
		o1 = oprrr(p->as);
		r = p->reg;
		if(r == NREG)
			r = p->to.reg;
		o1 |= (p->from.reg << 16) | (r<<5) | p->to.reg;
		break;

	case 10:	/* brk/hvc/.../svc [$con] */
		o1 = opimm(p->as);
		if(p->to.type != D_NONE)
			o1 |= (p->to.offset & 0xffff)<<5;
		break;

	case 11:	/* dword */
		switch(aclass(&p->to)) {
		case C_VCON:
		case C_ZCON:
		case C_LCON:
			if(!dlm)
				break;
			if(p->to.name != D_EXTERN && p->to.name != D_STATIC)
				break;
		case C_ADDR:
			if(p->to.sym->type == SUNDEF)
				ckoff(p->to.sym, p->to.offset);
			dynreloc(p->to.sym, p->pc, 1);
		}
		o1 = instoffset;
		o2 = instoffset >> 32;
		break;

	case 12:	/* movT $lcon, reg */
		o1 = omovlit(p->as, p, &p->from, p->to.reg);
		break;

	case 13:	/* addop $lcon, [R], R (64 bit literal); cmp $lcon,R -> addop $lcon,R, ZR */
		o1 = omovlit(AMOV, p, &p->from, REGTMP);
		if(!o1)
			break;
		rt = p->to.reg;
		if(p->to.type == D_NONE)
			rt = REGZERO;
		r = p->reg;
		if(r == NREG)
			r = rt;
		if(p->to.type != D_NONE && (p->to.reg == REGSP || r == REGSP)){
			o2 = opxrrr(p->as);
			o2 |= REGTMP<<16;
			o2 |= LSL0_64;
		}else{
			o2 = oprrr(p->as);
			o2 |= REGTMP << 16;	/* shift is 0 */
		}
		o2 |= r << 5;
		o2 |= rt;
		break;

	case 14:	/* word */
		if(aclass(&p->to) == C_ADDR)
			diag("address constant needs DWORD\n%P", p);
		o1 = instoffset;
		break;

	case 15:	/* mul/mneg/umulh/umull r,[r,]r; madd/msub Rm,Rn,Ra,Rd */
		o1 = oprrr(p->as);
		rf = p->from.reg;
		rt = p->to.reg;
		if(p->from3.type == D_REG){
			r = p->from3.reg;
			ra = p->reg;
			if(ra == NREG)
				ra = REGZERO;
		}else{
			r = p->reg;
			if(r == NREG)
				r = rt;
			ra = REGZERO;
		}
		o1 |= (rf<<16) | (ra<<10) | (r<<5) | rt;
		break;

	case 16:	/* XremY R[,R],R -> XdivY; XmsubY */
		o1 = oprrr(p->as);
		rf = p->from.reg;
		rt = p->to.reg;
		r = p->reg;
		if(r == NREG)
			r = rt;
		o1 |= (rf<<16) | (r<<5) | REGTMP;
		o2 = oprrr(AMSUBW);
		o2 |= o1 & (1<<31);	/* same size */
		o2 |= (rf<<16) | (r<<10) | (REGTMP<<5) | rt;
		break;

	case 17:		/* op Rm,[Rn],Rd; default Rn=ZR */
		o1 = oprrr(p->as);
		rf = p->from.reg;
		rt = p->to.reg;
		r = p->reg;
		if(p->to.type == D_NONE)
			rt = REGZERO;
		if(r == NREG)
			r = REGZERO;
		o1 |= (rf<<16) | (r<<5) | rt;
		break;

	case 18:	/* csel cond,Rn,Rm,Rd; cinc/cinv/cneg cond,Rn,Rd; cset cond,Rd */
		o1 = oprrr(p->as);
		cond = p->from.reg;
		r = p->reg;
		if(r != NREG){
			if(p->from3.type == D_NONE){
				/* CINC/CINV/CNEG */
				rf = r;
				cond ^= 1;
			}else
				rf = p->from3.reg;	/* CSEL */
		}else{
			/* CSET */
			if(p->from3.type != D_NONE)
				diag("invalid combination\n%P", p);
			r = rf = REGZERO;
			cond ^= 1;
		}
		rt = p->to.reg;
		o1 |= (r<<16) | (cond<<12) | (rf<<5) | rt;
		break;

	case 19:	/* CCMN cond, (Rm|uimm5),Rn, uimm4 -> ccmn Rn,Rm,uimm4,cond */
		nzcv = p->to.offset;
		cond = p->from.reg;
		if(p->from3.type == D_REG){
			o1 = oprrr(p->as);
			rf = p->from3.reg;	/* Rm */
		}else{
			o1 = opirr(p->as);
			rf = p->from3.offset & 0x1F;
		}
		o1 |= (rf<<16) | (cond<<12) | (p->reg<<5) | nzcv;
		break;

	case 20:	/* movT R,O(R) -> strT */
		v = regoff(&p->to);
		r = p->to.reg;
		if(r == NREG)
			r = o->param;
		if(v < 0){	/* unscaled 9-bit signed */
			o1 = olsr9s(opstr9(p->as), v, r, p->from.reg);
		}else{
			v = offsetshift(v, o->a3);
			o1 = olsr12u(opstr12(p->as), v, r, p->from.reg);
		}
		break;

	case 21:	/* movT O(R),R -> ldrT */
		v = regoff(&p->from);
		r = p->from.reg;
		if(r == NREG)
			r = o->param;
		if(v < 0){	/* unscaled 9-bit signed */
			o1 = olsr9s(opldr9(p->as), v, r, p->to.reg);
		}else{
			v = offsetshift(v, o->a1);
			//print("offset=%lld v=%ld a1=%d\n", instoffset, v, o->a1);
			o1 = olsr12u(opldr12(p->as), v, r, p->to.reg);
		}
		break;

	case 22:	/* movT (R)O!,R; movT O(R)!, R -> ldrT */
		v = p->from.offset;
		if(v < -256 || v > 255)
			diag("offset out of range\n%P", p);
		o1 = opldrpp(p->as);
		if(p->from.type == D_XPOST)
			o1 |= 1<<10;
		else
			o1 |= 3<<10;
		o1 |= ((v&0x1FF)<<12) | (p->from.reg<<5) | p->to.reg;
		break;

	case 23:	/* movT R,(R)O!; movT O(R)!, R -> strT */
		v = p->to.offset;
		if(v < -256 || v > 255)
			diag("offset out of range\n%P", p);
		o1 = LD2STR(opldrpp(p->as));
		if(p->to.type == D_XPOST)
			o1 |= 1<<10;
		else
			o1 |= 3<<10;
		o1 |= ((v&0x1FF)<<12) | (p->to.reg<<5) | p->from.reg;
		break;

	case 24:		/* mov/mvn Rs,Rd -> add $0,Rs,Rd or orr Rs,ZR,Rd */
		rf = p->from.reg;
		rt = p->to.reg;
		s = rf == REGSP || rt == REGSP;
		if(p->as == AMVN || p->as == AMVNW){
			if(s)
				diag("illegal SP reference\n%P", p);
			o1 = oprrr(p->as);
			o1 |= (rf<<16) | (REGZERO<<5) | rt;
		}else if(s){
			o1 = opirr(p->as);
			o1 |= (rf<<5) | rt;
		}else{
			o1 = oprrr(p->as);
			o1 |= (rf<<16) | (REGZERO<<5) | rt;
		}
		break;

	case 25: /* negX Rs, Rd -> subX Rs<<0, ZR, Rd */
		o1 = oprrr(p->as);
		rf = p->from.reg;
		rt = p->to.reg;
		o1 |= (rf<<16) | (REGZERO<<5) | rt;
		break;

	case 26: /* negX Rm<<s, Rd -> subX Rm<<s, ZR, Rd */
		o1 = oprrr(p->as);
		o1 |= p->from.offset;	/* includes reg, op, etc */
		rt = p->to.reg;
		o1 |= (REGZERO<<5) | rt;
		break;

	case 27:		/* op Rm<<n[,Rn],Rd (extended register) */
		o1 = opxrrr(p->as);
		if(p->from.type == D_EXTREG)
			o1 |= p->from.offset;	/* includes reg, op, etc */
		else
			o1 |= p->from.reg << 16;
		rt = p->to.reg;
		if(p->to.type == D_NONE)
			rt = REGZERO;
		r = p->reg;
		if(r == NREG)
			r = rt;
		o1 |= (r<<5) | rt;
		break;

	case 28:	/* logop $lcon, [R], R (64 bit literal) */
		o1 = omovlit(AMOV, p, &p->from, REGTMP);
		if(!o1)
			break;
		r = p->reg;
		if(r == NREG)
			r = p->to.reg;
		o2 = oprrr(p->as);
		o2 |= REGTMP << 16;	/* shift is 0 */
		o2 |= r << 5;
		o2 |= p->to.reg;
		break;

	case 29:	/* op Rn, Rd */
		o1 = oprrr(p->as);
		o1 |= p->from.reg<<5 | p->to.reg;
		break;

	case 30:	/* movT R,L(R) -> strT */
		s = movesize(o->as);
		if(s < 0)
			diag("unexpected long move, op %A tab %A\n%P", p->as, o->as, p);
		v = regoff(&p->to);
		/*
		 * TODO: negative offsets are fine and you only know it's
		 * negative here, not how big it is.
		 */
#ifdef normal
		if(v < 0)
			diag("negative large offset %#lux (%ld)\n%P", v, v, p);
#endif
		if((v & ((1<<s)-1)) != 0)
			diag("misaligned offset\n%P", p);
		hi = v - (v & (0xFFF<<s));
		if((hi & 0xFFF) != 0)
			diag("internal: miscalculated offset %ld [%d]\n%P", v, s, p);
		//fprint(2, "v=%ld (%#lux) s=%d hi=%ld (%#lux) v'=%ld (%#lux)\n", v, v, s, hi, hi, ((v-hi)>>s)&0xFFF, ((v-hi)>>s)&0xFFF);
		r = p->to.reg;
		if(r == NREG)
			r = o->param;
		o1 = oaddi(opirr(AADD), hi, r, REGTMP);
		o2 = olsr12u(opstr12(p->as), ((v-hi)>>s)&0xFFF, REGTMP, p->from.reg);
		break;

	case 31:	/* movT L(R), R -> ldrT */
		s = movesize(o->as);
		if(s < 0)
			diag("unexpected long move, op %A tab %A\n%P", p->as, o->as, p);
		v = regoff(&p->from);
		/*
		 * TODO: negative offsets are fine and you only know it's
		 * negative here, not how big it is.
		 */
#ifdef normal
		if(v < 0)
			diag("negative large offset %#lux (%ld)\n%P", v, v, p);
#endif
		if((v & ((1<<s)-1)) != 0)
			diag("misaligned offset\n%P", p);
		hi = v - (v & (0xFFF<<s));
		if((hi & 0xFFF) != 0)
			diag("internal: miscalculated offset %ld [%d]\n%P", v, s, p);
		//fprint(2, "v=%ld (%#lux) s=%d hi=%ld (%#lux) v'=%ld (%#lux)\n", v, v, s, hi, hi, ((v-hi)>>s)&0xFFF, ((v-hi)>>s)&0xFFF);
		r = p->from.reg;
		if(r == NREG)
			r = o->param;
		o1 = oaddi(opirr(AADD), hi, r, REGTMP);
		o2 = olsr12u(opldr12(p->as), ((v-hi)>>s)&0xFFF, REGTMP, p->to.reg);
		break;

	case 32:	/* mov $con, R -> movz/movn */
		r = 32;
		if(p->as == AMOV)
			r = 64;
		d = p->from.offset;
		s = movcon(d);
		if(s < 0 || s >= r){
			d = ~d;
			s = movcon(d);
			if(s < 0 || s >= r)
				diag("impossible move wide: %#llux\n%P", p->from.offset, p);
			if(p->as == AMOV)
				o1 = opirr(AMOVN);
			else
				o1 = opirr(AMOVNW);
		}else{
			if(p->as == AMOV)
				o1 = opirr(AMOVZ);
			else
				o1 = opirr(AMOVZW);
		}
		rt = p->to.reg;
		o1 |= (((d>>(s*16))& 0xFFFF) << 5) | ((s&3)<<21) | rt;
		break;

	case 33:	/* movk $uimm16 << pos */
		o1 = opirr(p->as);
		d = p->from.offset;
		if((d>>16) != 0)
			diag("requires uimm16\n%P", p);
		s = 0;
		if(p->from3.type != D_NONE){
			if(p->from3.type != D_CONST)
				diag("missing bit position\n%P", p);
			s = p->from3.offset;
			if((s&0xF) != 0 || (s /= 16) >= 4 || (o1&S64) == 0 && s >= 2)
				diag("illegal bit position\n%P", p);
		}
		rt = p->to.reg;
		o1 |= ((d & 0xFFFF) << 5) | ((s&3)<<21) | rt;
		break;
		
	case 34:	/* mov $lacon,R */
		o1 = omovlit(AMOV, p, &p->from, REGTMP);
		if(!o1)
			break;

		o2 = opxrrr(AADD);
		o2 |= REGTMP << 16;
		o2 |= LSL0_64;
		r = p->from.reg;
		if(r == NREG)
			r = o->param;
		o2 |= r << 5;
		o2 |= p->to.reg;
		break;

	case 35:	/* mov SPR,R -> mrs */
		o1 = oprrr(AMRS);
		v = p->from.offset;
		if((o1 & (v & ~(3<<19))) != 0)
			diag("MRS register value overlap\n%P", p);
		o1 |= v;
		o1 |= p->to.reg;
		break;

	case 36:	/* mov R,SPR */
		o1 = oprrr(AMSR);
		v = p->to.offset;
		if((o1 & (v & ~(3<<19))) != 0)
			diag("MSR register value overlap\n%P", p);
		o1 |= v;
		o1 |= p->from.reg;
		break;

	case 37:	/* mov $con,PSTATEfield -> MSR [immediate] */
		if((p->from.offset&~(uvlong)0xF) != 0)
			diag("illegal immediate for PSTATE field\n%P", p);
		o1 = opirr(AMSR);
		o1 |= (p->from.offset&0xF) << 8;	/* Crm */
		v = 0;
		for(i = 0; i < nelem(pstatefield); i++)
			if(pstatefield[i].a == p->to.offset){
				v = pstatefield[i].b;
				break;
			}
		if(v == 0)
			diag("illegal PSTATE field for immediate move\n%P", p);
		o1 |= v;
		break;

	case 38:	/* clrex [$imm] */
		o1 = opimm(p->as);
		if(p->to.type == D_NONE)
			o1 |= 0xF<<8;
		else
			o1 |= (p->to.offset & 0xF)<<8;
		break;

	case 39:	/* cbz R, rel */
		o1 = opirr(p->as);
		o1 |= p->from.reg;
		o1 |= brdist(p, 0, 19, 2) << 5;
		break;

	case 40:	/* tbz */
		o1 = opirr(p->as);
		v = p->from.offset;
		if(v < 0 || v > 63)
			diag("illegal bit number\n%P", p);
		o1 |= ((v&0x20)<<(31-5)) | ((v&0x1F)<<19);
		o1 |= brdist(p, 0, 14, 2)<<5;
		o1 |= p->reg;
		break;

	case 41:	/* eret, nop, others with no operands */
		o1 = op0(p->as);
		break;

	case 42:	/* bfm R,r,s,R */
		o1 = opbfm(p->as, p->from.offset, p->from3.offset, p->reg, p->to.reg);
		break;

	case 43:	/* bfm aliases */
		r = p->from.offset;
		s = p->from3.offset;
		rf = p->reg;
		rt = p->to.reg;
		if(rf == NREG)
			rf = rt;
		switch(p->as){
		case ABFI:		o1 = opbfm(ABFM, 64-r, s-1, rf, rt); break;
		case ABFIW:	o1 = opbfm(ABFMW, 32-r, s-1, rf, rt); break;
		case ABFXIL:	o1 = opbfm(ABFM, r, r+s-1, rf, rt); break;
		case ABFXILW:	o1 = opbfm(ABFMW, r, r+s-1, rf, rt); break;
		case ASBFIZ:	o1 = opbfm(ASBFM, 64-r, s-1, rf, rt); break;
		case ASBFIZW:	o1 = opbfm(ASBFMW, 32-r, s-1, rf, rt); break;
		case ASBFX:	o1 = opbfm(ASBFM, r, r+s-1, rf, rt); break;
		case ASBFXW:	o1 = opbfm(ASBFMW, r, r+s-1, rf, rt); break;
		case AUBFIZ:	o1 = opbfm(AUBFM, 64-r, s-1, rf, rt); break;
		case AUBFIZW:	o1 = opbfm(AUBFMW, 32-r, s-1, rf, rt); break;
		case AUBFX:	o1 = opbfm(AUBFM, r, r+s-1, rf, rt); break;
		case AUBFXW:	o1 = opbfm(AUBFMW, r, r+s-1, rf, rt); break;
		default:
			diag("bad bfm alias\n%P", curp);
			break;
		}
		break;

	case 44:	/* extr $b, Rn, Rm, Rd */
		o1 = opextr(p->as, p->from.offset, p->from3.reg, p->reg, p->to.reg);
		break;

	case 45:	/* sxt/uxt[bhw] R,R; movT R,R -> sxtT R,R */
		rf = p->from.reg;
		rt = p->to.reg;
		as = p->as;
		if(rf == REGZERO)
			as = AMOVWU;	/* clearer in disassembly */
		switch(as){
		case AMOVB:
		case ASXTB:	o1 = opbfm(ASBFM, 0, 7, rf, rt); break;
		case AMOVH:
		case ASXTH:	o1 = opbfm(ASBFM, 0, 15, rf, rt); break;
		case AMOVW:
		case ASXTW:	o1 = opbfm(ASBFM, 0, 31, rf, rt); break;
		case AMOVBU:
		case AUXTB:	o1 = opbfm(AUBFM, 0, 7, rf, rt); break;
		case AMOVHU:
		case AUXTH:	o1 = opbfm(AUBFM, 0, 15, rf, rt); break;
		case AMOVWU:	o1 = oprrr(as) | (rf<<16) | (REGZERO<<5) | rt; break;
		case AUXTW:	o1 = opbfm(AUBFM, 0, 31, rf, rt); break;
		case ASXTBW:	o1 = opbfm(ASBFMW, 0, 7, rf, rt); break;
		case ASXTHW:	o1 = opbfm(ASBFMW, 0, 15, rf, rt); break;
		case AUXTBW:	o1 = opbfm(AUBFMW, 0, 7, rf, rt); break;
		case AUXTHW:	o1 = opbfm(AUBFMW, 0, 15, rf, rt); break;
		default:	diag("bad sxt %A", as); break;
		}
		break;

	case 46:	/* cls */
		o1 = opbit(p->as);
		o1 |= p->from.reg<<5;
		o1 |= p->to.reg;
		break;

	case 47:	/* movT R,V(R) -> strT (huge offset) */
		o1 = omovlit(AMOVW, p, &p->to, REGTMP);
		if(!o1)
			break;
		r = p->to.reg;
		if(r == NREG)
			r = o->param;
		o2 = olsxrr(p->as, REGTMP,r, p->from.reg);
		break;

	case 48:	/* movT V(R), R -> ldrT (huge offset) */
		o1 = omovlit(AMOVW, p, &p->from, REGTMP);
		if(!o1)
			break;
		r = p->from.reg;
		if(r == NREG)
			r = o->param;
		o2 = olsxrr(p->as, REGTMP,r, p->to.reg);
		break;

	case 50:	/* sys/sysl */
		o1 = opirr(p->as);
		if((p->from.offset & ~SYSARG4(0x7, 0xF, 0xF, 0x7)) != 0)
			diag("illegal SYS argument\n%P", p);
		o1 |= p->from.offset;
		if(p->to.type == D_REG)
			o1 |= p->to.reg;
		else if(p->reg != NREG)
			o1 |= p->reg;
		else
			o1 |= 0x1F;
		break;

	case 51:	/* dmb */
		o1 = opirr(p->as);
		if(p->from.type == D_CONST)
			o1 |= (p->from.offset&0xF)<<8;
		break;

	case 52:	/* hint */
		o1 = opirr(p->as);
		o1 |= (p->from.offset&0x7F)<<5;
		break;

	case 53:	/* and/or/eor/bic/... $bimmN, Rn, Rd -> op (N,r,s), Rn, Rd */
		as = p->as;
		rt = p->to.reg;
		r = p->reg;
		if(r == NREG)
			r = rt;
		if(as == AMOV){
			as = AORR;
			r = REGZERO;
		}else if(as == AMOVW){
			as = AORRW;
			r = REGZERO;
		}
		o1 = opirr(as);
		s = o1 & S64? 64: 32;
		mask = findmask(p->from.offset);
		if(mask == nil)
			mask = findmask(p->from.offset | (p->from.offset<<32));
		if(mask != nil){
			o1 |= ((mask->r&(s-1))<<16) | (((mask->s-1)&(s-1))<<10);
			if(s == 64){
				if(mask->e == 64 && ((uvlong)p->from.offset>>32) != 0)
					o1 |= 1<<22;
			}else{
				u = (uvlong)p->from.offset >> 32;
				if(u != 0 && u != 0xFFFFFFFF)
					diag("mask needs 64 bits %#llux\n%P", p->from.offset, p);
			}
		}else
			diag("invalid mask %#llux\n%P", p->from.offset, p);	/* probably shouldn't happen */
		o1 |= (r<<5) | rt;
		break;

	case 54:	/* floating point arith */
		o1 = oprrr(p->as);
		if(p->from.type == D_FCONST) {
			rf = chipfloat(p->from.ieee);
			if(rf < 0 || 1){
				diag("invalid floating-point immediate\n%P", p);
				rf = 0;
			}
			rf |= (1<<3);
		} else
			rf = p->from.reg;
		rt = p->to.reg;
		r = p->reg;
		if((o1 & (0x1F<<24)) == (0x1E<<24) && (o1 & (1<<11)) == 0){	/* monadic */
			r = rf;
			rf = 0;
		}else if(r == NREG)
			r = rt;
		o1 |= (rf << 16) | (r<<5) | rt;
		break;

	case 56:	/* floating point compare */
		o1 = oprrr(p->as);
		if(p->from.type == D_FCONST) {
			if(p->from.ieee->h != 0 || p->from.ieee->l != 0)
				diag("invalid floating-point immediate\n%P", p);
			o1 |= 8;	/* zero */
			rf = 0;
		}else
			rf = p->from.reg;
		rt = p->reg;
		o1 |= rf<<16  | rt<<5;
		break;

	case 57:	/* floating point conditional compare */
		o1 = oprrr(p->as);
		cond = p->from.reg;
		nzcv = p->to.offset;
		if(nzcv & ~0xF)
			diag("implausible condition\n%P", p);
		rf = p->reg;
		if(p->from3.type != D_FREG)
			diag("illegal FCCMP\n%P", p);
		rt = p->from3.reg;
		o1 |= rf<<16 | cond<<12  | rt<<5 | nzcv;
		break;

	case 58:	/* ldxr */
		o1 = opload(p->as);
		o1 |= 0x1F<<16;
		o1 |= p->from.reg<<5;
		if(p->reg != NREG)
			o1 |= p->reg<<10;
		else
			o1 |= 0x1F<<10;
		o1 |= p->to.reg;
		break;

	case 59:	/* stxr */
		o1 = opstore(p->as);
		o1 |= p->reg << 16;
		if(p->from3.type != D_NONE)
			o1 |= p->from3.reg<<10;
		else
			o1 |= 0x1F<<10;
		o1 |= p->to.reg<<5;
		o1 |= p->from.reg;
		break;

	case 60:	/* adrp label,r */
		d = brdist(p, 12, 21, 0);
		o1 = ADR(1, d, p->to.reg);
		break;

	case 61:	/* adr label, r */
		d = brdist(p, 0, 21, 0);
		o1 = ADR(0, d, p->to.reg);
		break;

	case 62:	/* case Rv, Rt -> adr tab, Rt; movw Rt[R<<2], Rl; add Rt, Rl; br (Rl) */
		o1 = ADR(0, 4*4, p->to.reg);	/* adr 4(pc), Rt */
		o2 = (2<<30)|(7<<27)|(2<<22)|(1<<21)|(3<<13)|(1<<12)|(2<<10)|(p->from.reg<<16)|(p->to.reg<<5)|REGTMP;	/* movw Rt[Rv<<2], REGTMP */
		o3 = oprrr(AADD) | (p->to.reg<<16) | (REGTMP<<5) | REGTMP;	/* add Rt, REGTMP */
		o4 = (0x6b<<25)|(0x1F<<16)|(REGTMP<<5);	/* br (REGTMP) */
		lastcase = p;
		break;

	case 63:	/* bcase */
		if(lastcase == nil){
			diag("missing CASE\n%P", p);
			break;
		}
		if(p->cond != P) {
			o1 = p->cond->pc - (lastcase->pc + 4*4);
			if(dlm)
				dynreloc(S, p->pc, 1);
		}
		break;

	/* reloc ops */
	case 64:	/* movT R,addr */
		o1 = omovlit(AMOV, p, &p->to, REGTMP);
		if(!o1)
			break;
		o2 = olsr12u(opstr12(p->as), 0, REGTMP, p->from.reg);
		break;

	case 65:	/* movT addr,R */
		o1 = omovlit(AMOV, p, &p->from, REGTMP);
		if(!o1)
			break;
		o2 = olsr12u(opldr12(p->as), 0, REGTMP, p->to.reg);
		break;
	}

	if(debug['a'] > 1)
		Bprint(&bso, "%2d ", o->type);

	v = p->pc;
	switch(o->size) {
	default:
		if(debug['a'])
			Bprint(&bso, " %.8lux:\t\t%P\n", v, p);
		break;
	case 4:
		if(debug['a'])
			Bprint(&bso, " %.8lux: %.8lux\t%P\n", v, o1, p);
		lputl(o1);
		break;
	case 8:
		if(debug['a'])
			Bprint(&bso, " %.8lux: %.8lux %.8lux%P\n", v, o1, o2, p);
		lputl(o1);
		lputl(o2);
		break;
	case 12:
		if(debug['a'])
			Bprint(&bso, " %.8lux: %.8lux %.8lux %.8lux%P\n", v, o1, o2, o3, p);
		lputl(o1);
		lputl(o2);
		lputl(o3);
		break;
	case 16:
		if(debug['a'])
			Bprint(&bso, " %.8lux: %.8lux %.8lux %.8lux %.8lux%P\n",
				v, o1, o2, o3, o4, p);
		lputl(o1);
		lputl(o2);
		lputl(o3);
		lputl(o4);
		break;
	case 20:
		if(debug['a'])
			Bprint(&bso, " %.8lux: %.8lux %.8lux %.8lux %.8lux %.8lux%P\n",
				v, o1, o2, o3, o4, o5, p);
		lputl(o1);
		lputl(o2);
		lputl(o3);
		lputl(o4);
		lputl(o5);
		break;
	}
}

/*
 * basic Rm op Rn -> Rd (using shifted register with 0)
 * also op Rn -> Rt
 * also Rm*Rn op Ra -> Rd
 */
static long
oprrr(int a)
{
	switch(a) {
	case AADC:	return S64 | 0<<30 | 0<<29 | 0xd0<<21 | 0<<10;
	case AADCW:	return S32 | 0<<30 | 0<<29 | 0xd0<<21 | 0<<10;
	case AADCS:	return S64 | 0<<30 | 1<<29 | 0xd0<<21 | 0<<10;
	case AADCSW:	return S32 | 0<<30 | 1<<29 | 0xd0<<21 | 0<<10;

	case ANGC:
	case ASBC:	return S64 | 1<<30 | 0<<29 | 0xd0<<21 | 0<<10;
	case ANGCS:
	case ASBCS:	return S64 | 1<<30 | 1<<29 | 0xd0<<21 | 0<<10;
	case ANGCW:
	case ASBCW:	return S32 | 1<<30 | 0<<29 | 0xd0<<21 | 0<<10;
	case ANGCSW:
	case ASBCSW:	return S32 | 1<<30 | 1<<29 | 0xd0<<21 | 0<<10;

	case AADD:	return S64 | 0<<30 | 0<<29 | 0x0b<<24 | 0<<22 | 0<<21 | 0<<10;
	case AADDW:	return S32 | 0<<30 | 0<<29 | 0x0b<<24 | 0<<22 | 0<<21 | 0<<10;
	case ACMN:
	case AADDS:	return S64 | 0<<30 | 1<<29 | 0x0b<<24 | 0<<22 | 0<<21 | 0<<10;
	case ACMNW:
	case AADDSW:	return S32 | 0<<30 | 1<<29 | 0x0b<<24 | 0<<22 | 0<<21 | 0<<10;

	case ASUB:	return S64 | 1<<30 | 0<<29 | 0x0b<<24 | 0<<22 | 0<<21 | 0<<10;
	case ASUBW:	return S32 | 1<<30 | 0<<29 | 0x0b<<24 | 0<<22 | 0<<21 | 0<<10;
	case ACMP:
	case ASUBS:	return S64 | 1<<30 | 1<<29 | 0x0b<<24 | 0<<22 | 0<<21 | 0<<10;
	case ACMPW:
	case ASUBSW:	return S32 | 1<<30 | 1<<29 | 0x0b<<24 | 0<<22 | 0<<21 | 0<<10;

	case AAND:	return S64 | 0<<29 | 0xA<<24;
	case AANDW:	return S32 | 0<<29 | 0xA<<24;
	case AMOV:
	case AORR:	return S64 | 1<<29 | 0xA<<24;
//	case AMOVW:
	case AMOVWU:
	case AORRW:	return S32 | 1<<29 | 0xA<<24;
	case AEOR:	return S64 | 2<<29 | 0xA<<24;
	case AEORW:	return S32 | 2<<29 | 0xA<<24;
	case AANDS:	return S64 | 3<<29 | 0xA<<24;
	case AANDSW:	return S32 | 3<<29 | 0xA<<24;

	case ABIC:	return S64 | 0<<29 | 0xA<<24 | 1<<21;
	case ABICW:	return S32 | 0<<29 | 0xA<<24 | 1<<21;
	case ABICS:	return S64 | 3<<29 | 0xA<<24 | 1<<21;
	case ABICSW:	return S32 | 3<<29 | 0xA<<24 | 1<<21;
	case AEON:	return S64 | 2<<29 | 0xA<<24 | 1<<21;
	case AEONW:	return S32 | 2<<29 | 0xA<<24 | 1<<21;
	case AMVN:
	case AORN:	return S64 | 1<<29 | 0xA<<24 | 1<<21;
	case AMVNW:
	case AORNW:	return S32 | 1<<29 | 0xA<<24 | 1<<21;

	case AASR:	return S64 | OPDP2(10);	/* also ASRV */
	case AASRW:	return S32 | OPDP2(10);
	case ALSL:	return S64 | OPDP2(8);
	case ALSLW:	return S32 | OPDP2(8);
	case ALSR:	return S64 | OPDP2(9);
	case ALSRW:	return S32 | OPDP2(9);
	case AROR:	return S64 | OPDP2(11);
	case ARORW:	return S32 | OPDP2(11);

	case ACCMN:	return S64 | 0<<30 | 1<<29 | 0xD2<<21 | 0<<11 | 0<<10 | 0<<4;	/* cond<<12 | nzcv<<0 */
	case ACCMNW:	return S32 | 0<<30 | 1<<29 | 0xD2<<21 | 0<<11 | 0<<10 | 0<<4;
	case ACCMP:	return S64 | 1<<30 | 1<<29 | 0xD2<<21 | 0<<11 | 0<<10 | 0<<4;	/* imm5<<16 | cond<<12 | nzcv<<0 */
	case ACCMPW:	return S32 | 1<<30 | 1<<29 | 0xD2<<21 | 0<<11 | 0<<10 | 0<<4;

	case ACRC32B:		return S32 | OPDP2(16);
	case ACRC32H:		return S32 | OPDP2(17);
	case ACRC32W:	return S32 | OPDP2(18);
	case ACRC32X:		return S64 | OPDP2(19);
	case ACRC32CB:	return S32 | OPDP2(20);
	case ACRC32CH:	return S32 | OPDP2(21);
	case ACRC32CW:	return S32 | OPDP2(22);
	case ACRC32CX:	return S64 | OPDP2(23);

	case ACSEL:	return S64 | 0<<30 | 0<<29 | 0xD4<<21 | 0<<11 | 0<<10;
	case ACSELW:	return S32 | 0<<30 | 0<<29 | 0xD4<<21 | 0<<11 | 0<<10;
	case ACSET:	return S64 | 0<<30 | 0<<29 | 0xD4<<21 | 0<<11 | 1<<10;
	case ACSETW:	return S32 | 0<<30 | 0<<29 | 0xD4<<21 | 0<<11 | 1<<10;
	case ACSETM:	return S64 | 1<<30 | 0<<29 | 0xD4<<21 | 0<<11 | 0<<10;
	case ACSETMW:	return S32 | 1<<30 | 0<<29 | 0xD4<<21 | 0<<11 | 0<<10;
	case ACINC:
	case ACSINC:	return S64 | 0<<30 | 0<<29 | 0xD4<<21 | 0<<11 | 1<<10;
	case ACINCW:
	case ACSINCW:	return S32 | 0<<30 | 0<<29 | 0xD4<<21 | 0<<11 | 1<<10;
	case ACINV:
	case ACSINV:	return S64 | 1<<30 | 0<<29 | 0xD4<<21 | 0<<11 | 0<<10;
	case ACINVW:
	case ACSINVW:	return S32 | 1<<30 | 0<<29 | 0xD4<<21 | 0<<11 | 0<<10;
	case ACNEG:
	case ACSNEG:	return S64 | 1<<30 | 0<<29 | 0xD4<<21 | 0<<11 | 1<<10;
	case ACNEGW:
	case ACSNEGW:	return S32 | 1<<30 | 0<<29 | 0xD4<<21 | 0<<11 | 1<<10;

	case AMUL:
	case AMADD:	return S64 | 0<<29 | 0x1B<<24 | 0<<21 | 0<<15;
	case AMULW:
	case AMADDW:	return S32 | 0<<29 | 0x1B<<24 | 0<<21 | 0<<15;
	case AMNEG:
	case AMSUB:	return S64 | 0<<29 | 0x1B<<24 | 0<<21 | 1<<15;
	case AMNEGW:
	case AMSUBW:	return S32 | 0<<29 | 0x1B<<24 | 0<<21 | 1<<15;

	case AMRS:	return SYSOP(1,2,0,0,0,0,0);
	case AMSR:	return SYSOP(0,2,0,0,0,0,0);

	case ANEG:	return S64 | 1<<30 | 0<<29 | 0xB<<24 | 0<<21;
	case ANEGW:	return S32 | 1<<30 | 0<<29 | 0xB<<24 | 0<<21;
	case ANEGS:	return S64 | 1<<30 | 1<<29 | 0xB<<24 | 0<<21;
	case ANEGSW:	return S32 | 1<<30 | 1<<29 | 0xB<<24 | 0<<21;

	case AREM:
	case ASDIV:	return S64 | OPDP2(3);
	case AREMW:
	case ASDIVW:	return S32 | OPDP2(3);

	case ASMULL:
	case ASMADDL:	return OPDP3(1, 0, 1, 0);
	case ASMNEGL:
	case ASMSUBL:	return OPDP3(1, 0, 1, 1);
	case ASMULH:	return OPDP3(1, 0, 2, 0);
	case AUMULL:
	case AUMADDL:	return OPDP3(1, 0, 5, 0);
	case AUMNEGL:
	case AUMSUBL:	return OPDP3(1, 0, 5, 1);
	case AUMULH:	return OPDP3(1, 0, 6, 0);

	case AUREM:
	case AUDIV:	return S64 | OPDP2(2);
	case AUREMW:
	case AUDIVW:	return S32 | OPDP2(2);

	case AAESE:	return 0x4E<<24 | 2<<20 | 8<<16 | 4<<12 | 2<<10;
	case AAESD:	return 0x4E<<24 | 2<<20 | 8<<16 | 5<<12 | 2<<10;
	case AAESMC:	return 0x4E<<24 | 2<<20 | 8<<16 | 6<<12 | 2<<10;
	case AAESIMC:	return 0x4E<<24 | 2<<20 | 8<<16 | 7<<12 | 2<<10;

	case ASHA1C:	return 0x5E<<24 | 0<<12;
	case ASHA1P:	return 0x5E<<24 | 1<<12;
	case ASHA1M:	return 0x5E<<24 | 2<<12;
	case ASHA1SU0:	return 0x5E<<24 | 3<<12;
	case ASHA256H:	return 0x5E<<24 | 4<<12;
	case ASHA256H2:	return 0x5E<<24 | 5<<12;
	case ASHA256SU1:	return 0x5E<<24 | 6<<12;

	case ASHA1H:	return 0x5E<<24 | 2<<20 | 8<<16 | 0<<12 | 2<<10;
	case ASHA1SU1:	return 0x5E<<24 | 2<<20 | 8<<16 | 1<<12 | 2<<10;
	case ASHA256SU0:	return 0x5E<<24 | 2<<20 | 8<<16 | 2<<12 | 2<<10;

	case AFCVTZSD:	return FPCVTI(1, 0, 1, 3, 0);
	case AFCVTZSDW:	return FPCVTI(0, 0, 1, 3, 0);
	case AFCVTZSS:	return FPCVTI(1, 0, 0, 3, 0);
	case AFCVTZSSW:	return FPCVTI(0, 0, 0, 3, 0);

	case AFCVTZUD:	return FPCVTI(1, 0, 1, 3, 1);
	case AFCVTZUDW:	return FPCVTI(0, 0, 1, 3, 1);
	case AFCVTZUS:	return FPCVTI(1, 0, 0, 3, 1);
	case AFCVTZUSW:	return FPCVTI(0, 0, 0, 3, 1);

	case ASCVTFD:		return FPCVTI(1, 0, 1, 0, 2);
	case ASCVTFS:		return FPCVTI(1, 0, 0, 0, 2);
	case ASCVTFWD:	return FPCVTI(0, 0, 1, 0, 2);
	case ASCVTFWS:	return FPCVTI(0, 0, 0, 0, 2);

	case AUCVTFD:		return FPCVTI(1, 0, 1, 0, 3);
	case AUCVTFS:		return FPCVTI(1, 0, 0, 0, 3);
	case AUCVTFWD:	return FPCVTI(0, 0, 1, 0, 3);
	case AUCVTFWS:	return FPCVTI(0, 0, 0, 0, 3);

	case AFADDS:	return FPOP2S(0, 0, 0, 2);
	case AFADDD:	return FPOP2S(0, 0, 1, 2);
	case AFSUBS:	return FPOP2S(0, 0, 0, 3);
	case AFSUBD:	return FPOP2S(0, 0, 1, 3);
	case AFMULS:	return FPOP2S(0, 0, 0, 0);
	case AFMULD:	return FPOP2S(0, 0, 1, 0);
	case AFDIVS:	return FPOP2S(0, 0, 0, 1);
	case AFDIVD:	return FPOP2S(0, 0, 1, 1);
	case AFMAXS:	return FPOP2S(0, 0, 0, 4);
	case AFMINS:	return FPOP2S(0, 0, 0, 5);
	case AFMAXD:	return FPOP2S(0, 0, 1, 4);
	case AFMIND:	return FPOP2S(0, 0, 1, 5);
	case AFMAXNMS:	return FPOP2S(0, 0, 0, 6);
	case AFMAXNMD:	return FPOP2S(0, 0, 1, 6);
	case AFMINNMS:	return FPOP2S(0, 0, 0, 7);
	case AFMINNMD:	return FPOP2S(0, 0, 1, 7);
	case AFNMULS:		return FPOP2S(0, 0, 0, 8);
	case AFNMULD:	return FPOP2S(0, 0, 1, 8);

	case AFCMPS:	return FPCMP(0, 0, 0, 0, 0);
	case AFCMPD:	return FPCMP(0, 0, 1, 0, 0);
	case AFCMPES:	return FPCMP(0, 0, 0, 0, 16);
	case AFCMPED:	return FPCMP(0, 0, 1, 0, 16);

	case AFCCMPS:		return FPCCMP(0, 0, 0, 0);
	case AFCCMPD:	return FPCCMP(0, 0, 1, 0);
	case AFCCMPES:	return FPCCMP(0, 0, 0, 1);
	case AFCCMPED:	return FPCCMP(0, 0, 1, 1);

	case AFCSELS:	return 0x1E<<24 | 0<<22 | 1<<21 | 3<<10;
	case AFCSELD:	return 0x1E<<24 | 1<<22 | 1<<21 | 3<<10;

	case AFMOVS:	return FPOP1S(0, 0, 0, 0);
	case AFABSS:	return FPOP1S(0, 0, 0, 1);
	case AFNEGS:	return FPOP1S(0, 0, 0, 2);
	case AFSQRTS:	return FPOP1S(0, 0, 0, 3);
	case AFCVTSD:	return FPOP1S(0, 0, 0, 5);
	case AFCVTSH:	return FPOP1S(0, 0, 0, 7);
	case AFRINTNS: return FPOP1S(0, 0, 0, 8);
	case AFRINTPS:	return FPOP1S(0, 0, 0, 9);
	case AFRINTMS:	return FPOP1S(0, 0, 0, 10);
	case AFRINTZS:	return FPOP1S(0, 0, 0, 11);
	case AFRINTAS:	return FPOP1S(0, 0, 0, 12);
	case AFRINTXS:	return FPOP1S(0, 0, 0, 14);
	case AFRINTIS:	return FPOP1S(0, 0, 0, 15);

	case AFMOVD:	return FPOP1S(0, 0, 1, 0);
	case AFABSD:	return FPOP1S(0, 0, 1, 1);
	case AFNEGD:	return FPOP1S(0, 0, 1, 2);
	case AFSQRTD:	return FPOP1S(0, 0, 1, 3);
	case AFCVTDS:	return FPOP1S(0, 0, 1, 4);
	case AFCVTDH:	return FPOP1S(0, 0, 1, 7);
	case AFRINTND:	return FPOP1S(0, 0, 1, 8);
	case AFRINTPD:	return FPOP1S(0, 0, 1, 9);
	case AFRINTMD:	return FPOP1S(0, 0, 1, 10);
	case AFRINTZD:	return FPOP1S(0, 0, 1, 11);
	case AFRINTAD:	return FPOP1S(0, 0, 1, 12);
	case AFRINTXD:	return FPOP1S(0, 0, 1, 14);
	case AFRINTID:	return FPOP1S(0, 0, 1, 15);
	case AFCVTHS:	return FPOP1S(0, 0, 3, 4);
	case AFCVTHD:	return FPOP1S(0, 0, 3, 5);

	}
	diag("bad rrr %d %A", a, a);
	prasm(curp);
	return 0;
}

/*
 * imm -> Rd
 * imm op Rn -> Rd
 */
static long
opirr(int a)
{
	switch(a){

	/* op $addcon, Rn, Rd */
	case AMOV:
	case AADD:	return S64 | 0<<30 | 0<<29 | 0x11<<24;
	case ACMN:
	case AADDS:	return S64 | 0<<30 | 1<<29 | 0x11<<24;
	case AMOVW:
	case AADDW:	return S32 | 0<<30 | 0<<29 | 0x11<<24;
	case ACMNW:
	case AADDSW:	return S32 | 0<<30 | 1<<29 | 0x11<<24;
	case ASUB:	return S64 | 1<<30 | 0<<29 | 0x11<<24;
	case ACMP:
	case ASUBS:	return S64 | 1<<30 | 1<<29 | 0x11<<24;
	case ASUBW:	return S32 | 1<<30 | 0<<29 | 0x11<<24;
	case ACMPW:
	case ASUBSW:	return S32 | 1<<30 | 1<<29 | 0x11<<24;

	/* op $imm(SB), Rd; op label, Rd */
	case AADR:		return 0<<31 | 0x10<<24;
	case AADRP:	return 1<<31 | 0x10<<24;

	/* op $bimm, Rn, Rd */
	case AAND:	return S64 | 0<<29 | 0x24<<23;
	case AANDW:	return S32 | 0<<29 | 0x24<<23 | 0<<22;
	case AORR:	return S64 | 1<<29 | 0x24<<23;
	case AORRW:	return S32 | 1<<29 | 0x24<<23 | 0<<22;
	case AEOR:	return S64 | 2<<29 | 0x24<<23;
	case AEORW:	return S32 | 2<<29 | 0x24<<23 | 0<<22;
	case AANDS:	return S64 | 3<<29 | 0x24<<23;
	case AANDSW:	return S32 | 3<<29 | 0x24<<23 | 0<<22;

	case AASR:	return S64 | 0<<29 | 0x26<<23;	/* alias of SBFM */
	case AASRW:	return S32 | 0<<29 | 0x26<<23 | 0<<22;

	/* op $width, $lsb, Rn, Rd */
	case ABFI:		return S64 | 2<<29 | 0x26<<23 | 1<<22;	/* alias of BFM */
	case ABFIW:	return S32 | 2<<29 | 0x26<<23 | 0<<22;

	/* op $imms, $immr, Rn, Rd */
	case ABFM:	return S64 | 1<<29 | 0x26<<23 | 1<<22;
	case ABFMW:	return S32 | 1<<29 | 0x26<<23 | 0<<22;
	case ASBFM:	return S64 | 0<<29 | 0x26<<23 | 1<<22;
	case ASBFMW:	return S32 | 0<<29 | 0x26<<23 | 0<<22;
	case AUBFM:	return S64 | 2<<29 | 0x26<<23 | 1<<22;
	case AUBFMW:	return S32 | 2<<29 | 0x26<<23 | 0<<22;

	case ABFXIL:	return S64 | 1<<29 | 0x26<<23 | 1<<22;	/* alias of BFM */
	case ABFXILW:	return S32 | 1<<29 | 0x26<<23 | 0<<22;

	case AEXTR:	return S64 | 0<<29 | 0x27<<23 | 1<<22 | 0<<21;
	case AEXTRW:	return S32 | 0<<29 | 0x27<<23 | 0<<22 | 0<<21;

	case ACBNZ:	return S64 | 0x1A<<25 | 1<<24;
	case ACBNZW:	return S32 | 0x1A<<25 | 1<<24;
	case ACBZ:	return S64 | 0x1A<<25 | 0<<24;
	case ACBZW:	return S32 | 0x1A<<25 | 0<<24;

	case ACCMN:	return S64 | 0<<30 | 1<<29 | 0xD2<<21 | 1<<11 | 0<<10 | 0<<4;	/* imm5<<16 | cond<<12 | nzcv<<0 */
	case ACCMNW:	return S32 | 0<<30 | 1<<29 | 0xD2<<21 | 1<<11 | 0<<10 | 0<<4;
	case ACCMP:	return S64 | 1<<30 | 1<<29 | 0xD2<<21 | 1<<11 | 0<<10 | 0<<4;	/* imm5<<16 | cond<<12 | nzcv<<0 */
	case ACCMPW:	return S32 | 1<<30 | 1<<29 | 0xD2<<21 | 1<<11 | 0<<10 | 0<<4;

	case AMOVK:	return S64 | 3<<29 | 0x25<<23;
	case AMOVKW:	return S32 | 3<<29 | 0x25<<23;

	case AMOVN:	return S64 | 0<<29 | 0x25<<23;
	case AMOVNW:	return S32 | 0<<29 | 0x25<<23;
	case AMOVZ:	return S64 | 2<<29 | 0x25<<23;
	case AMOVZW:	return S32 | 2<<29 | 0x25<<23;

	case AMSR:	return SYSOP(0,0,0,4,0,0,0x1F);	/* MSR (immediate) */

	case AAT:
	case ADC:
	case AIC:
	case ATLBI:
	case ASYS:	return SYSOP(0,1,0,0,0,0,0);
	case ASYSL:	return SYSOP(1,1,0,0,0,0,0);

	case ATBZ:	return 0x36<<24;
	case ATBNZ:	return 0x37<<24;

	case ADSB:	return SYSOP(0,0,3,3,0,4,0x1F);
	case ADMB:	return SYSOP(0,0,3,3,0,5,0x1F);
	case AISB:		return SYSOP(0,0,3,3,0,6,0x1F);
	case AHINT:	return SYSOP(0,0,3,2,0,0,0x1F);

	}
	diag("bad irr %A", a);
	prasm(curp);
	return 0;
}

/*
 * bit operations
 */
#define	OPBIT(x)	(1<<30 | 0<<29 | 0xD6<<21 | 0<<16 | (x)<<10)

static long
opbit(int a)
{
	switch(a){
	case ACLS:	return S64 | OPBIT(5);
	case ACLSW:	return S32 | OPBIT(5);
	case ACLZ:	return S64 | OPBIT(4);
	case ACLZW:	return S32 | OPBIT(4);
	case ARBIT:	return S64 | OPBIT(0);
	case ARBITW:	return S32 | OPBIT(0);
	case AREV:	return S64 | OPBIT(3);
	case AREVW:	return S32 | OPBIT(2);
	case AREV16:	return S64 | OPBIT(1);
	case AREV16W:	return S32 | OPBIT(1);
	case AREV32:	return S64 | OPBIT(2);
	default:
		diag("bad bit op\n%P", curp);
		return 0;
	}
}

/*
 * add/subtract extended register
 */
static long
opxrrr(int a)
{
	switch(a) {
	case AADD:	return S64 | 0<<30 | 0<<29 | 0x0b<<24 | 0<<22 | 1<<21 | LSL0_64;
	case AADDW:	return S32 | 0<<30 | 0<<29 | 0x0b<<24 | 0<<22 | 1<<21 | LSL0_32;
	case ACMN:
	case AADDS:	return S64 | 0<<30 | 1<<29 | 0x0b<<24 | 0<<22 | 1<<21 | LSL0_64;
	case ACMNW:
	case AADDSW:	return S32 | 0<<30 | 1<<29 | 0x0b<<24 | 0<<22 | 1<<21 | LSL0_32;

	case ASUB:	return S64 | 1<<30 | 0<<29 | 0x0b<<24 | 0<<22 | 1<<21 | LSL0_64;
	case ASUBW:	return S32 | 1<<30 | 0<<29 | 0x0b<<24 | 0<<22 | 1<<21 | LSL0_32;
	case ACMP:
	case ASUBS:	return S64 | 1<<30 | 1<<29 | 0x0b<<24 | 0<<22 | 1<<21 | LSL0_64;
	case ACMPW:
	case ASUBSW:	return S32 | 1<<30 | 1<<29 | 0x0b<<24 | 0<<22 | 1<<21 | LSL0_32;

	}
	diag("bad opxrrr %A\n%P", a, curp);
	return 0;
}

static long
opimm(int a)
{
	switch(a){
	case ASVC:	return 0xD4<<24 | 0<<21 | 1;	/* imm16<<5 */
	case AHVC:	return 0xD4<<24 | 0<<21 | 2;
	case ASMC:	return 0xD4<<24 | 0<<21 | 3;
	case ABRK:	return 0xD4<<24 | 1<<21 | 0;
	case AHLT:	return 0xD4<<24 | 2<<21 | 0;
	case ADCPS1:	return 0xD4<<24 | 5<<21 | 1;
	case ADCPS2:	return 0xD4<<24 | 5<<21 | 2;
	case ADCPS3:	return 0xD4<<24 | 5<<21 | 3;

	case ACLREX:	return SYSOP(0,0,3,3,0,2,0x1F);
	}
	diag("bad imm %A", a);
	prasm(curp);
	return 0;
}

static vlong
brdist(Prog *p, int preshift, int flen, int shift)
{
	vlong v, t;
	Sym *s;

	v = 0;
	if(p->cond == UP) {
		s = p->to.sym;
		if(s->type != SUNDEF)
			diag("bad branch sym type");
		v = (uvlong)s->value >> (Roffset-2);
		dynreloc(s, p->pc, 0);	/* TO DO */
	}
	else if(p->cond != P)
		v = (p->cond->pc>>preshift) - (pc>>preshift);
	if((v & ((1<<shift)-1)) != 0)
		diag("misaligned label\n%P", p);
	v >>= shift;
	t = (vlong)1 << (flen-1);
	if(v < -t || v >= t)
		diag("branch too far\n%P", p);
	return v & ((t<<1)-1);
}

/*
 * pc-relative branches
 */
static long
opbra(int a)
{
	switch(a) {
	case ABEQ:	return OPBcc(0x0);
	case ABNE:	return OPBcc(0x1);
	case ABCS:	return OPBcc(0x2);
	case ABHS:	return OPBcc(0x2);
	case ABCC:	return OPBcc(0x3);
	case ABLO:	return OPBcc(0x3);
	case ABMI:	return OPBcc(0x4);
	case ABPL:	return OPBcc(0x5);
	case ABVS:	return OPBcc(0x6);
	case ABVC:	return OPBcc(0x7);
	case ABHI:	return OPBcc(0x8);
	case ABLS:	return OPBcc(0x9);
	case ABGE:	return OPBcc(0xa);
	case ABLT:	return OPBcc(0xb);
	case ABGT:	return OPBcc(0xc);
	case ABLE:	return OPBcc(0xd);		/* imm19<<5 | cond */
	case AB:		return 0<<31 | 5<<26;	/* imm26 */
	case ABL:		return 1<<31 | 5<<26;
	}
	diag("bad bra %A", a);
	prasm(curp);
	return 0;
}

static long
opbrr(int a)
{
	switch(a){
	case ABL:		return OPBLR(1);		/* BLR */
	case AB:		return OPBLR(0);		/* BR */
	case ARET:	return OPBLR(2);		/* RET */
	}
	diag("bad brr %A", a);
	prasm(curp);
	return 0;
}

static long
op0(int a)
{
	switch(a){
	case ADRPS:	return 0x6B<<25 | 5<<21 | 0x1F<<16 | 0x1F<<5;
	case AERET:	return 0x6B<<25 | 4<<21 | 0x1F<<16 | 0<<10 | 0x1F<<5;
	case ANOP:	return SYSHINT(0);
	case AYIELD:	return SYSHINT(1);
	case AWFE:	return SYSHINT(2);
	case AWFI:	return SYSHINT(3);
	case ASEV:	return SYSHINT(4);
	case ASEVL:	return SYSHINT(5);
	}
	diag("bad op0 %A", a);
	prasm(curp);
	return 0;
}

/*
 * register offset
 */
static long
opload(int a)
{
	switch(a){
	case ALDAR:	return LDSTX(3,1,1,0,1) | 0x1F<<10;
	case ALDARW:	return LDSTX(2,1,1,0,1) | 0x1F<<10;
	case ALDARB:	return LDSTX(0,1,1,0,1) | 0x1F<<10;
	case ALDARH:	return LDSTX(1,1,1,0,1) | 0x1F<<10;
	case ALDAXP:	return LDSTX(3,0,1,1,1);
	case ALDAXPW:	return LDSTX(2,0,1,1,1);
	case ALDAXR:	return LDSTX(3,0,1,0,1) | 0x1F<<10;
	case ALDAXRW:	return LDSTX(2,1,1,0,1) | 0x1F<<10;
	case ALDAXRB:	return LDSTX(0,0,1,0,1) | 0x1F<<10;
	case ALDAXRH:	return LDSTX(1,0,1,0,1) | 0x1F<<10;
	case ALDXR:		return LDSTX(3,0,1,0,0) | 0x1F<<10;
	case ALDXRB:		return LDSTX(0,0,1,0,0) | 0x1F<<10;
	case ALDXRH:		return LDSTX(1,0,1,0,0) | 0x1F<<10;
	case ALDXRW:		return LDSTX(2,0,1,0,0) | 0x1F<<10;
	case ALDXP:		return LDSTX(3,0,1,1,0);
	case ALDXPW:		return LDSTX(2,0,1,1,0);
	case AMOVNP:	return S64 | 0<<30 | 5<<27 | 0<<26 | 0<<23 | 1<<22;
	case AMOVNPW:	return S32 | 0<<30 | 5<<27 | 0<<26 | 0<<23 | 1<<22;
	}
	diag("bad opload %A\n%P", a, curp);
	return 0;
}

static long
opstore(int a)
{
	switch(a){
	case ASTLR:		return LDSTX(3,1,0,0,1) | 0x1F<<10;
	case ASTLRB:		return LDSTX(0,1,0,0,1) | 0x1F<<10;
	case ASTLRH:		return LDSTX(1,1,0,0,1) | 0x1F<<10;
	case ASTLP:		return LDSTX(3,0,0,1,1);
	case ASTLPW:		return LDSTX(2,0,0,1,1);
	case ASTLRW:		return LDSTX(2,1,0,0,1) | 0x1F<<10;
	case ASTLXP:		return LDSTX(2,0,0,1,1);
	case ASTLXPW:		return LDSTX(3,0,0,1,1);
	case ASTLXR:		return LDSTX(3,0,0,0,1) | 0x1F<<10;
	case ASTLXRB:		return LDSTX(0,0,0,0,1) | 0x1F<<10;
	case ASTLXRH:		return LDSTX(1,0,0,0,1) | 0x1F<<10;
	case ASTLXRW:		return LDSTX(2,0,0,0,1) | 0x1F<<10;
	case ASTXR:		return LDSTX(3,0,0,0,0) | 0x1F<<10;
	case ASTXRB:		return LDSTX(0,0,0,0,0) | 0x1F<<10;
	case ASTXRH:		return LDSTX(1,0,0,0,0) | 0x1F<<10;
	case ASTXP:		return LDSTX(3,0,0,1,0);
	case ASTXPW:		return LDSTX(2,0,0,1,0);
	case ASTXRW:		return LDSTX(2,0,0,0,0) | 0x1F<<10;
	case AMOVNP:	return S64 | 0<<30 | 5<<27 | 0<<26 | 0<<23 | 1<<22;
	case AMOVNPW:	return S32 | 0<<30 | 5<<27 | 0<<26 | 0<<23 | 1<<22;
	}
	diag("bad opstore %A\n%P", a, curp);
	return 0;
}

/*
 * load/store register (unsigned immediate) C3.3.13
 *	these produce 64-bit values (when there's an option)
 */

static long
olsr12u(long o, long v, int b, int r)
{
	if(v < 0 || v >= (1<<12))
		diag("offset out of range: %ld\n%P", v, curp);
	o |= (v&0xFFF)<<10;
	o |= b << 5;
	o |= r;
	return o;
}

static long
opldr12(int a)
{
	switch(a){
	case AMOV:	return LDSTR12U(3, 0, 1);	/* imm12<<10 | Rn<<5 | Rt */
	case AMOVW:	return LDSTR12U(2, 0, 2);
	case AMOVWU:	return LDSTR12U(2, 0, 1);
	case AMOVH:	return LDSTR12U(1, 0, 2);
	case AMOVHU:	return LDSTR12U(1, 0, 1);
	case AMOVB:	return LDSTR12U(0, 0, 2);
	case AMOVBU:	return LDSTR12U(0, 0, 1);
	case AFMOVS:	return LDSTR12U(2, 1, 1);
	case AFMOVD:	return LDSTR12U(3, 1, 1);
	}
	diag("bad opldr12 %A\n%P", a, curp);
	return 0;
}

static long
opstr12(int a)
{
	return LD2STR(opldr12(a));
}

/* 
 * load/store register (unscaled immediate) C3.3.12
 */

static long
olsr9s(long o, long v, int b, int r)
{
	if(v < -256 || v > 255)
		diag("offset out of range: %ld\n%P", v, curp);
	o |= (v&0x1FF)<<12;
	o |= b << 5;
	o |= r;
	return o;
}

static long
opldr9(int a)
{
	switch(a){
	case AMOV:	return LDSTR9S(3, 0, 1);	/* simm9<<12 | Rn<<5 | Rt */
	case AMOVW:	return LDSTR9S(2, 0, 2);
	case AMOVWU:	return LDSTR9S(2, 0, 1);
	case AMOVH:	return LDSTR9S(1, 0, 2);
	case AMOVHU:	return LDSTR9S(1, 0, 1);
	case AMOVB:	return LDSTR9S(0, 0, 2);
	case AMOVBU:	return LDSTR9S(0, 0, 1);
	case AFMOVS:	return LDSTR9S(2, 1, 1);
	case AFMOVD:	return LDSTR9S(3, 1, 1);
	}
	diag("bad opldr9 %A\n%P", a, curp);
	return 0;
}

static long
opstr9(int a)
{
	return LD2STR(opldr9(a));
}

static long
opldrpp(int a)
{
	switch(a){
	case AMOV:	return 3<<30 | 7<<27 | 0<<26 | 0<<24 | 1<<22;	/* simm9<<12 | Rn<<5 | Rt */
	case AMOVW:	return 2<<30 | 7<<27 | 0<<26 | 0<<24 | 2<<22;
	case AMOVWU:	return 2<<30 | 7<<27 | 0<<26 | 0<<24 | 1<<22;
	case AMOVH:	return 1<<30 | 7<<27 | 0<<26 | 0<<24 | 2<<22;
	case AMOVHU:	return 1<<30 | 7<<27 | 0<<26 | 0<<24 | 1<<22;
	case AMOVB:	return 0<<30 | 7<<27 | 0<<26 | 0<<24 | 2<<22;
	case AMOVBU:	return 0<<30 | 7<<27 | 0<<26 | 0<<24 | 1<<22;
	}
	diag("bad opldr %A\n%P", a, curp);
	return 0;
}

/*
 * load/store register (extended register)
 */
static long
olsxrr(int, int, int, int)
{
	diag("need load/store extended register\n%P", curp);
	return -1;
}

static long
oaddi(long o1, long v, int r, int rt)
{
	if((v & 0xFFF000) != 0){
		v >>= 12;
		o1 |= 1<<22;
	}
	o1 |= ((v & 0xFFF) << 10) | (r<<5) | rt;
	return o1;
}

/*
 * load a a literal value into dr
 */
static long
omovlit(int as, Prog *p, Adr *a, int dr)
{	
	long v, o1;
	int w, fp;

	if(p->cond == nil){	/* not in literal pool */
		aclass(a);
fprint(2, "omovlit add %lld (%#llux)\n", instoffset, instoffset);
		/* TO DO: could be clever, and use general constant builder */
		o1 = opirr(AADD);
		v = instoffset;
		if(v != 0 && (v & 0xFFF) == 0){
			v >>= 12;
			o1 |= 1<<22;	/* shift, by 12 */
		}
		o1 |= ((v& 0xFFF) << 10) | (REGZERO<<5) | dr;
	}else{
		fp = 0;
		w = 0;	/* default: 32 bit, unsigned */
		switch(as){
		case AFMOVS:
			fp = 1;
			break;
		case AFMOVD:
			fp = 1;
			w = 1;	/* 64 bit simd&fp */
			break;
		case AMOV:
			if(p->cond->as == ADWORD)
				w = 1;	/* 64 bit */
			else if(p->cond->to.offset < 0)
				w = 2;	/* sign extend */
			break;
		case AMOVB:
		case AMOVH:
		case AMOVW:
			w = 2;	/* 32 bit, sign-extended to 64 */
			break;
		}
		v = brdist(p, 0, 19, 2);
		o1 = (w<<30)|(fp<<26)|(3<<27);
		o1 |= (v&0x7FFFF)<<5;
		o1 |= dr;
	}
	return o1;
}

static long
opbfm(int a, int r, int s, int rf, int rt)
{
	long o, c;

	o = opirr(a);
	if((o & (1<<31)) == 0)
		c = 32;
	else
		c = 64;
	if(r < 0 || r >= c)
		diag("illegal bit number\n%P", curp);
	o |= (r&0x3F)<<16;
	if(s < 0 || s >= c)
		diag("illegal bit number\n%P", curp);
	o |= (s&0x3F)<<10;
	o |= (rf<<5) | rt;
	return o;
}

static long
opextr(int a, long v, int rn, int rm, int rt)
{
	long o, c;

	o = opirr(a);
	c = (o & (1<<31)) != 0? 63: 31;
	if(v < 0 || v > c)
		diag("illegal bit number\n%P", curp);
	o |= v<<10;
	o |= rn << 5;
	o |= rm << 16;
	o |= rt;
	return o;
}

/*
 * size in log2(bytes)
 */
static int
movesize(int a)
{
	switch(a){
	case AMOV:
		return 3;
	case AMOVW:
	case AMOVWU:
		return 2;
	case AMOVH:
	case AMOVHU:
		return 1;
	case AMOVB:
	case AMOVBU:
		return 0;
	case AFMOVS:
		return 2;
	case AFMOVD:
		return 3;
	default:
		return -1;
	}
}

/*
 * SIMD
 */
