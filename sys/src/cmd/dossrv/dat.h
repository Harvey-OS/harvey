typedef struct Dosboot		Dosboot;
typedef struct Dosboot32	Dosboot32;
typedef struct Dosbpb		Dosbpb;
typedef struct Dosdir		Dosdir;
typedef struct Dospart		Dospart;
typedef struct Dosptr		Dosptr;
typedef struct Fatinfo		Fatinfo;
typedef struct Xfs		Xfs;
typedef struct Xfile		Xfile;

struct Dospart{
	uchar	active;
	uchar	hstart;
	uchar	cylstart[2];
	uchar	type;
	uchar	hend;
	uchar	cylend[2];
	uchar	start[4];
	uchar	length[4];
};

enum
{
	/*
	 * dos partition types
	 */
	FAT12		= 0x01,
	FAT16		= 0x04,		/* partitions smaller than 32MB */
	FATHUGE		= 0x06,		/* fat16 partitions larger than 32MB */
	FAT32		= 0x0b,
	FAT32X		= 0x0c,
	FATHUGEX	= 0x0e,
	DMDDO		= 0x54,

	FATRESRV	= 2,		/* number of reserved fat entries */
};

/*
 * dos boot sector, the start of every dos partition
 */
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
	uchar	bigvolsize[4];		/* same as Dosboot32 up to here */
	uchar	driveno;
	uchar	reserved0;
	uchar	bootsig;
	uchar	volid[4];
	uchar	label[11];
	uchar	reserved1[8];
};

/*
 * dos boot sector for FAT32
 */
enum
{
	NOFATMIRROR	= 0x0080,	/* masks for extflags */
	ACTFATMASK	= 0x000f,
};

struct Dosboot32{
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
	uchar	bigvolsize[4];		/* same as Dosboot up to here */
	uchar	fatsize32[4];		/* sectors per fat */
	uchar	extflags[2];		/* active fat flags */
	uchar	version1[2];		/* fat32 version; major & minor bytes */
	uchar	rootstart[4];		/* starting cluster of root dir */
	uchar	infospec[2];		/* fat allocation info sector */
	uchar	backupboot[2];		/* backup boot sector */
	uchar	reserved[12];
};

/*
 * optional FAT32 info sector
 */
enum
{
	FATINFOSIG1	= 0x41615252UL,
	FATINFOSIG	= 0x61417272UL,
};

struct Fatinfo
{
	uchar	sig1[4];
	uchar	pad[480];
	uchar	sig[4];
	uchar	freeclust[4];	/* num frre clusters; -1 is unknown */
	uchar	nextfree[4];	/* most recently allocated cluster */
	uchar	resrv[4*3];
};

/*
 * BIOS paramater block
 */
struct Dosbpb{
	MLock;				/* access to fat */
	int	sectsize;		/* in bytes */
	int	clustsize;		/* in sectors */
	int	nresrv;			/* sectors */
	int	nfats;			/* usually 2; modified to 1 if fat mirroring disabled */
	int	rootsize;		/* number of entries, for fat12 and fat16 */
	long	volsize;		/* in sectors */
	int	mediadesc;
	long	fatsize;		/* in sectors */
	int	fatclusters;
	int	fatbits;		/* 12, 16, or 32 */
	long	fataddr;		/* sector number of first valid fat entry */
	long	rootaddr;		/* for fat16 or fat12, sector of root dir */
	long	rootstart;		/* for fat32, cluster of root dir */
	long	dataaddr;		/* initial sector of data clusters */
	long	freeptr;		/* next free cluster candidate */
	long	freeclusters;		/* count of free clusters, for fat32 */
	int	fatinfo;		/* fat info sector location; 0 => none */
};

enum
{
	DOSDIRSIZE	= 32,
	DOSEMPTY	= 0xe5,			/* first char in name if entry is unused */
	DOSRUNE		= 13,			/* runes per dosdir in a long file name */
	DOSNAMELEN	= 261			/* max dos file name length */
};

struct Dosdir{
	uchar	name[8];
	uchar	ext[3];
	uchar	attr;
	uchar	reserved[1];
	uchar	ctime[3];		/* creation time */
	uchar	cdate[2];		/* creation date */
	uchar	adate[2];		/* last access date */
	uchar	hstart[2];		/* high bits of start for fat32 */
	uchar	time[2];		/* last modified time */
	uchar	date[2];		/* last modified date */
	uchar	start[2];
	uchar	length[4];
};

enum
{
	DRONLY		= 0x01,
	DHIDDEN		= 0x02,
	DSYSTEM		= 0x04,
	DVLABEL		= 0x08,
	DDIR		= 0x10,
	DARCH		= 0x20,
};

#define	GSHORT(p)	(((p)[0])|(p)[1]<<8)
#define	GLONG(p)	(((long)(p)[0])|(p)[1]<<8|(p)[2]<<16|(p)[3]<<24)
#define PSHORT(p,v)	((p)[0]=(v),(p)[1]=(v)>>8)
#define PLONG(p,v)	((p)[0]=(v),(p)[1]=(v)>>8,(p)[2]=(v)>>16,(p)[3]=(v)>>24)

struct Dosptr{
	ulong	addr;		/* sector & entry within of file's directory entry */
	ulong	offset;
	ulong	paddr;		/* of parent's directory entry */
	ulong	poffset;
	ulong	iclust;		/* ordinal within file */
	ulong	clust;
	ulong	naddr;		/* next block in directory (for writing multi entry elements) */
	ulong	prevaddr;
	Iosect *p;
	Dosdir *d;
};

#define	QIDPATH(p)	((p)->addr*(Sectorsize/DOSDIRSIZE) + \
			 (p)->offset/DOSDIRSIZE)

struct Xfs{
	Xfs	*next;
	int omode;		/* of file containing external fs */
	char	*name;		/* of file containing external f.s. */
	Qid	qid;		/* of file containing external f.s. */
	long	ref;		/* attach count */
	Qid	rootqid;	/* of plan9 constructed root directory */
	uchar	isfat32;	/* is a fat 32 file system? */
	short	dev;
	short	fmt;
	long	offset;
	void	*ptr;
};

struct Xfile{
	Xfile	*next;		/* in hash bucket */
	long	fid;
	ulong	flags;
	Qid	qid;
	Xfs	*xf;
	Dosptr	*ptr;
};

enum{
	Asis, Clean, Clunk
};

enum{
	Invalid, Short, ShortLower, Long
};

enum{	/* Xfile flags */
	Oread = 1,
	Owrite = 2,
	Orclose = 4,
	Omodes = 3,
};

enum{
	Enevermind,
	Eformat,
	Eio,
	Enoauth,
	Enomem,
	Enonexist,
	Eperm,
	Enofilsys,
	Eauth,
	Econtig,
	Ebadfcall,
	Ebadstat,
	Eversion,
	Etoolong,
	Eerrstr,
	ESIZE
};

extern int	chatty;
extern int	errno;
extern int	readonly;
extern char	*deffile;
extern int trspaces;