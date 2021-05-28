#define nil		((void*)0)
typedef	unsigned short	ushort;
typedef	unsigned char	uchar;
typedef	unsigned long	ulong;
typedef	unsigned int	uint;
typedef	signed char	schar;
typedef	long long	vlong;
typedef	unsigned long long uvlong;
typedef vlong		intptr;
typedef unsigned long long uintptr;
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
#define	FPINEX	0
#define	FPUNFL	0
#define	FPOVFL	0
#define	FPZDIV	0
#define	FPINVAL	0
#define	FPRNR	(0<<5)
#define	FPRZ	(1<<5)
#define	FPRPINF	(3<<5)
#define	FPRNINF	(2<<5)
#define	FPRMASK	(15<<5)
#define	FPPEXT	0
#define	FPPSGL	0
#define	FPPDBL	0
#define	FPPMASK	0
/* FSR */
#define	FPAINEX	(1<<0)
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

/* stdarg - little-endian 64-bit */
typedef	uintptr	va_list;
#define va_start(list, start) list =\
	(sizeof(start) < 4?\
		(uintptr)((long*)&(start)+1):\
		(uintptr)(&(start)+1))
#define va_end(list)\
	USED(list)
#define va_arg(list, mode)\
	((sizeof(mode) == 1)?\
		((list += 4), (mode*)list)[-4]:\
	(sizeof(mode) == 2)?\
		((list += 4), (mode*)list)[-2]:\
	(sizeof(mode) == 4)?\
		((list += 4), (mode*)list)[-1]:\
		((list += sizeof(mode)+7), (list &= ~7), (mode*)list)[-1])

#define _BITS64		/* for ape */
