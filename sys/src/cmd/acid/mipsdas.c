#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ctype.h>
#include <mach.h>
#define Extern extern
#include "acid.h"

typedef struct
{
	uchar op;			/* bits 31-26 */
	uchar rs;			/* bits 25-21 */
	uchar rt;			/* bits 20-16 */
	uchar rd;			/* bits 15-11 */
	uchar sa;			/* bits 10-6 */
	uchar function;			/* bits 5-0 */
	long immediate;			/* bits 15-0 */
	ulong cofun;			/* bits 24-0 */
	ulong target;			/* bits 25-0 */
	ulong w0;
	ulong w1;
} Instr;

static	char FRAMENAME[] = ".frame";
static	void format(char*, Instr*, char*);
static	int myisp;
static	Map *mymap;

static void
mkinstr(Instr *i)
{
	ulong w;
	Instr x;

	get4(mymap, dot, myisp, (long*)&w);

	i->op = (w >> 26) & 0x3F;
	i->rs = (w >> 21) & 0x1F;
	i->rt = (w >> 16) & 0x1F;
	i->rd = (w >> 11) & 0x1F;
	i->sa = (w >> 6) & 0x1F;
	i->function = w & 0x3F;
	i->immediate = w & 0x0000FFFF;
	if (i->immediate & 0x8000)
		i->immediate |= 0xFFFF0000;
	i->cofun = w & 0x01FFFFFF;
	i->target = w & 0x03FFFFFF;
	i->w0 = w;
	/*
	 * if it's a LUI followed by an ORI,
	 * it's an immediate load of a large constant.
	 * fix the LUI immediate in any case.
	 */
	if (i->op == 0x0F) {
		dot += 4;
		mkinstr(&x);
		dot -= 4;
		i->immediate <<= 16;
		if (x.op == 0x0D && x.rs == x.rt && x.rt == i->rt) {
			i->immediate |= (x.immediate & 0xFFFF);
			i->w1 = x.w0;
			dotinc = 8;
			return;
		}
	}
	/*
	 * if it's a LWC1 followed by another LWC1
	 * into an adjacent register, it's a load of
	 * a floating point double.
	 */
	else if (i->op == 0x31 && (i->rt & 0x01)) {
		dot += 4;
		mkinstr(&x);
		dot -= 4;
		if (x.op == 0x31 && x.rt == (i->rt - 1) && x.rs == i->rs) {
			i->rt -= 1;
			i->w1 = x.w0;
			dotinc = 8;
			return;
		}
	}
	/*
	 * similarly for double stores
	 */
	else if (i->op == 0x39 && (i->rt & 0x01)) {
		dot += 4;
		mkinstr(&x);
		dot -= 4;
		if (x.op == 0x39 && x.rt == (i->rt - 1) && x.rs == i->rs) {
			i->rt -= 1;
			i->w1 = x.w0;
			dotinc = 8;
			return;
		}
	}
	dotinc = 4;
}

typedef struct Opcode Opcode;
struct Opcode {
	char *mnemonic;
	void (*f)(Opcode *, Instr *);
	char *ken;
};


static void
branch(Opcode *o, Instr *i)
{
	if (i->rs == 0 && i->rt == 0)
		format("JMP", i, "%b");
	else if (i->rs == 0)
		format(o->mnemonic, i, "R%t,%b");
	else if (i->rt == 0)
		format(o->mnemonic, i, "R%s,%b");
	else
		format(o->mnemonic, i, "R%s,R%t,%b");
}

static void
addi(Opcode *o, Instr *i)
{
	if (i->rs == i->rt)
		format(o->mnemonic, i, "%i,R%t");
	else if (i->rs == 0)
		format("MOVW", i, "%i,R%t");
	else if (i->rs == 30) {
		dprint("MOVW\t$");
		psymoff(i->immediate+mach->sb, SEGDATA, "(SB),");
		dprint("R%d", i->rt);
	}
	else
		format(o->mnemonic, i, o->ken);
}

static void
andi(Opcode *o, Instr *i)
{
	if (i->rs == i->rt)
		format(o->mnemonic, i, "%i,R%t");
	else
		format(o->mnemonic, i, o->ken);
}

static void
lw(Opcode *o, Instr *i, char r)
{
	char *m;
	Symbol s;
	ulong l, h;
	int offset;

	if (r == 'F') {
		if (dotinc == 8)
			m = "MOVD";
		else
			m = "MOVF";
	}
	else
		m = o->mnemonic;
	if(i->rs == 29) {
		if (findsym(dot, CTEXT, &s) && findlocal(&s, FRAMENAME, &s)) {
			offset = s.value-i->immediate;
			if(offset > 0 && getauto(&s, offset, CAUTO, &s)){
				dprint("%s\t%s-%d(SP),%c%d", m, s.name,
					offset, r, i->rt);
				return;
			}
			else if (getauto(&s, (-offset)-4, CPARAM, &s)) {
				dprint("%s\t%s+%d(FP),%c%d",
					m, s.name, -offset, r, i->rt);
				return;
			}
		}
	}
	if(i->rs == 30 && mach->sb) {
		dprint("%s\t", m);
		if (r == 'F') {
			offset = i->immediate+mach->sb;
			if (get1(symmap, offset, SEGDATA, (uchar*)&h, 4))
			if (get1(symmap, offset+4, SEGDATA, (uchar*)&l, 4)) {
				dprint("$%s,%c%d", ieeedtos("%g", h, l), r, i->rt);
				return;
			}
		}
		psymoff(i->immediate+mach->sb, SEGDATA, "(SB),");
		dprint("%c%d", r, i->rt);
		return;
	}
	if (r == 'F')
		format(m, i, "%l,F%t");
	else
		format(m, i, o->ken);
}

static void
load(Opcode *o, Instr *i)
{
	lw(o, i, 'R');
}

static void
lwc1(Opcode *o, Instr *i)
{
	lw(o, i, 'F');
}

static void
sw(Opcode *o, Instr *i, char r)
{
	int offset;
	char *m;
	Symbol s;

	if (r == 'F') {
		if (dotinc == 8)
			m = "MOVD";
		else
			m = "MOVF";
	}
	else
		m = o->mnemonic;

	if(i->rs == 29) {
		if (findsym(dot, CTEXT, &s) && findlocal(&s, FRAMENAME, &s)) {
			offset = s.value-i->immediate;
			if (offset > 0 && getauto(&s, offset, CAUTO, &s)) {
				dprint("%s\t%c%d,%s-%d(SP)", m, r, i->rt,
					s.name, offset);
				return;
			}
		}
	}

	if(i->rs == 30 && mach->sb) {
		dprint("%s\t%c%d,", m, r, i->rt);
		psymoff(i->immediate+mach->sb, SEGDATA, "(SB)");
		return;
	}
	if (r == 'F')
		format(m, i, "F%t,%l");
	else
		format(m, i, o->ken);
}

static void
store(Opcode *o, Instr *i)
{
	sw(o, i, 'R');
}

static void
swc1(Opcode *o, Instr *i)
{
	sw(o, i, 'F');
}

static void
sll(Opcode *o, Instr *i)
{
	if (i->w0 == 0)
		dprint("NOOP");
	else if (i->rd == i->rt)
		format(o->mnemonic, i, "$%a,R%d");
	else
		format(o->mnemonic, i, o->ken);
}

static void
sllv(Opcode *o, Instr *i)
{
	if (i->rd == i->rt)
		format(o->mnemonic, i, "R%s,R%d");
	else
		format(o->mnemonic, i, o->ken);
}

static void
jal(Opcode *o, Instr *i)
{
	if (i->rd == 31)
		format("JAL", i, "(R%s)");
	else
		format(o->mnemonic, i, o->ken);
}

static void
add(Opcode *o, Instr *i)
{
	if (i->rd == i->rs)
		format(o->mnemonic, i, "R%t,R%d");
	else if (i->rd == i->rt)
		format(o->mnemonic, i, "R%s,R%d");
	else
		format(o->mnemonic, i, o->ken);
}

static void
sub(Opcode *o, Instr *i)
{
	if (i->rd == i->rs)
		format(o->mnemonic, i, "R%t,R%d");
	else
		format(o->mnemonic, i, o->ken);
}

static void
or(Opcode *o, Instr *i)
{
	if (i->rs == 0 && i->rt == 0)
		format("MOVW", i, "$0,R%d");
	else if (i->rs == 0)
		format("MOVW", i, "R%t,R%d");
	else if (i->rt == 0)
		format("MOVW", i, "R%s,R%d");
	else
		add(o, i);
}

static void
nor(Opcode *o, Instr *i)
{
	if (i->rs == 0 && i->rt == 0 && i->rd == 0)
		format("NOP", i, 0);
	else
		add(o, i);
}

static char mipscoload[] = "r%t,%l";
static char mipsload[] = "%l,R%t";
static char mipsstore[] = "R%t,%l";
static char mipsalui[] = "%i,R%s,R%t";
static char mipsalu3op[] = "R%t,R%s,R%d";
static char mipsrtrs[] = "R%t,R%s";
static char mipscorsrt[] = "r%s,r%t";
static char mipscorsi[] = "r%s,%i";
static char mipscoxxx[] = "%w";
static char mipscofp3[] = "f%a,f%d,f%t";	/* fd,fs,ft */
static char mipsfp3[] = "F%t,F%d,F%a";
static char mipscofp2[] = "f%a,f%d";		/* fd,fs */
static char mipsfp2[] = "F%d,F%a";
static char mipscofpc[] = "f%d,f%t";		/* fs,ft */
static char mipsfpc[] = "F%t,F%d";

static Opcode opcodes[64] =
{
	0,		0,	0,
	0,		0,	0,
	"JMP",		0,	"%j",
	"JAL",		0,	"%j",
	"BEQ",	   branch,	0,
	"BNE",	   branch,	0,
	"BLEZ",	   branch,	0,
	"BGTZ",	   branch,	0,
	"ADD",	     addi,	mipsalui,
	"ADDU",	     addi,	mipsalui,
	"SGT",		0,	mipsalui,
	"SGTU",		0,	mipsalui,
	"AND",	     andi,	mipsalui,
	"OR",	     andi,	mipsalui,
	"XOR",	     andi,	mipsalui,
	"MOVW",		0,	"$%u,R%t",
	"cop0",		0,	0,
	"cop1",		0,	0,
	"cop2",		0,	0,
	"cop3",		0,	0,
	"BEQL",	   branch,	0,
	"BNEL",	   branch,	0,
	"BLEZL",   branch,	0,
	"BGTZL",   branch,	0,
	"instr18",	0,	mipscoxxx,
	"instr19",	0,	mipscoxxx,
	"instr1A",	0,	mipscoxxx,
	"instr1B",	0,	mipscoxxx,
	"instr1C",	0,	mipscoxxx,
	"instr1D",	0,	mipscoxxx,
	"instr1E",	0,	mipscoxxx,
	"instr1F",	0,	mipscoxxx,
	"MOVB",	     load,	mipsload,
	"MOVH",	     load,	mipsload,
	"lwl",		0,	mipscoload,
	"MOVW",	     load,	mipsload,
	"MOVBU",     load,	mipsload,
	"MOVHU",     load,	mipsload,
	"lwr",		0,	mipscoload,
	"instr27",	0,	mipscoxxx,
	"MOVB",	    store,	mipsstore,
	"MOVH",	    store,	mipsstore,
	"swl",		0,	mipscoload,
	"MOVW",	    store,	mipsstore,
	"instr2C",	0,	mipscoxxx,
	"instr2D",	0,	mipscoxxx,
	"swr",		0,	mipscoload,
	"CACHE",	0,	0,
	"ll",		0,	mipscoload,
	"MOVW",	     lwc1,	mipscoload,
	"lwc2",		0,	mipscoload,
	"lwc3",		0,	mipscoload,
	"instr34",	0,	mipscoxxx,
	"ldc1",		0,	mipscoload,
	"ldc2",		0,	mipscoload,
	"ldc3",		0,	mipscoload,
	"sc",		0,	mipscoload,
	"swc1",	     swc1,	mipscoload,
	"swc2",		0,	mipscoload,
	"swc3",		0,	mipscoload,
	"instr3C",	0,	mipscoxxx,
	"sdc1",		0,	mipscoload,
	"sdc2",		0,	mipscoload,
	"sdc3",		0,	mipscoload,
};

static Opcode sopcodes[64] =
{
	"SLL",	      sll,	"$%a,R%t,R%d,",
	"special01",	0,	mipscoxxx,
	"SRL",	      sll,	"$%a,R%t,R%d,",
	"SRA",	      sll,	"$%a,R%t,R%d,",
	"SLL",	     sllv,	"R%s,R%t,R%d",
	"special05",	0,	mipscoxxx,
	"SRL",	     sllv,	"R%s,R%t,R%d",
	"SRA",	     sllv,	"R%s,R%t,R%d",
	"JMP",		0,	"(R%s)",
	"jal",	      jal,	"r%d,r%s",
	"special0A",	0,	mipscoxxx,
	"special0B",	0,	mipscoxxx,
	"SYSCALL",	0,	0,
	"BREAK",	0,	0,
	"special0E",	0,	mipscoxxx,
	"SYNC",		0,	0,
	"MOVW",		0,	"HI,R%d",
	"MOVW",		0,	"R%s,HI",
	"MOVW",		0,	"LO,R%d",
	"MOVW",		0,	"R%s,LO",
	"special14",	0,	mipscoxxx,
	"special15",	0,	mipscoxxx,
	"special16",	0,	mipscoxxx,
	"special17",	0,	mipscoxxx,
	"MUL",		0,	mipsrtrs,
	"MULU",		0,	mipsrtrs,
	"DIV",		0,	mipsrtrs,
	"DIVU",		0,	mipsrtrs,
	"special1C",	0,	mipscoxxx,
	"special1D",	0,	mipscoxxx,
	"special1E",	0,	mipscoxxx,
	"special1F",	0,	mipscoxxx,
	"ADD",	      add,	mipsalu3op,
	"ADDU",	      add,	mipsalu3op,
	"SUB",	      sub,	mipsalu3op,
	"SUBU",	      sub,	mipsalu3op,
	"AND",	      add,	mipsalu3op,
	"OR",	       or,	mipsalu3op,
	"XOR",	      add,	mipsalu3op,
	"NOR",	      nor,	mipsalu3op,
	"special28",	0,	mipscoxxx,
	"special29",	0,	mipscoxxx,
	"SGT",		0,	mipsalu3op,
	"SGTU",		0,	mipsalu3op,
	"special2C",	0,	mipscoxxx,
	"special2D",	0,	mipscoxxx,
	"special2E",	0,	mipscoxxx,
	"special2F",	0,	mipscoxxx,
	"tge",		0,	mipscorsrt,
	"tgeu",		0,	mipscorsrt,
	"tlt",		0,	mipscorsrt,
	"tltu",		0,	mipscorsrt,
	"teq",		0,	mipscorsrt,
	"special35",	0,	mipscoxxx,
	"tne",		0,	mipscorsrt,
	"special37",	0,	mipscoxxx,
	"special38",	0,	mipscoxxx,
	"special39",	0,	mipscoxxx,
	"special3A",	0,	mipscoxxx,
	"special3B",	0,	mipscoxxx,
	"special3C",	0,	mipscoxxx,
	"special3D",	0,	mipscoxxx,
	"special3E",	0,	mipscoxxx,
	"special3F",	0,	mipscoxxx,
};

static Opcode ropcodes[32] = 
{
	"BLTZ",	   branch,	0,
	"BGEZ",	   branch,	0,
	"BLTZL",   branch,	0,
	"BGEZL",   branch,	0,
	"regimm04",	0,	mipscoxxx,
	"regimm05",	0,	mipscoxxx,
	"regimm06",	0,	mipscoxxx,
	"regimm07",	0,	mipscoxxx,
	"tgei",		0,	mipscorsi,
	"tgeiu",	0,	mipscorsi,
	"tlti",		0,	mipscorsi,
	"tltiu",	0,	mipscorsi,
	"teqi",		0,	mipscorsi,
	"regimm0D",	0,	mipscoxxx,
	"tnei",		0,	mipscorsi,
	"regimm0F",	0,	mipscoxxx,
	"BLTZAL",  branch,	0,
	"BGEZAL",  branch,	0,
	"BLTZALL", branch,	0,
	"BGEZALL", branch,	0,
	"regimm14",	0,	mipscoxxx,
	"regimm15",	0,	mipscoxxx,
	"regimm16",	0,	mipscoxxx,
	"regimm17",	0,	mipscoxxx,
	"regimm18",	0,	mipscoxxx,
	"regimm19",	0,	mipscoxxx,
	"regimm1A",	0,	mipscoxxx,
	"regimm1B",	0,	mipscoxxx,
	"regimm1C",	0,	mipscoxxx,
	"regimm1D",	0,	mipscoxxx,
	"regimm1E",	0,	mipscoxxx,
	"regimm1F",	0,	mipscoxxx,
};

static Opcode fopcodes[64] =
{
	"ADD%f",	0,	mipsfp3,
	"SUB%f",	0,	mipsfp3,
	"MUL%f",	0,	mipsfp3,
	"DIV%f",	0,	mipsfp3,
	"sqrt.%f",	0,	mipscofp2,
	"ABS%f",	0,	mipsfp2,
	"MOV%f",	0,	mipsfp2,
	"NEG%f",	0,	mipsfp2,
	"finstr08",	0,	mipscoxxx,
	"finstr09",	0,	mipscoxxx,
	"finstr0A",	0,	mipscoxxx,
	"finstr0B",	0,	mipscoxxx,
	"round.w.%f",	0,	mipscofp2,
	"trunc.w%f",	0,	mipscofp2,
	"ceil.w%f",	0,	mipscofp2,
	"floor.w%f",	0,	mipscofp2,
	"finstr10",	0,	mipscoxxx,
	"finstr11",	0,	mipscoxxx,
	"finstr12",	0,	mipscoxxx,
	"finstr13",	0,	mipscoxxx,
	"finstr14",	0,	mipscoxxx,
	"finstr15",	0,	mipscoxxx,
	"finstr16",	0,	mipscoxxx,
	"finstr17",	0,	mipscoxxx,
	"finstr18",	0,	mipscoxxx,
	"finstr19",	0,	mipscoxxx,
	"finstr1A",	0,	mipscoxxx,
	"finstr1B",	0,	mipscoxxx,
	"finstr1C",	0,	mipscoxxx,
	"finstr1D",	0,	mipscoxxx,
	"finstr1E",	0,	mipscoxxx,
	"finstr1F",	0,	mipscoxxx,
	"cvt.s.%f",	0,	mipscofp2,
	"cvt.d.%f",	0,	mipscofp2,
	"cvt.e.%f",	0,	mipscofp2,
	"cvt.q.%f",	0,	mipscofp2,
	"cvt.w.%f",	0,	mipscofp2,
	"finstr25",	0,	mipscoxxx,
	"finstr26",	0,	mipscoxxx,
	"finstr27",	0,	mipscoxxx,
	"finstr28",	0,	mipscoxxx,
	"finstr29",	0,	mipscoxxx,
	"finstr2A",	0,	mipscoxxx,
	"finstr2B",	0,	mipscoxxx,
	"finstr2C",	0,	mipscoxxx,
	"finstr2D",	0,	mipscoxxx,
	"finstr2E",	0,	mipscoxxx,
	"finstr2F",	0,	mipscoxxx,
	"c.f.%f",	0,	mipscofpc,
	"c.un.%f",	0,	mipscofpc,
	"CMPEQ%f",	0,	mipsfpc,
	"c.ueq.%f",	0,	mipscofpc,
	"c.olt.%f",	0,	mipscofpc,
	"c.ult.%f",	0,	mipscofpc,
	"c.ole.%f",	0,	mipscofpc,
	"c.ule.%f",	0,	mipscofpc,
	"c.sf.%f",	0,	mipscofpc,
	"c.ngle.%f",	0,	mipscofpc,
	"c.seq.%f",	0,	mipscofpc,
	"c.ngl.%f",	0,	mipscofpc,
	"CMPGT%f",	0,	mipsfpc,
	"c.nge.%f",	0,	mipscofpc,
	"CMPGE%f",	0,	mipsfpc,
	"c.ngt.%f",	0,	mipscofpc,
};

static char *cop0regs[32] =
{
	"INDEX",
	"RANDOM",
	"TLBPHYS",
	"EntryLo0",
	"CONTEXT",
	"PageMask",
	"Wired",
	"Error",
	"BADVADDR",
	"Count",
	"TLBVIRT",
	"Compare",
	"STATUS",
	"CAUSE",
	"EPC",
	"PRID",
	"Config",
	"LLadr",
	"WatchLo",
	"WatchHi",
	"20",
	"21",
	"22",
	"23",
	"24",
	"25",
	"26",
	"CacheErr",
	"TagLo",
	"TagHi",
	"ErrorEPC",
	"31"
};

static char fsub[16] = {
	'F', 'D', 'e', 'q', 'W', '?', '?', '?',
	'?', '?', '?', '?', '?', '?', '?', '?'
};

static void
format(char *mnemonic, Instr *i, char *f)
{
	if (mnemonic)
		format(0, i, mnemonic);
	if (f == 0)
		return;
	if (mnemonic)
		dprint("\t");
	for ( ; *f; f++) {
		if (*f != '%') {
			dprint("%c", *f);
			continue;
		}
		switch (*++f) {

		case 's':
			dprint("%d", i->rs);
			break;

		case 't':
			dprint("%d", i->rt);
			break;

		case 'd':
			dprint("%d", i->rd);
			break;

		case 'a':
			dprint("%d", i->sa);
			break;

		case 'l':
			dprint("%lx(R%d)", i->immediate, i->rs);
			break;

		case 'i':
			dprint("$%lx", i->immediate);
			break;

		case 'u':
			psymoff(i->immediate, SEGDATA, "(SB)");
			break;

		case 'j':
			psymoff((i->target<<2)|(dot & 0xF0000000), myisp, "(SB)");
			break;

		case 'b':
			psymoff((i->immediate<<2)+dot+4, myisp, "");
			break;

		case 'c':
			dprint("%lux", i->cofun);
			break;

		case 'w':
			dprint("[%lux]", i->w0);
			break;

		case 'm':
			dprint("M(%s)", cop0regs[i->rd]);
			break;

		case 'f':
			dprint("%c", fsub[i->rs & 0x0F]);
			break;

		case '\0':
			dprint("%c", '%');
			return;

		default:
			dprint("%%%c", *f);
			break;
		}
	}
}

static void
copz(int cop, Instr *i)
{
	char *f, *m, buf[16];

	m = buf;
	f = "%t,%d";
	switch (i->rs) {

	case 0:
		sprint(buf, "mfc%d", cop);
		break;

	case 2:
		sprint(buf, "cfc%d", cop);
		break;

	case 4:
		sprint(buf, "mtc%d", cop);
		break;

	case 6:
		sprint(buf, "ctc%d", cop);
		break;

	case 8:
		f = "%b";
		switch (i->rt) {

		case 0:
			sprint(buf, "bc%df", cop);
			break;

		case 1:
			sprint(buf, "bc%dt", cop);
			break;

		case 2:
			sprint(buf, "bc%dfl", cop);
			break;

		case 3:
			sprint(buf, "bc%dtl", cop);
			break;

		default:
			sprint(buf, "cop%d", cop);
			f = mipscoxxx;
			break;
		}
		break;

	default:
		sprint(buf, "cop%d", cop);
		if (i->rs & 0x10)
			f = "function %c";
		else
			f = mipscoxxx;
		break;
	}
	format(m, i, f);
}

static void
cop0(Instr *i)
{
	char *m = 0;

	if (i->rs < 8) {
		switch (i->rs) {

		case 0:
		case 1:
			format("MOVW", i, "%m,R%t");
			return;

		case 4:
		case 5:
			format("MOVW", i, "R%t,%m");
			return;
		}
	}
	else if (i->rs >= 0x10) {
		switch (i->cofun) {
	
		case 1:
			m = "TLBR";
			break;
	
		case 2:
			m = "TLBWI";
			break;
	
		case 6:
			m = "TLBWR";
			break;
	
		case 8:
			m = "TLBP";
			break;
	
		case 16:
			m = "RFE";
			break;
	
		case 32:
			m = "ERET";
			break;
		}
		if (m) {
			format(m, i, 0);
			return;
		}
	}
	copz(0, i);
}

static void
cop1(Instr *i)
{
	char *m = "MOVW";

	switch (i->rs) {

	case 0:
		format(m, i, "F%d,R%t");
		return;

	case 2:
		format(m, i, "FCR%d,R%t");
		return;

	case 4:
		format(m, i, "R%t,F%d");
		return;

	case 6:
		format(m, i, "R%t,FCR%d");
		return;

	case 8:
		switch (i->rt) {

		case 0:
			format("BFPF", i, "%b");
			return;

		case 1:
			format("BFPT", i, "%b");
			return;
		}
		break;
	}
	copz(1, i);
}

static void
printins(Map *map, char modifier, long isp)
{
	Instr i;
	Opcode *o;
	uchar op;

	USED(modifier);
	mymap = map;
	myisp = isp;
	mkinstr(&i);
	switch (i.op) {

	case 0x00:					/* SPECIAL */
		o = sopcodes;
		op = i.function;
		break;

	case 0x01:					/* REGIMM */
		o = ropcodes;
		op = i.rt;
		break;

	case 0x10:					/* COP0 */
		cop0(&i);
		return;

	case 0x11:					/* COP1 */
		if (i.rs & 0x10) {
			o = fopcodes;
			op = i.function;
			break;
		}
		cop1(&i);
		return;

	case 0x12:					/* COP2 */
	case 0x13:					/* COP3 */
		copz(i.op-0x10, &i);
		return;

	default:
		o = opcodes;
		op = i.op;
		break;
	}
	if (o[op].f)
		(*o[op].f)(&o[op], &i);
	else
		format(o[op].mnemonic, &i, o[op].ken);
}

extern	void	mipscoprintins(Map *, char, int);

void
mipsprintins(Map *map, char modifier, int isp)
{
/*
	if((asstype == AMIPSCO && modifier == 'i')
		|| (asstype == AMIPS && modifier == 'I'))
		mipscoprintins(map, modifier, isp);
	else
*/
		printins(map, modifier, isp);
}

int
mipsfoll(ulong pc, ulong *foll)
{
	ulong w, l;
	char buf[8];
	Instr i;

	dot = pc;
	mymap = cormap;
	myisp = SEGDATA;
	mkinstr(&i);
	w = i.w0;

	if((w&0xF3600000) == 0x41000000)	/* branch on coprocessor */
		goto cond;

	l = (w&0xFC000000)>>26;
	switch(l){
	case 0:		/* SPECIAL */
		if((w&0x3E) == 0x08){		/* JR, JALR */
			sprint(buf, "R%d", (w>>21)&0x1F);
			dot = look(buf)->v->ival;
			get4(mymap, dot, myisp, (long*)&foll[0]);
			return 1;
		}
		foll[0] = pc+dotinc;
		return 1;
	case 1:					/* BCOND */
	case 4:					/* BEQ */
	case 5:					/* BNE */
	case 6:					/* BLEZ */
	case 7:					/* BGTZ */
	cond:
		foll[0] = pc+8;
		l = ((w&0xFFFF)<<2);
		if(w & 0x8000)
			l |= 0xFFFC0000;
		foll[1] = pc+4 + l;
		return 2;
	case 2:					/* J */
	case 3:					/* JAL */
		foll[0] = (pc&0xF0000000) | ((w&0x03FFFFFF)<<2);
		return 1;
	}

	foll[0] = pc+dotinc;
	return 1;
}
