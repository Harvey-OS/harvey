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
/* not terribly useful; really should be uintptr */
typedef unsigned long	usize;
typedef	uint		Rune;
typedef union FPdbleword FPdbleword;

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
#define	FPINEX	0		/* trap enables: none on risc-v */
#define	FPUNFL	0
#define	FPOVFL	0
#define	FPZDIV	0
#define	FPINVAL	0
#define	FPRNR	(0<<5)		/* rounding modes */
#define	FPRZ	(1<<5)
#define	FPRPINF	(3<<5)
#define	FPRNINF	(2<<5)
#define	FPRMASK	(7<<5)
#define	FPPEXT	0		/* precision */
#define	FPPSGL	0
#define	FPPDBL	0
#define	FPPMASK	0
/* FSR */
#define	FPAINEX	(1<<0)		/* accrued exceptions */
#define	FPAOVFL	(1<<2)
#define	FPAUNFL	(1<<1)
#define	FPAZDIV	(1<<3)
#define	FPAINVAL (1<<4)

union FPdbleword
{
	double	x;
	struct {	/* little endian */
		ulong lo;
		ulong hi;
	};
};

/* stdarg - little-endian 32-bit */
typedef	char*	va_list;
#define va_start(list, start) list =\
	(sizeof(start) < 4?\
		(char*)((int*)&(start)+1):\
		(char*)(&(start)+1))
#define va_end(list)\
	USED(list)
#define va_arg(list, mode)\
	((sizeof(mode) == 1)?\
		((list += 4), (mode*)list)[-4]:\
	(sizeof(mode) == 2)?\
		((list += 4), (mode*)list)[-2]:\
	(sizeof(mode) == 4)?\
		((list += 4), (mode*)list)[-1]:\
		((list = (char*)((uintptr)(list+7) & ~7) + sizeof(mode)), (mode*)list)[-1])
