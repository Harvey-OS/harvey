#define nil		((void*)0)
typedef	unsigned short	ushort;
typedef	unsigned char	uchar;
typedef unsigned long	ulong;
typedef unsigned int	uint;
typedef   signed char	schar;
typedef	long long	vlong;
typedef	unsigned long long uvlong;
typedef unsigned long	uintptr;
typedef unsigned long	usize;
typedef	ushort		Rune;
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

/* FCR */
#define	FPINEX	(1<<30)
#define	FPOVFL	(1<<19)
#define	FPUNFL	((1<<29)|(1<<28))
#define	FPZDIV	(1<<18)
#define	FPINVAL	(1<<17)

#define	FPRNR	(2<<26)
#define	FPRZ		(0<<26)
#define	FPRPINF	(3<<26)
#define	FPRNINF	(1<<26)
#define	FPRMASK	(3<<26)

#define	FPPEXT	0
#define	FPPSGL	0
#define	FPPDBL	0
#define	FPPMASK	0
/* FSR */
#define	FPAINEX	(1<<24)
#define	FPAUNFL	(1<<23)
#define	FPAOVFL	(1<<22)
#define	FPAZDIV	(1<<21)
#define	FPAINVAL	(1<<20)
union FPdbleword
{
	double	x;
	struct {	/* little endian */
		ulong lo;
		ulong hi;
	};
};

/* stdarg */
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
	sizeof(mode)>4?\
		((mode*)(list = (char*)((uintptr)(list+7) & ~7) + sizeof(mode)))[-1]:\
		((list += sizeof(mode)), (mode*)list)[-1])
