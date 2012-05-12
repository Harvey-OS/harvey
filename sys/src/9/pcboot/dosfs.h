typedef struct Dosboot	Dosboot;
typedef struct Dos	Dos;
typedef struct Dosdir	Dosdir;
typedef struct Dosfile	Dosfile;
typedef struct Dospart	Dospart;
typedef struct File File;
typedef struct Bootfs Bootfs;

int fsread(File *file, void *a, long n);
int fsboot(Bootfs *fs, char *path, Boot *b);
int fswalk(Bootfs *fs, char *path, File *f);

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

typedef struct Dosboot Dosboot;
typedef struct Dosdir Dosdir;

struct Dosboot{
	uchar	magic[3];
	uchar	version[8];
	uchar	sectsize[2];
	uchar	clustsize;
	uchar	nresrv[2];
	uchar	nfats;
	uchar	rootsize[2];
	uchar	volsize[2];
	uchar	mediadesc;
	uchar	fatsize[2];
	uchar	trksize[2];
	uchar	nheads[2];
	uchar	nhidden[4];
	uchar	bigvolsize[4];
/* fat 32 */
	uchar	bigfatsize[4];
	uchar	extflags[2];
	uchar	fsversion[2];
	uchar	rootdirstartclust[4];
	uchar	fsinfosect[2];
	uchar	backupbootsect[2];
/* ???
	uchar	driveno;
	uchar	reserved0;
	uchar	bootsig;
	uchar	volid[4];
	uchar	label[11];
	uchar	reserved1[8];
*/
};

struct Dosdir{
	uchar	name[8];
	uchar	ext[3];
	uchar	attr;
	uchar	lowercase;
	uchar	hundredth;
	uchar	ctime[2];
	uchar	cdate[2];
	uchar	adate[2];
	uchar	highstart[2];
	uchar	mtime[2];
	uchar	mdate[2];
	uchar	start[2];
	uchar	length[4];
};

#define	DOSRONLY	0x01
#define	DOSHIDDEN	0x02
#define	DOSSYSTEM	0x04
#define	DOSVLABEL	0x08
#define	DOSDIR	0x10
#define	DOSARCH	0x20

// #pragma incomplete Bootfs

struct File{
	union{
		Dosfile	dos;
		int walked;
	};
	Bootfs	*fs;
	char	*path;
};

struct Bootfs{
	union {
		Dos dos;
	};
	Chan	*devch;
	char	*disk;

	/* for *bios.c */
	int	dev;				/* device id */
	long	(*diskread)(Bootfs*, void*, long);	/* disk read routine */
	vlong	(*diskseek)(Bootfs*, vlong);	/* disk seek routine */

	long	(*read)(File*, void*, long);
	int	(*walk)(File*, char*);
	File	root;
};

extern int	dosinit(Bootfs*, char *);

#define BADPTR(x) ((ulong)(x) < 0x80000000)
