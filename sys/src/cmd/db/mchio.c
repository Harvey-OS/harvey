/*
 * read or print machine-dependent data structures
 * generic non-cross-machine version
 */

#include "defs.h"
#include "fns.h"

/*
 * read a rune enclosed in quotes.  if more than one rune is between the
 * quotes, the trailing ones are discarded.
 */

extern char lastc;

WORD
ascval(void)
{
	Rune r;
	int i;
	char buf[UTFmax+1];

	for (i = 0; i < UTFmax; i++) {	/* extract a rune */
		if (fullrune(buf, i))
			break;
		if (readchar() == 0)
			return (0);
		buf[i] = lastc;
	}
	buf[i] = 0;
	chartorune(&r, buf);
	while(quotchar())	/*discard chars to ending quote */
		;
	return((WORD) r);
}

/*
 * read a floating point number
 * the result must fit in a WORD
 */

WORD
fpin(char *buf)
{
	union {
		WORD w;
		float f;
	} x;

	x.f = atof(buf);
	return (x.w);
}


/*
 * print a date
 */

void
printdate(WORD tvec)
{
	char *p;

	p = ctime(tvec);
	p[strlen(p)-1] = 0;		/* stomp on newline */
	dprint("%-25s", p);
}

static char fpbuf[64];

/*
 * These routines assume that if the number is representable
 * in IEEE floating point, it will be representable in the native
 * double format.  Naive but workable, probably.
 */
char*
ieeedtos(char *fmt, ulong h, ulong l)
{
	double fr;
	int exp;
	char *p = fpbuf;

	if(h & (1L<<31)){
		*p++ = '-';
		h &= ~(1L<<31);
	}else
		*p++ = ' ';
	if(l == 0 && h == 0){
		strcpy(p, "0.");
		goto ret;
	}
	exp = (h>>20) & ((1L<<11)-1L);
	if(exp == 0){
		sprint(p, "DeN(%.8lux%.8lux)", h, l);
		goto ret;
	}
	if(exp == ((1L<<11)-1L)){
		if(l==0 && (h&((1L<<20)-1L)) == 0)
			sprint(p, "Inf");
		else
			sprint(p, "NaN(%.8lux%.8lux)", h&((1L<<20)-1L), l);
		goto ret;
	}
	exp -= (1L<<10) - 2L;
	fr = l & ((1L<<16)-1L);
	fr /= 1L<<16;
	fr += (l>>16) & ((1L<<16)-1L);
	fr /= 1L<<16;
	fr += (h & (1L<<20)-1L) | (1L<<20);
	fr /= 1L<<21;
	fr = ldexp(fr, exp);
	sprint(p, fmt, fr);
    ret:
	return fpbuf;
}

char*
ieeeftos(char *fmt, ulong h)
{
	double fr;
	int exp;
	char *p = fpbuf;

	if(h & (1L<<31)){
		*p++ = '-';
		h &= ~(1L<<31);
	}else
		*p++ = ' ';
	if(h == 0){
		strcpy(p, "0.");
		goto ret;
	}
	exp = (h>>23) & ((1L<<8)-1L);
	if(exp == 0){
		sprint(p, "DeN(%.8lux)", h);
		goto ret;
	}
	if(exp == ((1L<<8)-1L)){
		if((h&((1L<<23)-1L)) == 0)
			sprint(p, "Inf");
		else
			sprint(p, "NaN(%.8lux)", h&((1L<<23)-1L));
		goto ret;
	}
	exp -= (1L<<7) - 2L;
	fr = (h & ((1L<<23)-1L)) | (1L<<23);
	fr /= 1L<<24;
	fr = ldexp(fr, exp);
	sprint(p, fmt, fr);
    ret:
	return fpbuf;
}

void
ieeesfpout(ulong h)
{
	dprint("%s", ieeeftos("%.9g", h));
}

void
ieeedfpout(ulong h, ulong l)
{
	dprint("%s", ieeedtos("%.18g", h, l));
}

void
beieeesfpout(char *s)
{
	ieeesfpout(beswal(*(ulong*)s));
}

void
beieeedfpout(char *s)
{
	ieeedfpout(beswal(*(ulong*)s), beswal(*(ulong*)(s+4)));
}

void
leieeesfpout(char *s)
{
	ieeesfpout(leswal(*(ulong*)s));
}

void
leieeedfpout(char *s)
{
	ieeedfpout(leswal(*(ulong*)(s+4)), leswal(*(ulong*)s));
}

void
beieee80fpout(char *s)	/* packed in 12 bytes, with s[2]==s[3]==0; mantissa starts at s[4]*/
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
		goto Print;
	if(x == 0x7FFF){
		if(memcmp(reg+4, ieee+1, 8) == 0){ /* infinity */
			x = 2047;
		}else{				/* NaN */
			x = 2047;
			ieee[7] = 0x1;		/* make sure */
		}
		ieee[0] |= x>>4;
		ieee[1] |= (x&0xF)<<4;
		goto Print;
	}
	x -= 0x3FFF;		/* exponent bias */
	x += 1023;
	if(x >= (1<<11) || ((reg[4]&0x80)==0 && x!=0)){
		dprint("not in range");
		return;
	}
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
    Print:
	beieeedfpout((char*)ieee);
}

void
leieee80fpout(char *s)
{
	int i, t;

	for(i=0; i<6; i++){
		t = s[i];
		s[i] = s[11-i];
		s[11-i] = t;
	}
	beieee80fpout(s);
}
