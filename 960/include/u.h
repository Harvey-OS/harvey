#define nil		((void*)0)
#define	float	long
#define	double	long
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
	vlong	length;
} Length;
typedef long	jmp_buf[2];
typedef unsigned int	mpdigit;	/* for /sys/include/mp.h */

typedef	char*	va_list;
#define va_start(list, start) list =\
	(sizeof(start)<4?\
		(char*)((int*)&(start)+1):\
		(char*)(&(start)+1))
#define va_end(list)
#define va_arg(list, mode)\
	((mode*)(list += sizeof(mode)))[-1]
