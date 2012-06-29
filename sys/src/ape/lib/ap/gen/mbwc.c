#include <stdlib.h>
#include <utf.h>

/*
 * Use the FSS-UTF transformation proposed by posix.
 *	We define 7 byte types:
 *	T0	0xxxxxxx	7 free bits
 *	Tx	10xxxxxx	6 free bits
 *	T1	110xxxxx	5 free bits
 *	T2	1110xxxx	4 free bits
 *	T3	11110xxx	3 free bits
 *
 *	Encoding is as follows.
 *	From hex	Thru hex	Sequence		Bits
 *	00000000	0000007F	T1		7
 *	00000080	000007FF	T2 Tx		11
 *	00000800	0000FFFF	T3 Tx Tx		16
 *	00010000	0010FFFF	T4 Tx Tx	Tx	20 (and change)
 */

int
mblen(const char *s, size_t n)
{

	return mbtowc(0, s, n);
}

int
mbtowc(wchar_t *pwc, const char *s, size_t n)
{
	int c, c1, c2, c3;
	long l;

	if(!s)
		return 0;

	if(n < 1)
		goto bad;
	c = s[0] & 0xff;
	if((c & 0x80) == 0x00) {
		if(pwc)
			*pwc = c;
		if(c == 0)
			return 0;
		return 1;
	}

	if(n < 2)
		goto bad;
	c1 = (s[1] ^ 0x80) & 0xff;
	if((c1 & 0xC0) != 0x00)
		goto bad;
	if((c & 0xE0) == 0xC0) {
		l = ((c << 6) | c1) & 0x7FF;
		if(l < 0x080)
			goto bad;
		if(pwc)
			*pwc = l;
		return 2;
	}

	if(n < 3)
		goto bad;
	c2 = (s[2] ^ 0x80) & 0xff;
	if((c2 & 0xC0) != 0x00)
		goto bad;
	if((c & 0xF0) == 0xE0) {
		l = ((((c << 6) | c1) << 6) | c2) & 0xFFFF;
		if(l < 0x0800)
			goto bad;
		if(pwc)
			*pwc = l;
		return 3;
	}

	if(n < 4)
		goto bad;
	if(UTFmax >= 4) {
		c3 = (s[3] ^ 0x80) & 0xff;
		if(c3 & 0xC0)
			goto bad;
		if(c < 0xf8) {
			l = ((((((c << 6) | c1) << 6) | c2) << 6) | c3) & 0x3fffff;
			if(l <= 0x10000)
				goto bad;
			if(l > Runemax)
				goto bad;
			if(pwc)
				*pwc = l;
			return 4;
		}
	}

	/*
	 * bad decoding
	 */
bad:
	return -1;

}

int
wctomb(char *s, wchar_t wchar)
{
	long c;

	if(!s)
		return 0;

	c = wchar;
	if(c > Runemax)
		c = Runeerror;
	if(c < 0x80) {
		s[0] = c;
		return 1;
	}

	if(c < 0x800) {
		s[0] = 0xC0 | (c >> 6);
		s[1] = 0x80 | (c & 0x3F);
		return 2;
	}

	if(c < 0x10000){
		s[0] = 0xE0 |  (c >> 12);
		s[1] = 0x80 | ((c >> 6) & 0x3F);
		s[2] = 0x80 |  (c & 0x3F);
		return 3;
	}
	s[0] = 0xf0 | c >> 18;
	s[1] = 0x80 | (c >> 12) & 0x3F;
	s[2] = 0x80 | (c >> 6) & 0x3F;
	s[3] = 0x80 | (c & 0x3F);
	return 4;
}

size_t
mbstowcs(wchar_t *pwcs, const char *s, size_t n)
{
	int i, d, c;

	for(i=0; i < n; i++) {
		c = *s & 0xff;
		if(c < 0x80) {
			*pwcs = c;
			if(c == 0)
				break;
			s++;
		} else {
			d = mbtowc(pwcs, s, UTFmax);
			if(d <= 0)
				return (size_t)((d<0) ? -1 : i);
			s += d;
		}
		pwcs++;
	}
	return i;
}

size_t
wcstombs(char *s, const wchar_t *pwcs, size_t n)
{
	int i, d;
	long c;
	char *p, *pe;
	char buf[UTFmax];

	p = s;
	pe = p+n-UTFmax;
	while(p < pe) {
		c = *pwcs++;
		if(c < 0x80)
			*p++ = c;
		else
			p += wctomb(p, c);
		if(c == 0)
			return p-s;
	}
	while(p < pe+UTFmax) {
		c = *pwcs++;
		d = wctomb(buf, c);
		if(p+d <= pe+UTFmax) {
			for(i = 0; i < d; i++)
				p[i] = buf[i];
			p += d;
		}
		if(c == 0)
			break;
	}
	return p-s;
}
