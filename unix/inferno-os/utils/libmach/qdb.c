#include <lib9.h>
#include <bio.h>
#include "mach.h"

/*
 * PowerPC-specific debugger interface,
 * including 64-bit modes
 *	forsyth@terzarima.net
 */

static	char	*powerexcep(Map*, Rgetter);
static	int	powerfoll(Map*, uvlong, Rgetter, uvlong*);
static	int	powerinst(Map*, uvlong, char, char*, int);
static	int	powerinstlen(Map*, uvlong);
static	int	powerdas(Map*, uvlong, char*, int);

/*
 *	Machine description
 */
Machdata powermach =
{
	{0x07f, 0xe0, 0x00, 0x08},		/* breakpoint (tw 31,r0,r0) */
	4,			/* break point size */

	beswab,			/* short to local byte order */
	beswal,			/* long to local byte order */
	beswav,			/* vlong to local byte order */
	risctrace,		/* print C traceback */
	riscframe,		/* frame finder */
	powerexcep,		/* print exception */
	0,			/* breakpoint fixup */
	beieeesftos,		/* single precision float printer */
	beieeedftos,		/* double precisioin float printer */
	powerfoll,		/* following addresses */
	powerinst,		/* print instruction */
	powerdas,		/* dissembler */
	powerinstlen,		/* instruction size */
};

static char *excname[] =
{
	"reserved 0",
	"system reset",
	"machine check",
	"data access",
	"instruction access",
	"external interrupt",
	"alignment",
	"program exception",
	"floating-point unavailable",
	"decrementer",
	"i/o controller interface error",
	"reserved B",
	"system call",
	"trace trap",
	"floating point assist",
	"reserved",
	"ITLB miss",
	"DTLB load miss",
	"DTLB store miss",
	"instruction address breakpoint"
	"SMI interrupt"
	"reserved 15",
	"reserved 16",
	"reserved 17",
	"reserved 18",
	"reserved 19",
	"reserved 1A",
	/* the following are made up on a program exception */
	"floating point exception",		/* FPEXC */
	"illegal instruction",
	"privileged instruction",
	"trap",
	"illegal operation",
	"breakpoint",	/* 20 */
};

static char*
powerexcep(Map *map, Rgetter rget)
{
	long c;
	static char buf[32];

	c = (*rget)(map, "CAUSE") >> 8;
	if(c < nelem(excname))
		return excname[c];
	sprint(buf, "unknown trap #%lx", c);
	return buf;
}

/*
 * disassemble PowerPC opcodes
 */

#define	REGSP	1	/* should come from q.out.h, but there's a clash */
#define	REGSB	2

static	char FRAMENAME[] = ".frame";

static Map *mymap;

/*
 * ibm conventions for these: bit 0 is top bit
 *	from table 10-1
 */
typedef struct {
	uchar	aa;		/* bit 30 */
	uchar	crba;		/* bits 11-15 */
	uchar	crbb;		/* bits 16-20 */
	long	bd;		/* bits 16-29 */
	uchar	crfd;		/* bits 6-8 */
	uchar	crfs;		/* bits 11-13 */
	uchar	bi;		/* bits 11-15 */
	uchar	bo;		/* bits 6-10 */
	uchar	crbd;		/* bits 6-10 */
	/* union { */
		short	d;	/* bits 16-31 */
		short	simm;
		ushort	uimm;
	/*}; */
	uchar	fm;		/* bits 7-14 */
	uchar	fra;		/* bits 11-15 */
	uchar	frb;		/* bits 16-20 */
	uchar	frc;		/* bits 21-25 */
	uchar	frs;		/* bits 6-10 */
	uchar	frd;		/* bits 6-10 */
	uchar	crm;		/* bits 12-19 */
	long	li;		/* bits 6-29 || b'00' */
	uchar	lk;		/* bit 31 */
	uchar	mb;		/* bits 21-25 */
	uchar	me;		/* bits 26-30 */
	uchar	xmbe;		/* bits 26,21-25: mb[5] || mb[0:4], also xme */
	uchar	xsh;		/* bits 30,16-20: sh[5] || sh[0:4] */
	uchar	nb;		/* bits 16-20 */
	uchar	op;		/* bits 0-5 */
	uchar	oe;		/* bit 21 */
	uchar	ra;		/* bits 11-15 */
	uchar	rb;		/* bits 16-20 */
	uchar	rc;		/* bit 31 */
	/* union { */
		uchar	rs;	/* bits 6-10 */
		uchar	rd;
	/*};*/
	uchar	sh;		/* bits 16-20 */
	ushort	spr;		/* bits 11-20 */
	uchar	to;		/* bits 6-10 */
	uchar	imm;		/* bits 16-19 */
	ushort	xo;		/* bits 21-30, 22-30, 26-30, or 30 (beware) */
	uvlong	imm64;
	long w0;
	long w1;
	uvlong	addr;		/* pc of instruction */
	short	target;
	short	m64;		/* 64-bit mode */
	char	*curr;		/* current fill level in output buffer */
	char	*end;		/* end of buffer */
	int 	size;		/* number of longs in instr */
	char	*err;		/* errmsg */
} Instr;

#define	IBF(v,a,b) (((ulong)(v)>>(32-(b)-1)) & ~(~0L<<(((b)-(a)+1))))
#define	IB(v,b) IBF((v),(b),(b))

#pragma	varargck	argpos	bprint		2

static void
bprint(Instr *i, char *fmt, ...)
{
	va_list arg;

	va_start(arg, fmt);
	i->curr = vseprint(i->curr, i->end, fmt, arg);
	va_end(arg);
}

static int
decode(uvlong pc, Instr *i)
{
	ulong w;

	if (get4(mymap, pc, &w) < 0) {
		werrstr("can't read instruction: %r");
		return -1;
	}
	i->m64 = asstype == APOWER64;
	i->aa = IB(w, 30);
	i->crba = IBF(w, 11, 15);
	i->crbb = IBF(w, 16, 20);
	i->bd = IBF(w, 16, 29)<<2;
	if(i->bd & 0x8000)
		i->bd |= ~0L<<16;
	i->crfd = IBF(w, 6, 8);
	i->crfs = IBF(w, 11, 13);
	i->bi = IBF(w, 11, 15);
	i->bo = IBF(w, 6, 10);
	i->crbd = IBF(w, 6, 10);
	i->uimm = IBF(w, 16, 31);	/* also d, simm */
	i->fm = IBF(w, 7, 14);
	i->fra = IBF(w, 11, 15);
	i->frb = IBF(w, 16, 20);
	i->frc = IBF(w, 21, 25);
	i->frs = IBF(w, 6, 10);
	i->frd = IBF(w, 6, 10);
	i->crm = IBF(w, 12, 19);
	i->li = IBF(w, 6, 29)<<2;
	if(IB(w, 6))
		i->li |= ~0<<25;
	i->lk = IB(w, 31);
	i->mb = IBF(w, 21, 25);
	i->me = IBF(w, 26, 30);
	i->xmbe = (IB(w,26)<<5) | i->mb;
	i->nb = IBF(w, 16, 20);
	i->op = IBF(w, 0, 5);
	i->oe = IB(w, 21);
	i->ra = IBF(w, 11, 15);
	i->rb = IBF(w, 16, 20);
	i->rc = IB(w, 31);
	i->rs = IBF(w, 6, 10);	/* also rd */
	i->sh = IBF(w, 16, 20);
	i->xsh = (IB(w, 30)<<5) | i->sh;
	i->spr = IBF(w, 11, 20);
	i->to = IBF(w, 6, 10);
	i->imm = IBF(w, 16, 19);
	i->xo = IBF(w, 21, 30);		/* bits 21-30, 22-30, 26-30, or 30 (beware) */
	if(i->op == 58){	/* class of 64-bit loads */
		i->xo = i->simm & 3;
		i->simm &= ~3;
	}
	i->imm64 = i->simm;
	if(i->op == 15)
		i->imm64 <<= 16;
	else if(i->op == 25 || i->op == 27 || i->op == 29)
		i->imm64 = (uvlong)(i->uimm<<16);
	i->w0 = w;
	i->target = -1;
	i->addr = pc;
	i->size = 1;
	return 1;
}

static int
mkinstr(uvlong pc, Instr *i)
{
	Instr x;

	if(decode(pc, i) < 0)
		return -1;
	/*
	 * combine ADDIS/ORI (CAU/ORIL) into MOVW
	 * also ORIS/ORIL for unsigned in 64-bit mode
	 */
	if ((i->op == 15 || i->op == 25) && i->ra==0) {
		if(decode(pc+4, &x) < 0)
			return -1;
		if (x.op == 24 && x.rs == x.ra && x.ra == i->rd) {
			i->imm64 |= (x.imm64 & 0xFFFF);
			if(i->op != 15)
				i->imm64 &= 0xFFFFFFFFUL;
			i->w1 = x.w0;
			i->target = x.rd;
			i->size++;
			return 1;
		}
	}
	return 1;
}

static int
plocal(Instr *i)
{
	long offset;
	Symbol s;

	if (!findsym(i->addr, CTEXT, &s) || !findlocal(&s, FRAMENAME, &s))
		return -1;
	offset = s.value - i->imm64;
	if (offset > 0) {
		if(getauto(&s, offset, CAUTO, &s)) {
			bprint(i, "%s+%lld(SP)", s.name, s.value);
			return 1;
		}
	} else {
		if (getauto(&s, -offset-4, CPARAM, &s)) {
			bprint(i, "%s+%ld(FP)", s.name, -offset);
			return 1;
		}
	}
	return -1;
}

static int
pglobal(Instr *i, uvlong off, int anyoff, char *reg)
{
	Symbol s, s2;
	uvlong off1;

	if(findsym(off, CANY, &s) &&
	   s.value-off < 4096 &&
	   (s.class == CDATA || s.class == CTEXT)) {
		if(off==s.value && s.name[0]=='$'){
			off1 = 0;
			geta(mymap, s.value, &off1);
			if(off1 && findsym(off1, CANY, &s2) && s2.value == off1){
				bprint(i, "$%s%s", s2.name, reg);
				return 1;
			}
		}
		bprint(i, "%s", s.name);
		if (s.value != off)
			bprint(i, "+%llux", off-s.value);
		bprint(i, reg);
		return 1;
	}
	if(!anyoff)
		return 0;
	bprint(i, "%llux%s", off, reg);
	return 1;
}

static void
address(Instr *i)
{
	if (i->ra == REGSP && plocal(i) >= 0)
		return;
	if (i->ra == REGSB && mach->sb && pglobal(i, mach->sb+i->imm64, 0, "(SB)") >= 0)
		return;
	if(i->simm < 0)
		bprint(i, "-%x(R%d)", -i->simm, i->ra);
	else
		bprint(i, "%llux(R%d)", i->imm64, i->ra);
}

static	char	*tcrbits[] = {"LT", "GT", "EQ", "VS"};
static	char	*fcrbits[] = {"GE", "LE", "NE", "VC"};

typedef struct Opcode Opcode;

struct Opcode {
	uchar	op;
	ushort	xo;
	ushort	xomask;
	char	*mnemonic;
	void	(*f)(Opcode *, Instr *);
	char	*ken;
	int	flags;
};

static void format(char *, Instr *, char *);

static void
branch(Opcode *o, Instr *i)
{
	char buf[8];
	int bo, bi;

	bo = i->bo & ~1;	/* ignore prediction bit */
	if(bo==4 || bo==12 || bo==20) {	/* simple forms */
		if(bo != 20) {
			bi = i->bi&3;
			sprint(buf, "B%s%%L", bo==12? tcrbits[bi]: fcrbits[bi]);
			format(buf, i, nil);
			bprint(i, "\t");
			if(i->bi > 4)
				bprint(i, "CR(%d),", i->bi/4);
		} else
			format("BR%L\t", i, nil);
		if(i->op == 16)
			format(0, i, "%J");
		else if(i->op == 19 && i->xo == 528)
			format(0, i, "(CTR)");
		else if(i->op == 19 && i->xo == 16)
			format(0, i, "(LR)");
	} else
		format(o->mnemonic, i, o->ken);
}

static void
addi(Opcode *o, Instr *i)
{
	if (i->op==14 && i->ra == 0)
		format("MOVW", i, "%i,R%d");
	else if (i->ra == REGSB) {
		bprint(i, "MOVW\t$");
		address(i);
		bprint(i, ",R%d", i->rd);
	} else if(i->op==14 && i->simm < 0) {
		bprint(i, "SUB\t$%d,R%d", -i->simm, i->ra);
		if(i->rd != i->ra)
			bprint(i, ",R%d", i->rd);
	} else if(i->ra == i->rd) {
		format(o->mnemonic, i, "%i");
		bprint(i, ",R%d", i->rd);
	} else
		format(o->mnemonic, i, o->ken);
}

static void
addis(Opcode *o, Instr *i)
{
	long v;

	v = i->imm64;
	if (i->op==15 && i->ra == 0)
		bprint(i, "MOVW\t$%lux,R%d", v, i->rd);
	else if (i->op==15 && i->ra == REGSB) {
		bprint(i, "MOVW\t$");
		address(i);
		bprint(i, ",R%d", i->rd);
	} else if(i->op==15 && v < 0) {
		bprint(i, "SUB\t$%ld,R%d", -v, i->ra);
		if(i->rd != i->ra)
			bprint(i, ",R%d", i->rd);
	} else {
		format(o->mnemonic, i, nil);
		bprint(i, "\t$%ld,R%d", v, i->ra);
		if(i->rd != i->ra)
			bprint(i, ",R%d", i->rd);
	}
}

static void
andi(Opcode *o, Instr *i)
{
	if (i->ra == i->rs)
		format(o->mnemonic, i, "%I,R%d");
	else
		format(o->mnemonic, i, o->ken);
}

static void
gencc(Opcode *o, Instr *i)
{
	format(o->mnemonic, i, o->ken);
}

static void
gen(Opcode *o, Instr *i)
{
	format(o->mnemonic, i, o->ken);
	if (i->rc)
		bprint(i, " [illegal Rc]");
}

static void
ldx(Opcode *o, Instr *i)
{
	if(i->ra == 0)
		format(o->mnemonic, i, "(R%b),R%d");
	else
		format(o->mnemonic, i, "(R%b+R%a),R%d");
	if(i->rc)
		bprint(i, " [illegal Rc]");
}

static void
stx(Opcode *o, Instr *i)
{
	if(i->ra == 0)
		format(o->mnemonic, i, "R%d,(R%b)");
	else
		format(o->mnemonic, i, "R%d,(R%b+R%a)");
	if(i->rc && i->xo != 150)
		bprint(i, " [illegal Rc]");
}

static void
fldx(Opcode *o, Instr *i)
{
	if(i->ra == 0)
		format(o->mnemonic, i, "(R%b),F%d");
	else
		format(o->mnemonic, i, "(R%b+R%a),F%d");
	if(i->rc)
		bprint(i, " [illegal Rc]");
}

static void
fstx(Opcode *o, Instr *i)
{
	if(i->ra == 0)
		format(o->mnemonic, i, "F%d,(R%b)");
	else
		format(o->mnemonic, i, "F%d,(R%b+R%a)");
	if(i->rc)
		bprint(i, " [illegal Rc]");
}

static void
dcb(Opcode *o, Instr *i)
{
	if(i->ra == 0)
		format(o->mnemonic, i, "(R%b)");
	else
		format(o->mnemonic, i, "(R%b+R%a)");
	if(i->rd)
		bprint(i, " [illegal Rd]");
	if(i->rc)
		bprint(i, " [illegal Rc]");
}

static void
lw(Opcode *o, Instr *i, char r)
{
	format(o->mnemonic, i, nil);
	bprint(i, "\t");
	address(i);
	bprint(i, ",%c%d", r, i->rd);
}

static void
load(Opcode *o, Instr *i)
{
	lw(o, i, 'R');
}

static void
fload(Opcode *o, Instr *i)
{
	lw(o, i, 'F');
}

static void
sw(Opcode *o, Instr *i, char r)
{
	int offset;
	Symbol s;

	if (i->rs == REGSP) {
		if (findsym(i->addr, CTEXT, &s) && findlocal(&s, FRAMENAME, &s)) {
			offset = s.value-i->imm64;
			if (offset > 0 && getauto(&s, offset, CAUTO, &s)) {
				format(o->mnemonic, i, nil);
				bprint(i, "\t%c%d,%s-%d(SP)", r, i->rd, s.name, offset);
				return;
			}
		}
	}
	if (i->rs == REGSB && mach->sb) {
		format(o->mnemonic, i, nil);
		bprint(i, "\t%c%d,", r, i->rd);
		address(i);
		return;
	}
	if (r == 'F')
		format(o->mnemonic, i, "F%d,%l");
	else
		format(o->mnemonic, i, o->ken);
}

static void
store(Opcode *o, Instr *i)
{
	sw(o, i, 'R');
}

static void
fstore(Opcode *o, Instr *i)
{
	sw(o, i, 'F');
}

static void
shifti(Opcode *o, Instr *i)
{
	if (i->ra == i->rs)
		format(o->mnemonic, i, "$%k,R%a");
	else
		format(o->mnemonic, i, o->ken);
}

static void
shift(Opcode *o, Instr *i)
{
	if (i->ra == i->rs)
		format(o->mnemonic, i, "R%b,R%a");
	else
		format(o->mnemonic, i, o->ken);
}

static void
add(Opcode *o, Instr *i)
{
	if (i->rd == i->ra)
		format(o->mnemonic, i, "R%b,R%d");
	else if (i->rd == i->rb)
		format(o->mnemonic, i, "R%a,R%d");
	else
		format(o->mnemonic, i, o->ken);
}

static void
sub(Opcode *o, Instr *i)
{
	format(o->mnemonic, i, nil);
	bprint(i, "\t");
	if(i->op == 31) {
		bprint(i, "\tR%d,R%d", i->ra, i->rb);	/* subtract Ra from Rb */
		if(i->rd != i->rb)
			bprint(i, ",R%d", i->rd);
	} else
		bprint(i, "\tR%d,$%d,R%d", i->ra, i->simm, i->rd);
}

static void
qmuldiv(Opcode *o, Instr *i)
{
	format(o->mnemonic, i, nil);
	if(i->op == 31)
		bprint(i, "\tR%d,R%d", i->rb, i->ra);
	else
		bprint(i, "\t$%d,R%d", i->simm, i->ra);
	if(i->ra != i->rd)
		bprint(i, ",R%d", i->rd);
}

static void
and(Opcode *o, Instr *i)
{
	if (i->op == 31) {
		/* Rb,Rs,Ra */
		if (i->ra == i->rs)
			format(o->mnemonic, i, "R%b,R%a");
		else if (i->ra == i->rb)
			format(o->mnemonic, i, "R%s,R%a");
		else
			format(o->mnemonic, i, o->ken);
	} else {
		/* imm,Rs,Ra */
		if (i->ra == i->rs)
			format(o->mnemonic, i, "%I,R%a");
		else
			format(o->mnemonic, i, o->ken);
	}
}

static void
or(Opcode *o, Instr *i)
{
	if (i->op == 31) {
		/* Rb,Rs,Ra */
		if (i->rs == 0 && i->ra == 0 && i->rb == 0)
			format("NOP", i, nil);
		else if (i->rs == i->rb)
			format("MOVW", i, "R%b,R%a");
		else
			and(o, i);
	} else
		and(o, i);
}

static void
shifted(Opcode *o, Instr *i)
{
	format(o->mnemonic, i, nil);
	bprint(i, "\t$%lux,", (ulong)i->uimm<<16);
	if (i->rs == i->ra)
		bprint(i, "R%d", i->ra);
	else
		bprint(i, "R%d,R%d", i->rs, i->ra);
}

static void
neg(Opcode *o, Instr *i)
{
	if (i->rd == i->ra)
		format(o->mnemonic, i, "R%d");
	else
		format(o->mnemonic, i, o->ken);
}

static	char	ir2[] = "R%a,R%d";		/* reverse of IBM order */
static	char	ir3[] = "R%b,R%a,R%d";
static	char	ir3r[] = "R%a,R%b,R%d";
static	char	il3[] = "R%b,R%s,R%a";
static	char	il2u[] = "%I,R%a,R%d";
static	char	il3s[] = "$%k,R%s,R%a";
static	char	il2[] = "R%s,R%a";
static	char	icmp3[] = "R%a,R%b,%D";
static	char	cr3op[] = "%b,%a,%d";
static	char	ir2i[] = "%i,R%a,R%d";
static	char	fp2[] = "F%b,F%d";
static	char	fp3[] = "F%b,F%a,F%d";
static	char	fp3c[] = "F%c,F%a,F%d";
static	char	fp4[] = "F%a,F%c,F%b,F%d";
static	char	fpcmp[] = "F%a,F%b,%D";
static	char	ldop[] = "%l,R%d";
static	char	stop[] = "R%d,%l";
static	char	fldop[] = "%l,F%d";
static	char	fstop[] = "F%d,%l";
static	char	rldc[] = "R%b,R%s,$%E,R%a";
static	char	rlim[] = "R%b,R%s,$%z,R%a";
static	char	rlimi[] = "$%k,R%s,$%z,R%a";
static	char	rldi[] = "$%e,R%s,$%E,R%a";

#define	OEM	IBF(~0,22,30)
#define	FP4	IBF(~0,26,30)
#define	ALL	(~0)
#define	RLDC	0xF
#define	RLDI	0xE
/*
notes:
	10-26: crfD = rD>>2; rD&3 mbz
		also, L bit (bit 10) mbz or selects 64-bit operands
*/

static Opcode opcodes[] = {
	{31,	266,	OEM,	"ADD%V%C",	add,	ir3},
	{31,	 10,	OEM,	"ADDC%V%C",	add,	ir3},
	{31,	138,	OEM,	"ADDE%V%C",	add,	ir3},
	{14,	0,	0,	"ADD",		addi,	ir2i},
	{12,	0,	0,	"ADDC",		addi,	ir2i},
	{13,	0,	0,	"ADDCCC",	addi,	ir2i},
	{15,	0,	0,	"ADD",		addis,	0},
	{31,	234,	OEM,	"ADDME%V%C",	gencc,	ir2},
	{31,	202,	OEM,	"ADDZE%V%C",	gencc,	ir2},

	{31,	28,	ALL,	"AND%C",	and,	il3},
	{31,	60,	ALL,	"ANDN%C",	and,	il3},
	{28,	0,	0,	"ANDCC",	andi,	il2u},
	{29,	0,	0,	"ANDCC",	shifted, 0},

	{18,	0,	0,	"B%L",		gencc,	"%j"},
	{16,	0,	0,	"BC%L",		branch,	"%d,%a,%J"},
	{19,	528,	ALL,	"BC%L",		branch,	"%d,%a,(CTR)"},
	{19,	16,	ALL,	"BC%L",		branch,	"%d,%a,(LR)"},

	{31,	0,	ALL,	"CMP",		0,	icmp3},
	{11,	0,	0,	"CMP",		0,	"R%a,%i,%D"},
	{31,	32,	ALL,	"CMPU",		0,	icmp3},
	{10,	0,	0,	"CMPU",		0,	"R%a,%I,%D"},

	{31,	58,	ALL,	"CNTLZD%C",	gencc,	ir2},	/* 64 */
	{31,	26,	ALL,	"CNTLZ%W%C",	gencc,	ir2},

	{19,	257,	ALL,	"CRAND",	gen,	cr3op},
	{19,	129,	ALL,	"CRANDN",	gen,	cr3op},
	{19,	289,	ALL,	"CREQV",	gen,	cr3op},
	{19,	225,	ALL,	"CRNAND",	gen,	cr3op},
	{19,	33,	ALL,	"CRNOR",	gen,	cr3op},
	{19,	449,	ALL,	"CROR",		gen,	cr3op},
	{19,	417,	ALL,	"CRORN",	gen,	cr3op},
	{19,	193,	ALL,	"CRXOR",	gen,	cr3op},

	{31,	86,	ALL,	"DCBF",		dcb,	0},
	{31,	470,	ALL,	"DCBI",		dcb,	0},
	{31,	54,	ALL,	"DCBST",	dcb,	0},
	{31,	278,	ALL,	"DCBT",		dcb,	0},
	{31,	246,	ALL,	"DCBTST",	dcb,	0},
	{31,	1014,	ALL,	"DCBZ",		dcb,	0},
	{31,	454,	ALL,	"DCCCI",	dcb,	0},
	{31,	966,	ALL,	"ICCCI",	dcb,	0},

	{31,	489,	OEM,	"DIVD%V%C",	qmuldiv,	ir3},	/* 64 */
	{31,	457,	OEM,	"DIVDU%V%C",	qmuldiv,	ir3},	/* 64 */
	{31,	491,	OEM,	"DIVW%V%C",	qmuldiv,	ir3},
	{31,	459,	OEM,	"DIVWU%V%C",	qmuldiv,	ir3},

	{31,	310,	ALL,	"ECIWX",	ldx,	0},
	{31,	438,	ALL,	"ECOWX",	stx,	0},
	{31,	854,	ALL,	"EIEIO",	gen,	0},

	{31,	284,	ALL,	"EQV%C",	gencc,	il3},

	{31,	954,	ALL,	"EXTSB%C",	gencc,	il2},
	{31,	922,	ALL,	"EXTSH%C",	gencc,	il2},
	{31,	986,	ALL,	"EXTSW%C",	gencc,	il2},	/* 64 */

	{63,	264,	ALL,	"FABS%C",	gencc,	fp2},
	{63,	21,	ALL,	"FADD%C",	gencc,	fp3},
	{59,	21,	ALL,	"FADDS%C",	gencc,	fp3},
	{63,	32,	ALL,	"FCMPO",	gen,	fpcmp},
	{63,	0,	ALL,	"FCMPU",	gen,	fpcmp},
	{63,	846,	ALL,	"FCFID%C",	gencc,	fp2},	/* 64 */
	{63,	814,	ALL,	"FCTID%C",	gencc,	fp2},	/* 64 */
	{63,	815,	ALL,	"FCTIDZ%C",	gencc,	fp2},	/* 64 */
	{63,	14,	ALL,	"FCTIW%C",	gencc,	fp2},
	{63,	15,	ALL,	"FCTIWZ%C",	gencc,	fp2},
	{63,	18,	ALL,	"FDIV%C",	gencc,	fp3},
	{59,	18,	ALL,	"FDIVS%C",	gencc,	fp3},
	{63,	29,	FP4,	"FMADD%C",	gencc,	fp4},
	{59,	29,	FP4,	"FMADDS%C",	gencc,	fp4},
	{63,	72,	ALL,	"FMOVD%C",	gencc,	fp2},
	{63,	28,	FP4,	"FMSUB%C",	gencc,	fp4},
	{59,	28,	FP4,	"FMSUBS%C",	gencc,	fp4},
	{63,	25,	FP4,	"FMUL%C",	gencc,	fp3c},
	{59,	25,	FP4,	"FMULS%C",	gencc,	fp3c},
	{63,	136,	ALL,	"FNABS%C",	gencc,	fp2},
	{63,	40,	ALL,	"FNEG%C",	gencc,	fp2},
	{63,	31,	FP4,	"FNMADD%C",	gencc,	fp4},
	{59,	31,	FP4,	"FNMADDS%C",	gencc,	fp4},
	{63,	30,	FP4,	"FNMSUB%C",	gencc,	fp4},
	{59,	30,	FP4,	"FNMSUBS%C",	gencc,	fp4},
	{59,	24,	ALL,	"FRES%C",	gencc,	fp2},	/* optional */
	{63,	12,	ALL,	"FRSP%C",	gencc,	fp2},
	{63,	26,	ALL,	"FRSQRTE%C",	gencc,	fp2},	/* optional */
	{63,	23,	FP4,	"FSEL%CC",	gencc,	fp4},	/* optional */
	{63,	22,	ALL,	"FSQRT%C",	gencc,	fp2},	/* optional */
	{59,	22,	ALL,	"FSQRTS%C",	gencc,	fp2},	/* optional */
	{63,	20,	FP4,	"FSUB%C",	gencc,	fp3},
	{59,	20,	FP4,	"FSUBS%C",	gencc,	fp3},

	{31,	982,	ALL,	"ICBI",		dcb,	0},	/* optional */
	{19,	150,	ALL,	"ISYNC",	gen,	0},

	{34,	0,	0,	"MOVBZ",	load,	ldop},
	{35,	0,	0,	"MOVBZU",	load,	ldop},
	{31,	119,	ALL,	"MOVBZU",	ldx,	0},
	{31,	87,	ALL,	"MOVBZ",	ldx,	0},
	{50,	0,	0,	"FMOVD",	fload,	fldop},
	{51,	0,	0,	"FMOVDU",	fload,	fldop},
	{31,	631,	ALL,	"FMOVDU",	fldx,	0},
	{31,	599,	ALL,	"FMOVD",	fldx,	0},
	{48,	0,	0,	"FMOVS",	load,	fldop},
	{49,	0,	0,	"FMOVSU",	load,	fldop},
	{31,	567,	ALL,	"FMOVSU",	fldx,	0},
	{31,	535,	ALL,	"FMOVS",	fldx,	0},
	{42,	0,	0,	"MOVH",		load,	ldop},
	{43,	0,	0,	"MOVHU",	load,	ldop},
	{31,	375,	ALL,	"MOVHU",	ldx,	0},
	{31,	343,	ALL,	"MOVH",		ldx,	0},
	{31,	790,	ALL,	"MOVHBR",	ldx,	0},
	{40,	0,	0,	"MOVHZ",	load,	ldop},
	{41,	0,	0,	"MOVHZU",	load,	ldop},
	{31,	311,	ALL,	"MOVHZU",	ldx,	0},
	{31,	279,	ALL,	"MOVHZ",	ldx,	0},
	{46,	0,	0,	"MOVMW",	load,	ldop},
	{31,	597,	ALL,	"LSW",		gen,	"(R%a),$%n,R%d"},
	{31,	533,	ALL,	"LSW",		ldx,	0},
	{31,	20,	ALL,	"LWAR",		ldx,	0},
	{31,	84,	ALL,	"LWARD",	ldx,	0},	/* 64 */

	{58,	0,	ALL,	"MOVD",		load,	ldop},	/* 64 */
	{58,	1,	ALL,	"MOVDU",	load,	ldop},	/* 64 */
	{31,	53,	ALL,	"MOVDU",	ldx,	0},	/* 64 */
	{31,	21,	ALL,	"MOVD",		ldx,	0},	/* 64 */

	{31,	534,	ALL,	"MOVWBR",	ldx,	0},

	{58,	2,	ALL,	"MOVW",		load,	ldop},	/* 64 (lwa) */
	{31,	373,	ALL,	"MOVWU",	ldx,	0},	/* 64 */
	{31,	341,	ALL,	"MOVW",		ldx,	0},	/* 64 */

	{32,	0,	0,	"MOVW%Z",	load,	ldop},
	{33,	0,	0,	"MOVW%ZU",	load,	ldop},
	{31,	55,	ALL,	"MOVW%ZU",	ldx,	0},
	{31,	23,	ALL,	"MOVW%Z",	ldx,	0},

	{19,	0,	ALL,	"MOVFL",	gen,	"%S,%D"},
	{63,	64,	ALL,	"MOVCRFS",	gen,	"%S,%D"},
	{31,	512,	ALL,	"MOVW",		gen,	"XER,%D"},
	{31,	19,	ALL,	"MOVW",		gen,	"CR,R%d"},

	{63,	583,	ALL,	"MOVW%C",	gen,	"FPSCR, F%d"},	/* mffs */
	{31,	83,	ALL,	"MOVW",		gen,	"MSR,R%d"},
	{31,	339,	ALL,	"MOVW",		gen,	"%P,R%d"},
	{31,	595,	ALL,	"MOVW",		gen,	"SEG(%a),R%d"},
	{31,	659,	ALL,	"MOVW",		gen,	"SEG(R%b),R%d"},
	{31,	323,	ALL,	"MOVW",		gen,	"DCR(%Q),R%d"},
	{31,	451,	ALL,	"MOVW",		gen,	"R%s,DCR(%Q)"},
	{31,	144,	ALL,	"MOVFL",	gen,	"R%s,%m,CR"},
	{63,	70,	ALL,	"MTFSB0%C",	gencc,	"%D"},
	{63,	38,	ALL,	"MTFSB1%C",	gencc,	"%D"},
	{63,	711,	ALL,	"MOVFL%C",	gencc,	"F%b,%M,FPSCR"},	/* mtfsf */
	{63,	134,	ALL,	"MOVFL%C",	gencc,	"%K,%D"},
	{31,	146,	ALL,	"MOVW",		gen,	"R%s,MSR"},
	{31,	178,	ALL,	"MOVD",		gen,	"R%s,MSR"},
	{31,	467,	ALL,	"MOVW",		gen,	"R%s,%P"},
	{31,	210,	ALL,	"MOVW",		gen,	"R%s,SEG(%a)"},
	{31,	242,	ALL,	"MOVW",		gen,	"R%s,SEG(R%b)"},

	{31,	73,	ALL,	"MULHD%C",	gencc,	ir3},
	{31,	9,	ALL,	"MULHDU%C",	gencc,	ir3},
	{31,	233,	OEM,	"MULLD%V%C",	gencc,	ir3},

	{31,	75,	ALL,	"MULHW%C",	gencc,	ir3},
	{31,	11,	ALL,	"MULHWU%C",	gencc,	ir3},
	{31,	235,	OEM,	"MULLW%V%C",	gencc,	ir3},

	{7,	0,	0,	"MULLW",	qmuldiv,	"%i,R%a,R%d"},

	{31,	476,	ALL,	"NAND%C",	gencc,	il3},
	{31,	104,	OEM,	"NEG%V%C",	neg,	ir2},
	{31,	124,	ALL,	"NOR%C",	gencc,	il3},
	{31,	444,	ALL,	"OR%C",		or,	il3},
	{31,	412,	ALL,	"ORN%C",	or,	il3},
	{24,	0,	0,	"OR",		and,	"%I,R%d,R%a"},
	{25,	0,	0,	"OR",		shifted, 0},

	{19,	50,	ALL,	"RFI",		gen,	0},
	{19,	51,	ALL,	"RFCI",		gen,	0},

	{30,	8,	RLDC,	"RLDCL%C",	gencc,	rldc},	/* 64 */
	{30,	9,	RLDC,	"RLDCR%C",	gencc,	rldc},	/* 64 */
	{30,	0,	RLDI,	"RLDCL%C",	gencc,	rldi},	/* 64 */
	{30,	1<<1, RLDI,	"RLDCR%C",	gencc,	rldi},	/* 64 */
	{30,	2<<1, RLDI,	"RLDC%C",	gencc,	rldi},	/* 64 */
	{30,	3<<1, RLDI,	"RLDMI%C",	gencc,	rldi},	/* 64 */

	{20,	0,	0,	"RLWMI%C",	gencc,	rlimi},
	{21,	0,	0,	"RLWNM%C",	gencc,	rlimi},
	{23,	0,	0,	"RLWNM%C",	gencc,	rlim},

	{17,	1,	ALL,	"SYSCALL",	gen,	0},

	{31,	27,	ALL,	"SLD%C",	shift,	il3},	/* 64 */
	{31,	24,	ALL,	"SLW%C",	shift,	il3},

	{31,	794,	ALL,	"SRAD%C",	shift,	il3},	/* 64 */
	{31,	(413<<1)|0,	ALL,	"SRAD%C",	shifti,	il3s},	/* 64 */
	{31,	(413<<1)|1,	ALL,	"SRAD%C",	shifti,	il3s},	/* 64 */
	{31,	792,	ALL,	"SRAW%C",	shift,	il3},
	{31,	824,	ALL,	"SRAW%C",	shifti,	il3s},

	{31,	539,	ALL,	"SRD%C",	shift,	il3},	/* 64 */
	{31,	536,	ALL,	"SRW%C",	shift,	il3},

	{38,	0,	0,	"MOVB",		store,	stop},
	{39,	0,	0,	"MOVBU",	store,	stop},
	{31,	247,	ALL,	"MOVBU",	stx,	0},
	{31,	215,	ALL,	"MOVB",		stx,	0},
	{54,	0,	0,	"FMOVD",	fstore,	fstop},
	{55,	0,	0,	"FMOVDU",	fstore,	fstop},
	{31,	759,	ALL,	"FMOVDU",	fstx,	0},
	{31,	727,	ALL,	"FMOVD",	fstx,	0},
	{52,	0,	0,	"FMOVS",	fstore,	fstop},
	{53,	0,	0,	"FMOVSU",	fstore,	fstop},
	{31,	695,	ALL,	"FMOVSU",	fstx,	0},
	{31,	663,	ALL,	"FMOVS",	fstx,	0},
	{44,	0,	0,	"MOVH",		store,	stop},
	{31,	918,	ALL,	"MOVHBR",	stx,	0},
	{45,	0,	0,	"MOVHU",	store,	stop},
	{31,	439,	ALL,	"MOVHU",	stx,	0},
	{31,	407,	ALL,	"MOVH",		stx,	0},
	{47,	0,	0,	"MOVMW",	store,	stop},
	{31,	725,	ALL,	"STSW",		gen,	"R%d,$%n,(R%a)"},
	{31,	661,	ALL,	"STSW",		stx,	0},
	{36,	0,	0,	"MOVW",		store,	stop},
	{31,	662,	ALL,	"MOVWBR",	stx,	0},
	{31,	150,	ALL,	"STWCCC",	stx,	0},
	{31,	214,	ALL,	"STDCCC",	stx,	0},	/* 64 */
	{37,	0,	0,	"MOVWU",	store,	stop},
	{31,	183,	ALL,	"MOVWU",	stx,	0},
	{31,	151,	ALL,	"MOVW",		stx,	0},

	{62,	0,	0,	"MOVD%U",	store,	stop},	/* 64 */
	{31,	149,	ALL,	"MOVD",		stx,	0,},	/* 64 */
	{31,	181,	ALL,	"MOVDU",	stx,	0},	/* 64 */

	{31,	498,	ALL,	"SLBIA",	gen,	0},	/* 64 */
	{31,	434,	ALL,	"SLBIE",	gen,	"R%b"},	/* 64 */
	{31,	466,	ALL,	"SLBIEX",	gen,	"R%b"},	/* 64 */
	{31,	915,	ALL,	"SLBMFEE",	gen,	"R%b,R%d"},	/* 64 */
	{31,	851,	ALL,	"SLBMFEV",	gen,	"R%b,R%d"},	/* 64 */
	{31,	402,	ALL,	"SLBMTE",	gen,	"R%s,R%b"},	/* 64 */

	{31,	40,	OEM,	"SUB%V%C",	sub,	ir3},
	{31,	8,	OEM,	"SUBC%V%C",	sub,	ir3},
	{31,	136,	OEM,	"SUBE%V%C",	sub,	ir3},
	{8,	0,	0,	"SUBC",		gen,	"R%a,%i,R%d"},
	{31,	232,	OEM,	"SUBME%V%C",	sub,	ir2},
	{31,	200,	OEM,	"SUBZE%V%C",	sub,	ir2},

	{31,	598,	ALL,	"SYNC",		gen,	0},	/* TO DO: there's a parameter buried in there */
	{2,	0,	0,	"TD",		gen,	"%d,R%a,%i"},	/* 64 */
	{31,	370,	ALL,	"TLBIA",	gen,	0},	/* optional */
	{31,	306,	ALL,	"TLBIE",	gen,	"R%b"},	/* optional */
	{31,	274,	ALL,	"TLBIEL",	gen,	"R%b"},	/* optional */
	{31,	1010,	ALL,	"TLBLI",	gen,	"R%b"},	/* optional */
	{31,	978,	ALL,	"TLBLD",	gen,	"R%b"},	/* optional */
	{31,	566,	ALL,	"TLBSYNC",	gen,	0},	/* optional */
	{31,	68,	ALL,	"TD",		gen,	"%d,R%a,R%b"},	/* 64 */
	{31,	4,	ALL,	"TW",		gen,	"%d,R%a,R%b"},
	{3,	0,	0,	"TW",		gen,	"%d,R%a,%i"},

	{31,	316,	ALL,	"XOR",		and,	il3},
	{26,	0,	0,	"XOR",		and,	il2u},
	{27,	0,	0,	"XOR",		shifted, 0},

	{0},
};

typedef struct Spr Spr;
struct Spr {
	int	n;
	char	*name;
};

static	Spr	sprname[] = {
	{0, "MQ"},
	{1, "XER"},
	{268, "TBL"},
	{269, "TBU"},
	{8, "LR"},
	{9, "CTR"},
	{528, "IBAT0U"},
	{529, "IBAT0L"},
	{530, "IBAT1U"},
	{531, "IBAT1L"},
	{532, "IBAT2U"},
	{533, "IBAT2L"},
	{534, "IBAT3U"},
	{535, "IBAT3L"},
	{536, "DBAT0U"},
	{537, "DBAT0L"},
	{538, "DBAT1U"},
	{539, "DBAT1L"},
	{540, "DBAT2U"},
	{541, "DBAT2L"},
	{542, "DBAT3U"},
	{543, "DBAT3L"},
	{25, "SDR1"},
	{19, "DAR"},
	{272, "SPRG0"},
	{273, "SPRG1"},
	{274, "SPRG2"},
	{275, "SPRG3"},
	{18, "DSISR"},
	{26, "SRR0"},
	{27, "SRR1"},
	{284, "TBLW"},
	{285, "TBUW"},	
	{22, "DEC"},
	{282, "EAR"},
	{1008, "HID0"},
	{1009, "HID1"},
	{976, "DMISS"},
	{977, "DCMP"},
	{978, "HASH1"},
	{979, "HASH2"},
	{980, "IMISS"},
	{981, "ICMP"},
	{982, "RPA"},
	{1010, "IABR"},
	{1013, "DABR"},
	{0,0},
};

static int
shmask(uvlong *m)
{
	int i;

	for(i=0; i<63; i++)
		if(*m & ((uvlong)1<<i))
			break;
	if(i > 63)
		return 0;
	if(*m & ~((uvlong)1<<i)){	/* more than one bit: do multiples of bytes */
		i = (i/8)*8;
		if(i == 0)
			return 0;
	}
	*m >>= i;
	return i;
}

static void
format(char *mnemonic, Instr *i, char *f)
{
	int n, s;
	ulong mask;
	uvlong vmask;

	if (mnemonic)
		format(0, i, mnemonic);
	if (f == 0)
		return;
	if (mnemonic)
		bprint(i, "\t");
	for ( ; *f; f++) {
		if (*f != '%') {
			bprint(i, "%c", *f);
			continue;
		}
		switch (*++f) {

		case 'a':
			bprint(i, "%d", i->ra);
			break;

		case 'b':
			bprint(i, "%d", i->rb);
			break;

		case 'c':
			bprint(i, "%d", i->frc);
			break;

		case 'd':
		case 's':
			bprint(i, "%d", i->rd);
			break;

		case 'C':
			if(i->rc)
				bprint(i, "CC");
			break;

		case 'D':
			if(i->rd & 3)
				bprint(i, "CR(INVAL:%d)", i->rd);
			else if(i->op == 63)
				bprint(i, "FPSCR(%d)", i->crfd);
			else
				bprint(i, "CR(%d)", i->crfd);
			break;

		case 'e':
			bprint(i, "%d", i->xsh);
			break;

		case 'E':
			switch(IBF(i->w0,27,30)){	/* low bit is top bit of shift in rldiX cases */
			case 8:	i->mb = i->xmbe; i->me = 63; break;	/* rldcl */
			case 9:	i->mb = 0; i->me = i->xmbe; break;	/* rldcr */
			case 4: case 5:
					i->mb = i->xmbe; i->me = 63-i->xsh; break;	/* rldic */
			case 0: case 1:
					i->mb = i->xmbe; i->me = 63; break;	/* rldicl */
			case 2: case 3:
					i->mb = 0; i->me = i->xmbe; break;	/* rldicr */
			case 6: case 7:
					i->mb = i->xmbe; i->me = 63-i->xsh; break;	/* rldimi */
			}
			vmask = (~(uvlong)0>>i->mb) & (~(uvlong)0<<(63-i->me));
			s = shmask(&vmask);
			if(s)
				bprint(i, "(%llux<<%d)", vmask, s);
			else
				bprint(i, "%llux", vmask);
			break;

		case 'i':
			bprint(i, "$%d", i->simm);
			break;

		case 'I':
			bprint(i, "$%ux", i->uimm);
			break;

		case 'j':
			if(i->aa)
				pglobal(i, i->li, 1, "(SB)");
			else
				pglobal(i, i->addr+i->li, 1, "");
			break;

		case 'J':
			if(i->aa)
				pglobal(i, i->bd, 1, "(SB)");
			else
				pglobal(i, i->addr+i->bd, 1, "");
			break;

		case 'k':
			bprint(i, "%d", i->sh);
			break;

		case 'K':
			bprint(i, "$%x", i->imm);
			break;

		case 'L':
			if(i->lk)
				bprint(i, "L");
			break;

		case 'l':
			if(i->simm < 0)
				bprint(i, "-%x(R%d)", -i->simm, i->ra);
			else
				bprint(i, "%x(R%d)", i->simm, i->ra);
			break;

		case 'm':
			bprint(i, "%ux", i->crm);
			break;

		case 'M':
			bprint(i, "%ux", i->fm);
			break;

		case 'n':
			bprint(i, "%d", i->nb==0? 32: i->nb);	/* eg, pg 10-103 */
			break;

		case 'P':
			n = ((i->spr&0x1f)<<5)|((i->spr>>5)&0x1f);
			for(s=0; sprname[s].name; s++)
				if(sprname[s].n == n)
					break;
			if(sprname[s].name) {
				if(s < 10)
					bprint(i, sprname[s].name);
				else
					bprint(i, "SPR(%s)", sprname[s].name);
			} else
				bprint(i, "SPR(%d)", n);
			break;

		case 'Q':
			n = ((i->spr&0x1f)<<5)|((i->spr>>5)&0x1f);
			bprint(i, "%d", n);
			break;

		case 'S':
			if(i->ra & 3)
				bprint(i, "CR(INVAL:%d)", i->ra);
			else if(i->op == 63)
				bprint(i, "FPSCR(%d)", i->crfs);
			else
				bprint(i, "CR(%d)", i->crfs);
			break;

		case 'U':
			if(i->rc)
				bprint(i, "U");
			break;

		case 'V':
			if(i->oe)
				bprint(i, "V");
			break;

		case 'w':
			bprint(i, "[%lux]", i->w0);
			break;

		case 'W':
			if(i->m64)
				bprint(i, "W");
			break;

		case 'Z':
			if(i->m64)
				bprint(i, "Z");
			break;

		case 'z':
			if(i->mb <= i->me)
				mask = ((ulong)~0L>>i->mb) & (~0L<<(31-i->me));
			else
				mask = ~(((ulong)~0L>>(i->me+1)) & (~0L<<(31-(i->mb-1))));
			bprint(i, "%lux", mask);
			break;

		case '\0':
			bprint(i, "%%");
			return;

		default:
			bprint(i, "%%%c", *f);
			break;
		}
	}
}

static int
printins(Map *map, uvlong pc, char *buf, int n)
{
	Instr i;
	Opcode *o;

	mymap = map;
	memset(&i, 0, sizeof(i));
	i.curr = buf;
	i.end = buf+n-1;
	if(mkinstr(pc, &i) < 0)
		return -1;
	for(o = opcodes; o->mnemonic != 0; o++)
		if(i.op == o->op && (i.xo & o->xomask) == o->xo) {
			if (o->f)
				(*o->f)(o, &i);
			else
				format(o->mnemonic, &i, o->ken);
			return i.size*4;
		}
	bprint(&i, "unknown %lux", i.w0);
	return i.size*4;
}

static int
powerinst(Map *map, uvlong pc, char modifier, char *buf, int n)
{
	USED(modifier);
	return printins(map, pc, buf, n);
}

static int
powerdas(Map *map, uvlong pc, char *buf, int n)
{
	Instr instr;

	mymap = map;
	memset(&instr, 0, sizeof(instr));
	instr.curr = buf;
	instr.end = buf+n-1;
	if (mkinstr(pc, &instr) < 0)
		return -1;
	if (instr.end-instr.curr > 8)
		instr.curr = _hexify(instr.curr, instr.w0, 7);
	if (instr.end-instr.curr > 9 && instr.size == 2) {
		*instr.curr++ = ' ';
		instr.curr = _hexify(instr.curr, instr.w1, 7);
	}
	*instr.curr = 0;
	return instr.size*4;
}

static int
powerinstlen(Map *map, uvlong pc)
{
	Instr i;

	mymap = map;
	if (mkinstr(pc, &i) < 0)
		return -1;
	return i.size*4;
}

static int
powerfoll(Map *map, uvlong pc, Rgetter rget, uvlong *foll)
{
	char *reg;
	Instr i;

	mymap = map;
	if (mkinstr(pc, &i) < 0)
		return -1;
	foll[0] = pc+4;
	foll[1] = pc+4;
	switch(i.op) {
	default:
		return 1;

	case 18:	/* branch */
		foll[0] = i.li;
		if(!i.aa)
			foll[0] += pc;
		break;
			
	case 16:	/* conditional branch */
		foll[0] = i.bd;
		if(!i.aa)
			foll[0] += pc;
		break;

	case 19:	/* conditional branch to register */
		if(i.xo == 528)
			reg = "CTR";
		else if(i.xo == 16)
			reg = "LR";
		else
			return 1;	/* not a branch */
		foll[0] = (*rget)(map, reg);
		break;
	}
	if(i.lk)
		return 2;
	return 1;
}
