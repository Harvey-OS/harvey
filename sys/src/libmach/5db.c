#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>

#define ROR(v, s)	(((ulong)(v) >> (s)) | (((v) & ((1 << (s))-1)) << (32 - (s))))

typedef struct	Instr	Instr;
struct	Instr
{
	Map	*map;
	ulong	w;
	ulong	addr;
	uchar	op;			/* super opcode */
	uchar	class;			/* bits 25-27 */

	uchar	cond;			/* bits 28-31 */
	uchar	store;			/* bit 20 (sometimes with bits 22 & 24) */

	uchar	rd;			/* bits 12-15 */
	uchar	rn;			/* bits 16-19 */
	uchar	rs;			/* bits 0-3 */

	long	imm;			/* rotated imm */
	char*	curr;			/* fill point in buffer */
	char*	end;			/* end of buffer */
	char*	err;			/* error message */
};

typedef struct Opcode Opcode;
struct Opcode
{
	char*	o;
	void	(*f)(Opcode*, Instr*);
	char*	a;
};

static	void	format(char*, Instr*, char*);
static	char	FRAMENAME[] = ".frame";

/*
 * Arm-specific debugger interface
 */

static	int	armfoll(Map*, ulong, Rgetter, ulong*);
static	int	arminst(Map*, ulong, char, char*, int);
static	int	armdas(Map*, ulong, char*, int);
static	int	arminstlen(Map*, ulong);

/*
 *	Debugger interface
 */
Machdata armmach =
{
	{0, 0, 0, 0xD},		/* break point */
	4,			/* break point size */

	leswab,			/* short to local byte order */
	leswal,			/* long to local byte order */
	leswav,			/* long to local byte order */
	risctrace,		/* C traceback */
	riscframe,		/* Frame finder */
	0,			/* print exception */
	0,			/* breakpoint fixup */
	0,			/* single precision float printer */
	0,			/* double precision float printer */
	armfoll,		/* following addresses */
	arminst,		/* print instruction */
	armdas,			/* dissembler */
	arminstlen,		/* instruction size */
};

static
char*	cond[16] =
{
	"EQ",	"NE",	"CS",	"CC",
	"MI",	"PL",	"VS",	"VC",
	"HI",	"LS",	"GE",	"LT",
	"GT",	"LE",	0,	"NV"
};

static
char*	shtype[4] =
{
	"<<",	">>",	"->",	"@>"
};

int
_armclass(int op, long w)
{
	switch(op) {
	case 0:	/* data processing r,r,r */
		op = ((w >> 4) & 0xf);
		if(op == 0x9) {
			op = 48+16;		/* mul */
			if(w & (1<<24)) {
				op += 2;
				if(w & (1<<22))
					op++;	/* swap */
				break;
			}
			if(w & (1<<21))
				op++;		/* mla */
			break;
		}
		op = (w >> 21) & 0xf;
		if(w & (1<<4))
			op += 32;
		else
		if(w & (31<<7))
			op += 16;
		break;
	case 1:	/* data processing i,r,r */
		op = (48) + ((w >> 21) & 0xf);
		break;
	case 2:	/* load/store byte/word i(r) */
		op = (48+20) + ((w >> 22) & 0x1) + ((w >> 19) & 0x2);
		break;
	case 3:	/* load/store byte/word (r)(r) */
		op = (48+20+4) + ((w >> 22) & 0x1) + ((w >> 19) & 0x2);
		break;
	case 4:	/* block data transfer (r)(r) */
		op = (48+20+4+4) + ((w >> 20) & 0x1);
		break;
	case 5:	/* branch / branch link */
		op = (48+20+4+4+2) + ((w >> 24) & 0x1);
		break;
	case 7:	/* coprocessor crap */
		op = (48+20+4+4+2+2) + ((w >> 3) & 0x2) + ((w >> 20) & 0x1);
		break;
	default:
		op = (48+20+4+4+2+2+4);
		break;
	}
	return op;
}

static int
decode(Map *map, ulong pc, Instr *i)
{
	long w;

	if(get4(map, pc, &w) < 0) {
		werrstr("can't read instruction: %r");
		return -1;
	}
	i->w = w;
	i->addr = pc;
	i->cond = (w >> 28) & 0xf;
	i->class = (w >> 25) & 0x7;
	i->op = _armclass(i->class, w);
	i->map = map;
	i->rd = (w >> 12) & 0xf;
	i->rn = (w >> 16) & 0xf;
	i->rs = (w >> 0) & 0xf;
	return 1;
}

static void
bprint(Instr *i, char *fmt, ...)
{
	va_list arg;

	va_start(arg, fmt);
	i->curr = doprint(i->curr, i->end, fmt, arg);
	va_end(arg);
}

static int
plocal(Instr *i, char *m, char r, int store)
{
	int offset;
	char *reg;
	Symbol s;

	if(!findsym(i->addr, CTEXT, &s) || !findlocal(&s, FRAMENAME, &s))
		return 0;
	if(s.value > i->imm) {
		if(!getauto(&s, s.value-i->imm, CAUTO, &s))
			return 0;
		reg = "(SP)";
		offset = i->imm;
	} else {
		offset = i->imm-s.value;
		if(!getauto(&s, offset-4, CPARAM, &s))
			return 0;
		reg = "(FP)";
	}
	if(store)
		bprint(i, "%s\t%c%d,%s+%d%s", m, r, i->rn, s.name, offset, reg);
	else
		bprint(i, "%s\t%s+%d%s,%c%d", m, s.name, offset, reg, r, i->rn);
	return 1;
}

static void
armdps(Opcode *o, Instr *i)
{
	i->store = (i->w >> 20) & 1;
	if(i->rn == 15 && i->rs == 0) {
		if(i->op == 8) {
			format("MOVW", i,"CPSR, R%d");
			return;
		} else
		if(i->op == 10) {
			format("MOVW", i,"SPSR, R%d");
			return;
		}
	} else
	if(i->rn == 9 && i->rd == 15) {
		if(i->op == 9) {
			format("MOVW", i, "R%s, CPSR");
			return;
		} else
		if(i->op == 11) {
			format("MOVW", i, "R%s, SPSR");
			return;
		}
	}
	format(o->o, i, o->a);
}

static void
armdpi(Opcode *o, Instr *i)
{
	ulong v;
	int c;

	v = i->w & 0xff;
	c = (i->w >> 8) & 0xf;
	while(c) {
		v = (v<<30) | (v>>2);
		c--;
	}
	i->imm = v;
	i->store = (i->w >> 20) & 1;

		/* RET is encoded as ADD #0,R14,R15 */
	if(i->w == 0xe282f000){
		format("RET", i, "");
		return;
	} else
	format(o->o, i, o->a);
}

static void
armsdti(Opcode *o, Instr *i)
{
	ulong v;

	v = i->w & 0xff;
	if(!(i->w & (1<<23)))
		v = -v;
	i->store = ((i->w >> 23) & 0x2) | ((i->w >>21) & 0x1);
	i->imm = v;
		/* convert load of offset(PC) to a load immediate */
	if(i->rn == 15 && (i->w & (1<<20)) && get4(i->map, i->addr+v+8, &i->imm) > 0)
		format(o->o, i, "$#%i,R%d");
	else
		format(o->o, i, o->a);
}

static void
armsdts(Opcode *o, Instr *i)
{
	i->store = ((i->w >> 23) & 0x2) | ((i->w >>21) & 0x1);
	format(o->o, i, o->a);
}

static void
armbdt(Opcode *o, Instr *i)
{
	i->store = (i->w >> 21) & 0x3;		/* S & W bits */
	i->imm = i->w & 0xffff;
	if(i->w == 0xe8fd8000)
		format("RFE", i, "");
	else
		format(o->o, i, o->a);
}

static void
armund(Opcode *o, Instr *i)
{
	format(o->o, i, o->a);
}

static void
armcdt(Opcode *o, Instr *i)
{
	format(o->o, i, o->a);
}

static void
armunk(Opcode *o, Instr *i)
{
	format(o->o, i, o->a);
}

static void
armb(Opcode *o, Instr *i)
{
	ulong v;

	v = i->w & 0xffffff;
	if(v & 0x800000)		/* sign extend */
		v |= ~0xffffff;
	i->imm = (v<<2) + i->addr + 8;
	format(o->o, i, o->a);
}

static void
armco(Opcode *o, Instr *i)		/* coprocessor instructions */
{
	int op, p, cp;

	char buf[1024];

	cp = (i->w >> 8) & 0xf;
	p = (i->w >> 5) & 0x7;
	if(i->w&0x10) {
		op = (i->w >> 20) & 0x0f;
		snprint(buf, sizeof(buf), "#%x, #%x, R%d, C(%d), C(%d), #%x\n", cp, op, i->rd, i->rn, i->rs, p);
	} else {
		op = (i->w >> 21) & 0x07;
		snprint(buf, sizeof(buf), "#%x, #%x, C(%d), C(%d), C(%d), #%x\n", cp, op, i->rd, i->rn, i->rs, p);
	}
	format(o->o, i, buf);
}

static ulong
armshiftval(Rgetter rget, Instr *i)
{
	char buf[8];
	ulong w, s, v;

	w = i->w;
	if(w & (1<<25))				/* immediate */
		return ROR(w&0xff, (w>>7)&0x0f);

	sprint(buf, "R%d", i->rs);
	v = rget(i->map, buf);

	s = (w>>7) & 0x1f;
	switch((w>>4) & 0x07) {
	case 0:					/* LSLIMM */
		return v<<s;
	case 1:					/* LSLREG */
		sprint(buf, "R%ld", s>>1);
		s = rget(i->map, buf) & 0xff;
		if(s >= 32)
			return 0;
		return v<<s;
	case 2:					/* LSRIMM */
		return v>>s;
	case 3:					/* LSRREG */
		sprint(buf, "R%ld", s>>1);
		s = rget(i->map, buf) & 0xff;
		if(s >= 32)
			return 0;
		return v>>s;
	case 4:					/* ASRIMM */
		if(s == 0) {
			if(v & (1<<31))
				return 0xffffffff;
			return 0;
		}
		return ((long)v)>>s;
	case 5:					/* ASRREG */
		sprint(buf, "R%ld", s >> 1);
		s = rget(i->map, buf) & 0xff;
		if(s >= 32) {
			if(v & (1<<31))
				return 0xffffffff;
			return 0;
		}
		return ((long)v)>>s;
	case 6:					/* RORIMM */
		if(s == 0)
			return (((long)rget(i->map, "PSR") & (1<<29))<<2) | (v>>1);
		return ROR(v, s);
	case 7:					/* RORREG */
		sprint(buf, "R%ld", s>>1);
		s = rget(i->map, buf) & 0xf;
		if(s == 0)
			return v;
		return ROR(v, s);
	}
	return 0;
}

static ulong
armaddr(Rgetter rget, Instr *i)
{
	char buf[8];
	ulong w, rn, rm, offset;
	int s;

	w = i->w;

	sprint(buf, "R%d", i->rn);
	rn = rget(i->map, buf);

	if((w & (1<<24)) == 0)			/* POSTIDX */
		return rn;

	if((w & (1<<25)) == 0)			/* IMMEDIATE */
		offset = (w & 0xfff);
	else {					/* REGOFF */
		sprint(buf, "R%d", i->rs);
		rm = rget(i->map, buf);
		s = (w>>7) & 0x1f;
		switch((w>>5) & 0x3) {
		case 0:
			offset = rm << s;
			break;
		case 1:
			offset = rm >> s;
			break;
		case 2:
			offset = ((long)rm) >> s;
			break;
		case 3:
		default:
			if(s == 0)
				offset = (((long)rget(i->map, "PSR") & (1<<29))<<2) | (rm>>1);
			else
				offset = ROR(rm, s);
			break;
		}
	}
	if(w & (1<<23))
		return rn + offset;
	return rn - offset;
}

static ulong
movmea(Rgetter rget, Instr *i)
{
	char buf[8];
	ulong w, addr, nb, j;

	w = i->w;

	sprint(buf,"R%d", i->rn);
	addr = rget(i->map, buf);

		/* nb = number of one bits in low 14 bits */
	nb = 0;
	for (j = (w & 0x7fff); j; j >>= 1)
		if(j & 0x01)
			nb++;

	switch((w>>23) & 0x03){
	case 1:				/* POSTINC */
		addr += nb*4;
		break;
	case 2:				/* PREDEC */
		addr -= 4;
		break;
	case 3:				/* PREINC */
		addr += nb*4+4;
		break;
	case 0:				/* POSTDEC */
	default:
		break;
	}
	return addr;
}

static Opcode opcodes[] =
{
	"AND%C%S",	armdps,	"R%s,R%n,R%d",
	"EOR%C%S",	armdps,	"R%s,R%n,R%d",
	"SUB%C%S",	armdps,	"R%s,R%n,R%d",
	"RSB%C%S",	armdps,	"R%s,R%n,R%d",
	"ADD%C%S",	armdps,	"R%s,R%n,R%d",
	"ADC%C%S",	armdps,	"R%s,R%n,R%d",
	"SBC%C%S",	armdps,	"R%s,R%n,R%d",
	"RSC%C%S",	armdps,	"R%s,R%n,R%d",
	"TST%C%S",	armdps,	"R%s,R%n,",
	"TEQ%C%S",	armdps,	"R%s,R%n,",
	"CMP%C%S",	armdps,	"R%s,R%n,",
	"CMN%C%S",	armdps,	"R%s,R%n,",
	"ORR%C%S",	armdps,	"R%s,R%n,R%d",
	"MOVW%C%S",	armdps,	"R%s,R%d",
	"BIC%C%S",	armdps,	"R%s,R%n,R%d",
	"MVN%C%S",	armdps,	"R%s,R%d",

	"AND%C%S",	armdps,	"(R%s%h#%m),R%n,R%d",
	"EOR%C%S",	armdps,	"(R%s%h#%m),R%n,R%d",
	"SUB%C%S",	armdps,	"(R%s%h#%m),R%n,R%d",
	"RSB%C%S",	armdps,	"(R%s%h#%m),R%n,R%d",
	"ADD%C%S",	armdps,	"(R%s%h#%m),R%n,R%d",
	"ADC%C%S",	armdps,	"(R%s%h#%m),R%n,R%d",
	"SBC%C%S",	armdps,	"(R%s%h#%m),R%n,R%d",
	"RSC%C%S",	armdps,	"(R%s%h#%m),R%n,R%d",
	"TST%C%S",	armdps,	"(R%s%h#%m),R%n,",
	"TEQ%C%S",	armdps,	"(R%s%h#%m),R%n,",
	"CMP%C%S",	armdps,	"(R%s%h#%m),R%n,",
	"CMN%C%S",	armdps,	"(R%s%h#%m),R%n,",
	"ORR%C%S",	armdps,	"(R%s%h#%m),R%n,R%d",
	"MOVW%C%S",	armdps,	"(R%s%h#%m),R%d",
	"BIC%C%S",	armdps,	"(R%s%h#%m),R%n,R%d",
	"MVN%C%S",	armdps,	"(R%s%h#%m),R%d",

	"AND%C%S",	armdps,	"(R%s%hR%m),R%n,R%d",
	"EOR%C%S",	armdps,	"(R%s%hR%m),R%n,R%d",
	"SUB%C%S",	armdps,	"(R%s%hR%m),R%n,R%d",
	"RSB%C%S",	armdps,	"(R%s%hR%m),R%n,R%d",
	"ADD%C%S",	armdps,	"(R%s%hR%m),R%n,R%d",
	"ADC%C%S",	armdps,	"(R%s%hR%m),R%n,R%d",
	"SBC%C%S",	armdps,	"(R%s%hR%m),R%n,R%d",
	"RSC%C%S",	armdps,	"(R%s%hR%m),R%n,R%d",
	"TST%C%S",	armdps,	"(R%s%hR%m),R%n,",
	"TEQ%C%S",	armdps,	"(R%s%hR%m),R%n,",
	"CMP%C%S",	armdps,	"(R%s%hR%m),R%n,",
	"CMN%C%S",	armdps,	"(R%s%hR%m),R%n,",
	"ORR%C%S",	armdps,	"(R%s%hR%m),R%n,R%d",
	"MOVW%C%S",	armdps,	"(R%s%hR%m),R%d",
	"BIC%C%S",	armdps,	"(R%s%hR%m),R%n,R%d",
	"MVN%C%S",	armdps,	"(R%s%hR%m),R%d",

	"AND%C%S",	armdpi,	"$#%i,R%n,R%d",
	"EOR%C%S",	armdpi,	"$#%i,R%n,R%d",
	"SUB%C%S",	armdpi,	"$#%i,R%n,R%d",
	"RSB%C%S",	armdpi,	"$#%i,R%n,R%d",
	"ADD%C%S",	armdpi,	"$#%i,R%n,R%d",
	"ADC%C%S",	armdpi,	"$#%i,R%n,R%d",
	"SBC%C%S",	armdpi,	"$#%i,R%n,R%d",
	"RSC%C%S",	armdpi,	"$#%i,R%n,R%d",
	"TST%C%S",	armdpi,	"$#%i,R%n,",
	"TEQ%C%S",	armdpi,	"$#%i,R%n,",
	"CMP%C%S",	armdpi,	"$#%i,R%n,",
	"CMN%C%S",	armdpi,	"$#%i,R%n,",
	"ORR%C%S",	armdpi,	"$#%i,R%n,R%d",
	"MOVW%C%S",	armdpi,	"$#%i,,R%d",
	"BIC%C%S",	armdpi,	"$#%i,R%n,R%d",
	"MVN%C%S",	armdpi,	"$#%i,,R%d",

	"MUL%C%S",	armdpi,	"R%s,R%m,R%n",
	"MULA%C%S",	armdpi,	"R%s,R%m,R%n,R%d",
	"SWPW",		armdpi,	"R%s,(R%n),R%d",
	"SWPB",		armdpi,	"R%s,(R%n),R%d",

	"MOVW%C%p",	armsdti,"R%d,#%i(R%n)",
	"MOVB%C%p",	armsdti,"R%d,#%i(R%n)",
	"MOVW%C%p",	armsdti,"#%i(R%n),R%d",
	"MOVB%C%p",	armsdti,"#%i(R%n),R%d",

	"MOVW%C%p",	armsdts,"R%d,(R%s%h#%m)(R%n)",
	"MOVB%C%p",	armsdts,"R%d,(R%s%h#%m)(R%n)",
	"MOVW%C%p",	armsdts,"(R%s%h#%m)(R%n),R%d",
	"MOVB%C%p",	armsdts,"(R%s%h#%m)(R%n),R%d",

	"MOVM%C%P%a",	armbdt,	"R%n,[%r]",
	"MOVM%C%P%a",	armbdt,	"[%r],R%n",

	"B%C",		armb,	"%b",
	"BL%C",		armb,	"%b",

	"CDP%C",	armco,	"",
	"CDP%C",	armco,	"",
	"MCR%C",	armco,	"",
	"MRC%C",	armco,	"",

	"UNK",		armunk,	"",
};

static	char *mode[] = { 0, "IA", "DB", "IB" };
static	char *pw[] = { "P", "PW", 0, "W" };
static	char *sw[] = { 0, "W", "S", "SW" };

static void
format(char *mnemonic, Instr *i, char *f)
{
	int j, k, m, n;

	if(mnemonic)
		format(0, i, mnemonic);
	if(f == 0)
		return;
	if(mnemonic)
		if(i->curr < i->end)
			*i->curr++ = '\t';
	for ( ; *f && i->curr < i->end; f++) {
		if(*f != '%') {
			*i->curr++ = *f;
			continue;
		}
		switch (*++f) {

		case 'C':	/* .CONDITION */
			if(cond[i->cond])
				bprint(i, ".%s", cond[i->cond]);
			break;

		case 'S':	/* .STORE */
			if(i->store)
				bprint(i, ".S");
			break;

		case 'P':	/* P & U bits for block move */
			n = (i->w >>23) & 0x3;
			if (mode[n])
				bprint(i, ".%s", mode[n]);
			break;

		case 'p':	/* P & W bits for single data xfer*/
			if (pw[i->store])
				bprint(i, ".%s", pw[i->store]);
			break;

		case 'a':	/* S & W bits for single data xfer*/
			if (sw[i->store])
				bprint(i, ".%s", sw[i->store]);
			break;

		case 's':
			bprint(i, "%d", i->rs & 0xf);
			break;
				
		case 'm':
			bprint(i, "%d", (i->w>>7) & 0x1f);
			break;

		case 'h':
			bprint(i, "%s", shtype[(i->w>>5) & 0x3]);
			break;

		case 'n':
			bprint(i, "%d", i->rn);
			break;

		case 'd':
			bprint(i, "%d", i->rd);
			break;

		case 'i':
			bprint(i, "%lux", i->imm);
			break;

		case 'b':
			i->curr += symoff(i->curr, i->end-i->curr,
				i->imm, CANY);
			break;

		case 'r':
			n = i->imm&0xffff;
			j = 0;
			k = 0;
			while(n) {
				m = j;
				while(n&0x1) {
					j++;
					n >>= 1;
				}
				if(j != m) {
					if(k)
						bprint(i, ",");
					if(j == m+1)
						bprint(i, "R%d", m);
					else
						bprint(i, "R%d-R%d", m, j-1);
					k = 1;
				}
				j++;
				n >>= 1;
			}
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
printins(Map *map, ulong pc, char *buf, int n)
{
	Instr i;

	i.curr = buf;
	i.end = buf+n-1;
	if(decode(map, pc, &i) < 0)
		return -1;

	(*opcodes[i.op].f)(&opcodes[i.op], &i);
	return 4;
}

static int
arminst(Map *map, ulong pc, char, char *buf, int n)
{
	return printins(map, pc, buf, n);
}

static int
armdas(Map *map, ulong pc, char *buf, int n)
{
	Instr i;

	i.curr = buf;
	i.end = buf+n;
	if(decode(map, pc, &i) < 0)
		return -1;
	if(i.end-i.curr > 8)
		i.curr = _hexify(buf, i.w, 7);
	*i.curr = 0;
	return 4;
}

static int
arminstlen(Map *map, ulong pc)
{
	Instr i;

	if(decode(map, pc, &i) < 0)
		return -1;
	return 4;
}

static int
armfoll(Map *map, ulong pc, Rgetter rget, ulong *foll)
{
	char buf[8];
	ulong w, v;
	Instr i;

	if(decode(map, pc, &i) < 0)
		return -1;

	w = i.w;

	foll[0] = pc+4;

	switch(i.class){
	case 0:
	case 1:			/* ADD -> PC or MOV -> PC */
		if(i.rd != 15)		/* rd can't be R15 on MUL */
			break;
		v = (w>>21) & 0x0f;
		if(v == 4){				/* ADD */
			sprint(buf, "R%d", i.rn);
			foll[1] = rget(map, buf) + armshiftval(rget, &i);
		} else
		if(v == 13)				/* MOV */
			foll[1] = armshiftval(rget, &i);
		break;
	case 2:
	case 3:			/* LD -> PC */
		if(i.rd != 15 || (w & (1<<20)) == 0)	/* Not PC or Store */
			break;
		if(get4(map, armaddr(rget, &i), (long*)(&foll[1])) < 0)
			return -1;
		break;
	case 4:			/* LDM  or STM */
		if((w & (0x21<<15)) == 0)		/* STM or PC not dest */
			break;
		if(get4(map, movmea(rget, &i), (long*)(&foll[1])) < 0)
				return -1;
		break;
	case 5:		/* branch or branch & link */
		foll[1] = pc + (((long)w << 8) >> 6) + 8;
		break;
	default:
		break;
	}
	return 1;
}
