#include "fconv.h"

/* strtod for IEEE-arithmetic machines (dmg).
 *
 * This strtod returns a nearest machine number to the input decimal
 * string.  Ties are broken by the IEEE round-even rule.
 *
 * Inspired loosely by William D. Clinger's paper "How to Read Floating
 * Point Numbers Accurately" [Proc. ACM SIGPLAN '90, pp. 92-101].
 *
 * Modifications:
 *
 *	1. We only require IEEE arithmetic (not IEEE double-extended).
 *	2. We get by with floating-point arithmetic in a case that
 *		Clinger missed -- when we're computing d * 10^n
 *		for a small integer d and the integer n is not too
 *		much larger than 22 (the maximum integer k for which
 *		we can represent 10^k exactly), we may be able to
 *		compute (d*10^k) * 10^(e-k) with just one roundoff.
 *	3. Rather than a bit-at-a-time adjustment of the binary
 *		result in the hard case, we use floating-point
 *		arithmetic to determine the adjustment to within
 *		one bit; only in really hard cases do we need to
 *		compute a second residual.
 *	4. Because of 3., we don't need a large table of powers of 10
 *		for ten-to-e (just some small tables, e.g. of 10^k
 *		for 0 <= k <= 22).
 */


static double
ulp(double xarg)
{
	long L;
	Dul a;
	Dul x;

	x.d = xarg;
	L = (word0(x) & Exp_mask) - (P-1)*Exp_msk1;
	if(Sudden_Underflow){
		word0(a) = L;
		word1(a) = 0;
	}else{
		if(L > 0){
			word0(a) = L;
			word1(a) = 0;
		}else{
			L = -L >> Exp_shift;
			if(L < Exp_shift){
				word0(a) = 0x80000 >> L;
				word1(a) = 0;
			}else{
				word0(a) = 0;
				L -= Exp_shift;
				word1(a) = L >= 31 ? 1 : 1 << 31 - L;
			}
		}
	}
	return a.d;
}

static Bigint *
s2b(char *s, int nd0, int nd, unsigned long y9)
{
	Bigint *b;
	int i, k;
	long x, y;

	x = (nd + 8) / 9;
	for(k = 0, y = 1; x > y; y <<= 1, k++) ;
	b = Balloc(k);
	b->x[0] = y9;
	b->wds = 1;

	i = 9;
	if(9 < nd0){
		s += 9;
		do b = multadd(b, 10, *s++ - '0');
			while(++i < nd0);
		s++;
	}else
		s += 10;
	for(; i < nd; i++)
		b = multadd(b, 10, *s++ - '0');
	return b;
}

static double
b2d(Bigint *a, int *e)
{
	unsigned long *xa, *xa0, w, y, z;
	int k;
	Dul d;
#define d0 word0(d)
#define d1 word1(d)

	xa0 = a->x;
	xa = xa0 + a->wds;
	y = *--xa;
	k = hi0bits(y);
	*e = 32 - k;
	if(k < Ebits){
		d0 = Exp_1 | y >> Ebits - k;
		w = xa > xa0 ? *--xa : 0;
		d1 = y << (32-Ebits) + k | w >> Ebits - k;
		goto ret_d;
	}
	z = xa > xa0 ? *--xa : 0;
	if(k -= Ebits){
		d0 = Exp_1 | y << k | z >> 32 - k;
		y = xa > xa0 ? *--xa : 0;
		d1 = z << k | y >> 32 - k;
	}else{
		d0 = Exp_1 | y;
		d1 = z;
	}
 ret_d:
#undef d0
#undef d1
	return d.d;
}

static double
ratio(Bigint *a, Bigint *b)
{
	Dul da, db;
	int k, ka, kb;

	da.d = b2d(a, &ka);
	db.d = b2d(b, &kb);
	k = ka - kb + 32*(a->wds - b->wds);
	if(k > 0)
		word0(da) += k*Exp_msk1;
	else{
		k = -k;
		word0(db) += k*Exp_msk1;
	}
	return da.d / db.d;
}

double
strtod(char *s00, char **se)
{
	int bb2, bb5, bbe, bd2, bd5, bbbits, bs2, c, dsign,
		 e, e1, esign, i, j, k, nd, nd0, nf, nz, nz0, sign;
	char *s, *s0, *s1;
	double aadj, aadj1, adj;
	Dul rv, rv0;
	long L;
	unsigned long y, z;
	Bigint *bb, *bb1, *bd, *bd0, *bs, *delta;

	sign = nz0 = nz = 0;
	rv.d = 0.;
	for(s = s00;;s++) switch(*s){
		case '-':
			sign = 1;
			/* no break */
		case '+':
			if (*++s)
				goto break2;
			/* no break */
		case 0:
			s = s00;
			goto ret;
		case '\t':
		case '\n':
		case '\v':
		case '\f':
		case '\r':
		case ' ':
			continue;
		default:
			goto break2;
	}
 break2:
	if(*s == '0'){
		nz0 = 1;
		while(*++s == '0') ;
		if(!*s)
			goto ret;
	}
	s0 = s;
	y = z = 0;
	for(nd = nf = 0; (c = *s) >= '0' && c <= '9'; nd++, s++)
		if (nd < 9)
			y = 10*y + c - '0';
		else if (nd < 16)
			z = 10*z + c - '0';
	nd0 = nd;
	if(c == '.'){
		c = *++s;
		if(!nd){
			for(; c == '0'; c = *++s)
				nz++;
			if(c > '0' && c <= '9'){
				s0 = s;
				nf += nz;
				nz = 0;
				goto have_dig;
			}
			goto dig_done;
		}
		for(; c >= '0' && c <= '9'; c = *++s){
 have_dig:
			nz++;
			if(c -= '0'){
				nf += nz;
				for(i = 1; i < nz; i++)
					if (nd++ < 9)
						y *= 10;
					else if (nd <= DBL_DIG + 1)
						z *= 10;
				if (nd++ < 9)
					y = 10*y + c;
				else if (nd <= DBL_DIG + 1)
					z = 10*z + c;	
				nz = 0;
			}
		}
	}
 dig_done:
	e = 0;
	if(c == 'e' || c == 'E'){
		if(!nd && !nz && !nz0){
			s = s00;
			goto ret;
		}
		s00 = s;
		esign = 0;
		switch(c = *++s){
			case '-':
				esign = 1;
			case '+':
				c = *++s;
		}
		if(c >= '0' && c <= '9'){
			while(c == '0')
				c = *++s;
			if(c > '0' && c <= '9'){
				e = c - '0';
				s1 = s;
				while((c = *++s) >= '0' && c <= '9')
					e = 10*e + c - '0';
				if(s - s1 > 8)
					/* Avoid confusion from exponents
					 * so large that e might overflow.
					 */
					e = 9999999;
				if(esign)
					e = -e;
			}else
				e = 0;
		}else
			s = s00;
	}
	if(!nd){
		if (!nz && !nz0)
			s = s00;
		goto ret;
	}
	e1 = e -= nf;

	/* Now we have nd0 digits, starting at s0, followed by a
	 * decimal point, followed by nd-nd0 digits.  The number we're
	 * after is the integer represented by those digits times
	 * 10**e */

	if(!nd0)
		nd0 = nd;
	k = nd < DBL_DIG + 1 ? nd : DBL_DIG + 1;
	rv.d = y;
	if(k > 9)
		rv.d = tens[k - 9] * rv.d + z;
	if(nd <= DBL_DIG && FLT_ROUNDS == 1){
		if(!e)
			goto ret;
		if(e > 0){
			if(e <= Ten_pmax){
				rv.d *= tens[e];
				goto ret;
			}
			i = DBL_DIG - nd;
			if(e <= Ten_pmax + i){
				/* A fancier test would sometimes let us do
				 * this for larger i values.
				 */
				e -= i;
				rv.d *= tens[i];
				rv.d *= tens[e];
				goto ret;
			}
		}else if (e >= -Ten_pmax){
			rv.d /= tens[-e];
			goto ret;
		}
	}
	e1 += nd - k;

	/* Get starting approximation = rv * 10**e1 */

	if(e1 > 0){
		if(i = e1 & 15)
			rv.d *= tens[i];
		if(e1 &= ~15){
			if(e1 > DBL_MAX_10_EXP){
 ovfl:
				rv.d = DBL_MAX;
				goto ret;
			}
			if(e1 >>= 4){
				for(j = 0; e1 > 1; j++, e1 >>= 1)
					if(e1 & 1)
						rv.d *= bigtens[j];
			/* The last multiplication could overflow. */
				word0(rv) -= P*Exp_msk1;
				rv.d *= bigtens[j];
				if((z = word0(rv) & Exp_mask)
				 > Exp_msk1*(DBL_MAX_EXP+Bias-P))
					goto ovfl;
				if(z > Exp_msk1*(DBL_MAX_EXP+Bias-1-P)){
					/* set to largest number */
					/* (Can't trust DBL_MAX) */
					word0(rv) = Big0;
					word1(rv) = Big1;
				}else
					word0(rv) += P*Exp_msk1;
			}

		}
	}else if(e1 < 0){
		e1 = -e1;
		if(i = e1 & 15)
			rv.d /= tens[i];
		if(e1 &= ~15){
			e1 >>= 4;
			for(j = 0; e1 > 1; j++, e1 >>= 1)
				if(e1 & 1)
					rv.d *= tinytens[j];
			/* The last multiplication could underflow. */
			rv0.d = rv.d;
			rv.d *= tinytens[j];
			if(rv.d == 0) {
				rv.d = 2.*rv0.d;
				rv.d *= tinytens[j];
				if(rv.d == 0){
 undfl:
					rv.d = 0.;
					goto ret;
				}
				word0(rv) = Tiny0;
				word1(rv) = Tiny1;
				/* The refinement below will clean
				 * this approximation up.
				 */
			}
		}
	}

	/* Now the hard part -- adjusting rv to the correct value.*/

	/* Put digits into bd: true value = bd * 10^e */

	bd0 = s2b(s0, nd0, nd, y);

	for(;;){
		bd = Balloc(bd0->k);
		Bcopy(bd, bd0);
		bb = d2b(rv.d, &bbe, &bbbits);	/* rv = bb * 2^bbe */
		bs = i2b(1);

		if(e >= 0){
			bb2 = bb5 = 0;
			bd2 = bd5 = e;
		}else{
			bb2 = bb5 = -e;
			bd2 = bd5 = 0;
		}
		if(bbe >= 0)
			bb2 += bbe;
		else
			bd2 -= bbe;
		bs2 = bb2;
		if(Sudden_Underflow)
			j = P + 1 - bbbits;
		else{
			i = bbe + bbbits - 1;	/* logb(rv) */
			if (i < Emin)	/* denormal */
				j = bbe + (P-Emin);
			else
				j = P + 1 - bbbits;
		}
		bb2 += j;
		bd2 += j;
		i = bb2 < bd2 ? bb2 : bd2;
		if(i > bs2)
			i = bs2;
		if(i > 0) {
			bb2 -= i;
			bd2 -= i;
			bs2 -= i;
			}
		if(bb5 > 0){
			bs = pow5mult(bs, bb5);
			bb1 = mult(bs, bb);
			Bfree(bb);
			bb = bb1;
		}
		if(bb2 > 0)
			bb = lshift(bb, bb2);
		if(bd5 > 0)
			bd = pow5mult(bd, bd5);
		if(bd2 > 0)
			bd = lshift(bd, bd2);
		if(bs2 > 0)
			bs = lshift(bs, bs2);
		delta = diff(bb, bd);
		dsign = delta->sign;
		delta->sign = 0;
		i = cmp(delta, bs);
		if(i < 0){
			/* Error is less than half an ulp -- check for
			 * special case of mantissa a power of two.
			 */
			if(dsign || word1(rv) || word0(rv) & Bndry_mask)
				break;
			delta = lshift(delta,Log2P);
			if (cmp(delta, bs) > 0)
				goto drop_down;
			break;
		}
		if(i == 0){
			/* exactly half-way between */
			if(dsign){
				if((word0(rv) & Bndry_mask1) == Bndry_mask1
				 &&  word1(rv) == 0xffffffff){
					/*boundary case -- increment exponent*/
					word0(rv) = (word0(rv) & Exp_mask)
						+ Exp_msk1;
					word1(rv) = 0;
					break;
				}
			}else if(!(word0(rv) & Bndry_mask) && !word1(rv)){
 drop_down:
				/* boundary case -- decrement exponent */
				if(Sudden_Underflow){
					L = word0(rv) & Exp_mask;
					if (L <= Exp_msk1)
						goto undfl;
					L -= Exp_msk1;
				}else
					L = (word0(rv) & Exp_mask) - Exp_msk1;
				word0(rv) = L | Bndry_mask1;
				word1(rv) = 0xffffffff;
				break;
			}
			if(ROUND_BIASED){
				if (dsign)
					rv.d += ulp(rv.d);
			}else{
				if (!(word1(rv) & LSB))
					break;
				if (dsign)
					rv.d += ulp(rv.d);
				else {
					rv.d -= ulp(rv.d);
					if(!Sudden_Underflow && rv.d == 0)
							goto undfl;
				}
			}
			break;
		}
		if((aadj = ratio(delta, bs)) <= 2.){
			if(dsign)
				aadj = aadj1 = 1.;
			else if(word1(rv) || word0(rv) & Bndry_mask){
				if (!Sudden_Underflow && word1(rv) == Tiny1 && !word0(rv))
					goto undfl;
				aadj = 1.;
				aadj1 = -1.;
			}else{
				/* special case -- power of FLT_RADIX to be */
				/* rounded down... */

				if(aadj < 2./FLT_RADIX)
					aadj = 1./FLT_RADIX;
				else
					aadj *= 0.5;
				aadj1 = -aadj;
			}
		}else{
			aadj *= 0.5;
			aadj1 = dsign ? aadj : -aadj;
			if(Check_FLT_ROUNDS)
				switch(FLT_ROUNDS) {
					case 2: /* towards +infinity */
						aadj1 -= 0.5;
						break;
					case 0: /* towards 0 */
					case 3: /* towards -infinity */
						aadj1 += 0.5;
				}
			else if (FLT_ROUNDS == 0)
				aadj1 += 0.5;
		}
		y = word0(rv) & Exp_mask;

		/* Check for overflow */

		if(y == Exp_msk1*(DBL_MAX_EXP+Bias-1)){
			rv0.d = rv.d;
			word0(rv) -= P*Exp_msk1;
			adj = aadj1 * ulp(rv.d);
			rv.d += adj;
			if((word0(rv) & Exp_mask) >=
					Exp_msk1*(DBL_MAX_EXP+Bias-P)){
				if(word0(rv0) == Big0 && word1(rv0) == Big1)
					goto ovfl;
				word0(rv) = Big0;
				word1(rv) = Big1;
				goto cont;
			}else
				word0(rv) += P*Exp_msk1;
		}else{
			if(Sudden_Underflow){
				if((word0(rv) & Exp_mask) <= P*Exp_msk1){
					rv0.d = rv.d;
					word0(rv) += P*Exp_msk1;
					adj = aadj1 * ulp(rv.d);
					rv.d += adj;
					if((word0(rv) & Exp_mask) <= P*Exp_msk1){
						if (word0(rv0) == Tiny0
						 && word1(rv0) == Tiny1)
							goto undfl;
						word0(rv) = Tiny0;
						word1(rv) = Tiny1;
						goto cont;
					}else
						word0(rv) -= P*Exp_msk1;
				}else{
					adj = aadj1 * ulp(rv.d);
					rv.d += adj;
				}
			}else{
				/* Compute adj so that the IEEE rounding rules will
				 * correctly round rv + adj in some half-way cases.
				 * If rv * ulp(rv) is denormalized (i.e.,
				 * y <= (P-1)*Exp_msk1), we must adjust aadj to avoid
				 * trouble from bits lost to denormalization;
				 * example: 1.2e-307 .
				 */
				if(y <= (P-1)*Exp_msk1 && aadj >= 1.){
					aadj1 = (double)(int)(aadj + 0.5);
					if(!dsign)
						aadj1 = -aadj1;
				}
				adj = aadj1 * ulp(rv.d);
				rv.d += adj;
			}
		}
		z = word0(rv) & Exp_mask;
		if(y == z){
			/* Can we stop now? */
			L = aadj;
			aadj -= L;
			/* The tolerances below are conservative. */
			if(dsign || word1(rv) || word0(rv) & Bndry_mask){
				if(aadj < .4999999 || aadj > .5000001)
					break;
				}
			else if(aadj < .4999999/FLT_RADIX)
				break;
		}
 cont:
		Bfree(bb);
		Bfree(bd);
		Bfree(bs);
		Bfree(delta);
	}
	Bfree(bb);
	Bfree(bd);
	Bfree(bs);
	Bfree(bd0);
	Bfree(delta);
 ret:
	if(se)
		*se = (char *)s;
	return sign ? -rv.d : rv.d;
}
