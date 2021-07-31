#include <u.h>
#include <libc.h>

/* from doprint.c */

extern int printcol;

enum
{
	NONE	= -1000,
	FMINUS	= 1<<1,
};

/* end from doprint.c */

static	void
qchar(Rune c, Fconv *fp, int hold, int *closed)
{
	int n;

	if(*closed)
		return;
	n = fp->eout - fp->out - hold;
	if(n > 0) {
		if(c < Runeself) {
			*fp->out++ = c;
			return;
		}
		if(n >= UTFmax || n >= runelen(c)) {
			n = runetochar(fp->out, &c);
			fp->out += n;
			return;
		}
	}
	if(hold)
		*closed = 1;
	else
		fp->eout = fp->out;
}

int
_needsquotes(char *s, int *quotelenp)
{
	char *t;
	int nextra, ok, w;
	Rune r;

	if(*s == '\0')
		return 1;
	nextra = 0;
	ok = 1;
	for(t=s; *t; t+=w){
		w = chartorune(&r, t);
		if(r <= L' ')
			ok = 0;
		else if(r == L'\''){
			ok = 0;
			nextra++;	/* ' becomes '' */
		}else if(doquote!=nil && doquote(r))
			ok = 0;
	}

	if(ok){
		*quotelenp = t-s;
		return 0;
	}
	*quotelenp = 1+ t-s + nextra +1;
	return 1;
}

int
_runeneedsquotes(Rune *s, int *quotelenp)
{
	Rune *t;
	int nextra, ok;
	Rune r;

	if(*s == L'\0')
		return 1;
	nextra = 0;
	ok = 1;
	for(t=s; *t; t++){
		r = *t;
		if(r <= L' ')
			ok = 0;
		else if(r == L'\''){
			ok = 0;
			nextra++;	/* ' becomes '' */
		}else if(doquote!=nil && doquote(r))
			ok = 0;
	}

	if(ok){
		*quotelenp = t-s;
		return 0;
	}
	*quotelenp = 1+ t-s + nextra +1;
	return 1;
}

static void
qadv(Rune c, Fconv *fp, int hold, int *np, int *closedp)
{
	if(fp->f2 == NONE || fp->f2 > 0) {
		qchar(c, fp, hold, closedp);
		(*np)++;
		if(fp->f2 != NONE)
			fp->f2--;
		switch(c) {
		default:
			printcol++;
			break;
		case '\n':
			printcol = 0;
			break;
		case '\t':
			printcol = (printcol+8) & ~7;
			break;
		}
	}
}

static int
qstrconv(char *s, Rune *r, int quotelen, Fconv *fp)
{
	int closed, i, n, c;
	Rune rune;

	closed = 0;
	if(fp->f3 & FMINUS)
		fp->f1 = -fp->f1;
	n = 0;
	if(fp->f1 != NONE && fp->f1 >= 0) {
		n = quotelen;
		while(n < fp->f1) {
			qchar(' ', fp, 0, &closed);
			printcol++;
			n++;
		}
	}

	if(fp->eout - fp->out < 2)	/* 2 because need at least '' */
		return 0;

	qchar('\'', fp, 0, &closed);
	n++;

	for(;;) {
		if(s){
			c = *s & 0xff;
			if(c >= Runeself) {
				i = chartorune(&rune, s);
				s += i;
				c = rune;
			} else
				s++;
		}else
			c = *r++;

		if(c == 0)
			break;
		if(c == '\'')
			qadv(c, fp, 2, &n, &closed);	/* '' plus closing quote */
		qadv(c, fp, 1, &n, &closed);
	}
	closed = 0;	/* there will always be room for closing quote */
	qchar('\'', fp, 0, &closed);
	n++;

	if(fp->f1 != NONE && fp->f1 < 0) {
		fp->f1 = -fp->f1;
		while(n < fp->f1) {
			qchar(' ', fp, 0, &closed);
			printcol++;
			n++;
		}
	}

	return 0;

}

int
quotestrconv(va_list *arg, Fconv *fp)
{
	char *s;
	int quotelen;

	s = va_arg(*arg, char*);
	if(s == nil){
		strconv("<null>", fp);
		return 0;
	}

	if(_needsquotes(s, &quotelen) == 0){
		strconv(s, fp);
		va_end(arg);
		return 0;
	}

	qstrconv(s, nil, quotelen, fp);

	va_end(arg);
	return 0;
}

int
quoterunestrconv(va_list *arg, Fconv *fp)
{
	Rune *r;
	int quotelen;

	r = va_arg(*arg, Rune*);
	if(r == nil){
		strconv("<null>", fp);
		return 0;
	}

	if(_runeneedsquotes(r, &quotelen) == 0){
		Strconv(r, fp);
		va_end(arg);
		return 0;
	}

	qstrconv(nil, r, quotelen, fp);

	va_end(arg);
	return 0;
}

void
quotefmtinstall(void)
{
	fmtinstall('q', quotestrconv);
	fmtinstall('Q', quoterunestrconv);
}
