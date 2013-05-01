#include <stdlib.h>
#include <limits.h>

/*
 * Use the FSS-UTF transformation proposed by posix.
 *	We define 7 byte types:
 *	T0	0xxxxxxx	7 free bits
 *	Tx	10xxxxxx	6 free bits
 *	T1	110xxxxx	5 free bits
 *	T2	1110xxxx	4 free bits
 *	T3	11110xxx	3 free bits
 *	T4	111110xx	2 free bits
 *	T5	1111110x	1 free bit
 *
 *	Encoding is as follows.
 *	From hex	Thru hex	Sequence		Bits
 *	00000000	0000007F	T0				7
 *	00000080	000007FF	T1 Tx			11
 *	00000800	0000FFFF	T2 Tx Tx			16
 *	00010000	001FFFFF	T3 Tx Tx Tx		21
 *	00200000	03FFFFFF	T4 Tx Tx Tx Tx		26
 *	04000000	7FFFFFFF	T5 Tx Tx  Tx Tx Tx	31
 */
int
mbtowc(wchar_t *pwc, const char *s, size_t n);

int
mblen(const char *s, size_t n)
{
	return mbtowc(0, s, n);
}

enum {
	C0MSK = 0x7F,
	C1MSK = 0x7FF,
	T1 = 0xC0,
	T2 = 0xE0,
	NT1BITS = 11,
	NSHFT = 5,
	NCSHFT = NSHFT + 1,
	WCHARMSK = (1<< (8*MB_LEN_MAX - 1)) - 1,
};

int
mbtowc(wchar_t *pwc, const char *s, size_t n)
{
	unsigned long long c[MB_LEN_MAX];
	unsigned long long l, m, wm, b;
	int i;

	if(!s)
		return 0;

	if(n < 1)
		goto bad;

	c[0] = s[0] & 0xff;		/* first one is special */
	if((c[0] & 0x80) == 0x00) {
		if(pwc)
			*pwc = c[0];
		if(c[0] == 0)
			return 0;
		return 1;
	}

	m = T2;
	b = m^0x20;
	l = c[0];
	wm = C1MSK;
	for(i = 1; i < MB_LEN_MAX + 1; i++){
		if(n < i+1)
			goto bad;
		c[i] = (s[i] ^ 0x80) & 0xff;
		l = (l << NCSHFT) | c[i];
		if((c[i] & 0xC0) != 0x00)
			goto bad;
		if((c[0] & m) == b) {
			if(pwc)
				*pwc = l & wm;
			return i + 1;
		}
		b = m;
		m = (m >> 1) | 0x80;
		wm = (wm << NSHFT) | wm;
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
	unsigned long long c, maxc, m;
	int i, j;

	if(!s)
		return 0;

	maxc = 0x80;
	c = wchar & WCHARMSK;
	if(c < maxc) {
		s[0] = c;
		return 1;
	}

	m = T1;
	for(i = 2; i < MB_LEN_MAX + 1; i++){
		maxc <<= 4;
		if(c < maxc || i == MB_LEN_MAX){
			s[0] = m | (c >> ((i - 1) * NCSHFT));
			for(j = i - 1; j >= 1; j--){
				s[i - j] = 0x80|((c>>(6 * (j - 1)))&0x3f);
			}
			return i;
		}
		m = (m >> 1) | 0x80;
	}
	return MB_LEN_MAX;
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
			d = mbtowc(pwcs, s, MB_LEN_MAX);
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
	char buf[MB_LEN_MAX];

	p = s;
	pe = p+n-MB_LEN_MAX;
	while(p < pe) {
		c = *pwcs++;
		if(c < 0x80)
			*p++ = c;
		else
			p += wctomb(p, c);
		if(c == 0)
			return p-s;
	}
	while(p < pe+MB_LEN_MAX) {
		c = *pwcs++;
		d = wctomb(buf, c);
		if(p+d <= pe+MB_LEN_MAX) {
			*p++ = buf[0];		/* first one is special */
			for(i = 2; i < MB_LEN_MAX + 1; i++){
				if(d <= i -1)
					break;
				*p++ = buf[i];
			}
		}
		if(c == 0)
			break;
	}
	return p-s;
}
