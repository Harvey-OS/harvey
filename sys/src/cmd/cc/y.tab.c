
#line	2	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
#include "cc.h"

#line	4	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
typedef union 	{
	Node*	node;
	Sym*	sym;
	Type*	type;
	struct
	{
		Type*	t;
		char	c;
	} tycl;
	struct
	{
		Type*	t1;
		Type*	t2;
	} tyty;
	struct
	{
		char*	s;
		long	l;
	} sval;
	long	lval;
	double	dval;
	vlong	vval;
} YYSTYPE;
extern	int	yyerrflag;
#ifndef	YYMAXDEPTH
#define	YYMAXDEPTH	150
#endif
YYSTYPE	yylval;
YYSTYPE	yyval;
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
#define YYEOFCODE 1
#define YYERRCODE 2

#line	1158	"/n/sources/plan9/sys/src/cmd/cc/cc.y"

short	yyexca[] =
{-1, 1,
	1, -1,
	-2, 178,
-1, 35,
	4, 8,
	5, 8,
	6, 9,
	-2, 5,
-1, 52,
	93, 191,
	-2, 190,
-1, 55,
	93, 195,
	-2, 194,
-1, 57,
	93, 199,
	-2, 198,
-1, 76,
	6, 9,
	-2, 8,
-1, 305,
	4, 96,
	93, 82,
	-2, 0,
-1, 322,
	6, 22,
	-2, 21,
-1, 327,
	93, 82,
	-2, 96,
-1, 333,
	4, 96,
	93, 82,
	-2, 0,
-1, 383,
	4, 96,
	93, 82,
	-2, 0,
-1, 385,
	4, 96,
	93, 82,
	-2, 0,
-1, 387,
	4, 96,
	93, 82,
	-2, 0,
-1, 397,
	4, 96,
	93, 82,
	-2, 0,
-1, 403,
	4, 96,
	93, 82,
	-2, 0,
};
#define	YYNPROD	241
#define	YYPRIVATE 57344
#define	YYLAST	1206
short	yyact[] =
{
 175, 324, 256, 328, 209, 321, 203,  40, 207, 326,
  87, 341,  38, 342, 266,  23, 205,  52,  55,  57,
  89, 257,  46,  46,  85,   4,   5, 265,  79, 103,
 210, 124,  90, 201, 133, 136, 201, 135,  86,  41,
  42,  65, 140, 138,  41,  42, 268,  80,  41,  42,
  35, 369, 277, 252, 204, 254, 123,  54, 310,  46,
 140, 253, 308, 288,  46,  88,  46, 142, 403, 389,
 252, 388, 316, 315, 309, 293, 290, 140, 253, 129,
 287, 117, 131, 128, 117,  46,  66, 375, 118,  58,
 397,  68, 217,  81, 195, 254, 254, 194,  54,  25,
  26, 125,   5, 127, 254, 174, 254,  76, 254, 386,
 217, 365, 116, 364, 182, 183, 184, 185, 186, 187,
 188, 189, 303,  25,  26, 200,  82, 360,  34, 134,
 136, 129,  41,  42, 190, 192, 357, 140, 138,  41,
  42,  88, 295, 368, 221, 222, 223, 224, 225, 226,
 227, 228, 229, 230, 231, 232, 233, 234, 235, 236,
 237, 238,  81, 240, 241, 242, 243, 244, 245, 246,
 247, 248, 249, 250, 206, 239, 218, 258, 214, 219,
 213, 399, 387,  25,  26,  66, 260, 261,  44,  75,
 385, 259, 383,  53, 255, 216, 215, 356, 121,  64,
  63, 355,  43, 273,  56, 174,  37, 174, 251, 129,
  48, 269,  88, 278,  39,  41,  42,  88, 173, 136,
 283, 220, 282, 141, 270, 254, 140, 138,  41,  42,
 117, 271,  67, 272,   7, 320, 367,  67, 289, 279,
  37,  45,  45,  50, 285,  81, 371, 196,  39,  41,
  42, 301, 284,  69,   6, 286, 347, 348,  67,  25,
  26, 116, 252,  49, 119,  37, 122,  22, 292, 140,
 253,  88, 281,  39,  41,  42,  51, 269,  45,  37,
 202, 302,  78,  45, 254,  45, 307,  39,  41,  42,
 311, 218, 298, 306, 291, 258, 300, 296, 297,   5,
 304, 402,  88, 294,  45, 275, 276, 117,  59,  60,
  33, 312, 319, 269, 129, 318, 314, 280, 262, 132,
 263, 398, 346, 206, 396, 395,  24,  74, 384, 284,
 143, 144, 145,  47,  47, 358, 354, 352, 359, 351,
  37, 378, 376, 362, 366, 199, 363, 361,  39,  41,
  42, 322, 353, 350, 370, 146, 147, 143, 144, 145,
 373, 317, 299,  73,  72, 258, 258, 379, 380, 372,
  47, 374, 129,  70, 377,  47, 382,  47, 181, 180,
 178, 179, 177, 176,  71, 390, 329, 392, 391, 394,
 264, 198, 120, 349,  62, 322,  47,  94, 126, 400,
 393,  77, 401,  61,   3, 404,  95,  96,  93,   2,
   1, 100,  99, 139, 137, 208,  91,  84,  12, 109,
 108, 104, 105, 106, 107, 110, 111, 114, 115,  27,
 267, 327,  13,  36, 113, 112,  20, 305,  29,  19,
 274,  92, 325,  15,  16,  32,   8,  14, 101,   0,
  28,   9,   0,  30,  31,  10,  18,   0,  21,  11,
  17,  25,  26,  94, 102,   0,   0,   0,   0,  97,
  98,   0,  95,  96,  93,   0,   0, 100,  99,   0,
   0,   0,  91, 345,   0, 109, 108, 104, 105, 106,
 107, 110, 111, 114, 115,   0, 336, 343,   0, 337,
 344, 333,   0,   0,   0,   0, 331, 338, 330,   0,
 325,   0, 334,   0, 101, 339,   0,   0, 335,   0,
   0,   0,   0, 332,   0,   0,   0,   0,   0, 340,
 102,  94,   0,   0, 323,  97,  98,   0,   0,   0,
  95,  96,  93,   0,   0, 100,  99,   0,   0,   0,
  91, 345,   0, 109, 108, 104, 105, 106, 107, 110,
 111, 114, 115,   0, 336, 343,   0, 337, 344, 333,
   0,   0,   0,   0, 331, 338, 330,   0,   0,   0,
 334,   0, 101, 339,   0,   0, 335,   0,   0,   0,
   0, 332,   0,   0,   0,   0,  94, 340, 102,   0,
   0,   0,   0,  97,  98,  95,  96,  93,   0,   0,
 100,  99,   0,   0,   0,  91, 345,   0, 109, 108,
 104, 105, 106, 107, 110, 111, 114, 115,   0, 336,
 343,   0, 337, 344, 333,   0,   0,   0,   0, 331,
 338, 330,   0,   0,   0, 334,   0, 101, 339,   0,
   0, 335,   0,   0,   0,   0, 332,   0,   0,   0,
   0,  94, 340, 102,   0,   0,   0,   0,  97,  98,
  95,  96,  93,   0,   0, 100,  99,   0, 212, 211,
  91,  84,   0, 109, 108, 104, 105, 106, 107, 110,
 111, 114, 115,   0,  94, 149, 148, 146, 147, 143,
 144, 145,   0,  95,  96,  93,   0,   0, 100,  99,
   0,   0, 101,  91,  84,   0, 109, 108, 104, 105,
 106, 107, 110, 111, 114, 115,   0,  94, 102,   0,
   0, 130,   0,  97,  98,   0,  95,  96,  93,   0,
   0, 100,  99,   0,   0, 101,  91,  84,   0, 109,
 108, 104, 105, 106, 107, 110, 111, 114, 115,   0,
  94, 102,   0,   0, 130,   0,  97,  98,   0,  95,
  96,  93,   0,   0, 100,  99,   0,   0, 101,  91,
  84,   0, 109, 108, 104, 105, 106, 107, 110, 111,
 114, 115,   0,  94, 102,   0,   0, 313,   0,  97,
  98,   0,  95,  96,  93,   0,   0, 100,  99,   0,
   0, 101, 193,  84,   0, 109, 108, 104, 105, 106,
 107, 110, 111, 114, 115,   0,  94, 102,   0,   0,
   0,   0,  97,  98,   0,  95,  96,  93,   0,   0,
 100,  99,   0,   0, 101, 191,  84,   0, 109, 108,
 104, 105, 106, 107, 110, 111, 114, 115,   0,   0,
 102,   0,   0,   0,   0,  97,  98,   0,   0,   0,
   0,   0,   0,   0,   0,   0,  12, 101,   0,   0,
   0,   0,   0,   0,   0,   0,   0,  27,   0,   0,
  13,   0,   0, 102,  20,   0,  29,  19,  97,  98,
   0,  15,  16,  32,   0,  14,   0,   0,  28,   9,
   0,  30,  31,  10,  18,   0,  21,  11,  17,  25,
  26,  83,   0,   0,  84,  12, 197,   0,   0,   0,
   0,   0,   0,   0,   0,   0,  27,   0,   0,  13,
   0,   0,   0,  20,   0,  29,  19,   0,   0,   0,
  15,  16,  32,   0,  14,   0,   0,  28,   9,  12,
  30,  31,  10,  18,   0,  21,  11,  17,  25,  26,
  27,   0,   0,  13,   0,   0,   0,  20,   0,  29,
  19,   0,   0,   0,  15,  16,  32,   0,  14,   0,
   0,  28,   9,   0,  30,  31,  10,  18,   0,  21,
  11,  17,  25,  26,  27,   0,   0,  13,   0,   0,
   0,  20,   0,  29,  19,   0,   0,   0,  15,  16,
  32,   0,  14,   0,   0,  28,   0,   0,  30,  31,
   0,  18,   0,  21,   0,  17,  25,  26, 162, 163,
 164, 165, 166, 167, 169, 168, 170, 171, 172, 161,
 381, 160, 159, 158, 157, 156, 154, 155, 150, 151,
 152, 153, 149, 148, 146, 147, 143, 144, 145, 162,
 163, 164, 165, 166, 167, 169, 168, 170, 171, 172,
 161,   0, 160, 159, 158, 157, 156, 154, 155, 150,
 151, 152, 153, 149, 148, 146, 147, 143, 144, 145,
 161,   0, 160, 159, 158, 157, 156, 154, 155, 150,
 151, 152, 153, 149, 148, 146, 147, 143, 144, 145,
 159, 158, 157, 156, 154, 155, 150, 151, 152, 153,
 149, 148, 146, 147, 143, 144, 145, 158, 157, 156,
 154, 155, 150, 151, 152, 153, 149, 148, 146, 147,
 143, 144, 145, 157, 156, 154, 155, 150, 151, 152,
 153, 149, 148, 146, 147, 143, 144, 145, 156, 154,
 155, 150, 151, 152, 153, 149, 148, 146, 147, 143,
 144, 145, 154, 155, 150, 151, 152, 153, 149, 148,
 146, 147, 143, 144, 145, 150, 151, 152, 153, 149,
 148, 146, 147, 143, 144, 145
};
short	yypact[] =
{
-1000, 915,-1000, 306,-1000,-1000, 949, 949, 915,   5,
   5,  -4,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,
-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,
-1000,-1000,-1000,-1000, 304,-1000, 158,-1000,-1000, 245,
-1000,-1000,-1000, 949,-1000,-1000,-1000,-1000, 949,-1000,
 949,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,
 245,-1000, 276, 881, 737, 172,  -3,-1000,  12, 949,
 -36, 915, -36, -37,  58,-1000,-1000, 915, 671,  -9,
 314,-1000, 185, 183,-1000,-1000, -25,-1000,1063,-1000,
-1000, 374, 341, 737, 737, 737, 737, 737, 737, 737,
 737, 803, 770,-1000,-1000,-1000,-1000,-1000,-1000,-1000,
-1000,-1000,  44,  40,-1000,-1000,-1000,-1000,-1000,-1000,
 832,-1000,-1000,-1000,  31, 274, -39, 245,-1000,1063,
 638,-1000, 881,-1000,-1000,-1000,-1000, 154,   1,-1000,
 737, 181,-1000, 737, 737, 737, 737, 737, 737, 737,
 737, 737, 737, 737, 737, 737, 737, 737, 737, 737,
 737, 737, 737, 737, 737, 737, 737, 737, 737, 737,
 737, 737, 737, 228, 103,1063, 737, 737,  89,  89,
-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,
-1000, 374,-1000, 374,-1000,-1000,-1000,-1000, 206,  58,
-1000,  58, 737,-1000,-1000, 301,-1000, -42, 638, 312,
 266, 737,  89,-1000,  96, 881, 737,-1000, -11, -29,
-1000,-1000,-1000,-1000, 296, 296, 323, 323, 665, 665,
 665, 665,1169,1169,1158,1145,1131,1116,1100, 220,
1063,1063,1063,1063,1063,1063,1063,1063,1063,1063,
1063, -15,-1000,  19, 737,-1000, -16, 298,1063,  50,
-1000,-1000, 228, 228, 206, 358, 291,-1000,-1000, 233,
 737,  28,-1000,1063, 915,-1000, 245,-1000, 281, 266,
-1000,-1000, -30,-1000,-1000, -17, -34,-1000,-1000, 737,
 704,  36,-1000,-1000, 737,-1000, -18, -19, 357,-1000,
 206, 737,-1000,-1000, 231, 440,-1000,-1000,-1000,-1000,
-1000,1083,-1000, 638,-1000,-1000,-1000,-1000,-1000,-1000,
-1000, 252,-1000,-1000,-1000, 349,-1000, 573, 348, -39,
 159, 155,  94, 508, 737,  85, 343, 339,  89,  71,
  69,-1000, 279, 737, 218, 125, -43,-1000, 245, 240,
-1000,-1000,-1000,-1000,-1000, 737, 737, 737,   4, 338,
 737,-1000,-1000, 337, 737, 737,1032,-1000,-1000,-1000,
-1000, 671, 101, 324,  99,  67,-1000,  91,-1000, -20,
 -22,-1000,-1000, 508, 737, 508, 737, 508, 321, 320,
  27, 317,-1000,  90,-1000,-1000,-1000, 508, 737, 297,
-1000, -23,-1000, 508,-1000
};
short	yypgo[] =
{
   0,   7, 188, 267, 326,  15, 234, 202, 446,  41,
 126, 193, 254,  24,  28,  47,   3,  29,   6,   1,
  13,   0,  20, 441,   2,  21, 440, 437,  32, 435,
 434,  46, 433, 431,  11,   9,   5, 430,  12,  30,
 415,  34,  37, 414, 413,  38,  10,   4,   8, 410,
 409, 404, 128, 403, 401, 398, 394,  25, 393,  16,
 392, 391,  27, 390,  14, 386, 384, 373, 364, 363,
 345,  31, 327
};
short	yyr1[] =
{
   0,  49,  49,  50,  50,  53,  55,  50,  52,  56,
  52,  52,  31,  31,  32,  32,  32,  32,  26,  26,
  26,  36,  58,  36,  36,  54,  54,  59,  59,  61,
  60,  63,  60,  62,  62,  64,  64,  37,  37,  37,
  41,  41,  42,  42,  42,  43,  43,  43,  44,  44,
  44,  47,  47,  39,  39,  39,  40,  40,  40,  40,
  48,  48,  48,  14,  14,  15,  15,  15,  15,  15,
  18,  27,  27,  33,  33,  34,  34,  34,  19,  19,
  19,  35,  65,  35,  35,  35,  35,  35,  35,  35,
  35,  35,  35,  35,  35,  35,  16,  16,  45,  45,
  46,  20,  20,  21,  21,  21,  21,  21,  21,  21,
  21,  21,  21,  21,  21,  21,  21,  21,  21,  21,
  21,  21,  21,  21,  21,  21,  21,  21,  21,  21,
  21,  21,  21,  21,  22,  22,  22,  28,  28,  28,
  28,  28,  28,  28,  28,  28,  28,  28,  23,  23,
  23,  23,  23,  23,  23,  23,  23,  23,  23,  23,
  23,  23,  23,  23,  23,  23,  23,  23,  29,  29,
  30,  30,  24,  24,  25,  25,  66,  11,  51,  51,
  13,  13,  13,  13,  13,  13,  13,  13,  10,  57,
  12,  67,  12,  12,  12,  68,  12,  12,  12,  69,
  70,  12,  72,  12,  12,   7,   7,   9,   9,   2,
   2,   2,   8,   8,   3,   3,  71,  71,  71,  71,
   6,   6,   6,   6,   6,   6,   6,   6,   6,   4,
   4,   4,   4,   4,   4,   5,   5,  17,  38,   1,
   1
};
short	yyr2[] =
{
   0,   0,   2,   2,   3,   0,   0,   6,   1,   0,
   4,   3,   1,   3,   1,   3,   4,   4,   0,   3,
   4,   1,   0,   4,   3,   0,   4,   1,   3,   0,
   4,   0,   5,   0,   1,   1,   3,   1,   3,   2,
   0,   1,   2,   3,   1,   1,   4,   4,   2,   3,
   3,   1,   3,   3,   2,   2,   2,   3,   1,   2,
   1,   1,   2,   0,   1,   1,   2,   2,   3,   3,
   4,   0,   2,   1,   2,   3,   2,   2,   2,   1,
   2,   2,   0,   2,   5,   7,   9,   5,   7,   3,
   5,   2,   2,   3,   5,   5,   0,   1,   0,   1,
   1,   1,   3,   1,   3,   3,   3,   3,   3,   3,
   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,
   3,   3,   5,   3,   3,   3,   3,   3,   3,   3,
   3,   3,   3,   3,   1,   5,   7,   1,   2,   2,
   2,   2,   2,   2,   2,   2,   2,   2,   3,   5,
   5,   4,   4,   3,   3,   2,   2,   1,   1,   1,
   1,   1,   1,   1,   1,   1,   1,   1,   1,   2,
   1,   2,   0,   1,   1,   3,   0,   4,   0,   1,
   1,   1,   1,   2,   2,   3,   2,   3,   1,   1,
   2,   0,   4,   2,   2,   0,   4,   2,   2,   0,
   0,   7,   0,   5,   1,   1,   2,   0,   2,   1,
   1,   1,   1,   2,   1,   1,   1,   3,   2,   3,
   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
   1
};
short	yychk[] =
{
-1000, -49, -50, -51, -57, -13, -12,  -6,  -8,  77,
  81,  85,  44,  58,  73,  69,  70,  86,  82,  65,
  62,  84,  -3,  -5,  -4,  87,  88,  55,  76,  64,
  79,  80,  71,   4, -52, -31, -32,  34, -38,  42,
  -1,  43,  44,  -7,  -2,  -6,  -5,  -4,  -7, -12,
  -6,  -3,  -1, -11,  93,  -1, -11,  -1,  93,   4,
   5, -53, -56,  42,  41,  -9, -31,  -2,  -9,  -7,
 -67, -66, -68, -69, -72, -52, -31, -54,   6, -14,
 -15, -17, -10,  40,  43, -13, -45, -46, -21, -22,
 -28,  42, -23,  34,  23,  32,  33,  95,  96,  38,
  37,  74,  90, -17,  47,  48,  49,  50,  46,  45,
  51,  52, -29, -30,  53,  54, -31,  -5,  91, -11,
 -60, -10, -11,  93, -71,  43, -55, -57, -47, -21,
  93,  91,   5, -41, -31, -42,  34, -43,  42, -44,
  41,  40,  92,  34,  35,  36,  32,  33,  31,  30,
  26,  27,  28,  29,  24,  25,  23,  22,  21,  20,
  19,  17,   6,   7,   8,   9,  10,  11,  13,  12,
  14,  15,  16, -10, -20, -21,  42,  41,  39,  40,
  38,  37, -22, -22, -22, -22, -22, -22, -22, -22,
 -28,  42, -28,  42,  53,  54, -10,  94, -61, -70,
  94,   5,   6, -18,  93, -59, -31, -48, -40, -47,
 -39,  41,  40, -15,  -9,  42,  41,  91, -42, -45,
  40, -21, -21, -21, -21, -21, -21, -21, -21, -21,
 -21, -21, -21, -21, -21, -21, -21, -21, -21, -20,
 -21, -21, -21, -21, -21, -21, -21, -21, -21, -21,
 -21, -41,  34,  42,   5,  91, -24, -25, -21, -20,
  -1,  -1, -10, -10, -63, -62, -64, -37, -31, -38,
  18, -71, -71, -21, -26,   4,   5,  94, -47, -39,
   5,   6, -46,  -1, -42, -14, -45,  91,  92,  18,
  91,  -9, -20,  91,   5,  92, -41, -41, -62,   4,
   5,  18, -46,  94, -57, -27, -59,   5,  92,  91,
  92, -21, -22,  93, -25,  91,  91,   4, -64, -46,
   4, -36, -31,  94, -19,   2, -35, -33, -16, -65,
  68,  66,  83,  61,  72,  78,  56,  59,  67,  75,
  89, -34, -20,  57,  60,  43, -48,   4,   5, -58,
   4, -34, -35,   4, -18,  42,  42,  42, -19, -16,
  42,   4,   4,  -1,  42,  42, -21,  18,  18,  94,
 -36,   6, -20, -16, -20,  83,   4, -20,   4, -24,
 -24,  18, -47,  91,   4,  91,  42,  91,  91,  91,
 -19, -16, -19, -20, -19,   4,   4,  63,   4,  91,
 -19, -16,   4,  91, -19
};
short	yydef[] =
{
   1,  -2,   2,   0, 179, 189, 180, 181, 182,   0,
   0,   0, 204, 220, 221, 222, 223, 224, 225, 226,
 227, 228, 212, 214, 215, 235, 236, 229, 230, 231,
 232, 233, 234,   3,   0,  -2,  12, 207,  14,   0,
 238, 239, 240, 183, 205, 209, 210, 211, 184, 207,
 186, 213,  -2, 193, 176,  -2, 197,  -2, 202,   4,
   0,  25,   0,  63,  98,   0,   0, 206, 185, 187,
   0,   0,   0,   0,   0,  11,  -2,   6,   0,   0,
  64,  65,  40,   0, 237, 188,   0,  99, 100, 103,
 134,   0, 137,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0, 157, 158, 159, 160, 161, 162, 163,
 164, 165, 166, 167, 168, 170,  13, 208,  15, 192,
   0,  29, 196, 200,   0, 216,   0,   0,  10,  51,
   0,  16,   0,  66,  67,  41, 207,  44,   0,  45,
  98,   0,  17,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,  40,   0, 101, 172,   0,   0,   0,
 155, 156, 138, 139, 140, 141, 142, 143, 144, 145,
 146,   0, 147,   0, 169, 171,  31, 177,  33,   0,
 203, 218,   0,   7,  18,   0,  27,   0,  60,  61,
  58,   0,   0,  69,  42,  63,  98,  48,   0,   0,
  68, 104, 105, 106, 107, 108, 109, 110, 111, 112,
 113, 114, 115, 116, 117, 118, 119, 120, 121,   0,
 123, 124, 125, 126, 127, 128, 129, 130, 131, 132,
 133,   0, 207,   0,   0, 148,   0, 173, 174,   0,
 153, 154,  40,  40,  33,   0,  34,  35,  37,  14,
   0,   0, 219, 217,  71,  26,   0,  52,  62,  59,
  56,  55,   0,  54,  43,   0,   0,  50,  49,   0,
   0,  42, 102, 151,   0, 152,   0,   0,   0,  30,
   0,   0,  39, 201,   0,  -2,  28,  57,  53,  46,
  47, 122, 135,   0, 175, 149, 150,  32,  36,  38,
  19,   0,  -2,  70,  72,   0,  79,  -2,   0,   0,
   0,   0,   0,  -2,  96,   0,   0,   0,   0,   0,
   0,  73,  97,   0,   0, 237,   0,  20,   0,   0,
  78,  74,  80,  81,  83,   0,  96,   0,   0,   0,
   0,  91,  92,   0, 172, 172,   0,  76,  77, 136,
  24,   0,   0,   0,   0,   0,  89,   0,  93,   0,
   0,  75,  23,  -2,  96,  -2,   0,  -2,   0,   0,
  84,   0,  87,   0,  90,  94,  95,  -2,  96,   0,
  85,   0,  88,  -2,  86
};
short	yytok1[] =
{
   1,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,  95,   0,   0,   0,  36,  23,   0,
  42,  91,  34,  32,   5,  33,  40,  35,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,  18,   4,
  26,   6,  27,  17,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,  41,   0,  92,  22,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,  93,  21,  94,  96
};
short	yytok2[] =
{
   2,   3,   7,   8,   9,  10,  11,  12,  13,  14,
  15,  16,  19,  20,  24,  25,  28,  29,  30,  31,
  37,  38,  39,  43,  44,  45,  46,  47,  48,  49,
  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,
  60,  61,  62,  63,  64,  65,  66,  67,  68,  69,
  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,
  80,  81,  82,  83,  84,  85,  86,  87,  88,  89,
  90
};
long	yytok3[] =
{
   0
};
#define YYFLAG 		-1000
#define	yyclearin	yychar = -1
#define	yyerrok		yyerrflag = 0

#ifdef	yydebug
#include	"y.debug"
#else
#define	yydebug		0
char*	yytoknames[1];		/* for debugging */
char*	yystates[1];		/* for debugging */
#endif

/*	parser for yacc output	*/

int	yynerrs = 0;		/* number of errors */
int	yyerrflag = 0;		/* error recovery flag */

extern	int	fprint(int, char*, ...);
extern	int	sprint(char*, char*, ...);

char*
yytokname(int yyc)
{
	static char x[10];

	if(yyc > 0 && yyc <= sizeof(yytoknames)/sizeof(yytoknames[0]))
	if(yytoknames[yyc-1])
		return yytoknames[yyc-1];
	sprint(x, "<%d>", yyc);
	return x;
}

char*
yystatname(int yys)
{
	static char x[10];

	if(yys >= 0 && yys < sizeof(yystates)/sizeof(yystates[0]))
	if(yystates[yys])
		return yystates[yys];
	sprint(x, "<%d>\n", yys);
	return x;
}

long
yylex1(void)
{
	long yychar;
	long *t3p;
	int c;

	yychar = yylex();
	if(yychar <= 0) {
		c = yytok1[0];
		goto out;
	}
	if(yychar < sizeof(yytok1)/sizeof(yytok1[0])) {
		c = yytok1[yychar];
		goto out;
	}
	if(yychar >= YYPRIVATE)
		if(yychar < YYPRIVATE+sizeof(yytok2)/sizeof(yytok2[0])) {
			c = yytok2[yychar-YYPRIVATE];
			goto out;
		}
	for(t3p=yytok3;; t3p+=2) {
		c = t3p[0];
		if(c == yychar) {
			c = t3p[1];
			goto out;
		}
		if(c == 0)
			break;
	}
	c = 0;

out:
	if(c == 0)
		c = yytok2[1];	/* unknown char */
	if(yydebug >= 3)
		fprint(2, "lex %.4lux %s\n", yychar, yytokname(c));
	return c;
}

int
yyparse(void)
{
	struct
	{
		YYSTYPE	yyv;
		int	yys;
	} yys[YYMAXDEPTH], *yyp, *yypt;
	short *yyxi;
	int yyj, yym, yystate, yyn, yyg;
	long yychar;
	YYSTYPE save1, save2;
	int save3, save4;

	save1 = yylval;
	save2 = yyval;
	save3 = yynerrs;
	save4 = yyerrflag;

	yystate = 0;
	yychar = -1;
	yynerrs = 0;
	yyerrflag = 0;
	yyp = &yys[-1];
	goto yystack;

ret0:
	yyn = 0;
	goto ret;

ret1:
	yyn = 1;
	goto ret;

ret:
	yylval = save1;
	yyval = save2;
	yynerrs = save3;
	yyerrflag = save4;
	return yyn;

yystack:
	/* put a state and value onto the stack */
	if(yydebug >= 4)
		fprint(2, "char %s in %s", yytokname(yychar), yystatname(yystate));

	yyp++;
	if(yyp >= &yys[YYMAXDEPTH]) {
		yyerror("yacc stack overflow");
		goto ret1;
	}
	yyp->yys = yystate;
	yyp->yyv = yyval;

yynewstate:
	yyn = yypact[yystate];
	if(yyn <= YYFLAG)
		goto yydefault; /* simple state */
	if(yychar < 0)
		yychar = yylex1();
	yyn += yychar;
	if(yyn < 0 || yyn >= YYLAST)
		goto yydefault;
	yyn = yyact[yyn];
	if(yychk[yyn] == yychar) { /* valid shift */
		yychar = -1;
		yyval = yylval;
		yystate = yyn;
		if(yyerrflag > 0)
			yyerrflag--;
		goto yystack;
	}

yydefault:
	/* default state action */
	yyn = yydef[yystate];
	if(yyn == -2) {
		if(yychar < 0)
			yychar = yylex1();

		/* look through exception table */
		for(yyxi=yyexca;; yyxi+=2)
			if(yyxi[0] == -1 && yyxi[1] == yystate)
				break;
		for(yyxi += 2;; yyxi += 2) {
			yyn = yyxi[0];
			if(yyn < 0 || yyn == yychar)
				break;
		}
		yyn = yyxi[1];
		if(yyn < 0)
			goto ret0;
	}
	if(yyn == 0) {
		/* error ... attempt to resume parsing */
		switch(yyerrflag) {
		case 0:   /* brand new error */
			yyerror("syntax error");
			yynerrs++;
			if(yydebug >= 1) {
				fprint(2, "%s", yystatname(yystate));
				fprint(2, "saw %s\n", yytokname(yychar));
			}

		case 1:
		case 2: /* incompletely recovered error ... try again */
			yyerrflag = 3;

			/* find a state where "error" is a legal shift action */
			while(yyp >= yys) {
				yyn = yypact[yyp->yys] + YYERRCODE;
				if(yyn >= 0 && yyn < YYLAST) {
					yystate = yyact[yyn];  /* simulate a shift of "error" */
					if(yychk[yystate] == YYERRCODE)
						goto yystack;
				}

				/* the current yyp has no shift onn "error", pop stack */
				if(yydebug >= 2)
					fprint(2, "error recovery pops state %d, uncovers %d\n",
						yyp->yys, (yyp-1)->yys );
				yyp--;
			}
			/* there is no state on the stack with an error shift ... abort */
			goto ret1;

		case 3:  /* no shift yet; clobber input char */
			if(yydebug >= 2)
				fprint(2, "error recovery discards %s\n", yytokname(yychar));
			if(yychar == YYEOFCODE)
				goto ret1;
			yychar = -1;
			goto yynewstate;   /* try again in the same state */
		}
	}

	/* reduction by production yyn */
	if(yydebug >= 2)
		fprint(2, "reduce %d in:\n\t%s", yyn, yystatname(yystate));

	yypt = yyp;
	yyp -= yyr2[yyn];
	yyval = (yyp+1)->yyv;
	yym = yyn;

	/* consult goto table to find next state */
	yyn = yyr1[yyn];
	yyg = yypgo[yyn];
	yyj = yyg + yyp->yys + 1;

	if(yyj >= YYLAST || yychk[yystate=yyact[yyj]] != -yyn)
		yystate = yyact[yyg];
	switch(yym) {
		
case 3:
#line	74	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		dodecl(xdecl, lastclass, lasttype, Z);
	} break;
case 5:
#line	79	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		lastdcl = T;
		firstarg = S;
		dodecl(xdecl, lastclass, lasttype, yypt[-0].yyv.node);
		if(lastdcl == T || lastdcl->etype != TFUNC) {
			diag(yypt[-0].yyv.node, "not a function");
			lastdcl = types[TFUNC];
		}
		thisfn = lastdcl;
		markdcl();
		firstdcl = dclstack;
		argmark(yypt[-0].yyv.node, 0);
	} break;
case 6:
#line	93	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		argmark(yypt[-2].yyv.node, 1);
	} break;
case 7:
#line	97	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		Node *n;

		n = revertdcl();
		if(n)
			yypt[-0].yyv.node = new(OLIST, n, yypt[-0].yyv.node);
		if(!debug['a'] && !debug['Z'])
			codgen(yypt[-0].yyv.node, yypt[-4].yyv.node);
	} break;
case 8:
#line	109	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		dodecl(xdecl, lastclass, lasttype, yypt[-0].yyv.node);
	} break;
case 9:
#line	113	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yypt[-0].yyv.node = dodecl(xdecl, lastclass, lasttype, yypt[-0].yyv.node);
	} break;
case 10:
#line	117	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		doinit(yypt[-3].yyv.node->sym, yypt[-3].yyv.node->type, 0L, yypt[-0].yyv.node);
	} break;
case 13:
#line	125	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OIND, yypt[-0].yyv.node, Z);
		yyval.node->garb = simpleg(yypt[-1].yyv.lval);
	} break;
case 15:
#line	133	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = yypt[-1].yyv.node;
	} break;
case 16:
#line	137	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OFUNC, yypt[-3].yyv.node, yypt[-1].yyv.node);
	} break;
case 17:
#line	141	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OARRAY, yypt[-3].yyv.node, yypt[-1].yyv.node);
	} break;
case 18:
#line	149	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = Z;
	} break;
case 19:
#line	153	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = dodecl(adecl, lastclass, lasttype, Z);
		if(yypt[-2].yyv.node != Z)
			if(yyval.node != Z)
				yyval.node = new(OLIST, yypt[-2].yyv.node, yyval.node);
			else
				yyval.node = yypt[-2].yyv.node;
	} break;
case 20:
#line	162	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = yypt[-3].yyv.node;
		if(yypt[-1].yyv.node != Z) {
			yyval.node = yypt[-1].yyv.node;
			if(yypt[-3].yyv.node != Z)
				yyval.node = new(OLIST, yypt[-3].yyv.node, yypt[-1].yyv.node);
		}
	} break;
case 21:
#line	173	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		dodecl(adecl, lastclass, lasttype, yypt[-0].yyv.node);
		yyval.node = Z;
	} break;
case 22:
#line	178	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yypt[-0].yyv.node = dodecl(adecl, lastclass, lasttype, yypt[-0].yyv.node);
	} break;
case 23:
#line	182	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		long w;

		w = yypt[-3].yyv.node->sym->type->width;
		yyval.node = doinit(yypt[-3].yyv.node->sym, yypt[-3].yyv.node->type, 0L, yypt[-0].yyv.node);
		yyval.node = contig(yypt[-3].yyv.node->sym, yyval.node, w);
	} break;
case 24:
#line	190	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = yypt[-2].yyv.node;
		if(yypt[-0].yyv.node != Z) {
			yyval.node = yypt[-0].yyv.node;
			if(yypt[-2].yyv.node != Z)
				yyval.node = new(OLIST, yypt[-2].yyv.node, yypt[-0].yyv.node);
		}
	} break;
case 27:
#line	207	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		dodecl(pdecl, lastclass, lasttype, yypt[-0].yyv.node);
	} break;
case 29:
#line	217	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		lasttype = yypt[-0].yyv.type;
	} break;
case 31:
#line	222	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		lasttype = yypt[-0].yyv.type;
	} break;
case 33:
#line	228	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		lastfield = 0;
		edecl(CXXX, lasttype, S);
	} break;
case 35:
#line	236	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		dodecl(edecl, CXXX, lasttype, yypt[-0].yyv.node);
	} break;
case 37:
#line	243	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		lastbit = 0;
		firstbit = 1;
	} break;
case 38:
#line	248	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OBIT, yypt[-2].yyv.node, yypt[-0].yyv.node);
	} break;
case 39:
#line	252	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OBIT, Z, yypt[-0].yyv.node);
	} break;
case 40:
#line	260	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = (Z);
	} break;
case 42:
#line	267	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OIND, (Z), Z);
		yyval.node->garb = simpleg(yypt[-0].yyv.lval);
	} break;
case 43:
#line	272	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OIND, yypt[-0].yyv.node, Z);
		yyval.node->garb = simpleg(yypt[-1].yyv.lval);
	} break;
case 46:
#line	281	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OFUNC, yypt[-3].yyv.node, yypt[-1].yyv.node);
	} break;
case 47:
#line	285	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OARRAY, yypt[-3].yyv.node, yypt[-1].yyv.node);
	} break;
case 48:
#line	291	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OFUNC, (Z), Z);
	} break;
case 49:
#line	295	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OARRAY, (Z), yypt[-1].yyv.node);
	} break;
case 50:
#line	299	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = yypt[-1].yyv.node;
	} break;
case 52:
#line	306	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OINIT, invert(yypt[-1].yyv.node), Z);
	} break;
case 53:
#line	312	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OARRAY, yypt[-1].yyv.node, Z);
	} break;
case 54:
#line	316	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OELEM, Z, Z);
		yyval.node->sym = yypt[-0].yyv.sym;
	} break;
case 57:
#line	325	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OLIST, yypt[-2].yyv.node, yypt[-1].yyv.node);
	} break;
case 59:
#line	330	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OLIST, yypt[-1].yyv.node, yypt[-0].yyv.node);
	} break;
case 62:
#line	338	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OLIST, yypt[-1].yyv.node, yypt[-0].yyv.node);
	} break;
case 63:
#line	343	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = Z;
	} break;
case 64:
#line	347	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = invert(yypt[-0].yyv.node);
	} break;
case 66:
#line	355	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OPROTO, yypt[-0].yyv.node, Z);
		yyval.node->type = yypt[-1].yyv.type;
	} break;
case 67:
#line	360	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OPROTO, yypt[-0].yyv.node, Z);
		yyval.node->type = yypt[-1].yyv.type;
	} break;
case 68:
#line	365	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(ODOTDOT, Z, Z);
	} break;
case 69:
#line	369	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OLIST, yypt[-2].yyv.node, yypt[-0].yyv.node);
	} break;
case 70:
#line	375	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = invert(yypt[-1].yyv.node);
		if(yypt[-2].yyv.node != Z)
			yyval.node = new(OLIST, yypt[-2].yyv.node, yyval.node);
		if(yyval.node == Z)
			yyval.node = new(OLIST, Z, Z);
	} break;
case 71:
#line	384	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = Z;
	} break;
case 72:
#line	388	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OLIST, yypt[-1].yyv.node, yypt[-0].yyv.node);
	} break;
case 74:
#line	395	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OLIST, yypt[-1].yyv.node, yypt[-0].yyv.node);
	} break;
case 75:
#line	401	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OCASE, yypt[-1].yyv.node, Z);
	} break;
case 76:
#line	405	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OCASE, Z, Z);
	} break;
case 77:
#line	409	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OLABEL, dcllabel(yypt[-1].yyv.sym, 1), Z);
	} break;
case 78:
#line	415	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = Z;
	} break;
case 80:
#line	420	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OLIST, yypt[-1].yyv.node, yypt[-0].yyv.node);
	} break;
case 82:
#line	426	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		markdcl();
	} break;
case 83:
#line	430	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = revertdcl();
		if(yyval.node)
			yyval.node = new(OLIST, yyval.node, yypt[-0].yyv.node);
		else
			yyval.node = yypt[-0].yyv.node;
	} break;
case 84:
#line	438	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OIF, yypt[-2].yyv.node, new(OLIST, yypt[-0].yyv.node, Z));
		if(yypt[-0].yyv.node == Z)
			warn(yypt[-2].yyv.node, "empty if body");
	} break;
case 85:
#line	444	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OIF, yypt[-4].yyv.node, new(OLIST, yypt[-2].yyv.node, yypt[-0].yyv.node));
		if(yypt[-2].yyv.node == Z)
			warn(yypt[-4].yyv.node, "empty if body");
		if(yypt[-0].yyv.node == Z)
			warn(yypt[-4].yyv.node, "empty else body");
	} break;
case 86:
#line	452	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OFOR, new(OLIST, yypt[-4].yyv.node, new(OLIST, yypt[-6].yyv.node, yypt[-2].yyv.node)), yypt[-0].yyv.node);
	} break;
case 87:
#line	456	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OWHILE, yypt[-2].yyv.node, yypt[-0].yyv.node);
	} break;
case 88:
#line	460	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(ODWHILE, yypt[-2].yyv.node, yypt[-5].yyv.node);
	} break;
case 89:
#line	464	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(ORETURN, yypt[-1].yyv.node, Z);
		yyval.node->type = thisfn->link;
	} break;
case 90:
#line	469	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OCONST, Z, Z);
		yyval.node->vconst = 0;
		yyval.node->type = types[TINT];
		yypt[-2].yyv.node = new(OSUB, yyval.node, yypt[-2].yyv.node);

		yyval.node = new(OCONST, Z, Z);
		yyval.node->vconst = 0;
		yyval.node->type = types[TINT];
		yypt[-2].yyv.node = new(OSUB, yyval.node, yypt[-2].yyv.node);

		yyval.node = new(OSWITCH, yypt[-2].yyv.node, yypt[-0].yyv.node);
	} break;
case 91:
#line	483	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OBREAK, Z, Z);
	} break;
case 92:
#line	487	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OCONTINUE, Z, Z);
	} break;
case 93:
#line	491	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OGOTO, dcllabel(yypt[-1].yyv.sym, 0), Z);
	} break;
case 94:
#line	495	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OUSED, yypt[-2].yyv.node, Z);
	} break;
case 95:
#line	499	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OSET, yypt[-2].yyv.node, Z);
	} break;
case 96:
#line	504	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = Z;
	} break;
case 98:
#line	510	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = Z;
	} break;
case 100:
#line	517	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OCAST, yypt[-0].yyv.node, Z);
		yyval.node->type = types[TLONG];
	} break;
case 102:
#line	525	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OCOMMA, yypt[-2].yyv.node, yypt[-0].yyv.node);
	} break;
case 104:
#line	532	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OMUL, yypt[-2].yyv.node, yypt[-0].yyv.node);
	} break;
case 105:
#line	536	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(ODIV, yypt[-2].yyv.node, yypt[-0].yyv.node);
	} break;
case 106:
#line	540	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OMOD, yypt[-2].yyv.node, yypt[-0].yyv.node);
	} break;
case 107:
#line	544	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OADD, yypt[-2].yyv.node, yypt[-0].yyv.node);
	} break;
case 108:
#line	548	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OSUB, yypt[-2].yyv.node, yypt[-0].yyv.node);
	} break;
case 109:
#line	552	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OASHR, yypt[-2].yyv.node, yypt[-0].yyv.node);
	} break;
case 110:
#line	556	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OASHL, yypt[-2].yyv.node, yypt[-0].yyv.node);
	} break;
case 111:
#line	560	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OLT, yypt[-2].yyv.node, yypt[-0].yyv.node);
	} break;
case 112:
#line	564	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OGT, yypt[-2].yyv.node, yypt[-0].yyv.node);
	} break;
case 113:
#line	568	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OLE, yypt[-2].yyv.node, yypt[-0].yyv.node);
	} break;
case 114:
#line	572	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OGE, yypt[-2].yyv.node, yypt[-0].yyv.node);
	} break;
case 115:
#line	576	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OEQ, yypt[-2].yyv.node, yypt[-0].yyv.node);
	} break;
case 116:
#line	580	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(ONE, yypt[-2].yyv.node, yypt[-0].yyv.node);
	} break;
case 117:
#line	584	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OAND, yypt[-2].yyv.node, yypt[-0].yyv.node);
	} break;
case 118:
#line	588	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OXOR, yypt[-2].yyv.node, yypt[-0].yyv.node);
	} break;
case 119:
#line	592	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OOR, yypt[-2].yyv.node, yypt[-0].yyv.node);
	} break;
case 120:
#line	596	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OANDAND, yypt[-2].yyv.node, yypt[-0].yyv.node);
	} break;
case 121:
#line	600	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OOROR, yypt[-2].yyv.node, yypt[-0].yyv.node);
	} break;
case 122:
#line	604	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OCOND, yypt[-4].yyv.node, new(OLIST, yypt[-2].yyv.node, yypt[-0].yyv.node));
	} break;
case 123:
#line	608	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OAS, yypt[-2].yyv.node, yypt[-0].yyv.node);
	} break;
case 124:
#line	612	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OASADD, yypt[-2].yyv.node, yypt[-0].yyv.node);
	} break;
case 125:
#line	616	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OASSUB, yypt[-2].yyv.node, yypt[-0].yyv.node);
	} break;
case 126:
#line	620	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OASMUL, yypt[-2].yyv.node, yypt[-0].yyv.node);
	} break;
case 127:
#line	624	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OASDIV, yypt[-2].yyv.node, yypt[-0].yyv.node);
	} break;
case 128:
#line	628	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OASMOD, yypt[-2].yyv.node, yypt[-0].yyv.node);
	} break;
case 129:
#line	632	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OASASHL, yypt[-2].yyv.node, yypt[-0].yyv.node);
	} break;
case 130:
#line	636	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OASASHR, yypt[-2].yyv.node, yypt[-0].yyv.node);
	} break;
case 131:
#line	640	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OASAND, yypt[-2].yyv.node, yypt[-0].yyv.node);
	} break;
case 132:
#line	644	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OASXOR, yypt[-2].yyv.node, yypt[-0].yyv.node);
	} break;
case 133:
#line	648	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OASOR, yypt[-2].yyv.node, yypt[-0].yyv.node);
	} break;
case 135:
#line	655	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OCAST, yypt[-0].yyv.node, Z);
		dodecl(NODECL, CXXX, yypt[-3].yyv.type, yypt[-2].yyv.node);
		yyval.node->type = lastdcl;
	} break;
case 136:
#line	661	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OSTRUCT, yypt[-1].yyv.node, Z);
		dodecl(NODECL, CXXX, yypt[-5].yyv.type, yypt[-4].yyv.node);
		yyval.node->type = lastdcl;
	} break;
case 138:
#line	670	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OIND, yypt[-0].yyv.node, Z);
	} break;
case 139:
#line	674	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OADDR, yypt[-0].yyv.node, Z);
	} break;
case 140:
#line	678	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OPOS, yypt[-0].yyv.node, Z);
	} break;
case 141:
#line	682	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(ONEG, yypt[-0].yyv.node, Z);
	} break;
case 142:
#line	686	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(ONOT, yypt[-0].yyv.node, Z);
	} break;
case 143:
#line	690	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OCOM, yypt[-0].yyv.node, Z);
	} break;
case 144:
#line	694	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OPREINC, yypt[-0].yyv.node, Z);
	} break;
case 145:
#line	698	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OPREDEC, yypt[-0].yyv.node, Z);
	} break;
case 146:
#line	702	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OSIZE, yypt[-0].yyv.node, Z);
	} break;
case 147:
#line	706	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OSIGN, yypt[-0].yyv.node, Z);
	} break;
case 148:
#line	712	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = yypt[-1].yyv.node;
	} break;
case 149:
#line	716	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OSIZE, Z, Z);
		dodecl(NODECL, CXXX, yypt[-2].yyv.type, yypt[-1].yyv.node);
		yyval.node->type = lastdcl;
	} break;
case 150:
#line	722	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OSIGN, Z, Z);
		dodecl(NODECL, CXXX, yypt[-2].yyv.type, yypt[-1].yyv.node);
		yyval.node->type = lastdcl;
	} break;
case 151:
#line	728	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OFUNC, yypt[-3].yyv.node, Z);
		if(yypt[-3].yyv.node->op == ONAME)
		if(yypt[-3].yyv.node->type == T)
			dodecl(xdecl, CXXX, types[TINT], yyval.node);
		yyval.node->right = invert(yypt[-1].yyv.node);
	} break;
case 152:
#line	736	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OIND, new(OADD, yypt[-3].yyv.node, yypt[-1].yyv.node), Z);
	} break;
case 153:
#line	740	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(ODOT, new(OIND, yypt[-2].yyv.node, Z), Z);
		yyval.node->sym = yypt[-0].yyv.sym;
	} break;
case 154:
#line	745	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(ODOT, yypt[-2].yyv.node, Z);
		yyval.node->sym = yypt[-0].yyv.sym;
	} break;
case 155:
#line	750	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OPOSTINC, yypt[-1].yyv.node, Z);
	} break;
case 156:
#line	754	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OPOSTDEC, yypt[-1].yyv.node, Z);
	} break;
case 158:
#line	759	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OCONST, Z, Z);
		yyval.node->type = types[TINT];
		yyval.node->vconst = yypt[-0].yyv.vval;
	} break;
case 159:
#line	765	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OCONST, Z, Z);
		yyval.node->type = types[TLONG];
		yyval.node->vconst = yypt[-0].yyv.vval;
	} break;
case 160:
#line	771	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OCONST, Z, Z);
		yyval.node->type = types[TUINT];
		yyval.node->vconst = yypt[-0].yyv.vval;
	} break;
case 161:
#line	777	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OCONST, Z, Z);
		yyval.node->type = types[TULONG];
		yyval.node->vconst = yypt[-0].yyv.vval;
	} break;
case 162:
#line	783	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OCONST, Z, Z);
		yyval.node->type = types[TDOUBLE];
		yyval.node->fconst = yypt[-0].yyv.dval;
	} break;
case 163:
#line	789	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OCONST, Z, Z);
		yyval.node->type = types[TFLOAT];
		yyval.node->fconst = yypt[-0].yyv.dval;
	} break;
case 164:
#line	795	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OCONST, Z, Z);
		yyval.node->type = types[TVLONG];
		yyval.node->vconst = yypt[-0].yyv.vval;
	} break;
case 165:
#line	801	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OCONST, Z, Z);
		yyval.node->type = types[TUVLONG];
		yyval.node->vconst = yypt[-0].yyv.vval;
	} break;
case 168:
#line	811	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OSTRING, Z, Z);
		yyval.node->type = typ(TARRAY, types[TCHAR]);
		yyval.node->type->width = yypt[-0].yyv.sval.l + 1;
		yyval.node->cstring = yypt[-0].yyv.sval.s;
		yyval.node->sym = symstring;
		yyval.node->etype = TARRAY;
		yyval.node->class = CSTATIC;
	} break;
case 169:
#line	821	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		char *s;
		int n;

		n = yypt[-1].yyv.node->type->width - 1;
		s = alloc(n+yypt[-0].yyv.sval.l+MAXALIGN);

		memcpy(s, yypt[-1].yyv.node->cstring, n);
		memcpy(s+n, yypt[-0].yyv.sval.s, yypt[-0].yyv.sval.l);
		s[n+yypt[-0].yyv.sval.l] = 0;

		yyval.node = yypt[-1].yyv.node;
		yyval.node->type->width += yypt[-0].yyv.sval.l;
		yyval.node->cstring = s;
	} break;
case 170:
#line	839	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OLSTRING, Z, Z);
		yyval.node->type = typ(TARRAY, types[TUSHORT]);
		yyval.node->type->width = yypt[-0].yyv.sval.l + sizeof(ushort);
		yyval.node->rstring = (ushort*)yypt[-0].yyv.sval.s;
		yyval.node->sym = symstring;
		yyval.node->etype = TARRAY;
		yyval.node->class = CSTATIC;
	} break;
case 171:
#line	849	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		char *s;
		int n;

		n = yypt[-1].yyv.node->type->width - sizeof(ushort);
		s = alloc(n+yypt[-0].yyv.sval.l+MAXALIGN);

		memcpy(s, yypt[-1].yyv.node->rstring, n);
		memcpy(s+n, yypt[-0].yyv.sval.s, yypt[-0].yyv.sval.l);
		*(ushort*)(s+n+yypt[-0].yyv.sval.l) = 0;

		yyval.node = yypt[-1].yyv.node;
		yyval.node->type->width += yypt[-0].yyv.sval.l;
		yyval.node->rstring = (ushort*)s;
	} break;
case 172:
#line	866	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = Z;
	} break;
case 175:
#line	874	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OLIST, yypt[-2].yyv.node, yypt[-0].yyv.node);
	} break;
case 176:
#line	880	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.tyty.t1 = strf;
		yyval.tyty.t2 = strl;
		strf = T;
		strl = T;
		lastbit = 0;
		firstbit = 1;
	} break;
case 177:
#line	889	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.type = strf;
		strf = yypt[-2].yyv.tyty.t1;
		strl = yypt[-2].yyv.tyty.t2;
	} break;
case 178:
#line	896	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		lastclass = CXXX;
		lasttype = types[TINT];
	} break;
case 180:
#line	904	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.tycl.t = yypt[-0].yyv.type;
		yyval.tycl.c = CXXX;
	} break;
case 181:
#line	909	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.tycl.t = simplet(yypt[-0].yyv.lval);
		yyval.tycl.c = CXXX;
	} break;
case 182:
#line	914	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.tycl.t = simplet(yypt[-0].yyv.lval);
		yyval.tycl.c = simplec(yypt[-0].yyv.lval);
		yyval.tycl.t = garbt(yyval.tycl.t, yypt[-0].yyv.lval);
	} break;
case 183:
#line	920	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.tycl.t = yypt[-1].yyv.type;
		yyval.tycl.c = simplec(yypt[-0].yyv.lval);
		yyval.tycl.t = garbt(yyval.tycl.t, yypt[-0].yyv.lval);
		if(yypt[-0].yyv.lval & ~BCLASS & ~BGARB)
			diag(Z, "duplicate types given: %T and %Q", yypt[-1].yyv.type, yypt[-0].yyv.lval);
	} break;
case 184:
#line	928	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.tycl.t = simplet(typebitor(yypt[-1].yyv.lval, yypt[-0].yyv.lval));
		yyval.tycl.c = simplec(yypt[-0].yyv.lval);
		yyval.tycl.t = garbt(yyval.tycl.t, yypt[-0].yyv.lval);
	} break;
case 185:
#line	934	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.tycl.t = yypt[-1].yyv.type;
		yyval.tycl.c = simplec(yypt[-2].yyv.lval);
		yyval.tycl.t = garbt(yyval.tycl.t, yypt[-2].yyv.lval|yypt[-0].yyv.lval);
	} break;
case 186:
#line	940	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.tycl.t = simplet(yypt[-0].yyv.lval);
		yyval.tycl.c = simplec(yypt[-1].yyv.lval);
		yyval.tycl.t = garbt(yyval.tycl.t, yypt[-1].yyv.lval);
	} break;
case 187:
#line	946	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.tycl.t = simplet(typebitor(yypt[-1].yyv.lval, yypt[-0].yyv.lval));
		yyval.tycl.c = simplec(yypt[-2].yyv.lval|yypt[-0].yyv.lval);
		yyval.tycl.t = garbt(yyval.tycl.t, yypt[-2].yyv.lval|yypt[-0].yyv.lval);
	} break;
case 188:
#line	954	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.type = yypt[-0].yyv.tycl.t;
		if(yypt[-0].yyv.tycl.c != CXXX)
			diag(Z, "illegal combination of class 4: %s", cnames[yypt[-0].yyv.tycl.c]);
	} break;
case 189:
#line	962	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		lasttype = yypt[-0].yyv.tycl.t;
		lastclass = yypt[-0].yyv.tycl.c;
	} break;
case 190:
#line	969	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		dotag(yypt[-0].yyv.sym, TSTRUCT, 0);
		yyval.type = yypt[-0].yyv.sym->suetag;
	} break;
case 191:
#line	974	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		dotag(yypt[-0].yyv.sym, TSTRUCT, autobn);
	} break;
case 192:
#line	978	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.type = yypt[-2].yyv.sym->suetag;
		if(yyval.type->link != T)
			diag(Z, "redeclare tag: %s", yypt[-2].yyv.sym->name);
		yyval.type->link = yypt[-0].yyv.type;
		suallign(yyval.type);
	} break;
case 193:
#line	986	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		taggen++;
		sprint(symb, "_%d_", taggen);
		yyval.type = dotag(lookup(), TSTRUCT, autobn);
		yyval.type->link = yypt[-0].yyv.type;
		suallign(yyval.type);
	} break;
case 194:
#line	994	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		dotag(yypt[-0].yyv.sym, TUNION, 0);
		yyval.type = yypt[-0].yyv.sym->suetag;
	} break;
case 195:
#line	999	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		dotag(yypt[-0].yyv.sym, TUNION, autobn);
	} break;
case 196:
#line	1003	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.type = yypt[-2].yyv.sym->suetag;
		if(yyval.type->link != T)
			diag(Z, "redeclare tag: %s", yypt[-2].yyv.sym->name);
		yyval.type->link = yypt[-0].yyv.type;
		suallign(yyval.type);
	} break;
case 197:
#line	1011	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		taggen++;
		sprint(symb, "_%d_", taggen);
		yyval.type = dotag(lookup(), TUNION, autobn);
		yyval.type->link = yypt[-0].yyv.type;
		suallign(yyval.type);
	} break;
case 198:
#line	1019	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		dotag(yypt[-0].yyv.sym, TENUM, 0);
		yyval.type = yypt[-0].yyv.sym->suetag;
		if(yyval.type->link == T)
			yyval.type->link = types[TINT];
		yyval.type = yyval.type->link;
	} break;
case 199:
#line	1027	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		dotag(yypt[-0].yyv.sym, TENUM, autobn);
	} break;
case 200:
#line	1031	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		en.tenum = T;
		en.cenum = T;
	} break;
case 201:
#line	1036	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.type = yypt[-5].yyv.sym->suetag;
		if(yyval.type->link != T)
			diag(Z, "redeclare tag: %s", yypt[-5].yyv.sym->name);
		if(en.tenum == T) {
			diag(Z, "enum type ambiguous: %s", yypt[-5].yyv.sym->name);
			en.tenum = types[TINT];
		}
		yyval.type->link = en.tenum;
		yyval.type = en.tenum;
	} break;
case 202:
#line	1048	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		en.tenum = T;
		en.cenum = T;
	} break;
case 203:
#line	1053	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.type = en.tenum;
	} break;
case 204:
#line	1057	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.type = tcopy(yypt[-0].yyv.sym->type);
	} break;
case 206:
#line	1064	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.lval = typebitor(yypt[-1].yyv.lval, yypt[-0].yyv.lval);
	} break;
case 207:
#line	1069	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.lval = 0;
	} break;
case 208:
#line	1073	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.lval = typebitor(yypt[-1].yyv.lval, yypt[-0].yyv.lval);
	} break;
case 213:
#line	1085	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.lval = typebitor(yypt[-1].yyv.lval, yypt[-0].yyv.lval);
	} break;
case 216:
#line	1095	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		doenum(yypt[-0].yyv.sym, Z);
	} break;
case 217:
#line	1099	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		doenum(yypt[-2].yyv.sym, yypt[-0].yyv.node);
	} break;
case 220:
#line	1106	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{ yyval.lval = BCHAR; } break;
case 221:
#line	1107	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{ yyval.lval = BSHORT; } break;
case 222:
#line	1108	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{ yyval.lval = BINT; } break;
case 223:
#line	1109	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{ yyval.lval = BLONG; } break;
case 224:
#line	1110	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{ yyval.lval = BSIGNED; } break;
case 225:
#line	1111	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{ yyval.lval = BUNSIGNED; } break;
case 226:
#line	1112	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{ yyval.lval = BFLOAT; } break;
case 227:
#line	1113	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{ yyval.lval = BDOUBLE; } break;
case 228:
#line	1114	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{ yyval.lval = BVOID; } break;
case 229:
#line	1117	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{ yyval.lval = BAUTO; } break;
case 230:
#line	1118	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{ yyval.lval = BSTATIC; } break;
case 231:
#line	1119	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{ yyval.lval = BEXTERN; } break;
case 232:
#line	1120	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{ yyval.lval = BTYPEDEF; } break;
case 233:
#line	1121	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{ yyval.lval = BTYPESTR; } break;
case 234:
#line	1122	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{ yyval.lval = BREGISTER; } break;
case 235:
#line	1125	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{ yyval.lval = BCONSTNT; } break;
case 236:
#line	1126	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{ yyval.lval = BVOLATILE; } break;
case 237:
#line	1130	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(ONAME, Z, Z);
		if(yypt[-0].yyv.sym->class == CLOCAL)
			yypt[-0].yyv.sym = mkstatic(yypt[-0].yyv.sym);
		yyval.node->sym = yypt[-0].yyv.sym;
		yyval.node->type = yypt[-0].yyv.sym->type;
		yyval.node->etype = TVOID;
		if(yyval.node->type != T)
			yyval.node->etype = yyval.node->type->etype;
		yyval.node->xoffset = yypt[-0].yyv.sym->offset;
		yyval.node->class = yypt[-0].yyv.sym->class;
		yypt[-0].yyv.sym->aused = 1;
	} break;
case 238:
#line	1145	"/n/sources/plan9/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(ONAME, Z, Z);
		yyval.node->sym = yypt[-0].yyv.sym;
		yyval.node->type = yypt[-0].yyv.sym->type;
		yyval.node->etype = TVOID;
		if(yyval.node->type != T)
			yyval.node->etype = yyval.node->type->etype;
		yyval.node->xoffset = yypt[-0].yyv.sym->offset;
		yyval.node->class = yypt[-0].yyv.sym->class;
	} break;
	}
	goto yystack;  /* stack new state and value */
}
