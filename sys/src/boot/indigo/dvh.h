typedef struct Ptable	Ptable;
typedef struct Vdir	Vdir;
typedef union  Vhdr	Vhdr;

struct Ptable
{
	int	nblks;
	int	firstlbn;
	int	type;
};

#define	VOLHDR	0	/* partition is volume header */
#define	VOLUME	6	/* partition is whole volume */

struct Vdir
{
	char	name[8];
	int	lbn;
	int	nbytes;
};

#define	VMAGIC	0xbe5a941

union Vhdr
{
	long	buf[128];
	struct
	{
		int	magic;
		short	rootpt;
		short	swappt;
		char	boot[16];
		uchar	skew;
		uchar	gap1;
		uchar	gap2;
		uchar	dummy;
		ushort	cyls;
		ushort	shd0;
		ushort	trks0;
		ushort	shd1;
		ushort	trks1;
		ushort	sec2trk;
		ushort	bytes2sec;
		ushort	interleave;
		int	flags;
		int	rate;
		int	retries;
		int	spare[4];
		Vdir	vd[15];
		Ptable	pt[16];
		int	csum;
	};
};
