
typedef union 	{
	Node*	node;
	Sym*	sym;
	Type*	type;
	struct
	{
		Type*	t;
		uchar	c;
	} tycl;
	struct
	{
		Type*	t1;
		Type*	t2;
		Type*	t3;
		uchar	c;
	} tyty;
	struct
	{
		char*	s;
		long	l;
	} sval;
	long	lval;
	double	dval;
	vlong	vval;
}	YYSTYPE;
extern	YYSTYPE	yylval;
#define	LPE	57346
#define	LME	57347
#define	LMLE	57348
#define	LDVE	57349
#define	LMDE	57350
#define	LRSHE	57351
#define	LLSHE	57352
#define	LANDE	57353
#define	LXORE	57354
#define	LORE	57355
#define	LOROR	57356
#define	LANDAND	57357
#define	LEQ	57358
#define	LNE	57359
#define	LLE	57360
#define	LGE	57361
#define	LLSH	57362
#define	LRSH	57363
#define	LMM	57364
#define	LPP	57365
#define	LMG	57366
#define	LNAME	57367
#define	LTYPE	57368
#define	LFCONST	57369
#define	LDCONST	57370
#define	LCONST	57371
#define	LLCONST	57372
#define	LUCONST	57373
#define	LULCONST	57374
#define	LVLCONST	57375
#define	LUVLCONST	57376
#define	LSTRING	57377
#define	LLSTRING	57378
#define	LAUTO	57379
#define	LBREAK	57380
#define	LCASE	57381
#define	LCHAR	57382
#define	LCONTINUE	57383
#define	LDEFAULT	57384
#define	LDO	57385
#define	LDOUBLE	57386
#define	LELSE	57387
#define	LEXTERN	57388
#define	LFLOAT	57389
#define	LFOR	57390
#define	LGOTO	57391
#define	LIF	57392
#define	LINT	57393
#define	LLONG	57394
#define	LREGISTER	57395
#define	LRETURN	57396
#define	LSHORT	57397
#define	LSIZEOF	57398
#define	LUSED	57399
#define	LSTATIC	57400
#define	LSTRUCT	57401
#define	LSWITCH	57402
#define	LTYPEDEF	57403
#define	LTYPESTR	57404
#define	LUNION	57405
#define	LUNSIGNED	57406
#define	LWHILE	57407
#define	LVOID	57408
#define	LENUM	57409
#define	LSIGNED	57410
#define	LCONSTNT	57411
#define	LVOLATILE	57412
#define	LSET	57413
#define	LSIGNOF	57414
#define	LRESTRICT	57415
#define	LINLINE	57416
