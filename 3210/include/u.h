typedef	unsigned short	ushort;
typedef	unsigned char	uchar;
typedef	unsigned long	ulong;
typedef	unsigned int	uint;
typedef	signed char	schar;
typedef	long		vlong;
typedef	unsigned long	uvlong;
typedef	ushort		Rune;
typedef	union
{
	char	clength[8];
	vlong	vlength;
	struct
	{
		long	hlength;
		long	length;
	};
} Length;
typedef long	jmp_buf[2];
#define	JMPBUFSP	0
#define	JMPBUFPC	1
#define	JMPBUFDPC	0

/* FCR */
/* could enable overflow and underflow exceptions at same time */
#define	FPINEX	0
#define	FPOVFL	0
#define	FPUNFL	0
#define	FPZDIV	0
#define	FPRNR	(0<<4)
#define	FPRZ	(3<<4)
#define	FPRPINF	0
#define	FPRNINF	(1<<4)
#define	FPRMASK	(3<<4)
#define	FPPEXT	0
#define	FPPSGL	0
#define	FPPDBL	0
#define	FPPMASK	0
/* FSR */
#define	FPAINEX	0
#define	FPAOVFL	0
#define	FPAUNFL	0
#define	FPAZDIV	0
