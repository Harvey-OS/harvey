#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>

/*
 * i960-specific debugger interface
 * 	i960 has an as yet unknown stack format - presotto
 */

static	char	*i960excep(Map*, Rgetter);
static	int	i960inst(Map*, ulong, char, char*, int);
static	int	i960das(Map*, ulong, char*, int);
static	int	i960foll(Map*, ulong, Rgetter, ulong*);
static	int	i960instlen(Map*, ulong);

Machdata i960mach =
{
	{0x66, 0, 0x06, 0},	/* break point: fmark */
	4,			/* break point size */

	leswab,			/* convert short to local byte order */
	leswal,			/* convert long to local byte order */
	leswav,			/* vlong to local byte order */
	cisctrace,		/* C traceback */
	ciscframe,		/* frame finder */
	i960excep,		/* print exception */
	0,			/* breakpoint fixup */
	leieeesftos,		/* single precision float printer */
	leieeedftos,		/* double precision float printer */
	i960foll,		/* following addresses */
	i960inst,		/* print instruction */
	i960das,		/* dissembler */
	i960instlen,		/* 0 */
};

static char*
i960excep(Map *map, Rgetter rget)
{
	USED(map, rget);
	return "intel machine exceptions are too hard to decode";
}

	/* I960 disassembler and related functions */

typedef struct Instr	Instr;
typedef struct Opcode	Opcode;

/*
 * operand types
 */
enum
{
	EA=1,
	R1, R2, R3,
	S1, S2, S3,
	IR1, IR2, IR3,
};

enum
{
	Treg,
	Tctrl,
	Tcobr,
	Tmem,
};


struct Opcode
{
	ushort	code;
	char	*name;
	uchar	and[3];
};

struct Instr
{
	Opcode	*op;		/* opcode line */
	ulong	mem[2];		/* memory for this instruction */
	int	n;		/* # longs for this instruction */
	char	and[3][64];
	ulong	rel;
	char	*err;

	int	type;
	int	index;
	int	base;
	int	mult;
	char	*curr;		/* fill point in output buffer */
	char	*end;		/* end of output buffer */
	ulong	addr;		/* pc of instruction */
};

static	Map* mymap;

Opcode optab[] = {
	{ 0x08, "B", EA		},
	{ 0x09, "CALL", EA	},
	{ 0x0A, "RET"		},
	{ 0x0B, "BAL", EA	},
	{ 0x10, "BNO", EA	},
	{ 0x11, "BG", EA	},
	{ 0x12, "BE", EA	},
	{ 0x13, "BGE", EA	},
	{ 0x14, "BL", EA	},
	{ 0x15, "BNE", EA	},
	{ 0x16, "BLE", EA	},
	{ 0x17, "BO", EA	},
	{ 0x18, "FAULTNO"	},
	{ 0x19, "FAULTG"	},
	{ 0x1A, "FAULTE"	},
	{ 0x1B, "FAULTGE"	},
	{ 0x1C, "FAULTL"	},
	{ 0x1D, "FAULTNE"	},
	{ 0x1E, "FAULTLE"	},
	{ 0x1F, "FAULTO"	},
	{ 0x20, "TESTNO", R1	},
	{ 0x21, "TESTG", R1	},
	{ 0x22, "TESTE", R1	},
	{ 0x23, "TESTGE", R1	},
	{ 0x24, "TESTL", R1	},
	{ 0x25, "TESTNE", R1	},
	{ 0x26, "TESTLE", R1	},
	{ 0x27, "TESTO", R1	},
	{ 0x30, "BBC", S1, R2, EA	},
	{ 0x31, "CMPOBG", S1, R2, EA	},
	{ 0x32, "CMPOBE", S1, R2, EA	},
	{ 0x33, "CMPOBGE", S1, R2, EA	},
	{ 0x34, "CMPOBL", S1, R2, EA	},
	{ 0x35, "CMPOBNE", S1, R2, EA	},
	{ 0x36, "CMPOBLE", S1, R2, EA	},
	{ 0x37, "CMPOBO", S1, R2, EA	},
	{ 0x38, "CMPIBNO", S1, R2, EA	},
	{ 0x39, "CMPIBG", S1, R2, EA	},
	{ 0x3A, "CMPIBE", S1, R2, EA	},
	{ 0x3B, "CMPIBGE", S1, R2, EA	},
	{ 0x3C, "CMPIBL", S1, R2, EA	},
	{ 0x3D, "CMPIBNE", S1, R2, EA	},
	{ 0x3E, "CMPIBLE", S1, R2, EA	},
	{ 0x3F, "CMPIBO", S1, R2, EA	},
	{ 0x80, "MOVOB", EA, R1	},
	{ 0x82, "MOVOB", R1, EA	},
	{ 0x84, "B", EA		},
	{ 0x85, "BAL", EA, R1	},
	{ 0x86, "CALL", EA	},
	{ 0x88, "MOVOS", EA, R1	},
	{ 0x8A, "MOVOS", R1, EA	},
	{ 0x8C, "LDA", EA, R1	},
	{ 0x90, "MOV", EA, R1	},
	{ 0x92, "MOV", R1, EA	},
	{ 0x98, "MOVV", EA, R1	},
	{ 0x9A, "MOVV", R1, EA	},
	{ 0xA0, "MOVT", EA, R1	},
	{ 0xA2, "MOVT", R1, EA	},
	{ 0xB0, "MOVQ", EA, R1	},
	{ 0xB2, "MOVQ", R1, EA	},
	{ 0xC0, "MOVIB", EA, R1	},
	{ 0xC2, "MOVIB", R1, EA	},
	{ 0xC8, "MOVIS", EA, R1	},
	{ 0xCA, "MOVIS", R1, EA	},
	{ 0x580, "NOTBIT", S1, S2, R3	},
	{ 0x581, "AND", S1, S2, R3	},
	{ 0x582, "ANDNOT", S1, S2, R3	},
	{ 0x583, "SETBIT", S1, S2, R3	},
	{ 0x584, "NOTAND", S1, R3	},
	{ 0x586, "XOR", S1, S2, R3	},
	{ 0x587, "OR", S1, S2, R3	},
	{ 0x588, "NOR", S1, S2, R3	},
	{ 0x589, "XNOR", S1, S2, R3	},
	{ 0x58A, "NOT", S1, R3		},
	{ 0x58B, "ORNOT", S1, S2, R3	},
	{ 0x58C, "CLRBIT", S1, S2, R3	},
	{ 0x58D, "NOTOR", S1, S2, R3	},
	{ 0x58E, "NAND", S1, S2, R3	},
	{ 0x58F, "ALTERBIT", S1, S2, R3	},
	{ 0x590, "ADDO", S1, S2, R3	},
	{ 0x591, "ADDI", S1, S2, R3	},
	{ 0x592, "SUBO", S1, S2, R3	},
	{ 0x593, "SUBI", S1, S2, R3	},
	{ 0x598, "SHRO", S1, S2, R3	},
	{ 0x59A, "SHRDI", S1, S2, R3	},
	{ 0x59B, "SHRI", S1, S2, R3	},
	{ 0x59C, "SHLO", S1, S2, R3	},
	{ 0x59D, "ROTATE", S1, S2, R3	},
	{ 0x59E, "SHLI", S1, S2, R3	},
	{ 0x5A0, "CMPO", S1, S2		},
	{ 0x5A1, "CMPI", S1, S2		},
	{ 0x5A2, "CONCMPO", S1, S2	},
	{ 0x5A3, "CONCMPI", S1, S2	},
	{ 0x5A4, "CMPINCI", S1, S2, R3	},
	{ 0x5A5, "CMPINCO", S1, S2, R3	},
	{ 0x5A6, "CMPDECI", S1, S2, R3	},
	{ 0x5A7, "CMPDECO", S1, S2, R3	},
	{ 0x5AC, "SCANBYTE", S1, S2	},
	{ 0x5AE, "CHKBIT", S1, S2	},
	{ 0x5B0, "ADDC", S1, S2, R3	},
	{ 0x5B2, "SUBC", S1, S2, R3	},
	{ 0x5CC, "MOV", S1, R3		},
	{ 0x5DC, "MOVV", S1, R3		},
	{ 0x5EC, "MOVT", S1, R3		},
	{ 0x5FC, "MOVQ", S1, R3		},
	{ 0x600, "SYNMOV", IR1, IR3	},
	{ 0x601, "SYNMOVV", IR1, IR3	},
	{ 0x602, "SYNMOVQ", IR1, IR3	},
	{ 0x610, "ATMOD", IR1, S2, R3	},
	{ 0x612, "ATADD", IR1, S2, R3	},
	{ 0x615, "SYNMOV", IR1, R2	},
	{ 0x640, "SPANBIT", S1, R2	},
	{ 0x641, "SCANBIT", S1, R2	},
	{ 0x642, "DADDC", S1, S2, R3	},
	{ 0x643, "DSUBC", S1, S2, R3	},
	{ 0x644, "DMOVT", S1, R2	},
	{ 0x645, "MODAC", S1, S2, R3	},
	{ 0x650, "MODIFY", S1, S2, R3	},
	{ 0x651, "EXTRACT", S1, S2, R3	},
	{ 0x654, "MODTC", S1, S2, R3	},
	{ 0x655, "MODPC", S1, S2	},
	{ 0x660, "CALLS", S1		},
	{ 0x66B, "MARK"			},
	{ 0x66C, "FMARK"	},
	{ 0x66D, "FLUSHREG"	},
	{ 0x66F, "SYNCF"	},
	{ 0x670, "EMUL", S1, S2, R3	},
	{ 0x671, "EDIV", S1, S2, R3	},
	{ 0x674, "CVTIF", S1, R2	},
	{ 0x675, "CVTILF", S1, R2	},
	{ 0x676, "SCALED", S1, S2, R3	},
	{ 0x677, "SCALEF", S1, S2, R3	},
	{ 0x680, "ATANF", S1, S2, R3	},
	{ 0x681, "LOGEPF", S1, S2, R3	},
	{ 0x682, "LOGF", S1, S2, R3	},
	{ 0x683, "REMF", S1, S2, R3	},
	{ 0x684, "CMPOF", S1, S2	},
	{ 0x685, "CMPF", S1, S2	},
	{ 0x688, "SQRTF", S1, R2	},
	{ 0x689, "EXPF", S1, R2	},
	{ 0x68A, "LOGBNTF", S1, R2	},
	{ 0x68B, "ROUNDF", S1, R2	},
	{ 0x68C, "SINF", S1, R2	},
	{ 0x68D, "COSF", S1, R2	},
	{ 0x68E, "TANF", S1, R2	},
	{ 0x68F, "CLASSF", S1	},
	{ 0x690, "ATAND", S1, S2, R3	},
	{ 0x691, "LOGEPD", S1, S2, R3	},
	{ 0x692, "LOGD", S1, S2, R3	},
	{ 0x693, "REMD", S1, S2, R3	},
	{ 0x694, "CMPOD", S1, S2	},
	{ 0x695, "CMPD", S1, S2	},
	{ 0x698, "SQRTD", S1, S2	},
	{ 0x699, "EXPD", S1, S2	},
	{ 0x69A, "LOGBND", S1, S2	},
	{ 0x69B, "ROUNDD", S1, S2	},
	{ 0x69C, "SIND", S1, S2	},
	{ 0x69D, "COSD", S1, S2	},
	{ 0x69E, "TAND", S1, S2	},
	{ 0x69F, "CLASSD", S1	},
	{ 0x6C0, "CVTRI", S1, R2	},
	{ 0x6C1, "CVTRI", S1, R2	},
	{ 0x6C2, "CVTRI", S1, R2	},
	{ 0x6C3, "CVTRI", S1, R2	},
	{ 0x6C9, "MOVF", S1, R2	},
	{ 0x6D9, "MOVD", S1, R2	},
	{ 0x6E2, "CPYSRE", S1, S2, R3	},
	{ 0x6E3, "CPYRSRE", S1, S2, R3	},
	{ 0x6E3, "MOVRE", S1, R2	},
	{ 0x701, "MULO", S1, S2, R3	},
	{ 0x708, "REMO", S1, S2, R3	},
	{ 0x70B, "DIVO", S1, S2, R3	},
	{ 0x741, "MULI", S1, S2, R3	},
	{ 0x748, "REMI", S1, S2, R3	},
	{ 0x749, "MODI", S1, S2, R3	},
	{ 0x74B, "DIVI", S1, S2, R3	},
	{ 0x78B, "DIVF", S1, S2, R3	},
	{ 0x78C, "MULF", S1, S2, R3	},
	{ 0x78D, "SUBF", S1, S2, R3	},
	{ 0x78F, "ADDR", S1, S2, R3	},
	{ 0x79B, "DIVD", S1, S2, R3	},
	{ 0x79C, "MULD", S1, S2, R3	},
	{ 0x79D, "SUBD", S1, S2, R3	},
	{ 0x79F, "MODI", S1, S2, R3	},
	{ 0 },
};
#define NOP (sizeof(optab)/sizeof(Opcode))

/*
 *  get an instruction long
 */
static int
igetl(Instr *ip, long *lp)
{
	if(ip->n >= 2){
		werrstr("instruction too big");
		return -1;
	}
	if (get4(mymap, ip->addr+ip->n*4, (long*) &ip->mem[ip->n]) < 0) {
		werrstr("can't read instruction: %r");
		return -1;
	}
	*lp = ip->mem[ip->n++];
	return 1;
}

static Opcode*
findopcode(ushort code)
{
	Opcode *o;

	for(o = optab; o->code; o++)
		if(code == o->code)
			return o;
	werrstr("unknown opcode");
	return 0;
}

static char*
genname(ulong l, int reg, int mode, int regonly, int ind)
{
	static char name[32];

	reg = (l>>reg) & 0x1f;
	if(mode >= 0)
		mode = (l>>mode) & 1;
	else
		mode = 0;
	if(mode)
		sprint(name, regonly?"?$%d?":"$%d", reg);
	else
		sprint(name, ind?"(R%d)":"R%d", reg);
	return name;
}

/*
 *  decode a reg instruction
 */
static char*
regfield(Instr *ip, uchar and, ulong l)
{
	switch(and){
	case S1:
	case R1:
	case IR1:
		return genname(l, 0, 11, and!=S1, and==IR1);
	case S2:
	case R2:
	case IR2:
		return genname(l, 14, 12, and!=S2, and==IR2);
	case S3:
	case R3:
	case IR3:
		return genname(l, 19, 13, and!=S3, and==IR3);
	default:
		ip->err = "missing regfield";
		return("???");
	}
}

static void
chk56(Instr *ip, ulong l)
{
	if(l & (3<<5))
		ip->err = "bits 5 and 6 not 0";
}

static int
reginst(Instr *ip, ulong l)
{
	ushort code;
	int i;

	ip->type = Treg;
	code = ((l>>20)&0xff0) | ((l>>7)&0xf);
	ip->op = findopcode(code);
	if(ip->op == 0)
		return -1;
	for(i = 0; i < 3; i++){
		if(ip->op->and[i] == 0)
			break;
		strcpy(ip->and[i], regfield(ip, ip->op->and[i], l));
	}
	return 0;
}

/*
 *  decode a control and branch instruction
 */
static char*
cobrfield(Instr *ip, uchar and, ulong l)
{
	long disp;

	switch(and){
	case S1:
	case R1:
	case IR1:
		return genname(l, 19, 13, and!=S1, and==IR1);
	case R2:
	case IR2:
		return genname(l, 14, -1, 1, and==IR2);
	case EA:
		if(l & (1<<12))
			disp = -(((~l) & 0xfff)+1);
		else
			disp = l & 0xfff;
		ip->rel = ip->addr + 4 + disp;
		return ".+";
	default:
		return "???";
	}
}
static int
cobrinst(Instr *ip, ulong l)
{
	ushort code;
	int i;

	ip->type = Tcobr;
	if(l&3)
		ip->err = "disp not mult of 4";
	code = l>>24;
	ip->op = findopcode(code);
	if(ip->op == 0)
		return -1;
	for(i = 0; i < 3; i++){
		if(ip->op->and[i] == 0)
			break;
		strcpy(ip->and[i], cobrfield(ip, ip->op->and[i], l));
	}
	return 0;
}

/*
 *  decode a control instruction
 */
static int
ctrlinst(Instr *ip, ulong l)
{
	ushort code;
	long disp;

	ip->type = Tctrl;
	if(l&3)
		ip->err = "disp not mult of 4";
	code = l>>24;
	ip->op = findopcode(code);
	if(ip->op == 0)
		return -1;
	if(ip->op->and[0] == 0)
		return 0;
	if(l & (1<<23))
		disp = -(((~l) & 0x7fffff)+1);
	else
		disp = l & 0x7fffff;
	ip->rel = ip->addr + 4 + disp;
	strcpy(ip->and[0], ".+");
	return 0;
}

static int
sprintoff(char *addr, int n, char *fmt, ulong v)
{
	Symbol s;
	long w;

	w = v+mach->sb;
	if (findsym(w, CDATA, &s)) {
		w = s.value-w;
		if (w <= 4096) {
			if (w)
				return snprint(addr, n, "%s+%lux(SB)", s.name, w);
			else
				return snprint(addr, n, "%s(SB)", s.name);
		}
	}
	return snprint(addr, n, fmt, v);
}

/*
 *  decode a memory instruction
 */
static char*
memfield(Instr *ip, uchar and, ulong l)
{
	long disp;
	static char addr[64];
	int abase;
	int scale;
	int index;

	ip->mult = 1;
	switch(and){
	case R1:
	case IR1:
		return genname(l, 19, -1, 1, and==IR1);
	case EA:
		abase = (l>>14)&0x1f;
		scale = 1 << ((l>>7)&0x7);
		index = l & 0x1f;
		switch((l>>10)&0xf){
		case 0x4:
			chk56(ip, l);
			ip->base = abase + 1;
			sprint(addr, "(R%d)", abase);
			break;
		case 0x5:
			chk56(ip, l);
			if (igetl(ip, &disp) < 0) {
				strcpy(addr, "???");
				break;
			}
			ip->rel = disp + ip->addr + 8;
			strcpy(addr, ".+");
			break;
		case 0x6:
			strcpy(addr, "???");
			break;
		case 0x7:
			chk56(ip, l);
			ip->base = abase + 1;
			ip->mult = scale;
			ip->index = index + 1;
			sprint(addr, "(R%d)(%d*R%d)", abase, scale, index);
			break;
		case 0xc:
			chk56(ip, l);
			if (igetl(ip, &disp) < 0) {
				strcpy(addr, "???");
				break;
			}
			ip->rel = disp;
			strcpy(addr, ".+");
			break;
		case 0xd:
			chk56(ip, l);
			if (igetl(ip, &disp) < 0) {
				strcpy(addr, "???");
				break;
			}
			ip->rel = disp;
			if(abase == 28)
				sprintoff(addr, sizeof(addr), "%lux(R28)", disp);
			else{
				ip->base = abase + 1;
				sprint(addr, "%lx(R%d)", disp, abase);
			}
			break;
		case 0xe:
			chk56(ip, l);
			if (igetl(ip, &disp) < 0) {
				strcpy(addr, "???");
				break;
			}
			ip->rel = disp;
			ip->mult = scale;
			ip->index = index + 1;
			sprint(addr, "%lx(%d*R%d)", disp, scale, index);
			break;
		case 0xf:
			chk56(ip, l);
			if (igetl(ip, &disp) < 0) {
				strcpy(addr, "???");
				break;
			}
			ip->rel = disp;
			ip->mult = scale;
			ip->index = index + 1;
			ip->base = abase + 1;
			sprint(addr, "%lx(R%d)(%d*R%d)", disp, abase,
				scale, index);
			break;
		default:
			disp = l&0xfff;
			ip->rel = disp;
			if(l & (1<<13)){
				if(abase == 28)
					sprintoff(addr, sizeof(addr), "%lux(R28)", disp);
				else{
					ip->base = abase + 1;
					sprint(addr, "%lux(R%d)", disp, abase);
				}
			} else {
				strcpy(addr, ".+");
			}
			break;
		}
		return addr;
	default:
		break;
	}
	return "???";
}
static int
meminst(Instr *ip, ulong l)
{
	ushort code;
	int i;

	ip->type = Tmem;
	code = l>>24;
	ip->op = findopcode(code);
	if(ip->op == 0)
		return -1;
	for(i = 0; i < 2; i++){
		if(ip->op->and[i] == 0)
			break;
		strcpy(ip->and[i], memfield(ip, ip->op->and[i], l));
	}
	return 0;
}

static int
mkinstr(Instr *ip, ulong pc)
{
	long l;

	memset(ip, 0, sizeof(Instr));
	ip->addr = pc;
	if (igetl(ip, &l) < 0)
		return -1;
	switch(l>>28){
	case 0x0:
	case 0x1:
		return ctrlinst(ip, l);
	case 0x2:
	case 0x3:
		return cobrinst(ip, l);
	case 0x5:
	case 0x6:
	case 0x7:
		return reginst(ip, l);
	case 0x8:
	case 0x9:
	case 0xa:
	case 0xb:
	case 0xc:
		return meminst(ip, l);
	}
	werrstr("unknown opcode");
	return -1;
}

static int
i960inst(Map *map, ulong pc, char modifier, char *buf, int n)
{
	Instr instr;
	int i;
	char *and;
	char *end;

	USED(modifier);
	mymap = map;
	if(mkinstr(&instr, pc) < 0)
		return -1;

	end = buf+n-1;
	buf += snprint(buf, end-buf, "%s ", instr.op->name);
	for(i = 0; i < 3; i++){
		and = instr.and[i];
		if(*and == 0)
			break;
		if(i != 0)
			buf += snprint(buf, end-buf, ",");
		if(strcmp(and, ".+") == 0){
			buf += sprintoff(buf, end-buf, "$%lux", instr.rel);
		} else
			buf += snprint(buf, end-buf, "%s", and);
	}

	if(instr.err)
		snprint(buf, end-buf, "\t\t;%s", instr.err);

	return instr.n*4;
}

static int
i960das(Map *map, ulong pc, char *buf, int n)
{
	Instr	instr;

	mymap = map;
	if (mkinstr(&instr, pc) < 0)
		return -1;
	if (n > 8) {
		_hexify(buf, instr.mem[0], 7);
		n -= 8;
		buf += 8;
	}
	if (n > 9 && instr.n == 2) {
		*buf++ = ' ';
		_hexify(buf, instr.mem[1], 7);
		buf += 8;
	}
	*buf = 0;
	return instr.n*4;
}

static int
i960instlen(Map *map, ulong pc)
{
	Instr	instr;

	mymap = map;
	if (mkinstr(&instr, pc) < 0)
		return -1;
	return instr.n*4;
}

static int
i960foll(Map *map, ulong pc, Rgetter rget, ulong *foll)
{
	Instr instr;
	ulong l;
	char buf[8];

	mymap = map;
	if (mkinstr(&instr, pc) < 0)
		return -1;
	foll[0] = pc + instr.n;
	switch(instr.type){
	case Tcobr:
	case Treg:
		break;
	case Tctrl:
		if(instr.op->name[0] == 'B'){
			foll[1] = instr.rel;
			return 2;
		}
		if(strcmp("CALL", instr.op->name) == 0){
			foll[0] = instr.rel;
			return 1;
		}
		if(strcmp("RET", instr.op->name) == 0)
			return -1;
		break;
	case Tmem:
		if(strcmp("BAL", instr.op->name) == 0
		|| strcmp("CALL", instr.op->name) == 0){
			l = instr.rel;
			if(instr.index){
				sprint(buf, "R%d", instr.index - 1);
				l += (*rget)(map, buf) * instr.mult;
			}
			if(instr.base){
				sprint(buf, "R%d", instr.base - 1);
				l += (*rget)(map, buf);
			}
			foll[0] = l;
			return 1;
		}
		break;
	}
	return 1;
}
