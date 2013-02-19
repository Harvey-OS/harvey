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
