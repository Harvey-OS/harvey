#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ctype.h>
#include <mach.h>
#define Extern extern
#include "acid.h"

#define	SZ_MASK		00300
#define	SZ_SHIFT	6		/* bits  (7-6) */

static	int	dorand(ulong, int, int);
static	int	ea(int, int, int);
static	int	doindex(int);
static	void	dcr(int);
static	char	*fcr(int);

#define	MAXRAND	4

struct m68020optab {
	ushort opcode;
	ushort mask;
	ushort op2;
	ushort mk2;
	char	*opname;
	char	flags;
	char	nrand;
	short	rand[MAXRAND];
};

/*
 * operand types
 */
#define	DSREG	00	/* special registers */
#define	DEA	01	/* E.A. to low order 6 bits */
#define	DRG	02	/* register to low order 3 bits */
#define	DRGL	03	/* register to bits 11-9 */
#define	DBR	04	/* branch offset (short) */
#define	DMQ	05	/* move-quick 8-bit value */
#define	DAQ	06	/* add-quick 3-bit value in 11-9 */
#define	DIM	07	/* Immediate value, according to size */
#define	DEAM	010	/* E.A. to bits 11-6 as in move */
#define	DBCC	011	/* branch address as in "dbcc" */
#define	DTRAP	012	/* immediate in low 4 bits */
#define D2L	013	/* register to bits 0-2 of next word */
#define D2H	014	/* register to bits 12-14 of next word */
#define DBL	015	/* qty in bits 0-5 of next word */
#define DBH	016	/* qty in bits 6-11 of next word */
#define DCR	017	/* control reg a bit combination in 0-11 */
#define	DBKPT	020	/* immediate in low three bits */
#define	DFSRC	021	/* floating source specifier */
#define	DFDRG	022	/* floating destination register */
#define	DFSRG	023	/* floating source register */
#define	DFCR	024	/* floating point constant register */
#define	DFBR	025	/* floating branch offset */
#define	DFMRGM	026	/* FMOVE register mask */
#define	DFMCRGM	027	/* FMOVEC register mask */
#define	DCH	030	/* cache indicator */
#define	DCHRGI	031	/* cache indicator plus indirect address register */

#define	DMASK	037

/*
 * operand flags
 */

#define	ADEC	0100	/* funny auto-decrement */
#define	AINC	0200	/* funny auto-increment */
#define	AAREG	0400	/* address register */
#define	ADREG	01000	/* data register */
#define	AONE	02000	/* immediate always 1 */
#define	AWORD	04000	/* immediate always two-byte */
#define	AIAREG	010000	/* indirect through address register */

/*
 * special registers
 */

#define	C	0100	/* the condition code register */
#define	SR	0200	/* the status register */
#define	U	0400	/* the user stack pointer */

/*
 * flags
 */

#define	B	0	/* byte */
#define	W	1	/* word */
#define	L	2	/* long */
#define	D	3	/* double float */
#define	F	4	/* single float */
#define	NZ	010	/* no size */
#define	SZ	017

#define I2W	020		/* two word instruction code */

static struct opmask {
	long mask;
	int shift;
} opmask[] = {
	0,		0,	/* DIG	0   ignore this address */
	0x0000003f,	0,	/* DEA	1   E.A. to low order 6 bits */
	0x00000007,	0,	/* DRG	2   register to low order 3 bits */
	0x00000e00,	9,	/* DRGL	3   register to bits 11-9 */
	0x000000ff,	0,	/* DBR	4   branch offset (short) */
	0x000000ff,	0,	/* DMQ	5   move-quick 8-bit value */
	0x00000e00,	9,	/* DAQ	6   add-quick 3-bit value in 11-9 */
	0,		0,	/* DIM	7   Immediate value, according to size */
	0x00000fc0,	6,	/* DEAM	8   E.A. to bits 11-6 as in move */
	0,		0,	/* DBCC	9   branch address as in "dbcc" */
	0x0000000f,	0,	/* DTRAP 10 immediate in low 4 bits */
	0x00070000,	16,	/* D2L	11  register to bits 0-2 of next word */
	0x70000000,	16+12,	/* D2H	12  register to bits 12-14 of next word */
	0x003f0000,	16,	/* DBL	13  qty in bits 0-5 of next word */
	0x0fc00000,	16+6,	/* DBH	14  qty in bits 6-11 of next word */
	0x0fff0000,	16,	/* DCR	15  control reg a bit combination in 0-11 */
	0x00000007,	0,	/* DBKPT 16 immediate in low 3 bits */
	0x1c000000,	16+10,	/* DFSRC 17 floating source specifier */
	0x03800000,	16+7,	/* DFDRG 18 floating destination register */
	0x1c000000,	16+10,	/* DFSRG 19 floating source register */
	0x007f0000,	16,	/* DFCR 20 floating constant register */
	0x00000040,	0,	/* DFBR 21 floating branch offset */
	0x00ff0000,	16,	/* DFMRGM 22 FMOVE register mask */
	0x1c000000,	16+10,	/* DFMRGM 23 FMOVE register mask */
};

static int dsp;
static Map *mymap;

#include "68020das2"

struct m68020optab*
mc68020op(ulong w, ulong *w1p, int isp)
{
	struct m68020optab *op;
	int w1f = 0;
	ushort s;
	ulong w1 = 0;

	for (op = m68020optab; op->opname; op++) {
		if ((w & op->mask) != op->opcode)
			continue;
		if ((op->flags & I2W) == 0)
			break;		/* 1-word match */
		if (w1f == 0) {
			get2(mymap, dot+2, isp, &s);
			w1 = s;
			w1f++;
		}
		if ((w1 & op->mk2) == op->op2)
			break;		/* 2-word match */
	}
	if (op->opname == 0)
		op = 0;
	*w1p = w1;
	return op;
}

int
m68020ins(Map *map, char modifier, int isp)
{
	struct m68020optab *op;
	int i, inc;
	ushort s;
	ulong w, w1;

	USED(modifier);
	dsp = isp;
	mymap = map;
	get2(map, dot, isp, &s);

	w = s;
	op = mc68020op(w, &w1, isp);
	inc = 2;
	dotinc = 2;
	if (op == 0) {
		dprint("\tnumber\t%lux", w);
		return inc;
	}
	w &= 0xffff;
	if ((op->flags & I2W) != 0) {
		w |= w1 << 16;
		dotinc = 4;
		inc += 2;
	}
	dprint("%s", op->opname);
	for (i = 0; i < op->nrand; i++) {
		if (i == 0)
			dprint("\t");
		else
			dprint(",");
		inc += dorand(w, op->rand[i], op->flags & SZ);
	}
	return inc;
}

int
instrsize(ulong pc)
{
	int i;

	dot = pc;
	xprint = 1;
	i = m68020ins(cormap, 'i', SEGDATA);
	xprint = 0;
	return i;
}

void
m68020printins(Map *map, char modifier, int space)
{
	m68020ins(map, modifier, space);
}

#define	ENSIGN(x)	((ulong)(short)(x))

int
dorand(ulong w, int rand, int size)
{
	struct opmask *om;
	long lval;
	ushort s;
	int val, inc;

	om = &opmask[rand & DMASK];
	if (om->mask)
		val = (w & om->mask) >> om->shift;
	else
		val = 0;
	inc = 0;
	switch(rand & DMASK) {
	case DEA:	/* effective address spec */
		inc = ea(val >> 3, val & 07, size);
		break;

	case DRG:	/* abs register */
	case DRGL:
	case D2H:
	case D2L:
		if (rand & ADEC)
			dprint("-(A%d)", val);
		else if (rand & AINC)
			dprint("(A%d)+", val);
		else if (rand & AAREG)
			dprint("A%d", val);
		else if (rand & ADREG)
			dprint("R%d", val);
		else if (rand & AIAREG)
			dprint("(A%d)", val);
		else
			dprint("DRGgok");
		break;

	case DBR:	/* branch displacement */
		lval = val;
		if (val == 0) {
			get2(mymap, dot+dotinc, dsp, &s);
			lval = s;
			if (lval & 0x8000)
				lval |= ~0xffff;
			inc += 2;
			dotinc += 2;
		}
		else if (val == 0xff) {
			get4(mymap, dot+dotinc, dsp, &lval);
			inc += 4;
			dotinc += 4;
		}
		else if (val & 0x80)
			lval |= ~0xff;
		lval += dot + 2;
		psymoff(lval, dsp, "");
		break;

	case DBH:	/* 6-bit strange quick */
	case DBL:	/* other 6-bit strange quick */
		if (val & 040) {
			dprint("R%d", val & 07);
			break;
		}
		/* fall through */
	case DMQ:	/* 8-bit quick */
	case DTRAP:	/* 4-bit quick */
		dprint("$Q");
		psymoff((ulong)val, dsp, "");
		break;

	case DBKPT:	/* 3-bit quick */
		dprint("$Q");
		psymoff((ulong)val, dsp, "");
		break;

	case DAQ:	/* silly 3-bit immediate */
		if (val == 0)
			val = 8;
		dprint("$Q");
		psymoff((ulong)val, dsp, "");
		break;

	case DIM:	/* immediate */
		if (rand & AONE) {
			dprint("$Q1");
			break;
		}
		if (rand & AWORD)
			size = W;
		switch ((int)size) {
		case B:
			get2(mymap, dot+dotinc, dsp, &s);
			lval = s&0377;
/*			if (val & 0200)
********>???			val |= ~0377;	/* sign extend */
			dotinc += 2;	/* sic */
			inc += 2;
			break;

		case W:
			get2(mymap, dot+dotinc, dsp, &s);
			lval = ENSIGN(s);
			dotinc += 2;
			inc += 2;
			break;

		case L:
			get4(mymap, dot+dotinc, dsp, &lval);
			dotinc += 4;
			inc += 4;
			break;

		default:
			lval = 0;
		}
		dprint("$");
		psymoff(lval, dsp, "");
		break;

	case DEAM:	/* assinine backwards ea */
		inc += ea(val & 07, val >> 3, size);
		break;

	case DBCC:	/* branch displacement a la dbcc */
		get2(mymap, dot+dotinc, dsp, &s);
		lval = s;
		dotinc += 2;
		inc += 2;
		lval += dot + 2;
		psymoff(lval, dsp, "");
		break;

	case DCR:
		dcr(val);
		break;

	case DSREG:
		if (rand & C)
			dprint("CCR");
		else if (rand & SR)
			dprint("SR");
		else if (rand & U)
			dprint("USP");
		else
			dprint("GOKdsreg");
		break;

	case DFSRC:
		dprint("F%d", val);
		break;

	case DFDRG:
	case DFSRG:
		dprint("F%d", val);
		break;

	case DFCR:
		dprint("$%s", fcr((int)val));
		break;

	case DFBR:	/* floating branch displacement */
		if (val == 0) {
			get2(mymap, dot+dotinc, dsp, &s);
			lval = s;
			if (lval & 0x8000)
				lval |= ~0xffff;
			dotinc += 2;
			inc += 2;
		}
		else {
			get4(mymap, dot+dotinc, dsp, &lval);
			dotinc += 4;
			inc += 4;
		}
		lval += dot + 2;
		psymoff(lval, dsp, "");
		break;

	case DFMRGM:
	case DFMCRGM:
		dprint("$%ux", val);
		break;

	case DCH:
	case DCHRGI:
		switch(w & 0xC0){
		case 0x00:
			dprint("<NC>");
			break;
		case 0x40:
			dprint("<DC>");
			break;
		case 0x80:
			dprint("<IC>");
			break;
		case 0xC0:
			dprint("<BC>");
			break;
		}
		if((rand&DMASK) == DCHRGI)
			dprint(",(A%d)", w&7);
		break;
	default:
		dprint("GOK");
		break;
	}
    Return:
	return inc;
}

int
ea(int mode, int reg, int size)
{
	long disp;
	ushort s;
	uchar ch;
	ulong h, l;

	switch((int)mode){
	case 0:			/* data reg */
		dprint("R%d", reg);
		return 0;

	case 1:			/* addr reg */
		dprint("A%d", reg);
		return 0;

	case 2:			/* addr reg indir */
		dprint("(A%d)", reg);
		return 0;

	case 3:			/* addr reg indir incr */
		dprint("(A%d)+", reg);
		return 0;

	case 4:			/* addr reg indir decr */
		dprint("-(A%d)", reg);
		return 0;

	case 5:			/* addr reg indir with displ */
		get2(mymap, dot+dotinc, dsp, &s);
		disp = ENSIGN(s);
		dotinc += 2;
		psymoff(disp, dsp, "");
		dprint("(A%d)", reg);
		if (reg == 6) {
			dprint(".");
			psymoff(disp+mach->sb, SEGDATA, "");
		}
		return 2;

	case 6:			/* wretched indexing */
		return doindex(reg);	/* ugh */

	case 7:			/* non-register stuff: */
		switch ((int)reg) {
		case 0:		/* absolute short */
			get2(mymap, dot+dotinc, dsp, &s);
			disp = ENSIGN(s);
			dotinc += 2;
			psymoff(disp, dsp, "");
			dprint("($0)");
			return 2;

		case 1:		/* absolute long */
			get4(mymap, dot+dotinc, dsp, &disp);
			dotinc += 4;
			psymoff(disp, dsp, "");
			dprint("($0)");
			return 4;

		case 4:		/* immediate */
			switch((int)size) {
			case B:
				get1(mymap, dot+dotinc, dsp, &ch, 1);
				disp = ENSIGN(ch);
				dotinc += 2;	/* sic */
				dprint("$");
				psymoff(disp, dsp, "");
				return 2;

			case W:
				get2(mymap, dot+dotinc, dsp, &s);
				disp = ENSIGN(s);
				dotinc += 2;
				dprint("$");
				psymoff(disp, dsp, "");
				return 2;

			case L:
				get4(mymap, dot+dotinc, dsp, &disp);
				dotinc += 4;
				dprint("$");
				psymoff(disp, dsp, "");
				return 4;

			case D:
				get1(mymap, dot+dotinc, dsp, (uchar*)&h, 4);
				get1(mymap, dot+dotinc+4, dsp, (uchar*)&l, 4);
				dprint("$%s", ieeedtos("%g", h, l));
				dotinc += 8;
				return 8;

			case F:
				get1(mymap, dot+dotinc, dsp, (uchar*)&h, 4);
				dprint("$%s", ieeeftos("%g", h));
				dotinc += 4;
				return 4;
			}
		}
	}
	dprint("gok%d:%d", mode, reg);
	return 0;
}

doindex(int reg)
{
	ulong w;
	ushort s;
	long base, outer;
	int inc;

	base = outer = 0;
	get2(mymap, dot+dotinc, dsp, &s);
	w = s;
	inc = 2;
	if ((w & 0x100) == 0) {		/* brief format */
		base = w & 0x7f;
		if (base & 0x40)
			base |= ~0x7f;
	}
	else {				/* full format */
		switch ((int)(w & 0x30)) {
		case 0:			/* ugh */
		case 0x10:		/* null displacement */
			break;

		case 0x20:
			get2(mymap, dot+dotinc+inc, dsp, &s);
			base = s;
			if (base & 0x8000)
				base |= ~0xFFFF;	/* sign extend */
			get2(mymap, dot+dotinc+inc+2, dsp, &s);
			outer = s;
			if (!(w&0x80))
				inc += 2;	/* for base */
			if ((w&7)==2 || (w&7)==3)
				inc += 2;	/* for outer */
			break;

		case 0x30:
			get4(mymap, dot+dotinc+inc, dsp, &base);
			get4(mymap, dot+dotinc+inc+4, dsp, &outer);
			if (!(w&0x80))
				inc += 4;	/* for base */
			if ((w&7)==2 || (w&7)==3)
				inc += 2;	/* for outer */
			break;
		}
	}
	if ((w & 0x100) && (w & 0x47)) {
		if ((w&7)==2 || (w&7)==3 && outer)
			psymoff(outer, dsp, "");
		dprint("(");
	}
	if (base)
		psymoff(base, dsp, "");
	dprint("(A%d)", reg);
	if ((w & 0x100) && (w & 0x4))
		dprint(")");
	if (reg == 6) {
		dprint(".");
		psymoff(base+mach->sb, SEGDATA, "");
	}
	if (!(w & 0x40)) {
		dprint("(%c%d.%c", w&0100000 ? 'A' : 'R', (int)(w>>12)&07,
				w&04000 ? 'L' : 'W');
		dprint("*%ld)", (ulong)1<<((w>>9)&03));
	}
	if ((w & 0x100) && (w & 0x43) && !(w & 0x4))
		dprint(")");
	dotinc += inc;
	return inc;
}

char *reg0tab[]={
	"SFC",		/* 000 */
	"DFC",		/* 001 */
	"CACR",		/* 002 */
	"TC",		/* 003 */
	"ITT0",		/* 004 */
	"ITT1",		/* 005 */
	"DTT0",		/* 006 */
	"DTT1",		/* 007 */
};

char *reg8tab[]={
	"USP",		/* 800 */
	"VBR",		/* 801 */
	"CAAR",		/* 802 */
	"MSP",		/* 803 */
	"ISP",		/* 804 */
	"MMUSR",	/* 805 */
	"URP",		/* 806 */
	"SRP",		/* 807 */
};

void
dcr(int reg)
{
	if((reg&0xFF8) == 0x800)
		dprint(reg8tab[reg&0x7]);
	else if((reg&0xFF8) == 0)
		dprint(reg0tab[reg&0x7]);
	else
		dprint("CR%ux", reg);
}

static struct{
	int	c;
	char	*name;
}fcrtab[]={
	0x00,	"C_PI",
	0x0b,	"C_LOG10(2)",
	0x0c,	"C_E",
	0x0d,	"C_LOG2(E)",
	0x0e,	"C_LOG10(E)",
	0x0f,	"C_0.0",
	0x30,	"C_LOGN(2)",
	0x31,	"C_LOGN(10)",
	0x32,	"C_TENTO0",
	0x33,	"C_TENTO1",
	0x34,	"C_TENTO2",
	0x35,	"C_TENTO4",
	0x36,	"C_TENTO8",
	0x37,	"C_TENTO16",
	0x38,	"C_TENTO32",
	0x39,	"C_TENTO64",
	0x3a,	"C_TENTO128",
	0x3b,	"C_TENTO256",
	0x3c,	"C_TENTO512",
	0x3d,	"C_TENTO1024",
	0x3e,	"C_TENTO2048",
	0x3f,	"C_TENTO4096",
	0x00,	(char *)0,
};

char *
fcr(int c)
{
	int i;

	for (i=0; fcrtab[i].name; i++)
		if (c == fcrtab[i].c)
			return fcrtab[i].name;
	return "strangeconstant";
}

int
easize(ulong w)
{
	int i;
	ulong w1;
	struct m68020optab *op;

	xprint = 1;
	op = mc68020op(w, &w1, SEGDATA);
	if(op == 0)
		error("unknown instruction");
	i = ea((w >> 3) & 07, w & 07, op->flags & SZ);
	xprint = 0;
	return i;
}

ulong
eaval(ulong w)
{
	int mode, reg;
	char buf[8];

	mode = (w>>3) & 07;
	reg = w & 07;

	switch((int)mode){
	case 2:			/* addr reg indir */
	case 3:			/* addr reg indir incr */
	case 4:			/* addr reg indir decr */
		sprint(buf, "A%d", reg);
		return rget(buf);
	}
	error("68020 address mode too hard");
	return 0;
}

int
m68020foll(ulong pc, ulong *foll)
{

	ulong dpc;
	long w, w1;
	ushort s;
	int inc;

	get2(cormap, pc, SEGDATA, &s);
	w = s;
	if((w&0xF000) == 0x6000){	/* Bcc or BRA or BSR */
		switch(w & 0x00FF){
		default:		/* 8-bit displ */
			inc = 2;
			dpc = (schar)(w&0xFF);
			break;
		case 0x00:		/* 16-bit displ */
			get2(cormap, pc+2, SEGDATA, &s);
			w1 = s;
			inc = 4;
			dpc = (short)(w1);
			break;
		case 0xFF:	
			get4(cormap, pc+2, SEGDATA, &w1);
			inc = 6;
			dpc = w1;
			break;
		}
		foll[0] = pc + inc;
		foll[1] = pc + 2 + dpc;
		return 2;
	}
	if((w&0xFFC0) == 0x06C0){	/* CALLM */
		dpc = eaval(w);
		foll[0] = pc + 2 + dpc;
		return 1;
	}
	if((w&0xF180) == 0xF080){	/* cpBcc */
		if(w & 0x40){
			get2(cormap, pc+4, SEGDATA, &s);
			w1 = s;
			inc = 6;
			dpc = (short)(w1);
		}else{
			get4(cormap, pc+4, SEGDATA, &w1);
			inc = 8;
			dpc = w1;
		}
		foll[0] = pc + inc;
		foll[1] = pc + 4 + dpc;
		return 2;
	}
	if((w&0xF1F8) == 0xF048){	/* cpDBcc */
		get2(cormap, pc+6, SEGDATA, &s);
		w1 = s;
		inc = 8;
		dpc = (short)(w1);
		foll[0] = pc + inc;
		foll[1] = pc + 2 + dpc;
		return 2;
	}
	if((w&0xF0F8) == 0x50C8){	/* DBcc */
		get2(cormap, pc+2, SEGDATA, &s);
		w1 = s;
		inc = 4;
		dpc = (short)(w1);
		foll[0] = pc + inc;
		foll[1] = pc + 2 + dpc;
		return 2;
	}
	if((w&0xFF80) == 0x4E80){	/* JMP or JSR */
		dpc = eaval(w);
		foll[0] = pc + 2 + dpc;
		return 1;
	}
	if(w==0x4E77 || w==0x4E75){	/* RTR, RTS */
		get4(cormap, rget("A7"), SEGDATA, &w1);
		foll[0] = w1;
		return 1;
	}
	foll[0] = pc + instrsize(pc);
	return 1;
}
