typedef long Word;
typedef unsigned long Single;
typedef struct {
	unsigned long h;
	unsigned long l;
} Double;

enum {
	FractBits	= 28,
	CarryBit	= 0x10000000,
	HiddenBit	= 0x08000000,
	MsBit		= HiddenBit,
	NGuardBits	= 3,
	GuardMask	= 0x07,
	LsBit		= (1<<NGuardBits),

	SingleExpBias	= 127,
	SingleExpMax	= 255,
	DoubleExpBias	= 1023,
	DoubleExpMax	= 2047,

	ExpBias		= DoubleExpBias,
	ExpInfinity	= DoubleExpMax,
};

typedef struct {
	unsigned char s;
	short e;
	long l;				/* 0000FFFFFFFFFFFFFFFFFFFFFFFFFGGG */
	long h;				/* 0000HFFFFFFFFFFFFFFFFFFFFFFFFFFF */
} Internal;

#define IsWeird(n)	((n)->e >= ExpInfinity)
#define	IsInfinity(n)	(IsWeird(n) && (n)->h == HiddenBit && (n)->l == 0)
#define	SetInfinity(n)	((n)->e = ExpInfinity, (n)->h = HiddenBit, (n)->l = 0)
#define IsNaN(n)	(IsWeird(n) && (((n)->h & ~HiddenBit) || (n)->l))
#define	SetQNaN(n)	((n)->s = 0, (n)->e = ExpInfinity, 		\
			 (n)->h = HiddenBit|(LsBit<<1), (n)->l = 0)
#define IsZero(n)	((n)->e == 1 && (n)->h == 0 && (n)->l == 0)
#define SetZero(n)	((n)->e = 1, (n)->h = 0, (n)->l = 0)

/*
 * fpi.c
 */
extern void fpiround(Internal *);
extern void fpiadd(Internal *, Internal *, Internal *);
extern void fpisub(Internal *, Internal *, Internal *);
extern void fpimul(Internal *, Internal *, Internal *);
extern void fpidiv(Internal *, Internal *, Internal *);
extern int fpicmp(Internal *, Internal *);
extern void fpinormalise(Internal*);

/*
 * fpimem.c
 */
extern void fpis2i(Internal *, void *);
extern void fpid2i(Internal *, void *);
extern void fpiw2i(Internal *, void *);
extern void fpii2s(void *, Internal *);
extern void fpii2d(void *, Internal *);
extern void fpii2w(Word *, Internal *);
