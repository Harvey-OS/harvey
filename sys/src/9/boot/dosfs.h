typedef struct Dosboot	Dosboot;
typedef struct Dos	Dos;
typedef struct Dosdir	Dosdir;
typedef struct Dosfile	Dosfile;
typedef struct Dospart	Dospart;

struct Dospart
{
	uchar flag;		/* active flag */
	uchar shead;		/* starting head */
	uchar scs[2];		/* starting cylinder/sector */
	uchar type;		/* partition type */
	uchar ehead;		/* ending head */
	uchar ecs[2];		/* ending cylinder/sector */
	uchar start[4];		/* starting sector */
	uchar len[4];		/* length in sectors */
};

#define FAT12	0x01
#define FAT16	0x04
#define EXTEND	0x05
#define FATHUGE	0x06
#define FAT32	0x0b
#define FAT32X	0x0c
#define EXTHUGE	0x0f
#define DMDDO	0x54
#define PLAN9	0x39
#define LEXTEND 0x85

struct Dosfile{
	Dos	*dos;		/* owning dos file system */
	char	name[8];
	char	ext[3];
	uchar	attr;
	long	length;
	long	pstart;		/* physical start cluster address */
	long	pcurrent;	/* physical current cluster address */
	long	lcurrent;	/* logical current cluster address */
	long	offset;
};

struct Dos{
	long	start;		/* start of file system */
	int	sectsize;	/* in bytes */
	int	clustsize;	/* in sectors */
	int	clustbytes;	/* in bytes */
	int	nresrv;		/* sectors */
	int	nfats;		/* usually 2 */
	int	rootsize;	/* number of entries */
	int	volsize;	/* in sectors */
	int	mediadesc;
	int	fatsize;	/* in sectors */
	int	fatclusters;
	int	fatbits;	/* 12 or 16 */
	long	fataddr;	/* sector number */
	long	rootaddr;
	long	rootclust;
	long	dataaddr;
	long	freeptr;
};

extern int	dosinit(Fs*);
