typedef	unsigned short	ushort;
typedef	unsigned char	uchar;
typedef unsigned long	ulong;
typedef unsigned int	uint;
typedef   signed char	schar;
typedef	long		vlong;
typedef	ushort		Rune;
typedef union
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
#define	FPINEX	(3<<8)
#define	FPOVFL	(1<<12)
#define	FPUNFL	(1<<11)
#define	FPZDIV	(1<<10)
#define	FPRNR	(0<<4)
#define	FPRZ	(1<<4)
#define	FPRPINF	(3<<4)
#define	FPRNINF	(2<<4)
#define	FPRMASK	(3<<4)
#define	FPPEXT	(0<<6)
#define	FPPSGL	(1<<6)
#define	FPPDBL	(2<<6)
#define	FPPMASK	(3<<6)
/* FSR */
#define	FPAINEX	FPINEX
#define	FPAOVFL	FPOVFL
#define	FPAUNFL	FPUNFL
#define	FPAZDIV	FPZDIV
