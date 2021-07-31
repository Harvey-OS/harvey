#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>
#define Extern
#include "3210.h"

/*
 * return r + s * t
 * don't round properly
 * keep 31 below the point from the multiply while doing the add
 * and all bits above the point
 */
Freg
fmuladd(int nega, int negp, Freg rr, Freg sr, Freg tr)
{
	ulong m, g, s, sg, t, tg, r, rg, x;
	int exp, rexp, sh;
	int swapped;

	s = sr.val;
	t = tr.val;
	if(!Exp(s) || !Exp(t)){
		g = m = 0;
		exp = -Bias;
	}else if(s == Bias){
		g = 0;
		if(Sign(t))
			m = (-2 << Mbits) + Mant(t);
		else
			m = (1 << Mbits) + Mant(t);
		exp = Exp(t) - Bias;
		if(negp)
			m = ~m + 1;
	}else{
		exp = Exp(s) - Bias + Exp(t) - Bias;
		sg = Mant(s) & 0xfff;
		if(Sign(s))
			s = (-2 << (Mbits-12)) + (Mant(s) >> 12);
		else
			s = (1 << (Mbits-12)) + (Mant(s) >> 12);
		tg = Mant(t) & 0x7ff;
		if(Sign(t))
			t = (-2 << (Mbits-11)) + (Mant(t) >> 11);
		else
			t = (1 << (Mbits-11)) + (Mant(t) >> 11);
		if(negp){
			sg = ~sg & 0xfff;
			s = ~s;
			sg++;
			s += sg >> 12;
			sg &= 0xfff;
		}
		g = sg * tg;
		x = s * tg;
		g += (x & 0x7ff) << 12;
		x >>= 11;
		if(x & (1 << (31 - 11)))
			x |= (~0 << (32 - 11));
		m = x;
		x = sg * t;
		g += (x & 0xfff) << 11;
		x >>= 12;
		if(x & (1 << (31 - 12)))
			x |= (~0 << (32 - 12));
		m += x;
		m += s * t;
		m += g >> 23;
		g = (g >> 15) & 0xff;
	}

	r = rr.val;
	rg = rr.guard;
	if(!Exp(r)){
		return normalize(m, g, exp);
	}else{
		rexp = Exp(r) - Bias;
		if(Sign(r))
			r = (-2 << Mbits) + Mant(r);
		else
			r = (1 << Mbits) + Mant(r);
	}

	if(exp == -Bias){
		if(nega){
			rg = ~rg & 0xff;
			r = ~r;
			rg++;
			r += rg >> 8;
			rg &= 0xff;
		}
		return normalize(r, rg, rexp);
	}

	/*
	 * find shift amount for adding
	 * and swap if addend has larger exponent
	 */
	sh = exp - rexp;
	swapped = 0;
	if(rexp > exp){
		swapped = 1;
		exp = rexp;
		sh = -sh;
		x = m;
		m = r;
		r = x;
		x = g;
		g = rg;
		rg = x;
	}

	/*
	 * shift addend
	 * we have up to 8 guard + 23 mantissa + 2 hidden bits
	 */
	x = 0;
	if(r & 0x80000000)
		x = ~0;
	if(sh == 0)
		;
	else if(sh < 8){
		rg = (rg + (r << 8)) >> sh;
		r = (r >> sh) | (x << (32 - sh));
	}else if(sh < 32){
		rg = r >> (sh - 8);
		r = (r >> sh) | (x << (32 - sh));
	}else if(sh < 40){
		rg = r >> (sh - 8);
		r = x;
	}else{
		rg = r = x;
	}
	rg &= 0xff;

	/*
	 * 3210 negates the addend after shifting for the add
	 */
	if(nega && swapped){
		g = ~g & 0xff;
		m = ~m;
		g++;
		m += g >> 8;
		g &= 0xff;
	}
	if(nega && !swapped){
		rg = ~rg & 0xff;
		r = ~r;
		rg++;
		r += rg >> 8;
		rg &= 0xff;
	}
	g += rg;
	m += r + (g >> 8);
	g &= 0xff;

	return normalize(m, g, exp);
}

Freg
fround(Freg f)
{
	ulong m;
	int exp;

	if(Sign(f.val))
		m = (-2 << Mbits) + Mant(f.val);
	else
		m = (1 << Mbits) + Mant(f.val);
	exp = Exp(f.val) - Bias;
	if(f.guard & 0x80)
		m++;
	return normalize(m, 0, exp);
}

/*
 * normalize a multiply-add
 * g has 8 bits, m 23 + 3 hidden bits
 * set the fpu flags
 */
Freg
normalize(ulong m, ulong g, int exp)
{
	Freg r;
	int top, neg, i;

	reg.c[CTR] = (reg.c[CTR] << 1) & 0x3e;
	reg.c[PS] &= ~(Fneg|Fzero|Funder|Fover);

	/*
	 * see if any of the hidden bits are non-sign bits
	 */
	top = 1;
	if(m & 0x80000000)
		top = 0;
	for(i = 29; i >= Mbits; i--)
		if(((m >> i) & 1) == top)
			break;
	/*
	 * 1 << Mbits is the normalized location of the hidden bit
	 */
	if(i >= Mbits){
		i = i - Mbits;
		g = ((m << 8) | g) >> i;
		m = m >> i;
		exp += i;
	}else{
		/*
		 * no hidden bits are data bits
		 * look for the first valid data bit
		 */
		top <<= 31;
		for(m = (m << 8) | g; m; m <<= 1){
			if((m & (1<<31)) == top)
				break;
			exp--;
		}
		if(!m && top){
			r.val = 0;
			r.guard = 0;
			reg.c[PS] |= Fzero;
			return r;
		}
		g = m;
		m = m >> 8;
	}

	neg = !top;
	if(exp > 127)
		reg.c[PS] |= Fover;
	else if(exp < -127){
		reg.c[PS] |= Funder;
		reg.c[PS] |= Fzero;
		exp = -Bias;
		g = 0;
		m = 0;
		neg = 0;
	}else if(neg){
		reg.c[CTR] |= 1;
		reg.c[PS] |= Fneg;
	}

	r.val = Fval(neg, m, exp + Bias);
	r.guard = g & 0xff;
	return r;
}

/*
 * set fpu flags & flush to 0 if underflowed
 */
Freg
fpuflags(Freg f)
{
	reg.c[CTR] = (reg.c[CTR] << 1) & 0x3e;
	reg.c[PS] &= ~(Fneg|Fzero|Funder|Fover);
	if(!Exp(f.val)){
		if(f.val)
			reg.c[PS] |= Funder;
		f.val = 0;
	}
	if(!f.val)		
		reg.c[PS] |= Fzero;
	if(Sign(f.val)){
		reg.c[CTR] |= 1;
		reg.c[PS] |= Fneg;
	}
	return f;
}

#ifdef test

ulong
ftodsp(Freg f)
{
	return f.val;
}

Freg
dsptof(ulong v)
{
	Freg f;

	f.val = v;
	f.guard = 0;
	return f;
}
void
main(int argc, char *argv[])
{
	Freg s;
	ulong a, b, c;
	union{
		double d;
		struct{
			ulong ms, ls;
		};
	}convd;
	ulong ms, ls;
	int rflag, nflag, sflag;

	rflag = 0;
	nflag = 0;
	sflag = 0;
	ARGBEGIN{
	case 'n':
		nflag = 1;
		break;
	case 's':
		sflag = 1;
		break;
	case 'r':
		rflag = 1;
		break;
	}ARGEND
	if(argc != 3){
		fprint(2, "usage: ma [-r] a b c\n"
				"	a + b * c\n");
		exits("usage");
	}
	a = dtodsp(atof(argv[0]));
	b = dtodsp(atof(argv[1]));
	c = dtodsp(atof(argv[2]));
	s = fmuladd(nflag, sflag, dsptof(a), dsptof(b), dsptof(c));
	print("fsim: %g %s(%.6lux %.2lux) E %d\n",
		dsptod(ftodsp(s)),
		Sign(s.val) ? "-" : "+",
		Mant(s.val), s.guard, Exp(s.val) - Bias);

	convd.d = dsptod(a) + (dsptod(b) * dsptod(c));
	ms = ((convd.ms & 0xfffff) << 3) | (convd.ls >> 29);
	ls = (convd.ls >> 21) & 0xff;
	print("host: %g %s(%.6lux %.2lux) E %d\n",
		convd.d, (convd.ms >> 31) ? "-" : "+",
		ms, ls, ((convd.ms >> 20) & 0x7ff) - 1023);

	if(!rflag)
		exits(0);

	/*
	 * for checking rounding:
	 * 0x7ffffe80 * 0x00000180 = 0x7fffff80
	 * 0x00000060 + 0x7ffffe80 * 0x00000180 = 0x7fffff80
	 * 0x00000061 + 0x7ffffe80 * 0x00000180 = 0x00000081
	 *
	 * 0x7ffffc80 * 0x40000180 = 0x3ffffd81
	 * 0x00000060 + 0x7ffffc80 * 0x40000180 = 0x3ffffd81
	 * 0x00000061 + 0x7ffffc80 * 0x40000180 = 0x3ffffe81
	 */
	s = fmuladd(0, 0, dsptof(0x00000000), dsptof(0x7ffffe80), dsptof(0x00000180));
	print("0x%.8lux sb 0x7fffff80\n", ftodsp(s));
	s = fmuladd(0, 0, dsptof(0x00000060), dsptof(0x7ffffe80), dsptof(0x00000180));
	print("0x%.8lux sb 0x7fffff80\n", ftodsp(s));
	s = fmuladd(0, 0, dsptof(0x00000061), dsptof(0x7ffffe80), dsptof(0x00000180));
	print("0x%.8lux sb 0x00000081\n", ftodsp(s));

	s = fmuladd(0, 0, dsptof(0x00000000), dsptof(0x7ffffc80), dsptof(0x40000180));
	print("0x%.8lux sb 0x3ffffd81\n", ftodsp(s));
	s = fmuladd(0, 0, dsptof(0x00000060), dsptof(0x7ffffc80), dsptof(0x40000180));
	print("0x%.8lux sb 0x3ffffd81\n", ftodsp(s));
	s = fmuladd(0, 0, dsptof(0x00000061), dsptof(0x7ffffc80), dsptof(0x40000180));
	print("0x%.8lux sb 0x3ffffe81\n", ftodsp(s));
	exits(0);
}
#endif
