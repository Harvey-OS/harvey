#include <lib9.h>
#include <bio.h>
#include "mach.h"

	/* mips native disassembler */

typedef struct {
	uvlong addr;			/* pc of instr */
	uchar op;			/* bits 31-26 */
	uchar rs;			/* bits 25-21 */
	uchar rt;			/* bits 20-16 */
	uchar rd;			/* bits 15-11 */
	uchar sa;			/* bits 10-6 */
	uchar function;			/* bits 5-0 */
	long immediate;			/* bits 15-0 */
	ulong cofun;			/* bits 24-0 */
	ulong target;			/* bits 25-0 */
	long w0;
	char *curr;			/* current fill point */
	char *end;			/* end of buffer */
	char *err;
} Instr;

typedef struct {
	char *mnemonic;
	char *mipsco;
} Opcode;

static char mipscoload[] = "r%t,%l";
static char mipscoalui[] = "r%t,r%s,%i";
static char mipscoalu3op[] = "r%d,r%s,r%t";
static char mipscoboc[] = "r%s,r%t,%b";
static char mipscoboc0[] = "r%s,%b";
static char mipscorsrt[] = "r%s,r%t";
static char mipscorsi[] = "r%s,%i";
static char mipscoxxx[] = "%w";
static char mipscofp3[] = "f%a,f%d,f%t";	/* fd,fs,ft */
static char mipscofp2[] = "f%a,f%d";		/* fd,fs */
static char mipscofpc[] = "f%d,f%t";		/* fs,ft */

static Opcode opcodes[64] = {
	0,		0,
	0,		0,
	"j",		"%j",
	"jal",		"%j",
	"beq",		mipscoboc,
	"bne",		mipscoboc,
	"blez",		mipscoboc0,
	"bgtz",		mipscoboc0,
	"addi",		mipscoalui,
	"addiu",	mipscoalui,
	"slti",		mipscoalui,
	"sltiu",	mipscoalui,
	"andi",		mipscoalui,
	"ori",		mipscoalui,
	"xori",		mipscoalui,
	"lui",		"r%t,%u",
	"cop0",		0,
	"cop1",		0,
	"cop2",		0,
	"cop3",		0,
	"beql",		mipscoboc,
	"bnel",		mipscoboc,
	"blezl",	mipscoboc0,
	"bgtzl",	mipscoboc0,
	"instr18",	mipscoxxx,
	"instr19",	mipscoxxx,
	"instr1A",	mipscoxxx,
	"instr1B",	mipscoxxx,
	"instr1C",	mipscoxxx,
	"instr1D",	mipscoxxx,
	"instr1E",	mipscoxxx,
	"instr1F",	mipscoxxx,
	"lb",		mipscoload,
	"lh",		mipscoload,
	"lwl",		mipscoload,
	"lw",		mipscoload,
	"lbu",		mipscoload,
	"lhu",		mipscoload,
	"lwr",		mipscoload,
	"instr27",	mipscoxxx,
	"sb",		mipscoload,
	"sh",		mipscoload,
	"swl",		mipscoload,
	"sw",		mipscoload,
	"instr2C",	mipscoxxx,
	"instr2D",	mipscoxxx,
	"swr",		mipscoload,
	"cache",	"",
	"ll",		mipscoload,
	"lwc1",		mipscoload,
	"lwc2",		mipscoload,
	"lwc3",		mipscoload,
	"instr34",	mipscoxxx,
	"ld",		mipscoload,
	"ld",		mipscoload,
	"ld",		mipscoload,
	"sc",		mipscoload,
	"swc1",		mipscoload,
	"swc2",		mipscoload,
	"swc3",		mipscoload,
	"instr3C",	mipscoxxx,
	"sd",		mipscoload,
	"sd",		mipscoload,
	"sd",		mipscoload,
};

static Opcode sopcodes[64] = {
	"sll",		"r%d,r%t,$%a",
	"special01",	mipscoxxx,
	"srl",		"r%d,r%t,$%a",
	"sra",		"r%d,r%t,$%a",
	"sllv",		"r%d,r%t,R%s",
	"special05",	mipscoxxx,
	"srlv",		"r%d,r%t,r%s",
	"srav",		"r%d,r%t,r%s",
	"jr",		"r%s",
	"jalr",		"r%d,r%s",
	"special0A",	mipscoxxx,
	"special0B",	mipscoxxx,
	"syscall",	"",
	"break",	"",
	"special0E",	mipscoxxx,
	"sync",		"",
	"mfhi",		"r%d",
	"mthi",		"r%s",
	"mflo",		"r%d",
	"mtlo",		"r%s",
	"special14",	mipscoxxx,
	"special15",	mipscoxxx,
	"special16",	mipscoxxx,
	"special17",	mipscoxxx,
	"mult",		mipscorsrt,
	"multu",	mipscorsrt,
	"div",		mipscorsrt,
	"divu",		mipscorsrt,
	"special1C",	mipscoxxx,
	"special1D",	mipscoxxx,
	"special1E",	mipscoxxx,
	"special1F",	mipscoxxx,
	"add",		mipscoalu3op,
	"addu",		mipscoalu3op,
	"sub",		mipscoalu3op,
	"subu",		mipscoalu3op,
	"and",		mipscoalu3op,
	"or",		mipscoalu3op,
	"xor",		mipscoalu3op,
	"nor",		mipscoalu3op,
	"special28",	mipscoxxx,
	"special29",	mipscoxxx,
	"slt",		mipscoalu3op,
	"sltu",		mipscoalu3op,
	"special2C",	mipscoxxx,
	"special2D",	mipscoxxx,
	"special2E",	mipscoxxx,
	"special2F",	mipscoxxx,
	"tge",		mipscorsrt,
	"tgeu",		mipscorsrt,
	"tlt",		mipscorsrt,
	"tltu",		mipscorsrt,
	"teq",		mipscorsrt,
	"special35",	mipscoxxx,
	"tne",		mipscorsrt,
	"special37",	mipscoxxx,
	"special38",	mipscoxxx,
	"special39",	mipscoxxx,
	"special3A",	mipscoxxx,
	"special3B",	mipscoxxx,
	"special3C",	mipscoxxx,
	"special3D",	mipscoxxx,
	"special3E",	mipscoxxx,
	"special3F",	mipscoxxx,
};

static Opcode ropcodes[32] = {
	"bltz",		mipscoboc0,
	"bgez",		mipscoboc0,
	"bltzl",	mipscoboc0,
	"bgezl",	mipscoboc0,
	"regimm04",	mipscoxxx,
	"regimm05",	mipscoxxx,
	"regimm06",	mipscoxxx,
	"regimm07",	mipscoxxx,
	"tgei",		mipscorsi,
	"tgeiu",	mipscorsi,
	"tlti",		mipscorsi,
	"tltiu",	mipscorsi,
	"teqi",		mipscorsi,
	"regimm0D",	mipscoxxx,
	"tnei",		mipscorsi,
	"regimm0F",	mipscoxxx,
	"bltzal",	mipscoboc0,
	"bgezal",	mipscoboc0,
	"bltzall",	mipscoboc0,
	"bgezall",	mipscoboc0,
	"regimm14",	mipscoxxx,
	"regimm15",	mipscoxxx,
	"regimm16",	mipscoxxx,
	"regimm17",	mipscoxxx,
	"regimm18",	mipscoxxx,
	"regimm19",	mipscoxxx,
	"regimm1A",	mipscoxxx,
	"regimm1B",	mipscoxxx,
	"regimm1C",	mipscoxxx,
	"regimm1D",	mipscoxxx,
	"regimm1E",	mipscoxxx,
	"regimm1F",	mipscoxxx,
};

static Opcode fopcodes[64] = {
	"add.%f",	mipscofp3,
	"sub.%f",	mipscofp3,
	"mul.%f",	mipscofp3,
	"div.%f",	mipscofp3,
	"sqrt.%f",	mipscofp2,
	"abs.%f",	mipscofp2,
	"mov.%f",	mipscofp2,
	"neg.%f",	mipscofp2,
	"finstr08",	mipscoxxx,
	"finstr09",	mipscoxxx,
	"finstr0A",	mipscoxxx,
	"finstr0B",	mipscoxxx,
	"round.w.%f",	mipscofp2,
	"trunc.w%f",	mipscofp2,
	"ceil.w%f",	mipscofp2,
	"floor.w%f",	mipscofp2,
	"finstr10",	mipscoxxx,
	"finstr11",	mipscoxxx,
	"finstr12",	mipscoxxx,
	"finstr13",	mipscoxxx,
	"finstr14",	mipscoxxx,
	"finstr15",	mipscoxxx,
	"finstr16",	mipscoxxx,
	"finstr17",	mipscoxxx,
	"finstr18",	mipscoxxx,
	"finstr19",	mipscoxxx,
	"finstr1A",	mipscoxxx,
	"finstr1B",	mipscoxxx,
	"finstr1C",	mipscoxxx,
	"finstr1D",	mipscoxxx,
	"finstr1E",	mipscoxxx,
	"finstr1F",	mipscoxxx,
	"cvt.s.%f",	mipscofp2,
	"cvt.d.%f",	mipscofp2,
	"cvt.e.%f",	mipscofp2,
	"cvt.q.%f",	mipscofp2,
	"cvt.w.%f",	mipscofp2,
	"finstr25",	mipscoxxx,
	"finstr26",	mipscoxxx,
	"finstr27",	mipscoxxx,
	"finstr28",	mipscoxxx,
	"finstr29",	mipscoxxx,
	"finstr2A",	mipscoxxx,
	"finstr2B",	mipscoxxx,
	"finstr2C",	mipscoxxx,
	"finstr2D",	mipscoxxx,
	"finstr2E",	mipscoxxx,
	"finstr2F",	mipscoxxx,
	"c.f.%f",	mipscofpc,
	"c.un.%f",	mipscofpc,
	"c.eq.%f",	mipscofpc,
	"c.ueq.%f",	mipscofpc,
	"c.olt.%f",	mipscofpc,
	"c.ult.%f",	mipscofpc,
	"c.ole.%f",	mipscofpc,
	"c.ule.%f",	mipscofpc,
	"c.sf.%f",	mipscofpc,
	"c.ngle.%f",	mipscofpc,
	"c.seq.%f",	mipscofpc,
	"c.ngl.%f",	mipscofpc,
	"c.lt.%f",	mipscofpc,
	"c.nge.%f",	mipscofpc,
	"c.le.%f",	mipscofpc,
	"c.ngt.%f",	mipscofpc,
};

static char fsub[16] = {
	's', 'd', 'e', 'q', 'w', '?', '?', '?',
	'?', '?', '?', '?', '?', '?', '?', '?'
};


static int
mkinstr(Instr *i, Map *map, uvlong pc)
{
	ulong w;

	if (get4(map, pc, &w) < 0) {
		werrstr("can't read instruction: %r");
		return -1;
	}
	i->addr = pc;
	i->op = (w >> 26) & 0x3F;
	i->rs = (w >> 21) & 0x1F;
	i->rt = (w >> 16) & 0x1F;
	i->rd = (w >> 11) & 0x1F;
	i->sa = (w >> 6) & 0x1F;
	i->function = w & 0x3F;
	i->immediate = w & 0x0000FFFF;
	if (i->immediate & 0x8000)
		i->immediate |= ~0x0000FFFF;
	i->cofun = w & 0x01FFFFFF;
	i->target = w & 0x03FFFFFF;
	i->w0 = w;
	return 1;
}

#pragma	varargck	argpos	bprint		2

static void
bprint(Instr *i, char *fmt, ...)
{
	va_list arg;

	va_start(arg, fmt);
	i->curr = vseprint(i->curr, i->end, fmt, arg);
	va_end(arg);
}

static void
format(char *mnemonic, Instr *i, char *f)
{
	if (mnemonic)
		format(0, i, mnemonic);
	if (f == 0)
		return;
	if (i->curr < i->end)
		*i->curr++ = '\t';
	for ( ; *f && i->curr < i->end; f++) {
		if (*f != '%') {
			*i->curr++ = *f;
			continue;
		}
		switch (*++f) {

		case 's':
			bprint(i, "%d", i->rs);
			break;

		case 't':
			bprint(i, "%d", i->rt);
			break;

		case 'd':
			bprint(i, "%d", i->rd);
			break;

		case 'a':
			bprint(i, "%d", i->sa);
			break;

		case 'l':
			if (i->rs == 30) {
				i->curr += symoff(i->curr, i->end-i->curr, i->immediate+mach->sb, CANY);
				bprint(i, "(SB)");
			} else 
				bprint(i, "%lx(r%d)", i->immediate, i->rs);
			break;

		case 'i':
			bprint(i, "$%lx", i->immediate);
			break;

		case 'u':
			*i->curr++ = '$';
			i->curr += symoff(i->curr, i->end-i->curr, i->immediate, CANY);
			bprint(i, "(SB)");
			break;

		case 'j':
			i->curr += symoff(i->curr, i->end-i->curr,
				(i->target<<2)|(i->addr & 0xF0000000), CANY);
			bprint(i, "(SB)");
			break;

		case 'b':
			i->curr += symoff(i->curr, i->end-i->curr,
				(i->immediate<<2)+i->addr+4, CANY);
			break;

		case 'c':
			bprint(i, "%lux", i->cofun);
			break;

		case 'w':
			bprint(i, "[%lux]", i->w0);
			break;

		case 'f':
			*i->curr++ = fsub[i->rs & 0x0F];
			break;

		case '\0':
			*i->curr++ = '%';
			return;

		default:
			bprint(i, "%%%c", *f);
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

	if (i->rs >= 0x10) {
		switch (i->cofun) {
	
		case 1:
			m = "tlbr";
			break;
	
		case 2:
			m = "tlbwi";
			break;
	
		case 6:
			m = "tlbwr";
			break;
	
		case 8:
			m = "tlbp";
			break;
	
		case 16:
			m = "rfe";
			break;
	
		case 32:
			m = "eret";
			break;
		}
		if (m) {
			format(m, i, 0);
			if (i->curr < i->end)
				*i->curr++ = 0;
			return;
		}
	}
	copz(0, i);
}

int
_mipscoinst(Map *map, uvlong pc, char *buf, int n)
{
	Instr i;
	Opcode *o;
	uchar op;

	i.curr = buf;
	i.end = buf+n-1;
	if (mkinstr(&i, map, pc) < 0)
		return -1;
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
		return 4;

	case 0x11:					/* COP1 */
		if (i.rs & 0x10) {
			o = fopcodes;
			op = i.function;
			break;
		}
		/*FALLTHROUGH*/
	case 0x12:					/* COP2 */
	case 0x13:					/* COP3 */
		copz(i.op-0x10, &i);
		return 4;

	default:
		o = opcodes;
		op = i.op;
		break;
	}
	format(o[op].mnemonic, &i, o[op].mipsco);
	return 4;
}
