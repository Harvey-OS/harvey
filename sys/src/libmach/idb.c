#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>
#include "../cmd/ic/i.out.h"

static char *riscvexcep(Map*, Rgetter);

/*
 * RISCV-specific debugger interface
 */

typedef struct	Instr	Instr;
struct	Instr
{
	Map	*map;
	ulong	w;
	uvlong	addr;
	char *fmt;
	int n;
	int	op;
	int aop;
	int	func3;
	int	func7;
	char	rs1, rs2, rs3, rd;
	char	rv64;
	long	imm;

	char*	curr;			/* fill point in buffer */
	char*	end;			/* end of buffer */
};

typedef struct Optab	Optab;
struct Optab {
	int	func7;
	int	op[8];
};
		
typedef struct Opclass	Opclass;
struct Opclass {
	char	*fmt;
	Optab	tab[4];
};

/* Major opcodes */
enum {
	OLOAD,	 OLOAD_FP,  Ocustom_0,	OMISC_MEM, OOP_IMM, OAUIPC, OOP_IMM_32,	O48b,
	OSTORE,	 OSTORE_FP, Ocustom_1,	OAMO,	   OOP,	    OLUI,   OOP_32,	O64b,
	OMADD,	 OMSUB,	    ONMSUB,	ONMADD,	   OOP_FP,  Ores_0, Ocustom_2,	O48b_2,
	OBRANCH, OJALR,	    Ores_1,	OJAL,	   OSYSTEM, Ores_2, Ocustom_3,	O80b
};

/* copy anames from compiler */
static
#include "../cmd/ic/enam.c"

static Opclass opOLOAD = {
	"a,d",
	0,	AMOVB,	AMOVH,	AMOVW,	AMOV,	AMOVBU,	AMOVHU,	AMOVWU,	0,
};
static Opclass opOLOAD_FP = {
	"a,fd",
	0,	0,	0,	AMOVF,	AMOVD,	0,	0,	0,	0,
};
static Opclass opOMISC_MEM = {
	"",
	0,	AFENCE,	AFENCE_I,0,	0,	0,	0,	0,	0,
};
static Opclass opOOP_IMM = {
	"$i,s,d",
	0x20,	0,	0,	0,	0,	0,	ASRA,	0,	0,
	0,	AADD,	ASLL,	ASLT,	ASLTU,	AXOR,	ASRL,	AOR,	AAND,
};
static Opclass opOAUIPC = {
	"$i(PC),d",
	0,	ALUI,	ALUI,	ALUI,	ALUI,	ALUI,	ALUI,	ALUI,	ALUI,
};
static Opclass opOOP_IMM_32 = {
	"$i,s,d",
	0x20,	0,	0,	0,	0,	0,	ASRAW,	0,	0,
	0,	AADDW,	ASLLW,	0,	0,	0,	ASRLW,	0,	0,
};
static Opclass opOSTORE = {
	"2,a",
	0,	AMOVB,	AMOVH,	AMOVW,	AMOV,	0,	0,	0,	0,
};
static Opclass opOSTORE_FP = {
	"f2,a",
	0,	0,	0,	AMOVF,	AMOVD,	0,	0,	0,	0,
};
static Opclass opOAMO = {
	"7,2,s,d",
	0x04,	0,	0,	ASWAP_W,ASWAP_D,0,	0,	0,	0,
	0x08,	0,	0,	ALR_W,	ALR_D,	0,	0,	0,	0,
	0x0C,	0,	0,	ASC_W,	ASC_D,	0,	0,	0,	0,
	0,	0,	0,	AAMO_W,	AAMO_D,	0,	0,	0,	0,
};
static Opclass opOOP = {
	"2,s,d",
	0x01,	AMUL,	AMULH,	AMULHSU,AMULHU,	ADIV,	ADIVU,	AREM,	AREMU,
	0x20,	ASUB,	0,	0,	0,	0,	ASRA,	0,	0,
	0,	AADD,	ASLL,	ASLT,	ASLTU,	AXOR,	ASRL,	AOR,	AAND,
};
static Opclass opOLUI = {
	"$i,d",
	0,	ALUI,	ALUI,	ALUI,	ALUI,	ALUI,	ALUI,	ALUI,	ALUI,
};
static Opclass opOOP_32 = {
	"2,s,d",
	0x01,	AMULW,	0,	0,	0,	ADIVW,	ADIVUW,	AREMW,	AREMUW,
	0x20,	ASUBW,	0,	0,	0,	0,	ASRAW,	0,	0,
	0,	AADDW,	ASLLW,	0,	0,	0,	ASRLW,	0,	0,
};
static Opclass opOBRANCH = {
	"2,s,p",
	0,	ABEQ,	ABNE,	0,	0,	ABLT,	ABGE,	ABLTU,	ABGEU,
};
static Opclass opOJALR = {
	"d,a",
	0,	AJALR,	AJALR,	AJALR,	AJALR,	AJALR,	AJALR,	AJALR,	AJALR,
};
static Opclass opOJAL = {
	"d,p",
	0,	AJAL,	AJAL,	AJAL,	AJAL,	AJAL,	AJAL,	AJAL,	AJAL,
};
static Opclass opOSYSTEM = {
	"",
	0,	ASYS,	ACSRRW,	ACSRRS,	ACSRRC,	0,	ACSRRWI,ACSRRSI,ACSRRCI,
};
static char fmtcsr[] = "c,s,d";
static char fmtcsri[] = "c,js,d";
static char *fmtOSYSTEM[8] = {
	"$i", fmtcsr, fmtcsr, fmtcsr, "", fmtcsri, fmtcsri, fmtcsri,
};
static Opclass opOOP_FP = {
	"fs,fd",
	0x0,	AADDF,	ASUBF,	AMULF,	ADIVF,	AMOVF,	0,	0,	0,
	0x1,	AMOVDF,	0,	0,	0,	0,	0,	0,	0,
	0x2,	ACMPLEF,ACMPLTF,ACMPEQF,0,	0,	0,	0,	0,
	0x3,	AMOVFW,	0,	AMOVFV,	0,	AMOVWF,	AMOVUF,	AMOVVF,	AMOVUVF,
};
static Opclass opOOP_DP = {
	"f2,fs,fd",
	0x0,	AADDD,	ASUBD,	AMULD,	ADIVD,	AMOVD,	0,	0,	0,
	0x1,	AMOVFD,	0,	0,	0,	0,	0,	0,	0,
	0x2,	ACMPLED,ACMPLTD,ACMPEQD,0,	0,	0,	0,	0,
	0x3,	AMOVDW,	0,	AMOVDV,	0,	AMOVWD,	AMOVUD,	AMOVVD,	AMOVUVD,
};

typedef struct Compclass Compclass;
struct Compclass {
	char	*fmt;
	uchar	immbits[18];
};

static Compclass rv32compressed[0x2E] = {
/* 00-07 ([1:0] = 0) ([15:13] = 0-7) */
	{"ADDI4SPN $i,d", 22, 6, 5, 11, 12, 7, 8, 9, 10},          /* 12:5 → 5:4|9:6|2|3 */
	{"FLD a,fd",      24, 10, 11, 12, 5, 6},                   /* 12:10|6:5 → 5:3|7:6 */
	{"LW a,d",        25, 6, 10, 11, 12, 5},                   /* 12:10|6:5 → 5:2|6 */
	{"FLW a,fd",      25, 6, 10, 11, 12, 5},                   /* 12:10|6:5 → 5:2|6 rv32 */
	{"? ",	0},
	{"FSD f2,a",      24, 10, 11, 12, 5, 6},                   /* 12:10|6:5 → 5:3|7:6 */
	{"SW 2,a",        25, 6, 10, 11, 12, 5},                   /* 12:10|6:5 → 5:2|6 */
	{"FSW f2,a",      25, 6, 10, 11, 12, 5},                   /* 12:10|6:5 → 5:2|6 rv32 */

/* 08-0F ([1:0] = 1) ([15:13] = 0-7 not 4) */
	{"ADDI $i,d",    ~26, 2, 3, 4, 5, 6, 12},                  /* 12|6:2 → * 5:0 */
	{"JAL p",        ~20, 3, 4, 5, 11, 2, 7, 6, 9, 10, 8, 12}, /* 12:2 → * 11|4|9:8|10|6|7|3:1|5 rv32 D*/
	{"LI $i,d",      ~26, 2, 3, 4, 5, 6, 12},                  /* 12|6:2 → * 5:0 */
	{"LUI $i,d",     ~14, 2, 3, 4, 5, 6, 12},                  /* 12|6:2 → * 17:12 */
	{"? ",	0},
	{"J p",          ~20, 3, 4, 5, 11, 2, 7, 6, 9, 10, 8, 12}, /* 12:2 → * 11|4|9:8|10|6|7|3:1|5 */
	{"BEQZ s,p",     ~23, 3, 4, 10, 11, 2, 5, 6, 12},          /* 12:10|6:2 → * 8|4|3|7:6|2:1|5 */
	{"BNEZ s,p",     ~23, 3, 4, 10, 11, 2, 5, 6, 12},          /* 12:10|6:2 → * 8|4|3|7:6|2:1|5 */

/* 10-17  ([1:0] = 2) ([15:13] = 0-7 not 4) */
	{"SLLI $i,d",     26, 2, 3, 4, 5, 6, 12},                  /* 12|6:2 → 5:0 */
	{"FLDSP i,fd",    23, 5, 6, 12, 2, 3, 4},                  /* 12|6:2 → 5:3|8:6 */
	{"LWSP i,d",      24, 4, 5, 6, 12, 2, 3},                  /* 12|6:2 → 5:2|7:6 */
	{"FLWSP i,fd",    24, 4, 5, 6, 12, 2, 3},                  /* 12|6:2 → 5:2|7:6 rv32 */
	{"? ",	0},
	{"FSDSP f2,$i",   23, 10, 11, 12, 7, 8, 9},                /* 12:7 → 5:3|8:6 */
	{"SWSP 2,$i",     24, 9, 10, 11, 12, 7, 8},                /* 12:7 → 5:2|7:6 */
	{"FSWSP f2,$i",   24, 9, 10, 11, 12, 7, 8},                /* 12:7 → 5:2|7:6 rv32 */

/* 18-1A  ([1:0] = 1) ([15:13] = 4) ([11:10] = 0-2) */
	{"SRLI $i,d",     26, 2, 3, 4, 5, 6, 12},                  /* 12|6:2 → 5:0 */
	{"SRAI $i,d",     26, 2, 3, 4, 5, 6, 12},                  /* 12|6:2 → 5:0 */
	{"ANDI $i,d",    ~26, 2, 3, 4, 5, 6, 12},                  /* 12|6:2 → * 5:0 */

/* 1B-22 ([1:0] = 1) ([15:13] = 4) ([11:10] = 3) ([12] = 0-1) ([6:5] = 0-3) */
	{"SUB 2,d",	0},
	{"XOR 2,d",	0},
	{"OR 2,d",	0},
	{"AND 2,d",	0},
	{"SUBW 2,d",	0},		/* rv64 */
	{"ADDW 2,d",	0},		/* rv64 */
	{"? ",	0},
	{"? ",	0},

/* 23-26 ([1:0] = 2) ([15:13] = 4) ([12] = 0-1) ((rs2 != 0) = 0-1) */
	{"JR s",	0},
	{"MV 2,d",	0},
	{"JALR s",	0},
	{"ADD 2,d",	0},

/* 27-27 ([1:0] = 1) ([15:13] = 3) ( rd = 2) */
	{"ADDI16SP $i",  ~22, 6, 2, 5, 3, 4, 12},                  /* 12|6:2 → * 9|4|6|8:7|5 */

/* 28-2C  rv64 alternates */
	{"LD a,d",	24, 10, 11, 12, 5, 6},                         /* 12:10|6:5 → 5:3|7:6 */
	{"SD 2,a",	24, 10, 11, 12, 5, 6},                         /* 12:10|6:5 → 5:3|7:6 */
	{"ADDIW $i,d",	~26, 2, 3, 4, 5, 6, 12},                   /* 12|6:2 → * 5:0 */
	{"LDSP i,d",	23, 5, 6, 12, 2, 3, },                     /* 12|6:2 → 5:3|8:6 */
	{"SDSP 2,i",	23, 10, 11, 12, 7, 8, 9},	               /* 12:7 → 5:3|8:6 */

/* 2D-2D  C.ADD with (rd = 0) */
	{"EBREAK",	0 }
};

/* map major opcodes to opclass table */
static Opclass *opclass[32] = {
	[OLOAD]		&opOLOAD,
	[OLOAD_FP]	&opOLOAD_FP,
	[OMISC_MEM]	&opOMISC_MEM,
	[OOP_IMM]	&opOOP_IMM,
	[OAUIPC]	&opOAUIPC,
	[OOP_IMM_32]	&opOOP_IMM_32,
	[OSTORE]	&opOSTORE,
	[OSTORE_FP]	&opOSTORE_FP,
	[OAMO]		&opOAMO,
	[OOP]		&opOOP,
	[OLUI]		&opOLUI,
	[OOP_FP]	&opOOP_FP,
	[OOP_32]	&opOOP_32,
	[OBRANCH]	&opOBRANCH,
	[OJALR]		&opOJALR,
	[OJAL]		&opOJAL,
	[OSYSTEM]	&opOSYSTEM,
};

/*
 * Print value v as name[+offset]
 */
static int
gsymoff(char *buf, int n, uvlong v, int space)
{
	Symbol s;
	int r;
	long delta;

	r = delta = 0;		/* to shut compiler up */
	if (v) {
		r = findsym(v, space, &s);
		if (r)
			delta = v-s.value;
		if (delta < 0)
			delta = -delta;
	}
	if (v == 0 || r == 0 || delta >= 4096)
		return snprint(buf, n, "#%llux", v);
	if (strcmp(s.name, ".string") == 0)
		return snprint(buf, n, "#%llux", v);
	if (!delta)
		return snprint(buf, n, "%s", s.name);
	return snprint(buf, n, "%s+%llux", s.name, v-s.value);
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
format(Instr *i, char *opcode, char *f)
{
	int c;
	long imm;
	char reg;

	reg = 'R';
	if(opcode != nil){
		bprint(i, "%s", opcode);
		if(f == 0)
			return;
		bprint(i, "\t");
	}else
		bprint(i, "C.");
	for(; (c = *f); f++){
		switch(c){
		default:
			bprint(i, "%c", c);
			break;
		case ' ':
			bprint(i, "\t");
			break;
		case 'f':
			reg = 'F';
			break;
		case 'j':
			reg = '$';
			break;
		case 's':
			bprint(i, "%c%d", reg, i->rs1);
			reg = 'R';
			break;
		case '2':
			bprint(i, "%c%d", reg, i->rs2);
			reg = 'R';
			break;
		case '3':
			bprint(i, "%c%d", reg, i->rs3);
			break;
		case 'd':
			bprint(i, "%c%d", reg, i->rd);
			reg = 'R';
			break;
		case 'i':
			imm = i->imm;
			if(imm < 0)
				bprint(i, "-%lux", -imm);
			else
				bprint(i, "%lux", imm);
			break;
		case 'p':
			i->curr += gsymoff(i->curr, i->end-i->curr, i->addr + i->imm, CANY);
			break;
		case 'a':
			if(i->rs1 == REGSB && mach->sb){
				i->curr += gsymoff(i->curr, i->end-i->curr, i->imm+mach->sb, CANY);
				bprint(i, "(SB)");
				break;
			}
			bprint(i, "%lx(R%d)", i->imm, i->rs1);
			break;
		case '7':
			bprint(i, "%ux", i->func7);
			break;
		case 'c':
			bprint(i, "CSR(%lx)", i->imm&0xFFF);
			break;
		}
	}
}

static int
badinst(Instr *i)
{
	format(i, "???", 0);
	return 4;
}

static long
immshuffle(uint w, uchar *p)
{
	int shift, i;
	ulong imm;

	shift = *p++;
	imm = 0;
	while((i = *p++) != 0){
		imm >>= 1;
		if((w>>i) & 0x01)
			imm |= (1<<31);
	}
	if(shift & 0x80)
		imm = (long)imm >> (shift ^ 0xFF);
	else
		imm >>= shift;
	return imm;
}

static int
decompress(Instr *i)
{
	ushort w;
	int op, aop;
	Compclass *cop;

	w = i->w;
	i->n = 2;
	i->func3 = (w>>13)&0x7;
	op = w&0x3;
	i->op = op;
	switch(op){
	case 0:
		i->rd  = 8 + ((w>>2)&0x7);
		i->rs1 = 8 + ((w>>7)&0x7);
		i->rs2 = i->rd;
		break;
	case 1:
		i->rd = (w>>7)&0x1F;
		if((i->func3&0x4) != 0)
			i->rd = 8 + (i->rd&0x7);
		i->rs1 = i->rd;
		i->rs2 = 8 + ((w>>2)&0x7);
		break;
	case 2:
		i->rd = (w>>7)&0x1F;
		i->rs1 = i->rd;
		i->rs2 = (w>>2)&0x1F;
	}
	aop = (op << 3) + i->func3;
	if((aop & 0x7) == 4){
		switch(op){
		case 1:
			aop = 0x18 + ((w>>10) & 0x3);
			if(aop == 0x1B)
				aop += ((w>>10) & 0x4) + ((w>>5) & 0x3);
			break;
		case 2:
			aop = 0x23 + ((w>>11) & 0x2) + (i->rs2 != 0);
			if(aop == 0x26 && i->rd == 0)
				aop = 0x2D;
			break;
		}
	}
	if(aop == 0x0B && i->rd == 2)
		aop = 0x27;
	if(i->rv64) switch(aop){
	case 0x03:	aop = 0x28; break;
	case 0x07:	aop = 0x29; break;
	case 0x09:	aop = 0x2A; break;
	case 0x13:	aop = 0x2B; break;
	case 0x17:	aop = 0x2C; break;
	}
	i->aop = aop;
	cop = &rv32compressed[aop];
	i->fmt = cop->fmt;
	i->imm = immshuffle(w, cop->immbits);
	return 2;
}

static int
decode(Map *map, uvlong pc, Instr *i)
{
	ulong w;
	int op;

	if(get4(map, pc, &w) < 0) {
		werrstr("can't read instruction: %r");
		return -1;
	}
	i->addr = pc;
	i->map = map;
	if((w&0x3) != 3){
		i->w = w & 0xFFFF;
		return decompress(i);
	}
	i->w = w;
	i->n = 4;
	op = (w&0x7F);
	i->op = op;
	i->func3 = (w>>12)&0x7;
	i->func7 = (w>>25)&0x7F;
	i->rs1 = (w>>15)&0x1F;
	i->rs2 = (w>>20)&0x1F;
	i->rs3 = (w>>27)&0x1F;
	i->rd = (w>>7)&0x1F;
#define FIELD(hi,lo,off)	(w>>(lo-off))&(((1<<(hi-lo+1))-1)<<off)
#define LFIELD(hi,lo,off)	(w<<(off-lo))&(((1<<(hi-lo+1))-1)<<off)
#define SFIELD(lo,off)		((long)(w&((~0)<<lo))>>(lo-off))
	switch(op>>2) {
	case OSTORE:	/* S-type */
	case OSTORE_FP:
		i->imm = SFIELD(25,5) | FIELD(11,7,0);
		break;
	case OBRANCH:	/* B-type */
		i->imm = SFIELD(31,12) | LFIELD(7,7,11) | FIELD(30,25,5) | FIELD(11,8,1);
		break;
	case OOP_IMM:	/* I-type */
	case OOP_IMM_32:
		if(i->func3 == 1 || i->func3 == 5){		/* special case ASL/ASR */
			i->imm = FIELD(25,20,0);
			break;
		}
	/* fall through */
	case OLOAD:
	case OLOAD_FP:
	case OMISC_MEM:
	case OJALR:
	case OSYSTEM:
		i->imm = SFIELD(20,0);
		break;
	case OAUIPC:	/* U-type */
	case OLUI:
		i->imm = SFIELD(12,12);
		break;
	case OJAL:	/* J-type */
		i->imm = SFIELD(31,20) | FIELD(19,12,12) | FIELD(20,20,11) | FIELD(30,21,1);
		break;
	}
	return 4;
}

static int
pseudo(Instr *i, int aop)
{
	char *op;

	switch(aop){
	case AJAL:
		if(i->rd == 0){
			format(i, "JMP",	"p");
			return 1;
		}
		break;
	case AJALR:
		if(i->rd == 0){
			format(i, "JMP", "a");
			return 1;
		}
		break;
	case AADD:
		if((i->op>>2) == OOP_IMM){
			op = i->rv64 ? "MOV" : "MOVW";
			if(i->rs1 == 0)
				format(i, op, "$i,d");
			else if(i->rs1 == REGSB && mach->sb && i->rd != REGSB)
				format(i, op, "$a,d");
			else if(i->imm == 0)
				format(i, op, "s,d");
			else break;
			return 1;
		}
		break;
	case ASYS:
		switch(i->imm){
		case 0:
			format(i, "ECALL", nil);
			return 1;
		case 1:
			format(i, "EBREAK", nil);
			return 1;
		}
	}
	return 0;
}

static int
mkinstr(Instr *i)
{
	Opclass *oc;
	Optab *o;
	char *fmt;
	int aop;

	if((i->op&0x3) != 0x3){
		format(i, nil, i->fmt);
		return 2;
	}
	oc = opclass[i->op>>2];
	if(oc == 0)
		return badinst(i);
	fmt = oc->fmt;
	if(oc == &opOSYSTEM)
		fmt = fmtOSYSTEM[i->func3];
	if(oc == &opOOP_FP){
		if(i->func7 & 1)
			oc = &opOOP_DP;
		o = &oc->tab[i->func7>>5];
		switch(o->func7){
		case 0:
			fmt = "f2,fs,fd";
		/* fall through */
		default:
			aop = o->op[(i->func7>>2)&0x7];
			if((i->func7&~1) == 0x10){
				if(i->func3 == 0 && i->rs1 == i->rs2)
					fmt = "fs,fd";
				else
					aop = 0;
			}
			break;
		case 2:
			aop = o->op[i->func3];
			break;
		case 3:
			if(i->func7 & 0x10)
				return badinst(i);
			aop = o->op[(i->func7>>1)&0x4 | (i->rs2&0x3)];
			if(i->func7 & 0x8)
				fmt = "s,fd";
			else
				fmt = "fs,d";
			break;
		}
		if(aop == 0)
			return badinst(i);
		format(i, anames[aop], fmt);
		return 4;
	}
	o = oc->tab;
	while(o->func7 != 0 && (i->func7 != o->func7 || o->op[i->func3] == 0))
		o++;
	if((aop = o->op[i->func3]) == 0)
		return badinst(i);
	if(pseudo(i, aop))
		return 4;
	format(i, anames[aop], fmt);
	return 4;
}

static int
riscvdas(Map *map, uvlong pc, char modifier, char *buf, int n)
{
	Instr i;

	USED(modifier);
	i.rv64 = 0;
	i.curr = buf;
	i.end = buf+n;
	if(decode(map, pc, &i) < 0)
		return -1;
	return mkinstr(&i);
}

static int
riscv64das(Map *map, uvlong pc, char modifier, char *buf, int n)
{
	Instr i;

	USED(modifier);
	i.rv64 = 1;
	i.curr = buf;
	i.end = buf+n;
	if(decode(map, pc, &i) < 0)
		return -1;
	return mkinstr(&i);
}

static int
riscvhexinst(Map *map, uvlong pc, char *buf, int n)
{
	Instr i;

	i.curr = buf;
	i.end = buf+n;
	if(decode(map, pc, &i) < 0)
		return -1;
	if(i.end-i.curr > 2*i.n)
		i.curr = _hexify(buf, i.w, 2*i.n - 1);
	*i.curr = 0;
	return i.n;
}

static int
riscvinstlen(Map *map, uvlong pc)
{
	Instr i;

	return decode(map, pc, &i);
}

static char*
riscvexcep(Map*, Rgetter)
{
	return "Trap";
}

static int
riscvfoll(Map *map, uvlong pc, Rgetter rget, uvlong *foll)
{
	Instr i;
	char buf[8];
	int len;

	len = decode(map, pc, &i);
	if(len < 0)
		return -1;
	foll[0] = pc + len;
	if(len == 2){
		switch(i.aop){
		case 0x0D: /* C.J */
		case 0x0E: /* C.BEQZ */
		case 0x0F: /* C.BNEZ */
			foll[1] = pc + i.imm;
			return 2;
		case 0x09:	/* C.JAL */
			foll[0] = pc + i.imm;
			break;
		case 0x23: /* C.JR */
		case 0x25: /* C.JALR */
			sprint(buf, "R%d", i.rs1);
			foll[0] = (*rget)(map, buf);
			break;
		}
		return 1;
	}
	switch(i.op>>2) {
	case OBRANCH:
		foll[1] = pc + i.imm;
		return 2;
	case OJAL:
		foll[0] = pc + i.imm;
		break;
	case OJALR:
		sprint(buf, "R%d", i.rd);
		foll[0] = (*rget)(map, buf);
		break;
	}
	return 1;
}

/*
 *	Debugger interface
 */
Machdata riscvmach =
{
	{0x02, 0x90},		/* break point */
	2,			/* break point size */

	leswab,			/* short to local byte order */
	leswal,			/* long to local byte order */
	leswav,			/* long to local byte order */
	risctrace,		/* C traceback */
	riscframe,		/* Frame finder */
	riscvexcep,		/* print exception */
	0,				/* breakpoint fixup */
	0,				/* single precision float printer */
	0,				/* double precisioin float printer */
	riscvfoll,		/* following addresses */
	riscvdas,		/* symbolic disassembly */
	riscvhexinst,	/* hex disassembly */
	riscvinstlen,	/* instruction size */
};

Machdata riscv64mach =
{
	{0x02, 0x90},		/* break point */
	2,			/* break point size */

	leswab,			/* short to local byte order */
	leswal,			/* long to local byte order */
	leswav,			/* long to local byte order */
	risctrace,		/* C traceback */
	riscframe,		/* Frame finder */
	riscvexcep,		/* print exception */
	0,				/* breakpoint fixup */
	0,				/* single precision float printer */
	0,				/* double precisioin float printer */
	riscvfoll,		/* following addresses */
	riscv64das,		/* symbolic disassembly */
	riscvhexinst,	/* hex disassembly */
	riscvinstlen,	/* instruction size */
};
