#define nil		((void*)0)

typedef	unsigned short	ushort;
typedef	unsigned char	uchar;
typedef	unsigned long	ulong;
typedef	unsigned int	uint;
typedef	signed char	schar;
typedef	long long	vlong;
typedef	unsigned long long uvlong;
typedef unsigned long	uintptr;
typedef unsigned long	usize;
typedef	uint		Rune;
typedef 	union FPdbleword FPdbleword;
typedef long	jmp_buf[2];
#define	JMPBUFSP	0
#define	JMPBUFPC	1
#define	JMPBUFDPC	0
typedef unsigned int	mpdigit;	/* for /sys/include/mp.h */
typedef unsigned char u8int;
typedef unsigned short u16int;
typedef unsigned int	u32int;
typedef unsigned long long u64int;

/* VFP FCR */
#define	FPINEX	(1<<12)			/* trap enables for exceptions */
#define	FPUNFL	(1<<11)
#define	FPOVFL	(1<<10)
#define	FPZDIV	(1<<9)
#define	FPINVAL	(1<<8)
#define	FPRNR	(0<<22)
#define	FPRZ	(1<<22)
#define	FPRPINF	(2<<22)
#define	FPRNINF	(3<<22)
#define	FPRMASK	(3<<22)
#define	FPPEXT	0
#define	FPPSGL	0
#define	FPPDBL	0
#define	FPPMASK	0
/* FSR */
#define	FPAINEX	(1<<4)			/* accrued exceptions */
#define	FPAUNFL	(1<<3)
#define	FPAOVFL	(1<<2)
#define	FPAZDIV	(1<<1)
#define	FPAINVAL	(1<<0)
union FPdbleword
{
	double	x;
	struct {	/* little endian */
		ulong lo;
		ulong hi;
	};
};

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
		((list += sizeof(mode)), (mode*)list)[-1])
