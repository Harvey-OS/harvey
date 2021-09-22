typedef struct Atr Atr;
typedef struct AtrPar AtrPar;

enum{
	Tagrdatr,
	Maxatr,
};


struct Atr{
	int id;
	int natr;
	uchar	atr[64];
	char desc[256];
	Iccops *o;
};

extern Atr atrlist[];


Atr *listmatchatr(uchar *atr, int natr);
void	fillparam(Param *p, ParsedAtr *pa);
int	parseatr(ParsedAtr *pa, uchar *atr, int size);