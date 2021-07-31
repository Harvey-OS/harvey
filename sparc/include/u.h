typedef	unsigned short	ushort;
typedef	unsigned char	uchar;
typedef	unsigned long	ulong;
typedef	unsigned int	uint;
typedef	signed char	schar;
typedef	long long	vlong;
typedef	unsigned long long uvlong;
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
#define	JMPBUFDPC	(-8)

/* FCR */
#define	FPINEX	(1<<23)
#define	FPOVFL	(1<<26)
#define	FPUNFL	(1<<25)
#define	FPZDIV	(1<<24)
#define	FPRNR	(0<<30)
#define	FPRZ	(1<<30)
#define	FPRPINF	(2<<30)
#define	FPRNINF	(3<<30)
#define	FPRMASK	(3<<30)
#define	FPPEXT	0
#define	FPPSGL	0
#define	FPPDBL	0
#define	FPPMASK	0
/* FSR */
#define	FPAINEX	(1<<5)
#define	FPAOVFL	(1<<8)
#define	FPAUNFL	(1<<7)
#define	FPAZDIV	(1<<6)
