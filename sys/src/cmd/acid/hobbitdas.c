#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ctype.h>
#include <mach.h>
#define Extern extern
#include "acid.h"

typedef ushort Parcel;
typedef struct {
	char	*mnemonic;
	void	(*f0)(Parcel, long);
	void	(*f1)(Parcel, long);
} Opcode;
static Opcode onepop[32];
static Opcode stackop[4];
static Opcode nilop[10];
static Opcode threepop[64];
static Opcode monop[16];
static char *addrmode[16];
static char *faddrmode[16];
static char *regname[32];

#define CPU	0x2C00

static Parcel cpu;
static long src, dst;
static long mydot;
static int myisp;
static Map *mymap;

static Parcel
getparcel(Map *map, ulong pc, int isp)
{
	long w;

	get4(map, pc & ~3, isp, &w);
	if(pc & 3)
		return w & 0xFFFF;
	return w>>16;
}

static Parcel
getinstr(Map *map, int isp)
{
	Parcel p;

	myisp = isp;
	mydot = dot;
	mymap = map;
	cpu = 0;
	p = getparcel(mymap, mydot-2, myisp);
	if(p == CPU)
		cpu = 1;
	dotinc = sizeof(Parcel);
	p = getparcel(mymap, mydot, myisp);
	if(p == CPU){
		cpu = 1;
		mydot += sizeof(Parcel);
		dotinc += sizeof(Parcel);
		p = getparcel(mymap, mydot, myisp);
	}
	if((p & 0x8000) == 0){
		src = (p>>5) & 0x1F;
		dst = p & 0x1F;
	}
	else if((p & 0x4000) == 0){
		src = getparcel(mymap, mydot+2, myisp);
		dst =  getparcel(mymap, mydot+4, myisp);
		dotinc += 2*sizeof(Parcel);
	}
	else {
		src = getparcel(mymap, mydot+2, myisp)<<16;
		src |= getparcel(mymap, mydot+4, myisp);
		dst = getparcel(mymap, mydot+6, myisp)<<16;
		dst |= getparcel(mymap, mydot+8, myisp);
		dotinc += 4*sizeof(Parcel);
	}
	return p;
}

static void
oneparcel(Parcel p)
{
	int opcode, offset;
	Opcode *o;

	opcode = (p>>10) & 0x1F;
	if (opcode == 0x02) {
		opcode = p & 0x03;
		offset = ((p & 0x03FC)>>2) * 16;
		if (opcode == 0)		/* ENTER */
			offset = (((p & 0x03FC)>>2)|0xFFFFFF00) * 16;
		dprint("%s\tR%d", stackop[opcode].mnemonic, offset);
	}
	else if (opcode == 0x0B) {
		offset = p & 0x3FF;
		if (offset >= sizeof(nilop)/sizeof(Opcode))
			dprint("niladic %lux", offset);
		else
			dprint("%s", nilop[offset].mnemonic);
	}
	else {
		o = &onepop[opcode];
		dprint("%s\t", o->mnemonic);
		if (o->f0)
			(*o->f0)(p, src);
		if (o->f1) {
			dprint(",");
			(*o->f1)(p, dst);
		}
	}
}

static void
fgen(Parcel m, long w)
{
	dprint(faddrmode[m], w);
}

static void
gen32(Parcel m, long w, void (*f)(Parcel, long))
{
	Symbol s;

	if (cpu == 0 || m != 7) {
		if (m < 4 || m == 12 || m == 0x0F) {
			if (findsym(w, CANY, &s) && s.value-w < 0x1000000) {
				if (s.class == CDATA || s.class == CTEXT) {
					if (m != 0x0F)
						dprint("*");
					dprint("$%s", s.name);
					if (s.value-w)
						dprint("+%lux", s.value-w);
					if (s.class == CDATA)
						dprint("(SB)");
					return;
				}
			}
		}
		if (f)
			(*f)(m, w);
		else
			dprint(addrmode[m], w);
	}
	else
		dprint("%%%s", regname[w & 0x1F]);
}

static void
gen16(Parcel m, long w, void (*f)(Parcel, long))
{
	if (m < 4 || m == 12) {
		dprint("*$");
		w = (w & 0x1FFFFFFF)|(mydot & 0xE0000000);
		psymoff(w, SEGDATA, "(SB)");
		return;
	}
	else if(w & 0x8000)
		w |= 0xFFFF0000;
	gen32(m, w, f);
}

static void
threeparcel(Parcel p)
{
	int opcode;
	Opcode *o;

	opcode = (p>>8) & 0x3F;
	if (opcode == 0) {				/* monadic */
		opcode = p & 0x0F;
		o = &monop[opcode];
		dprint("%s\t", o->mnemonic);
		if (o->f0)
			(*o->f0)(p, (src<<16)|dst);
	}
	else {
		o = &threepop[opcode];
		dprint("%s\t", o->mnemonic);
		if ((p & 0xC000) == 0xC000) {		/* 5-parcel */
			gen32((p>>4) & 0x0F, src, o->f0);
			dprint(",");
			gen32(p & 0x0F, dst, o->f1);
		}
		else {					/* 3-parcel */
			gen16((p>>4) & 0x0F, src, o->f0);
			dprint(",");
			gen16(p & 0x0F, dst, o->f1);
		}
	}
}

void
hobbitprintins(Map *map, char modifier, int isp)
{
	Parcel p;

	USED(modifier);
	p = getinstr(map, isp);
	if ((p & 0x8000) == 0)
		oneparcel(p);
	else 
		threeparcel(p);
}

static void
imm10(Parcel p, long w)
{
	USED(w);
	dprint("$%lux", (p & 0x3FF) * 4);
}

static void
pcrel10(Parcel p, long w)
{
	int offset;

	USED(w);
	offset = (((p & 0x200) ? (p|0xFFFFFE00): (p & 0x1FF))<<1);
	psymoff(offset + mydot, myisp, "");
	dprint(" [%lx(PC)]", offset);
}

static void
stk5(Parcel p, long w)
{
	USED(p);
	dprint("R%d",  w * 4);
}

static void
imm5(Parcel p, long w)
{
	int offset;

	USED(p);
	offset = ((w & 0x10) ? (w|0xFFFFFFF0): w);
	dprint("$%lux", offset);
}

static void
uimm5(Parcel p, long w)
{
	USED(p);
	dprint("$%lux", w & 0x1F);
}

static void
istk5(Parcel p, long w)
{
	USED(p);
	dprint("*R%d", w * 4);
}

static void
wai5(Parcel p, long w)
{
	USED(p);
	dprint("$%lux", w * 4);
}

static void
flow32(Parcel p, long w)
{
	switch ((p>>4) & 0x0F) {

	case 0x0C:
		dprint("**$");
		psymoff(w, myisp, "");
		break;

	case 0x0D:
		dprint("*R%d", w);
		break;

	case 0x0E:	
		psymoff(w + mydot, myisp, "");
		dprint(" [%lx(PC)]", w);
		break;

	case 0x0F:
		dprint("*$");
		psymoff(w, myisp, "");
		break;

	default:
		dprint("mode %lux, operand %lux", (p>>4) & 0x0F, w);
		break;
	}
}

static void
word32(Parcel p, long w)
{
	switch ((p>>4) & 0x0F) {

	case 0x0C:
	case 0x0D:
	case 0x0E:	
	case 0x0F:
		gen32((p>>4) & 0x0F, w, 0);
		break;

	default:
		dprint("mode %lux, operand %lux", (p>>4) & 0x0F, w);
		break;
	}
}

static void
abs32(Parcel p, long w)
{
	switch ((p>>4) & 0x0F) {

	case 0x0E:	
		psymoff(w + mydot, myisp, "");
		dprint(" [%lx(PC)]", w);
		break;

	case 0x0F:
		dprint("*$");
		psymoff(w, myisp, "");
		break;

	default:
		dprint("mode %lux, operand %lux", (p>>4) & 0x0F, w);
	}
}

static void
stk32(Parcel p, long w)
{
	switch ((p>>4) & 0x0F) {

	case 0x0D:
	case 0x0E:	
		gen32((p>>4) & 0x0F, w, 0);
		break;

	default:
		dprint("mode %lux, operand %lux", (p>>4) & 0x0F, w);
		break;
	}
}

static Opcode onepop[32] = {
	"KCALL",	imm10,	0,
	"CALL",		pcrel10,0,
	"stack",	0,	0,
	"JMP",		pcrel10,0,
	"JMPFN",	pcrel10,0,
	"JMPFY",	pcrel10,0,
	"JMPTN",	pcrel10,0,
	"JMPTY",	pcrel10,0,
	"op0x08",	0,	0,
	"op0x09",	0,	0,
	"MOV",		wai5,	stk5,
	"niladic",	0,	0,
	"op0x0C",	0,	0,
	"ADD3",		wai5,	stk5,
	"AND3",		imm5,	stk5,
	"AND",		stk5,	stk5,
	"CMPEQ",	imm5,	stk5,
	"CMPGT",	stk5,	stk5,
	"CMPGT",	imm5,	stk5,
	"CMPEQ",	stk5,	stk5,
	"ADD",		imm5,	stk5,
	"ADD3",		imm5,	stk5,
	"ADD",		stk5,	stk5,
	"ADD3",		stk5,	stk5,
	"MOV",		stk5,	stk5,
	"MOV",		istk5,	stk5,
	"MOV",		stk5,	istk5,
	"MOV",		istk5,	istk5,
	"MOV",		imm5,	stk5,
	"MOVA",		stk5,	stk5,
	"SHL3",		uimm5,	stk5,
	"SHR3",		uimm5,	stk5,
};

static Opcode stackop[4] = {
	"ENTER",	0,	0,
	"CATCH",	0,	0,
	"RETURN",	0,	0,
	"POPN",		0,	0,
};

static Opcode nilop[10] = {
	"CPU",		0,	0,
	"KRET",		0,	0,
	"NOP",		0,	0,
	"FLUSHI",	0,	0,
	"FLUSHP",	0,	0,
	"CRET",		0,	0,
	"FLUSHD",	0,	0,
	"op0x07",	0,	0,
	"TESTV",	0,	0,
	"TESTC",	0,	0,
};

static Opcode threepop[64] = {
	"monadic",	0,	0,
	"ORI",		0,	0,
	"ANDI",		0,	0,
	"ADDI",		0,	0,
	"MOVA",		0,	0,
	"UREM",		0,	0,
	"MOV",		0,	0,
	"DQM",		0,	0,
	"FNEXT",	fgen,	fgen,
	"FSCALB",	0,	fgen,
	"op0x0A",	0,	0,
	"FREM",		fgen,	fgen,
	"TADD",		0,	0,
	"TSUB",		0,	0,
	"op0x0E",	0,	0,
	"op0x0F",	0,	0,
	"FSQRT",	fgen,	fgen,
	"FMOV",		fgen,	fgen,
	"FLOGB",	fgen,	fgen,
	"FCLASS",	fgen,	0,
	"op0x14",	0,	0,
	"op0x15",	0,	0,
	"op0x16",	0,	0,
	"op0x17",	0,	0,
	"FCMPGE",	fgen,	fgen,
	"FCMPGT",	fgen,	fgen,
	"FCMPEQ",	fgen,	fgen,
	"FCMPEQN",	fgen,	fgen,
	"FCMPN",	fgen,	fgen,
	"CMPGT",	0,	0,
	"CMPHI",	0,	0,
	"CMPEQ",	0,	0,
	"SUB",		0,	0,
	"OR",		0,	0,
	"AND",		0,	0,
	"ADD",		0,	0,
	"XOR",		0,	0,
	"REM",		0,	0,
	"MUL",		0,	0,
	"DIV",		0,	0,
	"FSUB",		fgen,	fgen,
	"FMUL",		fgen,	fgen,
	"FDIV",		fgen,	fgen,
	"FADD",		fgen,	fgen,
	"SHR",		0,	0,
	"USHR",		0,	0,
	"SHL",		0,	0,
	"UDIV",		0,	0,
	"SUB3",		0,	0,
	"OR3",		0,	0,
	"AND3",		0,	0,
	"ADD3",		0,	0,
	"XOR3",		0,	0,
	"REM3",		0,	0,
	"MUL3",		0,	0,
	"DIV3",		0,	0,
	"FSUB3",	fgen,	fgen,
	"FMUL3",	fgen,	fgen,
	"FDIV3",	fgen,	fgen,
	"FADD3",	fgen,	fgen,
	"SHR3",		0,	0,
	"USHR3",	0,	0,
	"SHL3",		0,	0,
	"op0x1F",	0,	0,
};

static Opcode monop[16] = {
	"KCALL",	word32,	0,
	"CALL",		flow32,	0,
	"RETURN",	stk32,	0,
	"JMP",		flow32,	0,
	"JMPFN",	abs32,	0,
	"JMPFY",	abs32,	0,
	"JMPTN",	abs32,	0,
	"JMPTY",	abs32,	0,
	"CATCH",	word32,	0,
	"ENTER",	word32,	0,
	"LDRAA",	flow32,	0,
	"FLUSHPTE",	word32,	0,
	"FLUSHPBE",	word32,	0,
	"FLUSHDCE",	word32,	0,
	"op0x0E",	0,	0,
	"POPN",		stk32,	0,
};

static char *addrmode[] = {
	"*$%lux.B",
	"*$%lux.UB",
	"*$%lux.H",
	"*$%lux.UH",
	"R%d.B",
	"R%d.UB",
	"R%d.H",
	"R%d.UH",
	"*R%d.B",
	"*R%d.UB",
	"*R%d.H",
	"*R%d.UH",
	"*$%lux",
	"R%d",
	"*R%d",
	"$%lux",
};

static char *faddrmode[] = {
	"*$%lux.F",
	"*$%lux.D",
	"*$%lux.X",
	"",
	"R%d.F",
	"R%d.D",
	"R%d.X",
	"",
	"*R%d.F",
	"*R%d.D",
	"*R%d.X",
	"",
	"*$%lux.W",
	"R%d.W",
	"*R%d.W",
	"$%lux",
};

static char *regname[32] = {
	"REG0",
	"MSP",
	"ISP",
	"SP",
	"CONFIG",
	"PSW",
	"SHAD",
	"VB",
	"STB",
	"FAULT",
	"ID",
	"TIMER1",
	"TIMER2",
	"REG13",
	"REG14",
	"REG15",
	"FPSW",
	"REG17",
	"REG18",
	"REG19",
	"REG20",
	"REG21",
	"REG22",
	"REG23",
	"REG24",
	"REG25",
	"REG26",
	"REG27",
	"REG28",
	"REG29",
	"REG30",
	"REG31",
};


int
hobbitfoll(ulong pc, ulong *foll)
{
	ulong w;
	Parcel p;

	p = getinstr(cormap, SEGDATA);
	if ((p & 0x8000) == 0) {
		switch ((p>>10) & 0x1F) {

		case 0x01:				/* CALL */
		case 0x03:				/* JMP */
			if (p & 0x200)
				w = p|0xFFFFFE00;
			else
				w = p & 0x1FF;
			foll[0] = pc+(w<<1);
			return 1;

		case 0x04:				/* JMPFN */
		case 0x05:				/* JMPFY */
		case 0x06:				/* JMPTN */
		case 0x07:				/* JMPTY */
			if (p & 0x200)
				w = p|0xFFFFFE00;
			else
				w = p & 0x1FF;
			foll[0] = pc+2;
			foll[1] = pc+(w<<1);
			return 2;

		case 0x02:				/* stack */
			if ((p & 0x03) != 0x02)		/* RETURN */
				break;
			w = ((p & 0x03FC)>>2) * 16;
			get4(cormap, rget("SP")+w, SEGDATA, (long *) foll);
			return 1;
		}
	}
	else if ((p & 0xFF00) == 0x8000) {		/* 3-parcel monadic */
		w = (src<<16)|dst;
		switch (p & 0x0F) {

		case 0x01:				/* CALL */
		case 0x03:				/* JMP */
			switch ((p>>4) & 0x0F) {
		
			case 0x0C:			/* **$w */
				get4(cormap, w, SEGDATA, (long *)&w);
				get4(cormap, w, SEGDATA, (long *)foll);
				return 1;
		
			case 0x0D:			/* *Rw */
				get4(cormap, rget("SP")+w, SEGDATA, (long *) foll);
				return 1;
		
			case 0x0E:			/* pc+w */
				foll[0] = pc+w;
				return 1;
		
			case 0x0F:			/* *$w */
				get4(cormap, w, SEGDATA, (long *) foll);
				return 1;
			}
			break;

		case 0x04:				/* JMPFN */
		case 0x05:				/* JMPFY */
		case 0x06:				/* JMPTN */
		case 0x07:				/* JMPTY */
			switch ((p>>4) & 0x0F) {

			case 0x0E:			/* pc+w */
				foll[0] = pc+w;
				return 1;
		
			case 0x0F:			/* *$w */
				get4(cormap, w, SEGDATA, (long *) foll);
				return 1;
			}
			break;

		case 0x02:				/* RETURN */
			get4(cormap, rget("SP")+w, SEGDATA, (long *) foll);
			return 1;
		}
	}
	foll[0] = pc+dotinc;
	return 1;
}
