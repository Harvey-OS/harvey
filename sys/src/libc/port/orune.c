#include	<u.h>
#include	<libc.h>

enum
{
	Char1	= Runeself,	Rune1	= Runeself,
	Char21	= 0xA1,		Rune21	= 0x0100,
	Char22	= 0xF6,		Rune22	= 0x4016,
	Char3	= 0xFC,		Rune3	= 0x10000,	/* really 0x38E2E */
	Esc	= 0xBE,		Bad	= Runeerror,
};

static	uchar	U[256];
static	uchar	T[256];

static
void
mktable(void)
{
	int i, u;

	for(i=0; i<256; i++) {
		u = i + (0x5E-0xA0);
		if(i < 0xA0)
			u = i + (0xDF-0x7F);
		if(i < 0x7F)
			u = i + (0x00-0x21);
		if(i < 0x21)
			u = i + (0xBE-0x00);
		U[i] = u;
		T[u] = i;
	}
}

int
chartorune(Rune *rune, char *str)
{
	int c, c1, c2;
	long l;

	if(U[0] == 0)
		mktable();

	/*
	 * one character sequence
	 *	00000-0009F => 00-9F
	 */
	c = *(uchar*)str;
	if(c < Char1) {
		*rune = c;
		return 1;
	}

	/*
	 * two character sequence
	 *	000A0-000FF => A0; A0-FF
	 */
	c1 = *(uchar*)(str+1);
	if(c < Char21) {
		if(c1 >= Rune1 && c1 < Rune21) {
			*rune = c1;
			return 2;
		}
		goto bad;
	}

	/*
	 * two character sequence
	 *	00100-04015 => A1-F5; 21-7E/A0-FF
	 */
	c1 = U[c1];
	if(c1 >= Esc)
		goto bad;
	if(c < Char22) {
		*rune =  (c-Char21)*Esc + c1 + Rune21;
		return 2;
	}

	/*
	 * three character sequence
	 *	04016-38E2D => A6-FB; 21-7E/A0-FF
	 */
	c2 = U[*(uchar*)(str+2)];
	if(c2 >= Esc)
		goto bad;
	if(c < Char3) {
		l = (c-Char22)*Esc*Esc + c1*Esc + c2 + Rune22;
		if(l >= Rune3)
			goto bad;
		*rune = l;
		return 3;
	}

	/*
	 * bad decoding
	 */
bad:
	*rune = Bad;
	return 1;
}

int
runetochar(char *str, Rune *rune)
{
	long c;

	if(T[0] == 0)
		mktable();

	/*
	 * one character sequence
	 *	00000-0009F => 00-9F
	 */
	c = *rune;
	if(c < Rune1) {
		str[0] = c;
		return 1;
	}

	/*
	 * two character sequence
	 *	000A0-000FF => A0; A0-FF
	 */
	if(c < Rune21) {
		str[0] = Char1;
		str[1] = c;
		return 2;
	}

	/*
	 * two character sequence
	 *	00100-04015 => A1-F5; 21-7E/A0-FF
	 */
	if(c < Rune22) {
		c -= Rune21;
		str[0] = c/Esc + Char21;
		str[1] = T[c%Esc];
		return 2;
	}

	/*
	 * three character sequence
	 *	04016-38E2D => A6-FB; 21-7E/A0-FF
	 */
	c -= Rune22;
	str[0] = c/(Esc*Esc) + Char22;
	str[1] = T[c/Esc%Esc];
	str[2] = T[c%Esc];
	return 3;
}

int
runelen(long c)
{
	Rune rune;
	char str[10];

	rune = c;
	return runetochar(str, &rune);
}

int
fullrune(char *str, int n)
{
	int c;

	if(n > 0) {
		c = *(uchar*)str;
		if(c < Char1)
			return 1;
		if(n > 1)
			if(c < Char22 || n > 2)
				return 1;
	}
	return 0;
}
