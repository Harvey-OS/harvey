/*
 * The authors of this software are Rob Pike and Ken Thompson.
 *              Copyright (c) 2002 by Lucent Technologies.
 * Permission to use, copy, modify, and distribute this software for any
 * purpose without fee is hereby granted, provided that this entire notice
 * is included in all copies of any software which is or includes a copy
 * or modification of this software and in all copies of the supporting
 * documentation for such software.
 * THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTY.  IN PARTICULAR, NEITHER THE AUTHORS NOR LUCENT TECHNOLOGIES MAKE ANY
 * REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE MERCHANTABILITY
 * OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR PURPOSE.
 */
/*
 * dofmt -- format to a buffer
 * the number of characters formatted is returned,
 * or -1 if there was an error.
 * if the buffer is ever filled, flush is called.
 * it should reset the buffer and return whether formatting should continue.
 */

typedef int (*Fmts)(Fmt*);

typedef struct Quoteinfo Quoteinfo;
struct Quoteinfo
{
	int	quoted;		/* if set, string must be quoted */
	int	nrunesin;	/* number of input runes that can be accepted */
	int	nbytesin;	/* number of input bytes that can be accepted */
	int	nrunesout;	/* number of runes that will be generated */
	int	nbytesout;	/* number of bytes that will be generated */
};

void	*_fmtflush(Fmt*, void*, int);
void	*_fmtdispatch(Fmt*, void*, int);
int	_floatfmt(Fmt*, double);
int	_fmtpad(Fmt*, int);
int	_rfmtpad(Fmt*, int);
int	_fmtFdFlush(Fmt*);

int	_efgfmt(Fmt*);
int	_charfmt(Fmt*);
int	_countfmt(Fmt*);
int	_flagfmt(Fmt*);
int	_percentfmt(Fmt*);
int	_ifmt(Fmt*);
int	_runefmt(Fmt*);
int	_runesfmt(Fmt*);
int	_strfmt(Fmt*);
int	_badfmt(Fmt*);
int	_fmtcpy(Fmt*, void*, int, int);
int	_fmtrcpy(Fmt*, void*, int n);

void	_fmtlock(void);
void	_fmtunlock(void);

#define FMTCHAR(f, t, s, c)\
	do{\
	if(t + 1 > (char*)s){\
		t = _fmtflush(f, t, 1);\
		if(t != nil)\
			s = f->stop;\
		else\
			return -1;\
	}\
	*t++ = c;\
	}while(0)

#define FMTRCHAR(f, t, s, c)\
	do{\
	if(t + 1 > (Rune*)s){\
		t = _fmtflush(f, t, sizeof(Rune));\
		if(t != nil)\
			s = f->stop;\
		else\
			return -1;\
	}\
	*t++ = c;\
	}while(0)

#define FMTRUNE(f, t, s, r)\
	do{\
	Rune _rune;\
	int _runelen;\
	if(t + UTFmax > (char*)s && t + (_runelen = runelen(r)) > (char*)s){\
		t = _fmtflush(f, t, _runelen);\
		if(t != nil)\
			s = f->stop;\
		else\
			return -1;\
	}\
	if(r < Runeself)\
		*t++ = r;\
	else{\
		_rune = r;\
		t += runetochar(t, &_rune);\
	}\
	}while(0)

#ifndef va_copy
#define	va_copy(a, b) ((a) = (b))
#endif
