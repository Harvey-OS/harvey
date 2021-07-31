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
	char	clength[8];
	vlong	vlength;
	struct
	{
		long	hlength;
		long	length;
	};
} Length;
typedef long	jmp_buf[2];
