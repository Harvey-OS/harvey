#define nil		((void*)0)
typedef	unsigned short		ushort;
typedef	unsigned char		uchar;
typedef	signed char		schar;
typedef	unsigned long		ulong;
typedef	unsigned int	uint;
typedef	long long			vlong;
typedef	unsigned long long	uvlong;
typedef	union Length		Length;
typedef	ushort			Rune;

union Length
{
	vlong	length;
};

/* stdarg */
typedef	char*	va_list;
#define va_start(list, start) list = (char*)(&(start)+1)
#define va_end(list)
#define va_arg(list, mode)\
	(sizeof(mode)==1?\
		((mode*)(list += 4))[-1]:\
	sizeof(mode)==2?\
		((mode*)(list += 4))[-1]:\
	sizeof(mode)>4?\
		((mode*)(list = (char*)((long)(list+7) & ~7) + sizeof(mode)))[-1]:\
		((mode*)(list += sizeof(mode)))[-1])
