#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>
/*
 * amd29k-specific debugger interface
 */

static	int	a29000inst(Map*, ulong, char, char*, int);
static	int	a29000das(Map*, ulong, char*, int);
static	int	a29000instlen(Map*, ulong);
/*
 *	Debugger interface
 */
Machdata a29000mach =
{
	{0, 0, 0, 0},		/* break point */
	4,			/* break point size */

	beswab,			/* short to local byte order */
	beswal,			/* long to local byte order */
	beswav,			/* vlong to local byte order */
	risctrace,		/* C traceback */
	riscframe,		/* Frame finder */
	0,			/* print exception */
	0,			/* breakpoint fixup */
	beieeesftos,		/* single precision float printer */
	beieeedftos,		/* double precisioin float printer */
	0,			/* following addresses */
	a29000inst,		/* print instruction */
	a29000das,			/* dissembler */
	a29000instlen,		/* instruction size */
};

	/* mips disassembler and related functions */

static	char FRAMENAME[] = ".frame";

	/* amd 29k native disassembler */

typedef struct {
	long addr;			/* pc of instr */
	uchar op;			/* bits 31-24 */
	uchar rc;			/* bits 23-16 */
	uchar ra;			/* bits 15-8 */
	uchar rb;			/* bits 0-7 */
	ushort imm;			/* bits 23-16 and bits 0-7 */
	long w0;
	char *curr;			/* current fill point */
	char *end;			/* end of buffer */
	char *err;
} Instr;

typedef struct {
	char *mnemonic;
	char *fmt;
} Opcode;

static char amdunk[] = "%w";
static char amdalu2op[] = "r%c,r%a";
static char amdaluasop[] = "$%c,r%a,r%b";
static char amdaluasopi[] = "$%c,r%a,%s";
static char amdalu3op[] = "r%c,r%a,r%b";
static char amdalu3opi[] = "r%c,r%a,%s";
static char amdjs[] = "r%a,%j";
static char amdju[] = "r%a,%J";
static char amdji[] = "r%a,r%b";
static char amdmem[] = "%m,r%a,r%b";
static char amdmemi[] = "%m,r%a,%s";

static char *memacc[8] = { "L", "B", "H", "HW" };

static Opcode opcodes[256] = {
	0,		0,
	"constn",	"r%a,%i",
	"consth",	"r%a,%I0000",
	"const",	"r%a,%I",
	"mtsrim",	"s%a,%I",
	0,		0,
	"loadl",	amdmem,
	"loadl",	amdmemi,
	"clz",		amdunk,
	"clz",		amdunk,
	"exbyte",	amdalu3op,
	"exbyte",	amdalu3opi,
	"inbyte",	amdalu3op,
	"inbyte",	amdalu3opi,
	"storel",	amdmem,
	"storel",	amdmemi,
	"adds",		amdalu3op,
	"adds",		amdalu3opi,
	"addu",		amdalu3op,
	"addu",		amdalu3opi,
	"add",		amdalu3op,
	"add",		amdalu3opi,
	"load",		amdmem,
	"load",		amdmemi,
	"addcs",	amdalu3op,
	"addcs",	amdalu3opi,
	"addcu",	amdalu3op,
	"addcu",	amdalu3opi,
	"addc",		amdalu3op,
	"addc",		amdalu3opi,
	"store",	amdmem,
	"store",	amdmemi,
	"subs",		amdalu3op,
	"subs",		amdalu3opi,
	"subu",		amdalu3op,
	"subu",		amdalu3opi,
	"sub",		amdalu3op,
	"sub",		amdalu3opi,
	"loadset",	amdmem,
	"loadset",	amdmemi,
	"subcs",	amdalu3op,
	"subcs",	amdalu3opi,
	"subcu",	amdalu3op,
	"subcu",	amdalu3opi,
	"subc",		amdalu3op,
	"subc",		amdalu3opi,
	"cpbyte",	amdalu3op,
	"cpbyte",	amdalu3opi,
	"subrs",	amdalu3op,
	"subrs",	amdalu3opi,
	"subru",	amdalu3op,
	"subru",	amdalu3opi,
	"subr",		amdalu3op,
	"subr",		amdalu3opi,
	"loadm",	amdmem,
	"loadm",	amdmemi,
	"subrcs",	amdalu3op,
	"subrcs",	amdalu3opi,
	"subrcu",	amdalu3op,
	"subrcu",	amdalu3opi,
	"subrc",	amdalu3op,
	"subrc",	amdalu3opi,
	"storem",	amdmem,
	"storem",	amdmemi,
	"cplt",		amdalu3op,
	"cplt",		amdalu3opi,
	"cpltu",	amdalu3op,
	"cpltu",	amdalu3opi,
	"cple",		amdalu3op,
	"cple",		amdalu3opi,
	"cpleu",	amdalu3op,
	"cpleu",	amdalu3opi,
	"cpgt",		amdalu3op,
	"cpgt",		amdalu3opi,
	"cpgtu",	amdalu3op,
	"cpgtu",	amdalu3opi,
	"cpge",		amdalu3op,
	"cpge",		amdalu3opi,
	"cpgeu",	amdalu3op,
	"cpgeu",	amdalu3opi,
	"aslt",		amdaluasop,
	"aslt",		amdaluasopi,
	"asltu",	amdaluasop,
	"asltu",	amdaluasopi,
	"asle",		amdaluasop,
	"asle",		amdaluasopi,
	"asleu",	amdaluasop,
	"asleu",	amdaluasopi,
	"asgt",		amdaluasop,
	"asgt",		amdaluasopi,
	"asgtu",	amdaluasop,
	"asgtu",	amdaluasopi,
	"asge",		amdaluasop,
	"asge",		amdaluasopi,
	"asgeu",	amdaluasop,
	"asgeu",	amdaluasopi,
	"cpeq",		amdalu3op,
	"cpeq",		amdalu3opi,
	"cpneq",	amdalu3op,
	"cpneq",	amdalu3opi,
	"mul",		amdalu3op,
	"mul",		amdalu3opi,
	"mull",		amdalu3op,
	"mull",		amdalu3opi,
	"div0",		amdalu3op,
	"div0",		amdalu3opi,
	"div",		amdalu3op,
	"div",		amdalu3opi,
	"divl",		amdalu3op,
	"divl",		amdalu3opi,
	"divrem",	amdalu3op,
	"divrem",	amdalu3opi,
	"aseq",		amdaluasop,
	"aseq",		amdaluasopi,
	"asneq",	amdaluasop,
	"asneq",	amdaluasopi,
	"mulu",		amdalu3op,
	"mulu",		amdalu3opi,
	0,		0,
	0,		0,
	"inhw",		amdalu3op,
	"inhw",		amdalu3opi,
	"extract",	amdalu3op,
	"extract",	amdalu3opi,
	"exhw",		amdalu3op,
	"exhw",		amdalu3opi,
	"exhws",	amdalu2op,
	0,		0,
	"sll",		amdalu3op,
	"sll",		amdalu3opi,
	"srl",		amdalu3op,
	"srl",		amdalu3opi,
	0,		0,
	0,		0,
	"sra",		amdalu3op,
	"sra",		amdalu3opi,
	"iret",		amdunk,
	"halt",		amdunk,
	0,		0,
	0,		0,
	"iretinv",	amdunk,
	0,		0,
	0,		0,
	0,		0,
	"and",		amdalu3op,
	"and",		amdalu3opi,
	"or",		amdalu3op,
	"or",		amdalu3opi,
	"xor",		amdalu3op,
	"xor",		amdalu3opi,
	"xnor",		amdalu3op,
	"xnor",		amdalu3opi,
	"nor",		amdalu3op,
	"nor",		amdalu3opi,
	"nand",		amdalu3op,
	"nand",		amdalu3opi,
	"andn",		amdalu3op,
	"andn",		amdalu3opi,
	"setip",	amdalu3op,
	"inv",		amdunk,
	"jmp",		"%j",
	"jmp",		"%J",
	0,		0,
	0,		0,
	"jmpf",		amdjs,
	"jmpf",		amdju,
	0,		0,
	0,		0,
	"call",		amdjs,
	"call",		amdju,
	0,		0,
	0,		0,
	"jmpt",		amdjs,
	"jmpt",		amdju,
	0,		0,
	0,		0,
	0,		0,
	0,		0,
	0,		0,
	0,		0,
	"jmpfdec",	amdunk,
	"jmpfdec",	amdunk,
	"mftlb",	"r%c,r%a",
	0,		0,
	0,		0,
	0,		0,
	0,		0,
	0,		0,
	0,		0,
	0,		0,
	"mttlb",	"r%a,r%b",
	0,		0,
	"jmpi",		"r%b",
	0,		0,
	0,		0,
	0,		0,
	"jmpfi",	amdji,
	0,		0,
	"mfsr",		"r%c,s%a",
	0,		0,
	"calli",	amdji,
	0,		0,
	0,		0,
	0,		0,
	"jmpti",	amdji,
	0,		0,
	"mtsr",		"s%a,r%b",
	0,		0,
	0,		0,
	0,		0,
	0,		0,
	0,		0,
	0,		0,
	0,		0,
	0,		0,
	"emulate",	amdunk,
	0,		0,		/* D8: vector 24 */
	0,		0,
	0,		0,
	0,		0,
	0,		0,
	0,		0,		/* DD: vector 29 */
	"multm",	amdalu3op,
	"multmu",	amdalu3op,
	"multiply",	amdalu3op,
	"divide",	amdalu3op,
	"multiplu",	amdalu3op,
	"dividu",	amdalu3op,
	"convert",	amdunk,
	"sqrt",		amdunk,
	"class",	amdunk,
	0,		0,		/* E7: vector 39 */
	0,		0,
	0,		0,		/* E9: vector 41 */
	"feq",		amdunk,
	"deq",		amdunk,
	"fgt",		amdunk,
	"dgt",		amdunk,
	"fge",		amdunk,
	"dge",		amdunk,
	"fadd",		amdunk,
	"dadd",		amdunk,
	"fsub",		amdunk,
	"dsub",		amdunk,
	"fmul",		amdunk,
	"dmul",		amdunk,
	"fdiv",		amdunk,
	"ddiv",		amdunk,
	0,		0,		/* F8: vector 56 */
	"fdmul",	amdunk,
	0,		0,		/* FA: vector 58 */
	0,		0,
	0,		0,
	0,		0,
	0,		0,
	0,		0,		/* FF: vector 63 */
};


static int
mkinstr(Instr *i, ulong pc, ulong w)
{
	i->addr = pc;
	i->op = (w >> 24) & 0xFF;
	i->rc = (w >> 16) & 0xFF;
	i->ra = (w >> 8) & 0xFF;
	i->rb = (w >> 0) & 0xFF;
	i->imm = ((w >> 8) & 0xFF00) | (w & 0xFF);
	i->w0 = w;
	return 1;
}

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
	char *s;
	int a;

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
		case 'm':
			s = memacc[i->rc & 7];
			if(s == 0 || i->rc & (7<<6)){
				bprint(i, "?0x%.2x", i->rc);
				break;
			}
			if(i->rc & (1<<5))
				bprint(i, "%c", 'P');
			if(i->rc & (1<<4))
				bprint(i, "%c", 'S');
			if(i->rc & (1<<3))
				bprint(i, "%c", 'U');
			bprint(i, "%s", s);
			break;
		case 'a':
			bprint(i, "%d", i->ra);
			break;

		case 'b':
			bprint(i, "%d", i->rb);
			break;

		case 'c':
			bprint(i, "%d", i->rc);
			break;

		case 's':
			bprint(i, "$%.2ux", i->rb);
			break;

		case 'i':
			bprint(i, "$%.4ux", i->imm | ~0xFFFF);
			break;

		case 'I':
			bprint(i, "$%.4ux", i->imm);
			break;

		case 'j':
			a = i->imm;
			if(a & 0x8000)
				a |= ~0xFFFF;
			i->curr += symoff(i->curr, i->end-i->curr, i->addr+(a<<2), CANY);
			bprint(i, "(SB)");
			break;

		case 'J':
			a = i->imm;
			i->curr += symoff(i->curr, i->end-i->curr, a<<2, CANY);
			bprint(i, "(SB)");
			break;

		case 'w':
			bprint(i, "[%.8lux]", i->w0);
			break;

		case '\0':
			bprint(i, "%");
			return;

		default:
			bprint(i, "%%%c", *f);
			break;
		}
	}
}

/*
 *	used by 9i
 */

int
_a29000disinst(ulong pc, ulong w, char *buf, int n)
{
	Instr i;
	Opcode *o;

	i.curr = buf;
	i.end = buf+n-1;
	mkinstr(&i, pc, w);
	o = &opcodes[i.op];
	format(o->mnemonic, &i, o->fmt);
	*i.curr = '\0';
	return 4;
}

static int
a29000inst(Map *map, ulong pc, char, char *buf, int n)
{
	Instr i;
	Opcode *o;
	long w;

	if (get4(map, pc, &w) < 0) {
		werrstr("can't read instruction: %r");
		return -1;
	}
	i.curr = buf;
	i.end = buf+n-1;
	mkinstr(&i, pc, w);
	o = &opcodes[i.op];
	format(o->mnemonic, &i, o->fmt);
	return 4;
}

static int
a29000das(Map *map, ulong pc, char *buf, int n)
{
	Instr i;
	long w;

	if (get4(map, pc, &w) < 0) {
		werrstr("can't read instruction: %r");
		return -1;
	}

	i.curr = buf;
	i.end = buf+n;
	if (i.end-i.curr > 8)
		i.curr = _hexify(buf, w, 7);
	*i.curr = 0;
	return 4;
}

static int
a29000instlen(Map*, ulong)
{
	return 4;
}
