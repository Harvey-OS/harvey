#define nil		((void*)0)
typedef	unsigned short	ushort;
typedef	unsigned char	uchar;
typedef	unsigned long	ulong;
typedef	unsigned int	uint;
typedef	signed char	schar;
typedef	long long	vlong;
typedef	unsigned long long uvlong;
typedef	ushort		Rune;
typedef 	union FPdbleword FPdbleword;
typedef long	jmp_buf[2];
#define	JMPBUFSP	0
#define	JMPBUFPC	1
#define	JMPBUFDPC	0
typedef unsigned int	mpdigit;	/* for /sys/include/mp.h */
typedef unsigned int	u32int;
union FPdbleword
{
	double	x;
	struct {	/* big endian */
		long hi;
		long lo;
	};
};

typedef	char*	va_list;
#define va_start(list, start) list = (char*)&start + 4
#define va_end(list)
#define va_arg(list, mode)\
	(sizeof(mode)==1?\
		((mode*)(list += 4))[-4]:\
	sizeof(mode)==2?\
		((mode*)(list += 4))[-2]:\
		((mode*)(list += 4))[-1])
