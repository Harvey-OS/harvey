typedef struct	Duart	Duart;

#define	SYNCREG		((char*)0x40400000)
#define	DISPLAYRAM	0x02000000
#define	DUARTREG	((Duart*)0x40100000)
#define	PORT		((uchar *)0x40300000)

/*
 * Balu
 */
typedef struct Balu Balu;

struct Balu{
	ulong	src[4];
	ulong	dst[4];
	ulong	res[4];
	ulong	cr0;
	ulong	cr1;
};

#define BALU	((Balu *) 0xc0000000)
#define OR(x,n)	(ulong *) (((ulong) (x)) | n)
#define SOR(x)	(OR(x,0x30000000))
#define DOR(x)	(OR(x,0x20000000))

/* goo for cr0 manipulations */
#define	OPLENMA		0x0000ffff /* 15 bits only needed */
#define	OPLENSH		0
#define	OPCNTMA		0xffff0000 /* 16 bit signed */
#define	OPCNTSH		16

/* goo for cr1 manipulations */
#define	ALUOPMA		0x0000001f /* 5bits */
#define	ALUOPSH		0
#define	PIXELMA		0x00000060 /* 2bits */
#define	PIXELSH		5
#define	SHMAGMA		0x00000f80 /* 5bits */
#define	SHMAGSH		7
#define	ALIGNMA		0x00003000 /* 2bits */
#define	ALIGNSH		12
#define	RLSCANMA	0x00004000 /* 1bit */
#define	RLSCANSH	14
#define	QOPMA		0x00008000 /* 1bit */
#define	QOPSH		15
#define	RMASKMA		0x001f0000 /* 5bits */
#define	RMASKSH		16
#define	LMASKMA		0x03e00000 /* 5bits */
#define	LMASKSH		21

enum
{
	SUB		= 0x10,		/* balu arithmetic codes */
	SSUB		= 0x11,
	ADD		= 0x12,
	SADD		= 0x13,
	MIN		= 0x14,
	MAX		= 0x15
};
