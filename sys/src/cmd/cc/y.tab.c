
#line	2	"/sys/src/cmd/cc/cc.y"
#include "cc.h"

#line	4	"/sys/src/cmd/cc/cc.y"
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
#define	LRESTRICT	57415
#define	LINLINE	57416
#define YYEOFCODE 1
#define YYERRCODE 2
#define YYEMPTY (-2)

#line	1183	"/sys/src/cmd/cc/cc.y"

short	yyexca[] =
{-1, 1,
	1, -1,
	-2, 181,
-1, 37,
	4, 8,
	5, 8,
	6, 9,
	-2, 5,
-1, 54,
	95, 194,
	-2, 193,
-1, 57,
	95, 198,
	-2, 197,
-1, 59,
	95, 202,
	-2, 201,
-1, 78,
	6, 9,
	-2, 8,
-1, 276,
	4, 99,
	66, 88,
	95, 84,
	-2, 0,
-1, 312,
	66, 88,
	95, 84,
	-2, 99,
-1, 318,
	4, 99,
	66, 88,
	95, 84,
	-2, 0,
-1, 347,
	6, 21,
	-2, 20,
-1, 384,
	4, 99,
	66, 88,
	95, 84,
	-2, 0,
-1, 388,
	4, 99,
	66, 88,
	95, 84,
	-2, 0,
-1, 390,
	4, 99,
	66, 88,
	95, 84,
	-2, 0,
-1, 402,
	4, 99,
	66, 88,
	95, 84,
	-2, 0,
-1, 409,
	4, 99,
	66, 88,
	95, 84,
	-2, 0,
};
#define	YYNPROD	246
#define	YYPRIVATE 57344
#define	YYLAST	1239
short	yyact[] =
{
 177, 308, 313, 346, 211, 209,  87,   4,   5,  42,
 311, 205,  89, 327, 258, 326, 268, 259,  81,  54,
  57,  59,  91,  23,  40,  67, 207, 267, 212, 126,
  48,  48,  92, 138, 135, 203, 203, 137,  88, 381,
 142, 140,  43,  44, 138,  82, 270, 105, 279,  39,
  37, 142, 140,  43,  44, 206, 256,  41,  43,  44,
 125,  56, 335,  43,  44,  43,  44,  90, 333,  48,
 290, 254, 144, 409,  48, 254,  48,  70, 142, 255,
 392, 131, 142, 255, 391, 130,   5, 129,  68, 341,
 340, 119, 219, 334, 119,  48, 256,  25,  26, 295,
 256,  27,  25,  26, 256, 292,  27, 176, 256,  78,
 256, 289,  84,  83, 118,  60, 133,  56, 184, 185,
 186, 187, 188, 189, 190, 191, 305, 202,  25,  26,
 219, 136,  27, 131,  25,  26, 192, 194,  27, 120,
 374, 354, 402,  90, 197, 297, 223, 224, 225, 226,
 227, 228, 229, 230, 231, 232, 233, 234, 235, 236,
 237, 238, 239, 240, 216, 242, 243, 244, 245, 246,
 247, 248, 249, 250, 251, 252, 208, 241, 220, 260,
 215, 221,  83,  55, 404, 196, 123,  68, 390,  36,
 262, 263, 388, 261,  58,   7, 384, 127, 257,  46,
  45, 389,  47,  47,  52, 275, 175, 176,  50, 176,
 253, 131,  43,  44,  90, 280, 218, 217, 272,  90,
 138, 345,  66,  65, 285, 271, 284, 142, 140,  43,
  44, 273, 372, 274,  39, 198, 287, 363,  35, 281,
 119,  47,  41,  43,  44,  69,  47, 362,  47, 358,
  69,  39,  77,  71, 286, 355, 121, 288, 124,  41,
  43,  44, 353, 118, 222,  83, 143,  47,  39, 366,
 294,  69, 365,  90, 254, 303,  41,  43,  44,  39,
 293, 142, 255,   5, 309, 304,   6,  41,  43,  44,
 256, 271, 336, 220, 300,  51, 383, 260,  22, 298,
 299, 283,  24, 291,  90, 331, 264,  53, 265,  49,
  49, 145, 146, 147, 339, 337, 344, 119, 204, 343,
 356,  80, 357, 350, 369, 208, 352, 271, 349, 364,
 256, 286, 332, 361, 148, 149, 145, 146, 147, 131,
 368, 369, 277, 278, 367,  61,  62, 302,  49, 296,
 282, 134, 407,  49, 406,  49, 347, 183, 182, 180,
 181, 179, 178, 260, 260, 401, 400, 371, 395, 373,
 377, 375, 376, 382,  49, 386, 360, 378, 379,   5,
 387, 359, 351, 348, 131, 342, 394, 301, 393,  76,
 397, 396, 399, 201,  75,  74, 310,  72, 403,  73,
 316, 314, 266, 398, 405, 200, 122, 370,  64, 408,
 128, 410,  79,  63,   3,   2, 347,  96, 151, 150,
 148, 149, 145, 146, 147,   1,  97,  98,  95, 385,
 141, 102, 101, 139, 347, 210,  93, 330,  12, 111,
 110, 106, 107, 108, 109, 112, 113, 116, 117,  28,
 321, 328,  13, 322, 329, 318,  20, 269,  30,  19,
 312, 323, 315,  15,  16,  33, 319,  14, 103, 324,
  29,   9, 320,  31,  32,  10,  18, 317,  21,  11,
  17,  25,  26, 325, 104,  27,  34,  96,  38, 115,
 306,  99, 100, 114, 276, 307,  97,  98,  95,  94,
   8, 102, 101,   0,   0,   0,  93,  86,  12, 111,
 110, 106, 107, 108, 109, 112, 113, 116, 117,  28,
   0,   0,  13,   0,   0,   0,  20,   0,  30,  19,
   0,   0,   0,  15,  16,  33, 310,  14, 103,   0,
  29,   9,   0,  31,  32,  10,  18,   0,  21,  11,
  17,  25,  26,   0, 104,  27,  34,  96,   0,   0,
   0,  99, 100,   0,   0,   0,  97,  98,  95,   0,
   0, 102, 101,   0,   0,   0,  93, 330,   0, 111,
 110, 106, 107, 108, 109, 112, 113, 116, 117,   0,
 321, 328,   0, 322, 329, 318,   0,   0,   0,   0,
   0, 323, 315,   0,   0,   0, 319,   0, 103, 324,
   0,   0, 320,   0,   0,   0,   0, 317,   0,  96,
   0,   0,   0, 325, 104,   0,   0,   0,  97,  98,
  95,  99, 100, 102, 101,   0,   0,   0,  93, 330,
   0, 111, 110, 106, 107, 108, 109, 112, 113, 116,
 117,   0, 321, 328,   0, 322, 329, 318,   0,   0,
   0,   0,   0, 323, 315,   0,   0,   0, 319,   0,
 103, 324,   0,   0, 320,   0,   0,   0,   0, 317,
   0,  96,   0,   0,   0, 325, 104,   0,   0,   0,
  97,  98,  95,  99, 100, 102, 101,   0, 214, 213,
  93,  86,   0, 111, 110, 106, 107, 108, 109, 112,
 113, 116, 117,   0,  96,   0,   0,   0,   0,   0,
   0,   0,   0,  97,  98,  95,   0,   0, 102, 101,
   0,   0, 103,  93,  86,   0, 111, 110, 106, 107,
 108, 109, 112, 113, 116, 117,   0,   0, 104,  96,
   0,   0,   0, 132,   0,  99, 100,   0,  97,  98,
  95,   0,   0, 102, 101, 103,   0,   0,  93,  86,
   0, 111, 110, 106, 107, 108, 109, 112, 113, 116,
 117, 104,  96,   0,   0,   0, 132,   0,  99, 100,
   0,  97,  98,  95,   0,   0, 102, 101,   0,   0,
 103,  93,  86,   0, 111, 110, 106, 107, 108, 109,
 112, 113, 116, 117,   0,   0, 104,   0,   0,   0,
   0, 338,   0,  99, 100,   0,  12,   0,   0,   0,
   0,   0,   0, 103,   0,   0,   0,  28,   0,   0,
  13,   0,   0,   0,  20,   0,  30,  19,   0, 104,
   0,  15,  16,  33,   0,  14,  99, 100,  29,   9,
   0,  31,  32,  10,  18,   0,  21,  11,  17,  25,
  26,  96,   0,  27,  34,   0,   0,   0, 199,   0,
  97,  98,  95,   0,   0, 102, 101,   0,   0,   0,
 195,  86,   0, 111, 110, 106, 107, 108, 109, 112,
 113, 116, 117,   0,  96,   0,   0,   0,   0,   0,
   0,   0,   0,  97,  98,  95,   0,   0, 102, 101,
   0,   0, 103, 193,  86,   0, 111, 110, 106, 107,
 108, 109, 112, 113, 116, 117,   0,   0, 104,   0,
   0,   0,   0,   0,  85,  99, 100,  86,  12,   0,
   0,   0,   0,   0,   0, 103,   0,   0,   0,  28,
   0,   0,  13,   0,   0,   0,  20,   0,  30,  19,
   0, 104,   0,  15,  16,  33,   0,  14,  99, 100,
  29,   9,   0,  31,  32,  10,  18,  12,  21,  11,
  17,  25,  26,   0,   0,  27,  34,   0,  28,   0,
   0,  13,   0,   0,   0,  20,   0,  30,  19,   0,
   0,   0,  15,  16,  33,   0,  14,   0,   0,  29,
   9,   0,  31,  32,  10,  18,   0,  21,  11,  17,
  25,  26,   0,  28,  27,  34,  13,   0,   0,   0,
  20,   0,  30,  19,   0,   0,   0,  15,  16,  33,
   0,  14,   0,   0,  29,   0,   0,  31,  32,   0,
  18,   0,  21,   0,  17,  25,  26,   0,   0,  27,
  34, 164, 165, 166, 167, 168, 169, 171, 170, 172,
 173, 174, 163, 380, 162, 161, 160, 159, 158, 156,
 157, 152, 153, 154, 155, 151, 150, 148, 149, 145,
 146, 147, 164, 165, 166, 167, 168, 169, 171, 170,
 172, 173, 174, 163,   0, 162, 161, 160, 159, 158,
 156, 157, 152, 153, 154, 155, 151, 150, 148, 149,
 145, 146, 147, 163,   0, 162, 161, 160, 159, 158,
 156, 157, 152, 153, 154, 155, 151, 150, 148, 149,
 145, 146, 147, 161, 160, 159, 158, 156, 157, 152,
 153, 154, 155, 151, 150, 148, 149, 145, 146, 147,
 160, 159, 158, 156, 157, 152, 153, 154, 155, 151,
 150, 148, 149, 145, 146, 147, 159, 158, 156, 157,
 152, 153, 154, 155, 151, 150, 148, 149, 145, 146,
 147, 158, 156, 157, 152, 153, 154, 155, 151, 150,
 148, 149, 145, 146, 147, 156, 157, 152, 153, 154,
 155, 151, 150, 148, 149, 145, 146, 147, 152, 153,
 154, 155, 151, 150, 148, 149, 145, 146, 147
};
short	yypact[] =
{
-1000, 943,-1000, 234,-1000,-1000, 978, 978, 943,  22,
  22,  20,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,
-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,
-1000,-1000,-1000,-1000,-1000,-1000, 341,-1000, 181,-1000,
-1000, 245,-1000,-1000,-1000, 978,-1000,-1000,-1000,-1000,
 978,-1000, 978,-1000,-1000,-1000,-1000,-1000,-1000,-1000,
-1000,-1000, 245,-1000, 315, 904, 759,  15,  46,-1000,
  47, 978, -34, 943, -34, -35, 154,-1000,-1000, 943,
 691,  23, 346,-1000, 186, 226,-1000,-1000, -22,-1000,
1096,-1000,-1000, 464, 320, 759, 759, 759, 759, 759,
 759, 759, 759, 881, 848,-1000,-1000,-1000,-1000,-1000,
-1000,-1000,-1000,-1000, 132,  90,-1000,-1000,-1000,-1000,
-1000,-1000, 782,-1000,-1000,-1000,  31, 312, -40, 245,
-1000,1096, 658,-1000, 904,-1000,-1000,-1000,-1000, 175,
  -1,-1000, 759, 224,-1000, 759, 759, 759, 759, 759,
 759, 759, 759, 759, 759, 759, 759, 759, 759, 759,
 759, 759, 759, 759, 759, 759, 759, 759, 759, 759,
 759, 759, 759, 759, 759, 240, 105,1096, 759, 759,
 169, 169,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,
-1000,-1000,-1000, 464,-1000, 464,-1000,-1000,-1000,-1000,
 200, 154,-1000, 154, 759,-1000,-1000, 338,-1000, -48,
 658, 345, 295, 759, 169,-1000,  10, 904, 759,-1000,
  18, -24,-1000,-1000,-1000,-1000, 277, 277, 302, 302,
 388, 388, 388, 388,1202,1202,1191,1178,1164,1149,
1133, 285,1096,1096,1096,1096,1096,1096,1096,1096,
1096,1096,1096,  12,-1000,  37, 759,-1000,   6, 344,
1096,  51,-1000,-1000, 240, 240, 200, 383, 342,-1000,
-1000, 257, 759,  30,-1000,1096, 394,-1000, 245,-1000,
 327, 295,-1000,-1000, -26,-1000,-1000,   0, -32,-1000,
-1000, 759, 726,  41,-1000,-1000, 759,-1000,  -3,  -4,
 381,-1000, 200, 759,-1000,-1000,-1000,-1000,-1000, 217,
 379,-1000, 596, 378, -40, 220,  75, 213, 534, 759,
 207, 377, 372, 169, 205, 195,-1000, 325, 759, 254,
 251,-1000,-1000,-1000,-1000,-1000,1116,-1000, 658,-1000,
-1000,-1000,-1000,-1000,-1000,-1000, 336,-1000,-1000,-1000,
-1000,-1000,-1000, 759, 190, 759,  57, 367, 759,-1000,
-1000, 366, 759, 759,1065,-1000,-1000, -57,-1000, 245,
 290, 103, 464,  99, 159,-1000,  95,-1000,  -9, -13,
-1000,-1000,-1000, 691, 534, 364,-1000, 245, 534, 759,
 534, 362, 361,-1000,  79, 759, 319,-1000,  91,-1000,
-1000,-1000, 534, 350, 348,-1000, 759,-1000, -20, 534,
-1000
};
short	yypgo[] =
{
   0,   9, 199, 298, 302,  23, 195, 200, 500,  25,
 112, 183, 286,   6,  18,  45,   2,  47,  11,   1,
  13,   0,  22, 499,  14,  17, 495, 494,  32, 493,
 489,  46, 488, 460,  15,  10,   3, 457,  24,  28,
 435,  34,  37, 433, 430,  38,  12,   4,   5, 429,
 425, 415, 414, 189, 413, 412, 410, 408,   7, 407,
  26, 406, 405,  27, 402,  16, 401, 400, 399, 397,
 395, 394, 393,  29, 389
};
short	yyr1[] =
{
   0,  50,  50,  51,  51,  54,  56,  51,  53,  57,
  53,  53,  31,  31,  32,  32,  32,  32,  26,  26,
  36,  59,  36,  36,  55,  55,  60,  60,  62,  61,
  64,  61,  63,  63,  65,  65,  37,  37,  37,  41,
  41,  42,  42,  42,  43,  43,  43,  44,  44,  44,
  47,  47,  39,  39,  39,  40,  40,  40,  40,  48,
  48,  48,  14,  14,  15,  15,  15,  15,  15,  18,
  27,  27,  27,  33,  33,  34,  34,  34,  19,  19,
  19,  49,  49,  35,  66,  35,  35,  35,  67,  35,
  35,  35,  35,  35,  35,  35,  35,  35,  35,  16,
  16,  45,  45,  46,  20,  20,  21,  21,  21,  21,
  21,  21,  21,  21,  21,  21,  21,  21,  21,  21,
  21,  21,  21,  21,  21,  21,  21,  21,  21,  21,
  21,  21,  21,  21,  21,  21,  21,  22,  22,  22,
  28,  28,  28,  28,  28,  28,  28,  28,  28,  28,
  28,  23,  23,  23,  23,  23,  23,  23,  23,  23,
  23,  23,  23,  23,  23,  23,  23,  23,  23,  23,
  23,  29,  29,  30,  30,  24,  24,  25,  25,  68,
  11,  52,  52,  13,  13,  13,  13,  13,  13,  13,
  13,  10,  58,  12,  69,  12,  12,  12,  70,  12,
  12,  12,  71,  72,  12,  74,  12,  12,   7,   7,
   9,   9,   2,   2,   2,   8,   8,   3,   3,  73,
  73,  73,  73,   6,   6,   6,   6,   6,   6,   6,
   6,   6,   4,   4,   4,   4,   4,   4,   4,   5,
   5,   5,  17,  38,   1,   1
};
short	yyr2[] =
{
   0,   0,   2,   2,   3,   0,   0,   6,   1,   0,
   4,   3,   1,   3,   1,   3,   4,   4,   2,   3,
   1,   0,   4,   3,   0,   4,   1,   3,   0,   4,
   0,   5,   0,   1,   1,   3,   1,   3,   2,   0,
   1,   2,   3,   1,   1,   4,   4,   2,   3,   3,
   1,   3,   3,   2,   2,   2,   3,   1,   2,   1,
   1,   2,   0,   1,   1,   2,   2,   3,   3,   3,
   0,   2,   2,   1,   2,   3,   2,   2,   2,   1,
   2,   1,   2,   2,   0,   2,   5,   7,   0,  10,
   5,   7,   3,   5,   2,   2,   3,   5,   5,   0,
   1,   0,   1,   1,   1,   3,   1,   3,   3,   3,
   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,
   3,   3,   3,   3,   3,   5,   3,   3,   3,   3,
   3,   3,   3,   3,   3,   3,   3,   1,   5,   7,
   1,   2,   2,   2,   2,   2,   2,   2,   2,   2,
   2,   3,   5,   5,   4,   4,   3,   3,   2,   2,
   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
   1,   1,   2,   1,   2,   0,   1,   1,   3,   0,
   4,   0,   1,   1,   1,   1,   2,   2,   3,   2,
   3,   1,   1,   2,   0,   4,   2,   2,   0,   4,
   2,   2,   0,   0,   7,   0,   5,   1,   1,   2,
   0,   2,   1,   1,   1,   1,   2,   1,   1,   1,
   3,   2,   3,   1,   1,   1,   1,   1,   1,   1,
   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
   1,   1,   1,   1,   1,   1
};
short	yychk[] =
{
-1000, -50, -51, -52, -58, -13, -12,  -6,  -8,  77,
  81,  85,  44,  58,  73,  69,  70,  86,  82,  65,
  62,  84,  -3,  -5,  -4,  87,  88,  91,  55,  76,
  64,  79,  80,  71,  92,   4, -53, -31, -32,  34,
 -38,  42,  -1,  43,  44,  -7,  -2,  -6,  -5,  -4,
  -7, -12,  -6,  -3,  -1, -11,  95,  -1, -11,  -1,
  95,   4,   5, -54, -57,  42,  41,  -9, -31,  -2,
  -9,  -7, -69, -68, -70, -71, -74, -53, -31, -55,
   6, -14, -15, -17, -10,  40,  43, -13, -45, -46,
 -21, -22, -28,  42, -23,  34,  23,  32,  33,  97,
  98,  38,  37,  74,  90, -17,  47,  48,  49,  50,
  46,  45,  51,  52, -29, -30,  53,  54, -31,  -5,
  93, -11, -61, -10, -11,  95, -73,  43, -56, -58,
 -47, -21,  95,  93,   5, -41, -31, -42,  34, -43,
  42, -44,  41,  40,  94,  34,  35,  36,  32,  33,
  31,  30,  26,  27,  28,  29,  24,  25,  23,  22,
  21,  20,  19,  17,   6,   7,   8,   9,  10,  11,
  13,  12,  14,  15,  16, -10, -20, -21,  42,  41,
  39,  40,  38,  37, -22, -22, -22, -22, -22, -22,
 -22, -22, -28,  42, -28,  42,  53,  54, -10,  96,
 -62, -72,  96,   5,   6, -18,  95, -60, -31, -48,
 -40, -47, -39,  41,  40, -15,  -9,  42,  41,  93,
 -42, -45,  40, -21, -21, -21, -21, -21, -21, -21,
 -21, -21, -21, -21, -21, -21, -21, -21, -21, -21,
 -21, -20, -21, -21, -21, -21, -21, -21, -21, -21,
 -21, -21, -21, -41,  34,  42,   5,  93, -24, -25,
 -21, -20,  -1,  -1, -10, -10, -64, -63, -65, -37,
 -31, -38,  18, -73, -73, -21, -27,   4,   5,  96,
 -47, -39,   5,   6, -46,  -1, -42, -14, -45,  93,
  94,  18,  93,  -9, -20,  93,   5,  94, -41, -41,
 -63,   4,   5,  18, -46,  96,  96, -26, -19, -58,
   2, -35, -33, -16, -66,  68, -67,  83,  61,  72,
  78,  56,  59,  67,  75,  89, -34, -20,  57,  60,
  43, -60,   5,  94,  93,  94, -21, -22,  95, -25,
  93,  93,   4, -65, -46,   4, -36, -31,   4, -34,
 -35,   4, -18,  42,  66,  42, -19, -16,  42,   4,
   4,  -1,  42,  42, -21,  18,  18, -48,   4,   5,
 -59, -20,  42, -20,  83,   4, -20,   4, -24, -24,
  18,  96, -36,   6,  93, -49, -16, -58,  93,  42,
  93,  93,  93, -47, -19,   4, -36, -19, -20, -19,
   4,   4,  63, -16,  93, -19,   4,   4, -16,  93,
 -19
};
short	yydef[] =
{
   1,  -2,   2,   0, 182, 192, 183, 184, 185,   0,
   0,   0, 207, 223, 224, 225, 226, 227, 228, 229,
 230, 231, 215, 217, 218, 239, 240, 241, 232, 233,
 234, 235, 236, 237, 238,   3,   0,  -2,  12, 210,
  14,   0, 243, 244, 245, 186, 208, 212, 213, 214,
 187, 210, 189, 216,  -2, 196, 179,  -2, 200,  -2,
 205,   4,   0,  24,   0,  62, 101,   0,   0, 209,
 188, 190,   0,   0,   0,   0,   0,  11,  -2,   6,
   0,   0,  63,  64,  39,   0, 242, 191,   0, 102,
 103, 106, 137,   0, 140,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0, 160, 161, 162, 163, 164,
 165, 166, 167, 168, 169, 170, 171, 173,  13, 211,
  15, 195,   0,  28, 199, 203,   0, 219,   0,   0,
  10,  50,   0,  16,   0,  65,  66,  40, 210,  43,
   0,  44, 101,   0,  17,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,  39,   0, 104, 175,   0,
   0,   0, 158, 159, 141, 142, 143, 144, 145, 146,
 147, 148, 149,   0, 150,   0, 172, 174,  30, 180,
  32,   0, 206, 221,   0,   7,  70,   0,  26,   0,
  59,  60,  57,   0,   0,  68,  41,  62, 101,  47,
   0,   0,  67, 107, 108, 109, 110, 111, 112, 113,
 114, 115, 116, 117, 118, 119, 120, 121, 122, 123,
 124,   0, 126, 127, 128, 129, 130, 131, 132, 133,
 134, 135, 136,   0, 210,   0,   0, 151,   0, 176,
 177,   0, 156, 157,  39,  39,  32,   0,  33,  34,
  36,  14,   0,   0, 222, 220,  -2,  25,   0,  51,
  61,  58,  55,  54,   0,  53,  42,   0,   0,  49,
  48,   0,   0,  41, 105, 154,   0, 155,   0,   0,
   0,  29,   0,   0,  38, 204,  69,  71,  72,   0,
   0,  79,  -2,   0,   0,   0,   0,   0,  -2,  99,
   0,   0,   0,   0,   0,   0,  73, 100,   0,   0,
 242,  27,  56,  52,  45,  46, 125, 138,   0, 178,
 152, 153,  31,  35,  37,  18,   0,  -2,  78,  74,
  80,  83,  85,   0,   0,   0,   0,   0,   0,  94,
  95,   0, 175, 175,   0,  76,  77,   0,  19,   0,
   0,   0,  99,   0,   0,  92,   0,  96,   0,   0,
  75, 139,  23,   0,  -2,   0,  81,   0,  -2,   0,
  -2,   0,   0,  22,  86,  99,  82,  90,   0,  93,
  97,  98,  -2,   0,   0,  87,  99,  91,   0,  -2,
  89
};
short	yytok1[] =
{
   1,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,  97,   0,   0,   0,  36,  23,   0,
  42,  93,  34,  32,   5,  33,  40,  35,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,  18,   4,
  26,   6,  27,  17,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,  41,   0,  94,  22,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,  95,  21,  96,  98
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
  90,  91,  92
};
long	yytok3[] =
{
   0
};
#define YYFLAG 		-1000
#define YYERROR		goto yyerrlab
#define	yyclearin	yychar = YYEMPTY
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
	static char x[16];

	if(yyc > 0 && yyc <= sizeof(yytoknames)/sizeof(yytoknames[0]))
	if(yytoknames[yyc-1])
		return yytoknames[yyc-1];
	sprint(x, "<%d>", yyc);
	return x;
}

char*
yystatname(int yys)
{
	static char x[16];

	if(yys >= 0 && yys < sizeof(yystates)/sizeof(yystates[0]))
	if(yystates[yys])
		return yystates[yys];
	sprint(x, "<%d>\n", yys);
	return x;
}

int yychar;

int
yylex1(void)
{
	long *t3p;
	int c;

	yychar = yylex();
	if(yychar <= 0) {
		yychar = 0;
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
		fprint(2, "lex %.4ux %s\n", yychar, yytokname(c));
	return c;
}

int yystate;

int
yyparse(void)
{
	struct
	{
		YYSTYPE	yyv;
		int	yys;
	} yys[YYMAXDEPTH], *yyp, *yypt;
	short *yyxi;
	int yyj, yym, yyn, yyg;
	int yyc;
	YYSTYPE save1, save2;
	int save3, save4;

	save1 = yylval;
	save2 = yyval;
	save3 = yynerrs;
	save4 = yyerrflag;

	yystate = 0;
	yychar = YYEMPTY;
	yyc = YYEMPTY;
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
		fprint(2, "char %s in %s", yytokname(yyc), yystatname(yystate));

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
	if(yyc < 0)
		yyc = yylex1();
	yyn += yyc;
	if(yyn < 0 || yyn >= YYLAST)
		goto yydefault;
	yyn = yyact[yyn];
	if(yychk[yyn] == yyc) { /* valid shift */
		yyc = YYEMPTY;
		yychar = YYEMPTY;
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
		if(yyc < 0)
			yyc = yylex1();

		/* look through exception table */
		for(yyxi=yyexca;; yyxi+=2)
			if(yyxi[0] == -1 && yyxi[1] == yystate)
				break;
		for(yyxi += 2;; yyxi += 2) {
			yyn = yyxi[0];
			if(yyn < 0 || yyn == yyc)
				break;
		}
		yyn = yyxi[1];
		if(yyn < 0) {
			yychar = YYEMPTY;
			goto ret0;
		}
	}
	if(yyn == 0) {
		/* error ... attempt to resume parsing */
		switch(yyerrflag) {
		case 0:   /* brand new error */
			yyerror("syntax error");
			if(yydebug >= 2) {
				fprint(2, "%s", yystatname(yystate));
				fprint(2, "saw %s\n", yytokname(yyc));
			}
			goto yyerrlab;
yyerrlab:
			yynerrs++;

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
				fprint(2, "error recovery discards %s\n", yytokname(yyc));
			if(yyc == YYEOFCODE)
				goto ret1;
			yyc = YYEMPTY;
			yychar = YYEMPTY;
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
#line	77	"/sys/src/cmd/cc/cc.y"
{
		dodecl(xdecl, lastclass, lasttype, Z);
	} break;
case 5:
#line	82	"/sys/src/cmd/cc/cc.y"
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
#line	96	"/sys/src/cmd/cc/cc.y"
{
		argmark(yypt[-2].yyv.node, 1);
	} break;
case 7:
#line	100	"/sys/src/cmd/cc/cc.y"
{
		Node *n;

		n = revertdcl();
		if(n)
			yypt[-0].yyv.node = new(OLIST, n, yypt[-0].yyv.node);
		if(!debug['a'] && !debug['Z'])
			codgen(yypt[-0].yyv.node, yypt[-4].yyv.node);
	} break;
case 8:
#line	112	"/sys/src/cmd/cc/cc.y"
{
		dodecl(xdecl, lastclass, lasttype, yypt[-0].yyv.node);
	} break;
case 9:
#line	116	"/sys/src/cmd/cc/cc.y"
{
		yypt[-0].yyv.node = dodecl(xdecl, lastclass, lasttype, yypt[-0].yyv.node);
	} break;
case 10:
#line	120	"/sys/src/cmd/cc/cc.y"
{
		doinit(yypt[-3].yyv.node->sym, yypt[-3].yyv.node->type, 0L, yypt[-0].yyv.node);
	} break;
case 13:
#line	128	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OIND, yypt[-0].yyv.node, Z);
		yyval.node->garb = simpleg(yypt[-1].yyv.lval);
	} break;
case 15:
#line	136	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = yypt[-1].yyv.node;
	} break;
case 16:
#line	140	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OFUNC, yypt[-3].yyv.node, yypt[-1].yyv.node);
	} break;
case 17:
#line	144	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OARRAY, yypt[-3].yyv.node, yypt[-1].yyv.node);
	} break;
case 18:
#line	153	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = dodecl(adecl, lastclass, lasttype, Z);
	} break;
case 19:
#line	157	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = yypt[-1].yyv.node;
	} break;
case 20:
#line	163	"/sys/src/cmd/cc/cc.y"
{
		dodecl(adecl, lastclass, lasttype, yypt[-0].yyv.node);
		yyval.node = Z;
	} break;
case 21:
#line	168	"/sys/src/cmd/cc/cc.y"
{
		yypt[-0].yyv.node = dodecl(adecl, lastclass, lasttype, yypt[-0].yyv.node);
	} break;
case 22:
#line	172	"/sys/src/cmd/cc/cc.y"
{
		long w;

		w = yypt[-3].yyv.node->sym->type->width;
		yyval.node = doinit(yypt[-3].yyv.node->sym, yypt[-3].yyv.node->type, 0L, yypt[-0].yyv.node);
		yyval.node = contig(yypt[-3].yyv.node->sym, yyval.node, w);
	} break;
case 23:
#line	180	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = yypt[-2].yyv.node;
		if(yypt[-0].yyv.node != Z) {
			yyval.node = yypt[-0].yyv.node;
			if(yypt[-2].yyv.node != Z)
				yyval.node = new(OLIST, yypt[-2].yyv.node, yypt[-0].yyv.node);
		}
	} break;
case 26:
#line	197	"/sys/src/cmd/cc/cc.y"
{
		dodecl(pdecl, lastclass, lasttype, yypt[-0].yyv.node);
	} break;
case 28:
#line	207	"/sys/src/cmd/cc/cc.y"
{
		lasttype = yypt[-0].yyv.type;
	} break;
case 30:
#line	212	"/sys/src/cmd/cc/cc.y"
{
		lasttype = yypt[-0].yyv.type;
	} break;
case 32:
#line	218	"/sys/src/cmd/cc/cc.y"
{
		lastfield = 0;
		edecl(CXXX, lasttype, S);
	} break;
case 34:
#line	226	"/sys/src/cmd/cc/cc.y"
{
		dodecl(edecl, CXXX, lasttype, yypt[-0].yyv.node);
	} break;
case 36:
#line	233	"/sys/src/cmd/cc/cc.y"
{
		lastbit = 0;
		firstbit = 1;
	} break;
case 37:
#line	238	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OBIT, yypt[-2].yyv.node, yypt[-0].yyv.node);
	} break;
case 38:
#line	242	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OBIT, Z, yypt[-0].yyv.node);
	} break;
case 39:
#line	250	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = (Z);
	} break;
case 41:
#line	257	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OIND, (Z), Z);
		yyval.node->garb = simpleg(yypt[-0].yyv.lval);
	} break;
case 42:
#line	262	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OIND, yypt[-0].yyv.node, Z);
		yyval.node->garb = simpleg(yypt[-1].yyv.lval);
	} break;
case 45:
#line	271	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OFUNC, yypt[-3].yyv.node, yypt[-1].yyv.node);
	} break;
case 46:
#line	275	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OARRAY, yypt[-3].yyv.node, yypt[-1].yyv.node);
	} break;
case 47:
#line	281	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OFUNC, (Z), Z);
	} break;
case 48:
#line	285	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OARRAY, (Z), yypt[-1].yyv.node);
	} break;
case 49:
#line	289	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = yypt[-1].yyv.node;
	} break;
case 51:
#line	296	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OINIT, invert(yypt[-1].yyv.node), Z);
	} break;
case 52:
#line	302	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OARRAY, yypt[-1].yyv.node, Z);
	} break;
case 53:
#line	306	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OELEM, Z, Z);
		yyval.node->sym = yypt[-0].yyv.sym;
	} break;
case 56:
#line	315	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OLIST, yypt[-2].yyv.node, yypt[-1].yyv.node);
	} break;
case 58:
#line	320	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OLIST, yypt[-1].yyv.node, yypt[-0].yyv.node);
	} break;
case 61:
#line	328	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OLIST, yypt[-1].yyv.node, yypt[-0].yyv.node);
	} break;
case 62:
#line	333	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = Z;
	} break;
case 63:
#line	337	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = invert(yypt[-0].yyv.node);
	} break;
case 65:
#line	345	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OPROTO, yypt[-0].yyv.node, Z);
		yyval.node->type = yypt[-1].yyv.type;
	} break;
case 66:
#line	350	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OPROTO, yypt[-0].yyv.node, Z);
		yyval.node->type = yypt[-1].yyv.type;
	} break;
case 67:
#line	355	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(ODOTDOT, Z, Z);
	} break;
case 68:
#line	359	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OLIST, yypt[-2].yyv.node, yypt[-0].yyv.node);
	} break;
case 69:
#line	365	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = invert(yypt[-1].yyv.node);
	//	if($2 != Z)
	//		$$ = new(OLIST, $2, $$);
		if(yyval.node == Z)
			yyval.node = new(OLIST, Z, Z);
	} break;
case 70:
#line	374	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = Z;
	} break;
case 71:
#line	378	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OLIST, yypt[-1].yyv.node, yypt[-0].yyv.node);
	} break;
case 72:
#line	382	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OLIST, yypt[-1].yyv.node, yypt[-0].yyv.node);
	} break;
case 74:
#line	389	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OLIST, yypt[-1].yyv.node, yypt[-0].yyv.node);
	} break;
case 75:
#line	395	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OCASE, yypt[-1].yyv.node, Z);
	} break;
case 76:
#line	399	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OCASE, Z, Z);
	} break;
case 77:
#line	403	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OLABEL, dcllabel(yypt[-1].yyv.sym, 1), Z);
	} break;
case 78:
#line	409	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = Z;
	} break;
case 80:
#line	414	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OLIST, yypt[-1].yyv.node, yypt[-0].yyv.node);
	} break;
case 82:
#line	421	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = yypt[-0].yyv.node;
	} break;
case 84:
#line	427	"/sys/src/cmd/cc/cc.y"
{
		markdcl();
	} break;
case 85:
#line	431	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = revertdcl();
		if(yyval.node)
			yyval.node = new(OLIST, yyval.node, yypt[-0].yyv.node);
		else
			yyval.node = yypt[-0].yyv.node;
	} break;
case 86:
#line	439	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OIF, yypt[-2].yyv.node, new(OLIST, yypt[-0].yyv.node, Z));
		if(yypt[-0].yyv.node == Z)
			warn(yypt[-2].yyv.node, "empty if body");
	} break;
case 87:
#line	445	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OIF, yypt[-4].yyv.node, new(OLIST, yypt[-2].yyv.node, yypt[-0].yyv.node));
		if(yypt[-2].yyv.node == Z)
			warn(yypt[-4].yyv.node, "empty if body");
		if(yypt[-0].yyv.node == Z)
			warn(yypt[-4].yyv.node, "empty else body");
	} break;
case 88:
#line	452	"/sys/src/cmd/cc/cc.y"
{ markdcl(); } break;
case 89:
#line	453	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = revertdcl();
		if(yyval.node){
			if(yypt[-6].yyv.node)
				yypt[-6].yyv.node = new(OLIST, yyval.node, yypt[-6].yyv.node);
			else
				yypt[-6].yyv.node = yyval.node;
		}
		yyval.node = new(OFOR, new(OLIST, yypt[-4].yyv.node, new(OLIST, yypt[-6].yyv.node, yypt[-2].yyv.node)), yypt[-0].yyv.node);
	} break;
case 90:
#line	464	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OWHILE, yypt[-2].yyv.node, yypt[-0].yyv.node);
	} break;
case 91:
#line	468	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(ODWHILE, yypt[-2].yyv.node, yypt[-5].yyv.node);
	} break;
case 92:
#line	472	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(ORETURN, yypt[-1].yyv.node, Z);
		yyval.node->type = thisfn->link;
	} break;
case 93:
#line	477	"/sys/src/cmd/cc/cc.y"
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
case 94:
#line	491	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OBREAK, Z, Z);
	} break;
case 95:
#line	495	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OCONTINUE, Z, Z);
	} break;
case 96:
#line	499	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OGOTO, dcllabel(yypt[-1].yyv.sym, 0), Z);
	} break;
case 97:
#line	503	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OUSED, yypt[-2].yyv.node, Z);
	} break;
case 98:
#line	507	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OSET, yypt[-2].yyv.node, Z);
	} break;
case 99:
#line	512	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = Z;
	} break;
case 101:
#line	518	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = Z;
	} break;
case 103:
#line	525	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OCAST, yypt[-0].yyv.node, Z);
		yyval.node->type = types[TLONG];
	} break;
case 105:
#line	533	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OCOMMA, yypt[-2].yyv.node, yypt[-0].yyv.node);
	} break;
case 107:
#line	540	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OMUL, yypt[-2].yyv.node, yypt[-0].yyv.node);
	} break;
case 108:
#line	544	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(ODIV, yypt[-2].yyv.node, yypt[-0].yyv.node);
	} break;
case 109:
#line	548	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OMOD, yypt[-2].yyv.node, yypt[-0].yyv.node);
	} break;
case 110:
#line	552	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OADD, yypt[-2].yyv.node, yypt[-0].yyv.node);
	} break;
case 111:
#line	556	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OSUB, yypt[-2].yyv.node, yypt[-0].yyv.node);
	} break;
case 112:
#line	560	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OASHR, yypt[-2].yyv.node, yypt[-0].yyv.node);
	} break;
case 113:
#line	564	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OASHL, yypt[-2].yyv.node, yypt[-0].yyv.node);
	} break;
case 114:
#line	568	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OLT, yypt[-2].yyv.node, yypt[-0].yyv.node);
	} break;
case 115:
#line	572	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OGT, yypt[-2].yyv.node, yypt[-0].yyv.node);
	} break;
case 116:
#line	576	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OLE, yypt[-2].yyv.node, yypt[-0].yyv.node);
	} break;
case 117:
#line	580	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OGE, yypt[-2].yyv.node, yypt[-0].yyv.node);
	} break;
case 118:
#line	584	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OEQ, yypt[-2].yyv.node, yypt[-0].yyv.node);
	} break;
case 119:
#line	588	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(ONE, yypt[-2].yyv.node, yypt[-0].yyv.node);
	} break;
case 120:
#line	592	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OAND, yypt[-2].yyv.node, yypt[-0].yyv.node);
	} break;
case 121:
#line	596	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OXOR, yypt[-2].yyv.node, yypt[-0].yyv.node);
	} break;
case 122:
#line	600	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OOR, yypt[-2].yyv.node, yypt[-0].yyv.node);
	} break;
case 123:
#line	604	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OANDAND, yypt[-2].yyv.node, yypt[-0].yyv.node);
	} break;
case 124:
#line	608	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OOROR, yypt[-2].yyv.node, yypt[-0].yyv.node);
	} break;
case 125:
#line	612	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OCOND, yypt[-4].yyv.node, new(OLIST, yypt[-2].yyv.node, yypt[-0].yyv.node));
	} break;
case 126:
#line	616	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OAS, yypt[-2].yyv.node, yypt[-0].yyv.node);
	} break;
case 127:
#line	620	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OASADD, yypt[-2].yyv.node, yypt[-0].yyv.node);
	} break;
case 128:
#line	624	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OASSUB, yypt[-2].yyv.node, yypt[-0].yyv.node);
	} break;
case 129:
#line	628	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OASMUL, yypt[-2].yyv.node, yypt[-0].yyv.node);
	} break;
case 130:
#line	632	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OASDIV, yypt[-2].yyv.node, yypt[-0].yyv.node);
	} break;
case 131:
#line	636	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OASMOD, yypt[-2].yyv.node, yypt[-0].yyv.node);
	} break;
case 132:
#line	640	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OASASHL, yypt[-2].yyv.node, yypt[-0].yyv.node);
	} break;
case 133:
#line	644	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OASASHR, yypt[-2].yyv.node, yypt[-0].yyv.node);
	} break;
case 134:
#line	648	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OASAND, yypt[-2].yyv.node, yypt[-0].yyv.node);
	} break;
case 135:
#line	652	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OASXOR, yypt[-2].yyv.node, yypt[-0].yyv.node);
	} break;
case 136:
#line	656	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OASOR, yypt[-2].yyv.node, yypt[-0].yyv.node);
	} break;
case 138:
#line	663	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OCAST, yypt[-0].yyv.node, Z);
		dodecl(NODECL, CXXX, yypt[-3].yyv.type, yypt[-2].yyv.node);
		yyval.node->type = lastdcl;
		yyval.node->xcast = 1;
	} break;
case 139:
#line	670	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OSTRUCT, yypt[-1].yyv.node, Z);
		dodecl(NODECL, CXXX, yypt[-5].yyv.type, yypt[-4].yyv.node);
		yyval.node->type = lastdcl;
	} break;
case 141:
#line	679	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OIND, yypt[-0].yyv.node, Z);
	} break;
case 142:
#line	683	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OADDR, yypt[-0].yyv.node, Z);
	} break;
case 143:
#line	687	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OPOS, yypt[-0].yyv.node, Z);
	} break;
case 144:
#line	691	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(ONEG, yypt[-0].yyv.node, Z);
	} break;
case 145:
#line	695	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(ONOT, yypt[-0].yyv.node, Z);
	} break;
case 146:
#line	699	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OCOM, yypt[-0].yyv.node, Z);
	} break;
case 147:
#line	703	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OPREINC, yypt[-0].yyv.node, Z);
	} break;
case 148:
#line	707	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OPREDEC, yypt[-0].yyv.node, Z);
	} break;
case 149:
#line	711	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OSIZE, yypt[-0].yyv.node, Z);
	} break;
case 150:
#line	715	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OSIGN, yypt[-0].yyv.node, Z);
	} break;
case 151:
#line	721	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = yypt[-1].yyv.node;
	} break;
case 152:
#line	725	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OSIZE, Z, Z);
		dodecl(NODECL, CXXX, yypt[-2].yyv.type, yypt[-1].yyv.node);
		yyval.node->type = lastdcl;
	} break;
case 153:
#line	731	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OSIGN, Z, Z);
		dodecl(NODECL, CXXX, yypt[-2].yyv.type, yypt[-1].yyv.node);
		yyval.node->type = lastdcl;
	} break;
case 154:
#line	737	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OFUNC, yypt[-3].yyv.node, Z);
		if(yypt[-3].yyv.node->op == ONAME)
		if(yypt[-3].yyv.node->type == T)
			dodecl(xdecl, CXXX, types[TINT], yyval.node);
		yyval.node->right = invert(yypt[-1].yyv.node);
	} break;
case 155:
#line	745	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OIND, new(OADD, yypt[-3].yyv.node, yypt[-1].yyv.node), Z);
	} break;
case 156:
#line	749	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(ODOT, new(OIND, yypt[-2].yyv.node, Z), Z);
		yyval.node->sym = yypt[-0].yyv.sym;
	} break;
case 157:
#line	754	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(ODOT, yypt[-2].yyv.node, Z);
		yyval.node->sym = yypt[-0].yyv.sym;
	} break;
case 158:
#line	759	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OPOSTINC, yypt[-1].yyv.node, Z);
	} break;
case 159:
#line	763	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OPOSTDEC, yypt[-1].yyv.node, Z);
	} break;
case 161:
#line	768	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OCONST, Z, Z);
		yyval.node->type = types[TINT];
		yyval.node->vconst = yypt[-0].yyv.vval;
		yyval.node->cstring = strdup(symb);
	} break;
case 162:
#line	775	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OCONST, Z, Z);
		yyval.node->type = types[TLONG];
		yyval.node->vconst = yypt[-0].yyv.vval;
		yyval.node->cstring = strdup(symb);
	} break;
case 163:
#line	782	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OCONST, Z, Z);
		yyval.node->type = types[TUINT];
		yyval.node->vconst = yypt[-0].yyv.vval;
		yyval.node->cstring = strdup(symb);
	} break;
case 164:
#line	789	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OCONST, Z, Z);
		yyval.node->type = types[TULONG];
		yyval.node->vconst = yypt[-0].yyv.vval;
		yyval.node->cstring = strdup(symb);
	} break;
case 165:
#line	796	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OCONST, Z, Z);
		yyval.node->type = types[TDOUBLE];
		yyval.node->fconst = yypt[-0].yyv.dval;
		yyval.node->cstring = strdup(symb);
	} break;
case 166:
#line	803	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OCONST, Z, Z);
		yyval.node->type = types[TFLOAT];
		yyval.node->fconst = yypt[-0].yyv.dval;
		yyval.node->cstring = strdup(symb);
	} break;
case 167:
#line	810	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OCONST, Z, Z);
		yyval.node->type = types[TVLONG];
		yyval.node->vconst = yypt[-0].yyv.vval;
		yyval.node->cstring = strdup(symb);
	} break;
case 168:
#line	817	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OCONST, Z, Z);
		yyval.node->type = types[TUVLONG];
		yyval.node->vconst = yypt[-0].yyv.vval;
		yyval.node->cstring = strdup(symb);
	} break;
case 171:
#line	828	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OSTRING, Z, Z);
		yyval.node->type = typ(TARRAY, types[TCHAR]);
		yyval.node->type->width = yypt[-0].yyv.sval.l + 1;
		yyval.node->cstring = yypt[-0].yyv.sval.s;
		yyval.node->sym = symstring;
		yyval.node->etype = TARRAY;
		yyval.node->class = CSTATIC;
	} break;
case 172:
#line	838	"/sys/src/cmd/cc/cc.y"
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
case 173:
#line	856	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OLSTRING, Z, Z);
		yyval.node->type = typ(TARRAY, types[TRUNE]);
		yyval.node->type->width = yypt[-0].yyv.sval.l + sizeof(Rune);
		yyval.node->rstring = (Rune*)yypt[-0].yyv.sval.s;
		yyval.node->sym = symstring;
		yyval.node->etype = TARRAY;
		yyval.node->class = CSTATIC;
	} break;
case 174:
#line	866	"/sys/src/cmd/cc/cc.y"
{
		char *s;
		int n;

		n = yypt[-1].yyv.node->type->width - sizeof(Rune);
		s = alloc(n+yypt[-0].yyv.sval.l+MAXALIGN);

		memcpy(s, yypt[-1].yyv.node->rstring, n);
		memcpy(s+n, yypt[-0].yyv.sval.s, yypt[-0].yyv.sval.l);
		*(Rune*)(s+n+yypt[-0].yyv.sval.l) = 0;

		yyval.node = yypt[-1].yyv.node;
		yyval.node->type->width += yypt[-0].yyv.sval.l;
		yyval.node->rstring = (Rune*)s;
	} break;
case 175:
#line	883	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = Z;
	} break;
case 178:
#line	891	"/sys/src/cmd/cc/cc.y"
{
		yyval.node = new(OLIST, yypt[-2].yyv.node, yypt[-0].yyv.node);
	} break;
case 179:
#line	897	"/sys/src/cmd/cc/cc.y"
{
		yyval.tyty.t1 = strf;
		yyval.tyty.t2 = strl;
		yyval.tyty.t3 = lasttype;
		yyval.tyty.c = lastclass;
		strf = T;
		strl = T;
		lastbit = 0;
		firstbit = 1;
		lastclass = CXXX;
		lasttype = T;
	} break;
case 180:
#line	910	"/sys/src/cmd/cc/cc.y"
{
		yyval.type = strf;
		strf = yypt[-2].yyv.tyty.t1;
		strl = yypt[-2].yyv.tyty.t2;
		lasttype = yypt[-2].yyv.tyty.t3;
		lastclass = yypt[-2].yyv.tyty.c;
	} break;
case 181:
#line	919	"/sys/src/cmd/cc/cc.y"
{
		lastclass = CXXX;
		lasttype = types[TINT];
	} break;
case 183:
#line	927	"/sys/src/cmd/cc/cc.y"
{
		yyval.tycl.t = yypt[-0].yyv.type;
		yyval.tycl.c = CXXX;
	} break;
case 184:
#line	932	"/sys/src/cmd/cc/cc.y"
{
		yyval.tycl.t = simplet(yypt[-0].yyv.lval);
		yyval.tycl.c = CXXX;
	} break;
case 185:
#line	937	"/sys/src/cmd/cc/cc.y"
{
		yyval.tycl.t = simplet(yypt[-0].yyv.lval);
		yyval.tycl.c = simplec(yypt[-0].yyv.lval);
		yyval.tycl.t = garbt(yyval.tycl.t, yypt[-0].yyv.lval);
	} break;
case 186:
#line	943	"/sys/src/cmd/cc/cc.y"
{
		yyval.tycl.t = yypt[-1].yyv.type;
		yyval.tycl.c = simplec(yypt[-0].yyv.lval);
		yyval.tycl.t = garbt(yyval.tycl.t, yypt[-0].yyv.lval);
		if(yypt[-0].yyv.lval & ~BCLASS & ~BGARB)
			diag(Z, "duplicate types given: %T and %Q", yypt[-1].yyv.type, yypt[-0].yyv.lval);
	} break;
case 187:
#line	951	"/sys/src/cmd/cc/cc.y"
{
		yyval.tycl.t = simplet(typebitor(yypt[-1].yyv.lval, yypt[-0].yyv.lval));
		yyval.tycl.c = simplec(yypt[-0].yyv.lval);
		yyval.tycl.t = garbt(yyval.tycl.t, yypt[-0].yyv.lval);
	} break;
case 188:
#line	957	"/sys/src/cmd/cc/cc.y"
{
		yyval.tycl.t = yypt[-1].yyv.type;
		yyval.tycl.c = simplec(yypt[-2].yyv.lval);
		yyval.tycl.t = garbt(yyval.tycl.t, yypt[-2].yyv.lval|yypt[-0].yyv.lval);
	} break;
case 189:
#line	963	"/sys/src/cmd/cc/cc.y"
{
		yyval.tycl.t = simplet(yypt[-0].yyv.lval);
		yyval.tycl.c = simplec(yypt[-1].yyv.lval);
		yyval.tycl.t = garbt(yyval.tycl.t, yypt[-1].yyv.lval);
	} break;
case 190:
#line	969	"/sys/src/cmd/cc/cc.y"
{
		yyval.tycl.t = simplet(typebitor(yypt[-1].yyv.lval, yypt[-0].yyv.lval));
		yyval.tycl.c = simplec(yypt[-2].yyv.lval|yypt[-0].yyv.lval);
		yyval.tycl.t = garbt(yyval.tycl.t, yypt[-2].yyv.lval|yypt[-0].yyv.lval);
	} break;
case 191:
#line	977	"/sys/src/cmd/cc/cc.y"
{
		yyval.type = yypt[-0].yyv.tycl.t;
		if(yypt[-0].yyv.tycl.c != CXXX)
			diag(Z, "illegal combination of class 4: %s", cnames[yypt[-0].yyv.tycl.c]);
	} break;
case 192:
#line	985	"/sys/src/cmd/cc/cc.y"
{
		lasttype = yypt[-0].yyv.tycl.t;
		lastclass = yypt[-0].yyv.tycl.c;
	} break;
case 193:
#line	992	"/sys/src/cmd/cc/cc.y"
{
		dotag(yypt[-0].yyv.sym, TSTRUCT, 0);
		yyval.type = yypt[-0].yyv.sym->suetag;
	} break;
case 194:
#line	997	"/sys/src/cmd/cc/cc.y"
{
		dotag(yypt[-0].yyv.sym, TSTRUCT, autobn);
	} break;
case 195:
#line	1001	"/sys/src/cmd/cc/cc.y"
{
		yyval.type = yypt[-2].yyv.sym->suetag;
		if(yyval.type->link != T)
			diag(Z, "redeclare tag: %s", yypt[-2].yyv.sym->name);
		yyval.type->link = yypt[-0].yyv.type;
		sualign(yyval.type);
	} break;
case 196:
#line	1009	"/sys/src/cmd/cc/cc.y"
{
		taggen++;
		sprint(symb, "_%d_", taggen);
		yyval.type = dotag(lookup(), TSTRUCT, autobn);
		yyval.type->link = yypt[-0].yyv.type;
		sualign(yyval.type);
	} break;
case 197:
#line	1017	"/sys/src/cmd/cc/cc.y"
{
		dotag(yypt[-0].yyv.sym, TUNION, 0);
		yyval.type = yypt[-0].yyv.sym->suetag;
	} break;
case 198:
#line	1022	"/sys/src/cmd/cc/cc.y"
{
		dotag(yypt[-0].yyv.sym, TUNION, autobn);
	} break;
case 199:
#line	1026	"/sys/src/cmd/cc/cc.y"
{
		yyval.type = yypt[-2].yyv.sym->suetag;
		if(yyval.type->link != T)
			diag(Z, "redeclare tag: %s", yypt[-2].yyv.sym->name);
		yyval.type->link = yypt[-0].yyv.type;
		sualign(yyval.type);
	} break;
case 200:
#line	1034	"/sys/src/cmd/cc/cc.y"
{
		taggen++;
		sprint(symb, "_%d_", taggen);
		yyval.type = dotag(lookup(), TUNION, autobn);
		yyval.type->link = yypt[-0].yyv.type;
		sualign(yyval.type);
	} break;
case 201:
#line	1042	"/sys/src/cmd/cc/cc.y"
{
		dotag(yypt[-0].yyv.sym, TENUM, 0);
		yyval.type = yypt[-0].yyv.sym->suetag;
		if(yyval.type->link == T)
			yyval.type->link = types[TINT];
		yyval.type = yyval.type->link;
	} break;
case 202:
#line	1050	"/sys/src/cmd/cc/cc.y"
{
		dotag(yypt[-0].yyv.sym, TENUM, autobn);
	} break;
case 203:
#line	1054	"/sys/src/cmd/cc/cc.y"
{
		en.tenum = T;
		en.cenum = T;
	} break;
case 204:
#line	1059	"/sys/src/cmd/cc/cc.y"
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
case 205:
#line	1071	"/sys/src/cmd/cc/cc.y"
{
		en.tenum = T;
		en.cenum = T;
	} break;
case 206:
#line	1076	"/sys/src/cmd/cc/cc.y"
{
		yyval.type = en.tenum;
	} break;
case 207:
#line	1080	"/sys/src/cmd/cc/cc.y"
{
		yyval.type = tcopy(yypt[-0].yyv.sym->type);
	} break;
case 209:
#line	1087	"/sys/src/cmd/cc/cc.y"
{
		yyval.lval = typebitor(yypt[-1].yyv.lval, yypt[-0].yyv.lval);
	} break;
case 210:
#line	1092	"/sys/src/cmd/cc/cc.y"
{
		yyval.lval = 0;
	} break;
case 211:
#line	1096	"/sys/src/cmd/cc/cc.y"
{
		yyval.lval = typebitor(yypt[-1].yyv.lval, yypt[-0].yyv.lval);
	} break;
case 216:
#line	1108	"/sys/src/cmd/cc/cc.y"
{
		yyval.lval = typebitor(yypt[-1].yyv.lval, yypt[-0].yyv.lval);
	} break;
case 219:
#line	1118	"/sys/src/cmd/cc/cc.y"
{
		doenum(yypt[-0].yyv.sym, Z);
	} break;
case 220:
#line	1122	"/sys/src/cmd/cc/cc.y"
{
		doenum(yypt[-2].yyv.sym, yypt[-0].yyv.node);
	} break;
case 223:
#line	1129	"/sys/src/cmd/cc/cc.y"
{ yyval.lval = BCHAR; } break;
case 224:
#line	1130	"/sys/src/cmd/cc/cc.y"
{ yyval.lval = BSHORT; } break;
case 225:
#line	1131	"/sys/src/cmd/cc/cc.y"
{ yyval.lval = BINT; } break;
case 226:
#line	1132	"/sys/src/cmd/cc/cc.y"
{ yyval.lval = BLONG; } break;
case 227:
#line	1133	"/sys/src/cmd/cc/cc.y"
{ yyval.lval = BSIGNED; } break;
case 228:
#line	1134	"/sys/src/cmd/cc/cc.y"
{ yyval.lval = BUNSIGNED; } break;
case 229:
#line	1135	"/sys/src/cmd/cc/cc.y"
{ yyval.lval = BFLOAT; } break;
case 230:
#line	1136	"/sys/src/cmd/cc/cc.y"
{ yyval.lval = BDOUBLE; } break;
case 231:
#line	1137	"/sys/src/cmd/cc/cc.y"
{ yyval.lval = BVOID; } break;
case 232:
#line	1140	"/sys/src/cmd/cc/cc.y"
{ yyval.lval = BAUTO; } break;
case 233:
#line	1141	"/sys/src/cmd/cc/cc.y"
{ yyval.lval = BSTATIC; } break;
case 234:
#line	1142	"/sys/src/cmd/cc/cc.y"
{ yyval.lval = BEXTERN; } break;
case 235:
#line	1143	"/sys/src/cmd/cc/cc.y"
{ yyval.lval = BTYPEDEF; } break;
case 236:
#line	1144	"/sys/src/cmd/cc/cc.y"
{ yyval.lval = BTYPESTR; } break;
case 237:
#line	1145	"/sys/src/cmd/cc/cc.y"
{ yyval.lval = BREGISTER; } break;
case 238:
#line	1146	"/sys/src/cmd/cc/cc.y"
{ yyval.lval = 0; } break;
case 239:
#line	1149	"/sys/src/cmd/cc/cc.y"
{ yyval.lval = BCONSTNT; } break;
case 240:
#line	1150	"/sys/src/cmd/cc/cc.y"
{ yyval.lval = BVOLATILE; } break;
case 241:
#line	1151	"/sys/src/cmd/cc/cc.y"
{ yyval.lval = 0; } break;
case 242:
#line	1155	"/sys/src/cmd/cc/cc.y"
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
case 243:
#line	1170	"/sys/src/cmd/cc/cc.y"
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
