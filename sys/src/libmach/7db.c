#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>
/*
 * Alpha-specific debugger interface
 */

static 	char	*alphaexcep(Map*, Rgetter);
static	int	alphafoll(Map*, uvlong, Rgetter, uvlong*);
static	int	alphainst(Map*, uvlong, char, char*, int);
static	int	alphadas(Map*, uvlong, char*, int);
static	int	alphainstlen(Map*, uvlong);
/*
 *	Debugger interface
 */
Machdata alphamach =
{
	{0x80, 0, 0, 0},		/* break point */
	4,			/* break point size */

	leswab,			/* short to local byte order */
	leswal,			/* long to local byte order */
	leswav,			/* vlong to local byte order */
	risctrace,		/* C traceback */
	riscframe,		/* Frame finder */
	alphaexcep,		/* print exception */
	0,			/* breakpoint fixup */
	leieeesftos,		/* single precision float printer */
	leieeedftos,		/* double precisioin float printer */
	alphafoll,		/* following addresses */
	alphainst,		/* print instruction */
	alphadas,		/* dissembler */
	alphainstlen,		/* instruction size */
};

static char *illegaltype[] = {
	"breakpoint",
	"bugchk",
	"gentrap",
	"fen",
	"illegal instruction",
};

static char *
alphaexcep(Map *map, Rgetter rget)
{
	ulong type, a0, a1;
	static char buf[256];

	type = (*rget)(map, "TYPE");
	a0 = (*rget)(map, "A0");
	a1 = (*rget)(map, "A1");
/*	a2 = (*rget)(map, "A2"); */

	switch (type) {
	case 1:	/* arith */
		sprint(buf, "trap: arithmetic trap 0x%lux", a0);
		break;
	case 2:	/* bad instr or FEN */
		if (a0 <= 4)
			return illegaltype[a0];
		else
			sprint(buf, "illegal instr trap, unknown type %lud", a0);
		break;
	case 3:	/* intr */
		sprint(buf, "interrupt type %lud", a0);
		break;
	case 4:	/* memory fault */
		sprint(buf, "fault %s addr=0x%lux", (a1&1)?"write":"read", a0);
		break;
	case 5:	/* syscall() */
		return "system call";
	case 6:	/* alignment fault */
		sprint(buf, "unaligned op 0x%lux addr 0x%lux", a1, a0);
		break;
	default:	/* cannot happen */
		sprint(buf, "unknown exception type %lud", type);
		break;
	}
	return buf;
}

	/* alpha disassembler and related functions */

static	char FRAMENAME[] = ".frame";

typedef struct {
	uvlong addr;
	uchar op;			/* bits 31-26 */
	uchar ra;			/* bits 25-21 */
	uchar rb;			/* bits 20-16 */
	uchar rc;			/* bits 4-0 */
	long mem;			/* bits 15-0 */
	long branch;			/* bits 20-0 */
	uchar function;			/* bits 11-5 */
	uchar literal;			/* bits 20-13 */
	uchar islit;			/* bit 12 */
	uchar fpfn;			/* bits 10-5 */
	uchar fpmode;			/* bits 15-11 */
	long w0;
	long w1;
	int size;			/* instruction size */
	char *curr;			/* fill point in buffer */
	char *end;			/* end of buffer */
	char *err;			/* error message */
} Instr;

static Map *mymap;

static int
decode(uvlong pc, Instr *i)
{
	ulong w;

	if (get4(mymap, pc, &w) < 0) {
		werrstr("can't read instruction: %r");
		return -1;
	}
	i->addr = pc;
	i->size = 1;
	i->op = (w >> 26) & 0x3F;
	i->ra = (w >> 21) & 0x1F;
	i->rb = (w >> 16) & 0x1F;
	i->rc = w & 0x1F;
	i->function = (w >> 5) & 0x7F;
	i->mem = w & 0xFFFF;
	if (i->mem & 0x8000)
		i->mem -= 0x10000;
	i->branch = w & 0x1FFFFF;
	if (i->branch & 0x100000)
		i->branch -= 0x200000;
	i->function = (w >> 5) & 0x7F;
	i->literal = (w >> 13) & 0xFF;
	i->islit = (w >> 12) & 0x01;
	i->fpfn = (w >> 5) & 0x3F;
	i->fpmode = (w >> 11) & 0x1F;
	i->w0 = w;
	return 1;
}

static int
mkinstr(uvlong pc, Instr *i)
{
/*	Instr x; */

	if (decode(pc, i) < 0)
		return -1;

#ifdef	frommips
/* we probably want to do something like this for alpha... */
	/*
	 * if it's a LUI followed by an ORI,
	 * it's an immediate load of a large constant.
	 * fix the LUI immediate in any case.
	 */
	if (i->op == 0x0F) {
		if (decode(pc+4, &x) < 0)
			return 0;
		i->immediate <<= 16;
		if (x.op == 0x0D && x.rs == x.rt && x.rt == i->rt) {
			i->immediate |= (x.immediate & 0xFFFF);
			i->w1 = x.w0;
			i->size++;
			return 1;
		}
	}
#endif
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

typedef struct Opcode Opcode;

struct Opcode {
	char *mnemonic;
	void (*f)(Opcode *, Instr *);
	char *ken;
};

static void format(char *, Instr *, char *);

static int
plocal(Instr *i, char *m, char r, int store)
{
	int offset;
	char *reg;
	Symbol s;

	if (!findsym(i->addr, CTEXT, &s) || !findlocal(&s, FRAMENAME, &s))
		return 0;
	if (s.value > i->mem) {
		if(!getauto(&s, s.value-i->mem, CAUTO, &s))
			return 0;
		reg = "(SP)";
		offset = i->mem;
	} else {
		offset = i->mem-s.value-8;
		if (!getauto(&s, offset, CPARAM, &s))
			return 0;
		reg = "(FP)";
	}
	if (store)
		bprint(i, "%s\t%c%d,%s+%d%s", m, r, i->ra, s.name, offset, reg);
	else
		bprint(i, "%s\t%s+%d%s,%c%d", m, s.name, offset, reg, r, i->ra);
	return 1;
}

static void
_load(Opcode *o, Instr *i, char r)
{
	char *m;

	m = o->mnemonic;
	if (i->rb == 30 && plocal(i, m, r, 0))
		return;
	if (i->rb == 29 && mach->sb) {
		bprint(i, "%s\t", m);
		i->curr += symoff(i->curr, i->end-i->curr, i->mem+mach->sb, CANY);
		bprint(i, "(SB),%c%d", r, i->ra);
		return;
	}
	format(m, i, o->ken);
}

static void
load(Opcode *o, Instr *i)
{
	_load(o, i, 'R');
}

static void
loadf(Opcode *o, Instr *i)
{
	_load(o, i, 'F');
}

static void
_store(Opcode *o, Instr *i, char r)
{
	char *m;

	m = o->mnemonic;
	if (i->rb == 30 && plocal(i, m, r, 1))
		return;
	if (i->rb == 29 && mach->sb) {
		bprint(i, "%s\t%c%d,", m, r, i->ra);
		i->curr += symoff(i->curr, i->end-i->curr, i->mem+mach->sb, CANY);
		bprint(i, "(SB)");
		return;
	}
	format(o->mnemonic, i, o->ken);
}

static void
store(Opcode *o, Instr *i)
{
	_store(o, i, 'R');
}

static void
storef(Opcode *o, Instr *i)
{
	_store(o, i, 'F');
}

static void
misc(Opcode *o, Instr *i)
{
	char *f;

	USED(o);
	switch (i->mem&0xFFFF) {
	case 0x0000:
		f = "TRAPB";
		break;
	case 0x4000:
		f = "MB";
		break;
	case 0x8000:
		f = "FETCH\t0(R%b)";
		break;
	case 0xA000:
		f = "FETCH_M\t0(R%b)";
		break;
	case 0xC000:
		f = "RPCC\tR%a";
		break;
	case 0xE000:
		f = "RC\tR%a";
		break;
	case 0xF000:
		f = "RS\tR%a";
		break;
	default:
		f = "%w";
	}
	format(0, i, f);
}

static char	*jmpcode[4] = { "JMP", "JSR", "RET", "JSR_COROUTINE" };

static void
jmp(Opcode *o, Instr *i)
{
	int hint;
	char *m;

	USED(o);
	hint = (i->mem >> 14) & 3;
	m = jmpcode[hint];
	if (i->ra == 31) {
		if (hint == 2 && i->rb == 29)
			bprint(i, m);
		else
			format(m, i, "(R%b)");
	}
	else
		format(m, i, "R%a,(R%b)");
}

static void
br(Opcode *o, Instr *i)
{
	if (i->ra == 31)
		format(o->mnemonic, i, "%B");
	else
		format(o->mnemonic, i, o->ken);
}

static void
bsr(Opcode *o, Instr *i)
{
	if (i->ra == 26)
		format(o->mnemonic, i, "%B");
	else
		format(o->mnemonic, i, o->ken);
}

static void
mult(Opcode *o, Instr *i)
{
	char *m;

	switch (i->function) {
	case 0x00:
		m = "MULL";
		break;
	case 0x20:
		m = "MULQ";
		break;
	case 0x40:
		m = "MULL/V";
		break;
	case 0x60:
		m = "MULQ/V";
		break;
	case 0x30:
		m = "UMULH";
		break;
	default:
		format("???", i, "%w");
		return;
	}
	format(m, i, o->ken);
}

static char	alphaload[] = "%l,R%a";
static char	alphafload[] = "%l,F%a";
static char	alphastore[] = "R%a,%l";
static char	alphafstore[] = "F%a,%l";
static char	alphabranch[] = "R%a,%B";
static char	alphafbranch[] = "F%a,%B";
static char	alphaint[] = "%v,R%a,R%c";
static char	alphafp[] = "F%b,F%a,F%c";
static char	alphafp2[] = "F%b,F%c";
static char	alphaxxx[] = "%w";

static Opcode opcodes[64] = {
	"PAL",		0,	alphaxxx,
	"OPC01",	0,	alphaxxx,
	"OPC02",	0,	alphaxxx,
	"OPC03",	0,	alphaxxx,
	"OPC04",	0,	alphaxxx,
	"OPC05",	0,	alphaxxx,
	"OPC06",	0,	alphaxxx,
	"OPC07",	0,	alphaxxx,
	"MOVQA",	load,	alphaload,
	"MOVQAH",	load,	alphaload,
	"MOVBU",	load,	alphaload,		/* v 3 */
	"MOVQU",	load,	alphaload,
	"MOVWU",	load,	alphaload,		/* v 3 */
	"MOVWU",	store,	alphastore,		/* v 3 */
	"MOVBU",	store,	alphastore,		/* v 3 */
	"MOVQU",	store,	alphastore,
	0,		0,	0,			/* int arith */
	0,		0,	0,			/* logical */
	0,		0,	0,			/* shift */
	0,		mult,	alphaint,
	"OPC14",	0,	alphaxxx,
	"vax",		0,	alphafp,		/* vax */
	0,		0,	0,			/* ieee */
	0,		0,	0,			/* fp */
	0,		misc,	alphaxxx,
	"PAL19 [HW_MFPR]",0,	alphaxxx,
	"JSR",		jmp,	0,
	"PAL1B [HW_LD]",0,	alphaxxx,
	"OPC1C",	0,	alphaxxx,
	"PAL1D [HW_MTPR]",0,	alphaxxx,
	"PAL1E [HW_REI]",0,	alphaxxx,
	"PAL1F [HW_ST]",0,	alphaxxx,
	"MOVF",		loadf,	alphafload,
	"MOVG",		loadf,	alphafload,
	"MOVS",		loadf,	alphafload,
	"MOVT",		loadf,	alphafload,
	"MOVF",		storef,	alphafstore,
	"MOVG",		storef,	alphafstore,
	"MOVS",		storef,	alphafstore,
	"MOVT",		storef,	alphafstore,
	"MOVL",		load,	alphaload,
	"MOVQ",		load,	alphaload,
	"MOVLL",	load,	alphaload,
	"MOVQL",	load,	alphaload,
	"MOVL",		store,	alphastore,
	"MOVQ",		store,	alphastore,
	"MOVLC",	store,	alphastore,
	"MOVQC",	store,	alphastore,
	"JMP",		br,	alphabranch,
	"FBEQ",		0,	alphafbranch,
	"FBLT",		0,	alphafbranch,
	"FBLE",		0,	alphafbranch,
	"JSR",		bsr,	alphabranch,
	"FBNE",		0,	alphafbranch,
	"FBGE",		0,	alphafbranch,
	"FBGT",		0,	alphafbranch,
	"BLBC",		0,	alphafbranch,
	"BEQ",		0,	alphabranch,
	"BLT",		0,	alphabranch,
	"BLE",		0,	alphabranch,
	"BLBS",		0,	alphabranch,
	"BNE",		0,	alphabranch,
	"BGE",		0,	alphabranch,
	"BGT",		0,	alphabranch,
};

static Opcode fpopcodes[64] = {
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,

	"CVTLQ",	0,	alphafp2,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,

	"CPYS",		0,	alphafp,
	"CPYSN",	0,	alphafp,
	"CPYSE",	0,	alphafp,
	"???",		0,	alphaxxx,
	"MOVT",		0,	"FPCR,F%a",
	"MOVT",		0,	"F%a,FPCR",
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"FCMOVEQ",	0,	alphafp,
	"FCMOVNE",	0,	alphafp,
	"FCMOVLT",	0,	alphafp,
	"FCMOVGE",	0,	alphafp,
	"FCMOVLE",	0,	alphafp,
	"FCMOVGT",	0,	alphafp,

	"CVTQL",	0,	alphafp2,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
};

static Opcode ieeeopcodes[64] = {
	"ADDS",		0,	alphafp,
	"SUBS",		0,	alphafp,
	"MULS",		0,	alphafp,
	"DIVS",		0,	alphafp,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,

	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,

	"ADDT",		0,	alphafp,
	"SUBT",		0,	alphafp,
	"MULT",		0,	alphafp,
	"DIVT",		0,	alphafp,
	"CMPTUN",	0,	alphafp,
	"CMPTEQ",	0,	alphafp,
	"CMPTLT",	0,	alphafp,
	"CMPTLE",	0,	alphafp,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"CVTTS",	0,	alphafp2,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"CVTTQ",	0,	alphafp2,

	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"???",		0,	alphaxxx,
	"CVTQS",	0,	alphafp2,
	"???",		0,	alphaxxx,
	"CVTQT",	0,	alphafp2,
	"???",		0,	alphaxxx,
};

static uchar	amap[128] = {
	[0x00]	1,
	[0x40]	2,
	[0x20]	3,
	[0x60]	4,
	[0x09]	5,
	[0x49]	6,
	[0x29]	7,
	[0x69]	8,
	[0x2D]	9,
	[0x4D]	10,
	[0x6D]	11,
	[0x1D]	12,	
	[0x3D]	13,	
	[0x0F]	14,	
	[0x02]	15,	
	[0x0B]	16,	
	[0x12]	17,	
	[0x1B]	18,
	[0x22]	19,	
	[0x2B]	20,	
	[0x32]	21,	
	[0x3B]	22,
};

static Opcode arithopcodes[64] = {
	"???",		0,	alphaxxx,
	"ADDL",		0,	alphaint,
	"ADDL/V",	0,	alphaint,
	"ADDQ",		0,	alphaint,
	"ADDQ/V",	0,	alphaint,
	"SUBL",		0,	alphaint,
	"SUBL/V",	0,	alphaint,
	"SUBQ",		0,	alphaint,
	"SUBQ/V",	0,	alphaint,
	"CMPEQ",	0,	alphaint,
	"CMPLT",	0,	alphaint,
	"CMPLE",	0,	alphaint,
	"CMPULT",	0,	alphaint,
	"CMPULE",	0,	alphaint,
	"CMPBGE",	0,	alphaint,
	"S4ADDL",	0,	alphaint,
	"S4SUBL",	0,	alphaint,
	"S8ADDL",	0,	alphaint,
	"S8SUBL",	0,	alphaint,
	"S4ADDQ",	0,	alphaint,
	"S4SUBQ",	0,	alphaint,
	"S8ADDQ",	0,	alphaint,
	"S8SUBQ",	0,	alphaint,
};

static uchar	lmap[128] = {
	[0x00]	1,
	[0x20]	2,
	[0x40]	3,
	[0x08]	4,
	[0x28]	5,
	[0x48]	6,
	[0x24]	7,
	[0x44]	8,
	[0x64]	9,
	[0x26]	7,
	[0x46]	8,
	[0x66]	9,
	[0x14]	10,
	[0x16]	11,
};

static Opcode logicalopcodes[64] = {
	"???",		0,	alphaxxx,
	"AND",		0,	alphaint,
	"OR",		0,	alphaint,
	"XOR",		0,	alphaint,
	"ANDNOT",	0,	alphaint,
	"ORNOT",	0,	alphaint,
	"XORNOT",	0,	alphaint,
	"CMOVEQ",	0,	alphaint,
	"CMOVLT",	0,	alphaint,
	"CMOVLE",	0,	alphaint,
	"CMOVNE",	0,	alphaint,
	"CMOVGE",	0,	alphaint,
	"CMOVGT",	0,	alphaint,
	"CMOVLBS",	0,	alphaint,
	"CMOVLBC",	0,	alphaint,
};

static uchar	smap[128] = {
	[0x39]	1,
	[0x3C]	2,
	[0x34]	3,
	[0x06]	4,
	[0x16]	5,
	[0x26]	6,
	[0x36]	7,
	[0x5A]	8,
	[0x6A]	9,
	[0x7A]	10,
	[0x0B]	11,
	[0x1B]	12,
	[0x2B]	13,
	[0x3B]	14,
	[0x57]	15,
	[0x67]	16,
	[0x77]	17,
	[0x02]	18,
	[0x12]	19,
	[0x22]	20,
	[0x32]	21,
	[0x52]	22,
	[0x62]	23,
	[0x72]	24,
	[0x30]	25,
	[0x31]	26,
};

static Opcode shiftopcodes[64] = {
	"???",		0,	alphaxxx,
	"SLLQ",		0,	alphaint,
	"SRAQ",		0,	alphaint,
	"SRLQ",		0,	alphaint,
	"EXTBL",	0,	alphaint,
	"EXTWL",	0,	alphaint,
	"EXTLL",	0,	alphaint,
	"EXTQL",	0,	alphaint,
	"EXTWH",	0,	alphaint,
	"EXTLH",	0,	alphaint,
	"EXTQH",	0,	alphaint,
	"INSBL",	0,	alphaint,
	"INSWL",	0,	alphaint,
	"INSLL",	0,	alphaint,
	"INSQL",	0,	alphaint,
	"INSWH",	0,	alphaint,
	"INSLH",	0,	alphaint,
	"INSQH",	0,	alphaint,
	"MSKBL",	0,	alphaint,
	"MSKWL",	0,	alphaint,
	"MSKLL",	0,	alphaint,
	"MSKQL",	0,	alphaint,
	"MSKWH",	0,	alphaint,
	"MSKLH",	0,	alphaint,
	"MSKQH",	0,	alphaint,
	"ZAP",		0,	alphaint,
	"ZAPNOT",	0,	alphaint,
};

static void
format(char *mnemonic, Instr *i, char *f)
{
	if (mnemonic)
		format(0, i, mnemonic);
	if (f == 0)
		return;
	if (mnemonic)
		if (i->curr < i->end)
			*i->curr++ = '\t';
	for ( ; *f && i->curr < i->end; f++) {
		if (*f != '%') {
			*i->curr++ = *f;
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
			bprint(i, "%d", i->rc);
			break;

		case 'v':
			if (i->islit)
				bprint(i, "$%ux", i->literal);
			else
				bprint(i, "R%d", i->rb);
			break;

		case 'l':
			bprint(i, "%lx(R%d)", i->mem, i->rb);
			break;

		case 'i':
			bprint(i, "$%lx", i->mem);
			break;

		case 'B':
			i->curr += symoff(i->curr, i->end-i->curr,
				(i->branch<<2)+i->addr+4, CANY);
			break;

		case 'w':
			bprint(i, "[%lux]", i->w0);
			break;

		case '\0':
			*i->curr++ = '%';
			return;

		default:
			bprint(i, "%%%c", *f);
			break;
		}
	}
	*i->curr = 0;
}

static int
printins(Map *map, uvlong pc, char *buf, int n)
{
	Instr i;
	Opcode *o;
	uchar op;

	i.curr = buf;
	i.end = buf+n-1;
	mymap = map;
	if (mkinstr(pc, &i) < 0)
		return -1;
	switch (i.op) {

	case 0x10:					/* INTA */
		o = arithopcodes;
		op = amap[i.function];
		break;

	case 0x11:					/* INTL */
		o = logicalopcodes;
		op = lmap[i.function];
		break;

	case 0x12:					/* INTS */
		o = shiftopcodes;
		op = smap[i.function];
		break;

	case 0x16:					/* FLTI */
		o = ieeeopcodes;
		op = i.fpfn;
		break;

	case 0x17:					/* FLTL */
		o = fpopcodes;
		op = i.fpfn;
		break;

	default:
		o = opcodes;
		op = i.op;
		break;
	}
	if (o[op].f)
		(*o[op].f)(&o[op], &i);
	else
		format(o[op].mnemonic, &i, o[op].ken);
	return i.size*4;
}

static int
alphainst(Map *map, uvlong pc, char modifier, char *buf, int n)
{
	USED(modifier);
	return printins(map, pc, buf, n);
}

static int
alphadas(Map *map, uvlong pc, char *buf, int n)
{
	Instr i;

	i.curr = buf;
	i.end = buf+n;
	mymap = map;
	if (mkinstr(pc, &i) < 0)
		return -1;
	if (i.end-i.curr > 8)
		i.curr = _hexify(buf, i.w0, 7);
	if (i.size == 2 && i.end-i.curr > 9) {
		*i.curr++ = ' ';
		i.curr = _hexify(i.curr, i.w1, 7);
	}
	*i.curr = 0;
	return i.size*4;
}

static int
alphainstlen(Map *map, uvlong pc)
{
	Instr i;

	mymap = map;
	if (mkinstr(pc, &i) < 0)
		return -1;
	return i.size*4;
}

static int
alphafoll(Map *map, uvlong pc, Rgetter rget, uvlong *foll)
{
	char buf[8];
	Instr i;

	mymap = map;
	if (mkinstr(pc, &i) < 0)
		return -1;

	switch(i.op) {
	case 0x1A:				/* JMP/JSR/RET */
		sprint(buf, "R%d", i.rb);
		foll[0] = (*rget)(map, buf);
		return 1;
	case 0x30:				/* BR */
	case 0x34:				/* BSR */
		foll[0] = pc+4 + (i.branch<<2);
		return 1;
	default:
		if (i.op > 0x30) {		/* cond */
			foll[0] = pc+4;
			foll[1] = pc+4 + (i.branch<<2);
			return 2;
		}
		foll[0] = pc+i.size*4;
		return 1;
	}
}
