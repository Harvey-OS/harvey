typedef	unsigned short	ushort;
typedef	unsigned char	uchar;
typedef	signed char	schar;
typedef unsigned long	ulong;
typedef	long		vlong;
typedef union Length	Length;
typedef unsigned short	Rune;

union Length
{
	char	clength[8];
	vlong	vlength;
	struct{
		long	hlength;
		long	length;
	};
};
