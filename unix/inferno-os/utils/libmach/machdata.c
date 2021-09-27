/*
 * Debugger utilities shared by at least two architectures
 */

#include <lib9.h>
#include <bio.h>
#include "mach.h"

#define STARTSYM	"_main"
#define PROFSYM		"_mainp"
#define	FRAMENAME	".frame"

extern	Machdata	mipsmach;

int	asstype = AMIPS;		/* disassembler type */
Machdata *machdata;		/* machine-dependent functions */

int
localaddr(Map *map, char *fn, char *var, uvlong *r, Rgetter rget)
{
	Symbol s;
	uvlong fp, pc, sp, link;

	if (!lookup(fn, 0, &s)) {
		werrstr("function not found");
		return -1;
	}
	pc = rget(map, mach->pc);
	sp = rget(map, mach->sp);
	if(mach->link)
		link = rget(map, mach->link);
	else
		link = 0;
	fp = machdata->findframe(map, s.value, pc, sp, link);
	if (fp == 0) {
		werrstr("stack frame not found");
		return -1;
	}

	if (!var || !var[0]) {
		*r = fp;
		return 1;
	}

	if (findlocal(&s, var, &s) == 0) {
		werrstr("local variable not found");
		return -1;
	}

	switch (s.class) {
	case CAUTO:
		*r = fp - s.value;
		break;
	case CPARAM:		/* assume address size is stack width */
		*r = fp + s.value + mach->szaddr;
		break;
	default:
		werrstr("local variable not found: %d", s.class);
		return -1;
	}
	return 1;
}

/*
 * Print value v as s.name[+offset] if possible, or just v.
 */
int
symoff(char *buf, int n, uvlong v, int space)
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
	if (v == 0 || r == 0)
		return snprint(buf, n, "%llux", v);
	if (s.type != 't' && s.type != 'T' && delta >= 4096)
		return snprint(buf, n, "%llux", v);
	else if (strcmp(s.name, ".string") == 0)
		return snprint(buf, n, "%llux", v);
	else if (delta)
		return snprint(buf, n, "%s+%lux", s.name, delta);
	else
		return snprint(buf, n, "%s", s.name);
}

/*
 *	Format floating point registers
 *
 *	Register codes in format field:
 *	'X' - print as 32-bit hexadecimal value
 *	'F' - 64-bit double register when modif == 'F'; else 32-bit single reg
 *	'f' - 32-bit ieee float
 *	'8' - big endian 80-bit ieee extended float
 *	'3' - little endian 80-bit ieee extended float with hole in bytes 8&9
 */
int
fpformat(Map *map, Reglist *rp, char *buf, int n, int modif)
{
	char reg[12];
	ulong r;

	switch(rp->rformat)
	{
	case 'X':
		if (get4(map, rp->roffs, &r) < 0)
			return -1;
		snprint(buf, n, "%lux", r);
		break;
	case 'F':	/* first reg of double reg pair */
		if (modif == 'F')
		if ((rp->rformat=='F') || (((rp+1)->rflags&RFLT) && (rp+1)->rformat == 'f')) {
			if (get1(map, rp->roffs, (uchar *)reg, 8) < 0)
				return -1;
			machdata->dftos(buf, n, reg);
			if (rp->rformat == 'F')
				return 1;
			return 2;
		}	
			/* treat it like 'f' */
		if (get1(map, rp->roffs, (uchar *)reg, 4) < 0)
			return -1;
		machdata->sftos(buf, n, reg);
		break;
	case 'f':	/* 32 bit float */
		if (get1(map, rp->roffs, (uchar *)reg, 4) < 0)
			return -1;
		machdata->sftos(buf, n, reg);
		break;
	case '3':	/* little endian ieee 80 with hole in bytes 8&9 */
		if (get1(map, rp->roffs, (uchar *)reg, 10) < 0)
			return -1;
		memmove(reg+10, reg+8, 2);	/* open hole */
		memset(reg+8, 0, 2);		/* fill it */
		leieee80ftos(buf, n, reg);
		break;
	case '8':	/* big-endian ieee 80 */
		if (get1(map, rp->roffs, (uchar *)reg, 10) < 0)
			return -1;
		beieee80ftos(buf, n, reg);
		break;
	default:	/* unknown */
		break;
	}
	return 1;
}

char *
_hexify(char *buf, ulong p, int zeros)
{
	ulong d;

	d = p/16;
	if(d)
		buf = _hexify(buf, d, zeros-1);
	else
		while(zeros--)
			*buf++ = '0';
	*buf++ = "0123456789abcdef"[p&0x0f];
	return buf;
}

/*
 * These routines assume that if the number is representable
 * in IEEE floating point, it will be representable in the native
 * double format.  Naive but workable, probably.
 */
int
ieeedftos(char *buf, int n, ulong h, ulong l)
{
	double fr;
	int exp;

	if (n <= 0)
		return 0;


	if(h & (1L<<31)){
		*buf++ = '-';
		h &= ~(1L<<31);
	}else
		*buf++ = ' ';
	n--;
	if(l == 0 && h == 0)
		return snprint(buf, n, "0.");
	exp = (h>>20) & ((1L<<11)-1L);
	if(exp == 0)
		return snprint(buf, n, "DeN(%.8lux%.8lux)", h, l);
	if(exp == ((1L<<11)-1L)){
		if(l==0 && (h&((1L<<20)-1L)) == 0)
			return snprint(buf, n, "Inf");
		else
			return snprint(buf, n, "NaN(%.8lux%.8lux)", h&((1L<<20)-1L), l);
	}
	exp -= (1L<<10) - 2L;
	fr = l & ((1L<<16)-1L);
	fr /= 1L<<16;
	fr += (l>>16) & ((1L<<16)-1L);
	fr /= 1L<<16;
	fr += (h & (1L<<20)-1L) | (1L<<20);
	fr /= 1L<<21;
	fr = ldexp(fr, exp);
	return snprint(buf, n, "%.18g", fr);
}

int
ieeesftos(char *buf, int n, ulong h)
{
	double fr;
	int exp;

	if (n <= 0)
		return 0;

	if(h & (1L<<31)){
		*buf++ = '-';
		h &= ~(1L<<31);
	}else
		*buf++ = ' ';
	n--;
	if(h == 0)
		return snprint(buf, n, "0.");
	exp = (h>>23) & ((1L<<8)-1L);
	if(exp == 0)
		return snprint(buf, n, "DeN(%.8lux)", h);
	if(exp == ((1L<<8)-1L)){
		if((h&((1L<<23)-1L)) == 0)
			return snprint(buf, n, "Inf");
		else
			return snprint(buf, n, "NaN(%.8lux)", h&((1L<<23)-1L));
	}
	exp -= (1L<<7) - 2L;
	fr = (h & ((1L<<23)-1L)) | (1L<<23);
	fr /= 1L<<24;
	fr = ldexp(fr, exp);
	return snprint(buf, n, "%.9g", fr);
}

int
beieeesftos(char *buf, int n, void *s)
{
	return ieeesftos(buf, n, beswal(*(ulong*)s));
}

int
beieeedftos(char *buf, int n, void *s)
{
	return ieeedftos(buf, n, beswal(*(ulong*)s), beswal(((ulong*)(s))[1]));
}

int
leieeesftos(char *buf, int n, void *s)
{
	return ieeesftos(buf, n, leswal(*(ulong*)s));
}

int
leieeedftos(char *buf, int n, void *s)
{
	return ieeedftos(buf, n, leswal(((ulong*)(s))[1]), leswal(*(ulong*)s));
}

/* packed in 12 bytes, with s[2]==s[3]==0; mantissa starts at s[4]*/
int
beieee80ftos(char *buf, int n, void *s)
{
	uchar *reg = (uchar*)s;
	int i;
	ulong x;
	uchar ieee[8+8];	/* room for slop */
	uchar *p, *q;

	memset(ieee, 0, sizeof(ieee));
	/* sign */
	if(reg[0] & 0x80)
		ieee[0] |= 0x80;

	/* exponent */
	x = ((reg[0]&0x7F)<<8) | reg[1];
	if(x == 0)		/* number is ±0 */
		goto done;
	if(x == 0x7FFF){
		if(memcmp(reg+4, ieee+1, 8) == 0){ /* infinity */
			x = 2047;
		}else{				/* NaN */
			x = 2047;
			ieee[7] = 0x1;		/* make sure */
		}
		ieee[0] |= x>>4;
		ieee[1] |= (x&0xF)<<4;
		goto done;
	}
	x -= 0x3FFF;		/* exponent bias */
	x += 1023;
	if(x >= (1<<11) || ((reg[4]&0x80)==0 && x!=0))
		return snprint(buf, n, "not in range");
	ieee[0] |= x>>4;
	ieee[1] |= (x&0xF)<<4;

	/* mantissa */
	p = reg+4;
	q = ieee+1;
	for(i=0; i<56; i+=8, p++, q++){	/* move one byte */
		x = (p[0]&0x7F) << 1;
		if(p[1] & 0x80)
			x |= 1;
		q[0] |= x>>4;
		q[1] |= (x&0xF)<<4;
	}
    done:
	return beieeedftos(buf, n, (void*)ieee);
}

int
leieee80ftos(char *buf, int n, void *s)
{
	int i;
	char *cp;
	char b[12];

	cp = (char*) s;
	for(i=0; i<12; i++)
		b[11-i] = *cp++;
	return beieee80ftos(buf, n, b);
}

int
cisctrace(Map *map, uvlong pc, uvlong sp, uvlong link, Tracer trace)
{
	Symbol s;
	int found, i;
	uvlong opc, moved;

	USED(link);
	i = 0;
	opc = 0;
	while(pc && opc != pc) {
		moved = pc2sp(pc);
		if (moved == ~0)
			break;
		found = findsym(pc, CTEXT, &s);
		if (!found)
			break;
		if(strcmp(STARTSYM, s.name) == 0 || strcmp(PROFSYM, s.name) == 0)
			break;

		sp += moved;
		opc = pc;
		if (geta(map, sp, &pc) < 0)
			break;
		(*trace)(map, pc, sp, &s);
		sp += mach->szaddr;	/*assumes address size = stack width*/
		if(++i > 40)
			break;
	}
	return i;
}

int
risctrace(Map *map, uvlong pc, uvlong sp, uvlong link, Tracer trace)
{
	int i;
	Symbol s, f;
	uvlong oldpc;

	i = 0;
	while(findsym(pc, CTEXT, &s)) {
		if(strcmp(STARTSYM, s.name) == 0 || strcmp(PROFSYM, s.name) == 0)
			break;

		if(pc == s.value)	/* at first instruction */
			f.value = 0;
		else if(findlocal(&s, FRAMENAME, &f) == 0)
			break;

		oldpc = pc;
		if(s.type == 'L' || s.type == 'l' || pc <= s.value+mach->pcquant)
			pc = link;
		else
			if (geta(map, sp, &pc) < 0)
				break;

		if(pc == 0 || (pc == oldpc && f.value == 0))
			break;

		sp += f.value;
		(*trace)(map, pc-8, sp, &s);

		if(++i > 40)
			break;
	}
	return i;
}

uvlong
ciscframe(Map *map, uvlong addr, uvlong pc, uvlong sp, uvlong link)
{
	Symbol s;
	uvlong moved;

	USED(link);
	for(;;) {
		moved = pc2sp(pc);
		if (moved  == ~0)
			break;
		sp += moved;
		findsym(pc, CTEXT, &s);
		if (addr == s.value)
			return sp;
		if (geta(map, sp, &pc) < 0)
			break;
		sp += mach->szaddr;	/*assumes sizeof(addr) = stack width*/
	}
	return 0;
}

uvlong
riscframe(Map *map, uvlong addr, uvlong pc, uvlong sp, uvlong link)
{
	Symbol s, f;

	while (findsym(pc, CTEXT, &s)) {
		if(strcmp(STARTSYM, s.name) == 0 || strcmp(PROFSYM, s.name) == 0)
			break;

		if(pc == s.value)	/* at first instruction */
			f.value = 0;
		else
		if(findlocal(&s, FRAMENAME, &f) == 0)
			break;

		sp += f.value;
		if (s.value == addr)
			return sp;

		if (s.type == 'L' || s.type == 'l' || pc-s.value <= mach->szaddr*2)
			pc = link;
		else
		if (geta(map, sp-f.value, &pc) < 0)
			break;
	}
	return 0;
}
