#include	<u.h>
#include	<libc.h>
#include	"gen.h"

static	int	fmtmode;
static	int	fmtformat;
static	int	fmtchar;
		
void
chessinit(int chr, int mode, int fmt)
{

	fmtmode = mode;
	fmtformat = fmt;
	if(chr != fmtchar)
		fmtinstall(chr, chessfmt);
}

int
chessfmt(void *o, Fconv *fp)
{
	char str1[50], str2[50], *p1, *p2;
	Rune r;
	int c;

	((fmtformat&1)? dscout: algout)(*(int*)o, str1, fmtmode);
	if(fmtformat&2) {
		p2 = str2;
		for(p1=str1; c=*p1; p1++)
		switch(c) {
		default:
			fprint(2, "dont now %c\n", c);
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':

		case 'a':
		case 'b':
		case 'c':
		case 'd':
		case 'e':
		case 'f':
		case 'g':
		case 'h':

		case 'O':
		case '+':
		case '-':
		case '/':
			*p2++ = c;
			break;
		case 'x':
		case ':':
			r = L'×';
			goto putr;
		case 'P':
			r = L'♙';
			goto putr;
		case 'N':
			r = L'♘';
			goto putr;
		case 'B':
			r = L'♗';
			goto putr;
		case 'R':
			r = L'♖';
			goto putr;
		case 'Q':
			r = L'♕';
			goto putr;
		case 'K':
			r = L'♔';
			goto putr;
		putr:
			p2 += runetochar(p2, &r);
			break;
		}
		*p2 = 0;
		strconv(str2, fp);
		return sizeof(int);
	}
	strconv(str1, fp);
	return sizeof(int);
}

int
chessin(char *str)
{
	int m;
	short mv[MAXMG];

	getamv(mv);
	m = xalgin(mv, str);
	if(m == 0)
		m = xdscin(mv, str);
	return m;
}

int
xalgin(short *mv, char *str)
{
	int p, t, fx, fy, tx, ty, x, m, i, c;
	char *s, *sstr;

	sstr = str;

	str = sstr;
	c = *str++;
	t = -1;
	p = -1;

	if(c == '*' && str[0] == '*' && str[1] == '*' && str[2] == 0)
		return TNULL << 12;
	for(i=0; c=='o'||c=='O'||c=='0'; i++) {
		c = *str++;
		while(c == '-')
			c = *str++;
		if(i > 0) {
			p = WKING;
			t = TOO;
			if(i > 1)
				t = TOOO;
		}
	}
	s = "P1N2n2B3p3R4r4Q5q5K6k6";
	for(; s[0]; s+=2)
		if(c == s[0]) {
			p = s[1] - '0';
			c = *str++;
			break;
		}

	fx = -1;
	fy = -1;

loop:
	tx = -1;
	ty = -1;
	if(c >= 'a' && c <= 'h') {
		tx = c-'a';
		c = *str++;
	}
	if(c >= '1' && c <= '8') {
		ty = '8'-c;
		c = *str++;
	}
/* should  really distinguish : form - */
	if(c == ':')
		c = *str++;
	if((c >= 'a' && c <= 'h') ||
	  (c >= '1' && c <= '8')) {
		fx = tx;
		fy = ty;
		goto loop;
	}
	if(c == '(' || c == '=')
		c = *str++;
	s = "N0n0B1b1p1R2r2Q3q3";
	for(; s[0]; s+=2)
		if(c == s[0]) {
			t = TNPRO + s[1]-'0';
			c = *str++;
			break;
		}
	if(c == ')')
		c = *str++;
	while(c == '+')
		c = *str++;
	if(c != 0)
		return 0;

	if(p == -1) {
		p = WPAWN;
		if(fx >= 0 && fy >= 0)
			p = board[fy*8+fx] & 07;
	}
	c = 0;
	for(i=0; m=mv[i]; i++) {
		if(p == (board[(m>>6)&077]&7))
		if(match(m, p, t, fx, fy, -1, tx, ty)) {
			if(moveno & 1) {
				bmove(m);
				x = wattack(bkpos);
				bremove();
			} else {
				wmove(m);
				x = battack(wkpos);
				wremove();
			}
			if(!x)
				continue;
			if(c)
				return 0;	/* ambiguous */
			c = m;
		}
	}
	return c;
}

int
xdscin(short *mv, char *str)
{
	int pf, pt, t, fx, fy, tx, ty, x, i, c;
	char *s;

	t = -1;
	pf = -1;
	pt = 0;
	fx = fy = -1;
	tx = ty = -1;

	str = getpie(&pf, &fx, str);
	c = *str++;
	if(c == '/') {
		str = getpla(&fx, &fy, str);
		c = *str++;
	}
	if(c == 'x' || c == '*') {
		str = getpie(&pt, &tx, str);
		c = *str++;
		if(c == '/') {
			str = getpla(&tx, &ty, str);
			c = *str++;
		}
	} else
	if(c == '-') {
		str = getpla(&tx, &ty, str);
		c = *str++;
	}
	if(c == '(' || c == '=') {
		c = *str++;
		t = -1;
		for(s = "N0n0B1b1R2r2Q3q3"; s[0]; s+=2)
			if(c == s[0])
				t = TNPRO + s[1]-'0';
		if(t < 0)
			return 0;
		c = *str++;
		if(c == ')')
			c = *str++;
	}
	if(c == 'e' || c == 'E')
	if(*str == 'p' || *str == 'P') {
		str++;
		c = *str++;
		t = TENPAS;
		pt = -1;
	}
	while(c == '+')
		c = *str++;
	if(c != 0)
		return 0;

	c = 0;
	for(i=0; mv[i]; i++) {
		if(match(mv[i], pf, t, fx, fy, pt, tx, ty)) {
			if(moveno & 1) {
				bmove(mv[i]);
				x = wattack(bkpos);
				bremove();
			} else {
				wmove(mv[i]);
				x = battack(wkpos);
				wremove();
			}
			if(!x)
				continue;
			if(c) {
				return 0;
			}
			c = mv[i];
		}
	}
	return c;
}

char*
getpie(int *p, int *x, char *str)
{
	char *s;
	int kq, rbn, pwn, c;

	kq = 0;
	rbn = 0;
	pwn = 0;
	c = *str++;
	for(s = "Q5q5K6k6"; s[0]; s+=2)
		if(c == s[0]) {
			kq = s[1] - '0';
			c = *str++;
			break;
		}
	for(s = "N2n2B3b3R4r4"; s[0]; s+=2)
		if(c == s[0]) {
			rbn = s[1] - '0';
			c = *str++;
			break;
		}
	for(s = "P1p1"; s[0]; s+=2)
		if(c == s[0]) {
			pwn = s[1] - '0';
			str++;
			break;
		}
	str--;
	if(pwn) {
		*p = pwn;
		if(rbn) {
			rbn--;
			if(rbn == 3)
				rbn = 0;
			if(kq == 5)
				*x = rbn;
			else
			if(kq == 6)
				*x = 7-rbn;
			else
				*x = 8+rbn;
		} else
		if(kq)
			*x = kq-2;
		return str;
	}
	if(rbn) {
		*p = rbn;
		if(kq == 5)
			*x = 16;
		else
		if(kq == 6)
			*x = 17;
		return str;
	}
	if(kq)
		*p = kq;
	return str;
}

char*
getpla(int *x, int *y, char *str)
{
	char *s;
	int c;

	c = *str++;
	for(s = "Q3q3K4k4"; s[0]; s+=2)
		if(c == s[0]) {
			*x = s[1] - '0';
			c = *str++;
			break;
		}
	for(s = "R0r0N1n1B2b2"; s[0]; s+=2)
		if(c == s[0]) {
			if(*x == 3)
				*x = s[1]-'0';
			else
			if(*x == 4)
				*x = 7 - (s[1]-'0');
			else
				*x = 8 + (s[1]-'0');
			c = *str++;
			break;
		}
	if(c >= '1' && c <= '8') {
		if(moveno & 1)
			*y = c-'1';
		else
			*y = '8'-c;
		return str;
	}
	str--;
	return str;
}

void
algout(int m, char *str, int f)
{
	short mv[MAXMG];

	getamv(mv);
	xalgout(mv, m, str, f);
}

void
xalgout(short *mv, int m, char *str, int f)
{
	int p, c, t, fx, fy, tx, ty, i, j, n;

	t = (m>>12) & 017;
	c = 0;
	if(t == TENPAS || board[m&077])
		c++;
	if(t == TOO || t == TOOO) {
		*str++ = 'O';
		*str++ = '-';
		*str++ = 'O';
		if(t == TOOO) {
			*str++ = '-';
			*str++ = 'O';
		}
		goto chk;
	}
	p = board[(m>>6) & 077] & 7;
	tx = (m>>0) & 7;
	ty = (m>>3) & 7;
	if(t < TNPRO || t > TQPRO)
		t = -1;
	for(i=0; i<4; i++) {
		if(f)
			i = 3;
		fx = -1;
		if((i & 1) || (c && p == 1))
			fx = (m>>6) & 7;
		fy = -1;
		if((i & 2))
			fy = (m>>9) & 7;
		n = 0;
		for(j=0; mv[j]; j++)
			if(match(mv[j], p, t, fx, fy, -1, tx, ty))
				n++;

		if(n == 1) {
			if(f || (p != WPAWN))
				*str++ = "XPNBRQKY"[p];
			if(fx >= 0)
				*str++ = 'a'+fx;
			if(fy >= 0)
				*str++ = '8'-fy;
			if(c)
				*str++ = ':';
			*str++ = 'a'+tx;
			*str++ = '8'-ty;
			goto chk;
		}
	}
	*str++ = '?';
	*str++ = '?';
	*str++ = '?';
	*str++ = '?';
	*str = 0;
	return;

chk:
	if(t >= TNPRO && t <= TQPRO)
		*str++ = "NBRQ"[t-TNPRO];
	move(m);
	if(check()) {
		*str++ = '+';
		if(mate())
			*str++ = '+';
	}
	rmove();
	*str = 0;
}

void
dscout(int m, char *str, int f)
{
	short mv[MAXMG];

	getamv(mv);
	xdscout(mv, m, str, f);
}

void
xdscout(short *mv, int m, char *str, int f)
{
	int pf, pt, t, fx, fy, tx, ty, i, j, n;

	t = (m>>12) & 017;
	if(t == TOO || t == TOOO) {
		*str++ = 'O';
		*str++ = '-';
		*str++ = 'O';
		if(t == TOOO) {
			*str++ = '-';
			*str++ = 'O';
		}
		goto chk;
	}
	pf = board[(m>>6) & 077] & 7;
	pt = board[m&077] & 07;
	if(t < TNPRO || t > TQPRO)
		t = -1;
	for(i=0; i<10; i++) {
		if(f)
			i = 9;
		if(pt) {
			/*
			 * RxR
			 * RxR/B
			 * R/NxR
			 * R/1xR
			 * RxR/2
			 * R/N1xR
			 * RxR/B2
			 * R/QN1xR
			 * RxR/QB2
			 * R/QN1xR/QB2
			 */
			setxy("0012030404"[i], m>>6, &fx, &fy);
			setxy("0100203044"[i], m>>0, &tx, &ty);
		} else {
			/*
			 * R-N5
			 * R-QN5
			 * R/1-N5
			 * R/N-N5
			 * R/N1-N5
			 * R/1-QN5
			 * R/N-QN5
			 * R/N1-QN5
			 * R/QN1-N5
			 * R/QN1-QN5
			 */
			setxy("0021321344"[i], m>>6, &fx, &fy);
			setxy("3433344434"[i], m>>0, &tx, &ty);
		}
		n = 0;
		for(j=0; mv[j]; j++)
			if(match(mv[j], pf, t, fx, fy, pt, tx, ty))
				n++;

		if(n == 1) {
			str = putpie(pf, fx, fy, str);
			if(pt) {
				*str++ = 'x';
				str = putpie(pt, tx, ty, str);
			} else {
				*str++ = '-';
				str = putpla(tx, ty, str);
			}
			goto chk;
		}
	}
	*str++ = '?';
	*str++ = '?';
	*str++ = '?';
	*str++ = '?';
	*str = 0;
	return;

chk:
	if(t >= TNPRO && t <= TQPRO) {
		*str++ = '(';
		*str++ = "NBRQ"[t-TNPRO];
		*str++ = ')';
	}
	move(m);
	if(check()) {
		*str++ = '+';
		if(mate())
			*str++ = '+';
	}
	rmove();
	*str = 0;
}

void
setxy(int f, int xy, int *x, int *y)
{
	*x = -1;
	*y = -1;
	xy &= 077;
	switch(f) {
	case '2':
		*y = xy>>3;
		break;

	case '3':
		*y = xy>>3;

	case '1':
		*x = xy & 7;
		if(*x >= 5)
			*x = 15-*x;
		else
		if(*x <= 2)
			*x = 8+*x;
		break;

	case '4':
		*x = xy&7;
		*y = xy>>3;
		break;
	}
}

char*
putpie(int p, int x, int y, char *str)
{

	if(p == WPAWN && x >= 0 && y < 0) {
		str = putpla(x, y, str);
		x = -1;
	}
	*str++ = "XPNBRQKY"[p];
	if(x >= 0 || y >= 0)
		*str++ = '/';
	str = putpla(x, y, str);
	return str;
}

char*
putpla(int x, int y, char *str)
{

	if(x >= 0) {
		if(x < 8) {
			if(x < 3)
				*str++ = 'Q';
			if(x >= 5)
				*str++ = 'K';
			*str++ = "RNBQKBNR"[x];
		} else
			*str++ = "RNBQK"[x-8];
	}
	if(y >= 0)
		if(moveno&1)
			*str++ = '1'+y; else
			*str++ = '8'-y;
	return str;
}

int
mate(void)
{
	short mv[MAXMG];

	getmv(mv);
	return mv[0] == 0;
}

int
match(int m, int pf, int t, int fx, int fy, int pt, int tx, int ty)
{

	if(pf >= 0)
		if(!mat(pf, board[(m>>6)&077]&7))
			return 0;
	if(pt >= 0)
		if(!mat(pt, board[m&077]&7))
			return 0;
	if(t >= 0)
		if(((m>>12)&017) != t)
			return 0;
	if(fx >= 0)
		if(!mat(fx, (m>>6)&7))
			return 0;
	if(fy >= 0)
		if(!mat(fy, (m>>9)&7))
			return 0;
	if(tx >= 0)
		if(!mat(tx, (m>>0)&7))
			return 0;
	if(ty >= 0)
		if(!mat(ty, (m>>3)&7))
			return 0;
	return 1;
}

int
mat(int a, int b)
{

	if(a < 0)
		return 1;
	if(a >= 8) {
		if(a == 16) {
			if(b < 4)
				return 1;
			return 0;
		}
		if(a == 17) {
			if(b >= 4)
				return 1;
			return 0;
		}
		if(a-8 == b)
			return 1;
		if(15-a == b)
			return 1;
		return 0;
	}
	if(a == b)
		return 1;
	return 0;
}
