#define nil		((void*)0)
typedef	unsigned short	ushort;
typedef	unsigned char	uchar;
typedef	unsigned long	ulong;
typedef	unsigned int	uint;
typedef	signed char	schar;
typedef	long long	vlong;
typedef	unsigned long long uvlong;
typedef long		intptr;
typedef unsigned long	uintptr;
typedef unsigned long	usize;
typedef	uint		Rune;
typedef 	union FPdbleword FPdbleword;
typedef uintptr	jmp_buf[2];
#define	JMPBUFSP	0
#define	JMPBUFPC	1
#define	JMPBUFDPC	0
typedef unsigned int	mpdigit;	/* for /sys/include/mp.h */
typedef unsigned char u8int;
typedef unsigned short u16int;
typedef unsigned int	u32int;
typedef unsigned long long u64int;

/* FCR */
#define	FPINEX	(1<<7)
#define	FPUNFL	(1<<8)
#define	FPOVFL	(1<<9)
#define	FPZDIV	(1<<10)
#define	FPINVAL	(1<<11)
#define	FPRNR	(0<<0)
#define	FPRZ	(1<<0)
#define	FPRPINF	(2<<0)
#define	FPRNINF	(3<<0)
#define	FPRMASK	(3<<0)
#define	FPPEXT	0
#define	FPPSGL	0
#define	FPPDBL	0
#define	FPPMASK	0
/* FSR */
#define	FPAINEX	(1<<2)
#define	FPAOVFL	(1<<4)
#define	FPAUNFL	(1<<3)
#define	FPAZDIV	(1<<5)
#define	FPAINVAL	(1<<6)
union FPdbleword
{
	double	x;
	struct {	/* big endian */
		ulong hi;
		ulong lo;
	};
};

/* stdarg */
typedef	char*	va_list;
#define va_start(list, start) (list = (char*)&start+4)
#define va_end(list) USED(list)
#define va_arg(list, mode)\
	(sizeof(mode)==1?\
		((mode*)(list += 4))[-4]:\
	sizeof(mode)==2?\
		((mode*)(list += 4))[-2]:\
	sizeof(mode)<=4?\
		((mode*)(list += 4))[-1]:\
		((mode*)(list = (char*)((long)(list+7) & ~7) + sizeof(mode)))[-1])
