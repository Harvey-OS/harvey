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
#define	FPINEX	(3<<8)
#define	FPOVFL	(1<<12)
#define	FPUNFL	(1<<11)
#define	FPZDIV	(1<<10)
#define	FPRNR	(0<<4)
#define	FPRZ	(1<<4)
#define	FPINVAL	(3<<13)
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
#define	FPAINVAL	FPINVAL
union FPdbleword
{
	double	x;
	struct {	/* big endian */
		ulong hi;
		ulong lo;
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
		((list += 4), (mode*)list)[-1]:\
	(sizeof(mode) == 2)?\
		((list += 4), (mode*)list)[-1]:\
		((list += sizeof(mode)), (mode*)list)[-1])
