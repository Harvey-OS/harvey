typedef	unsigned short	Ushort;
typedef	unsigned char	Uchar;
typedef unsigned long	Ulong;
typedef union
{
	char	clength[8];
#ifdef	BIGEND
	struct{
		Ulong	hlength;
		Ulong	length;
	}l;
#else
	struct{
		Ulong	length;
		Ulong	hlength;
	}l;
#endif
}Length;
