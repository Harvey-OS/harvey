/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

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
	unsigned char	active;
	unsigned char	hstart;
	unsigned char	cylstart[2];
	unsigned char	type;
	unsigned char	hend;
	unsigned char	cylend[2];
	unsigned char	start[4];
	unsigned char	length[4];
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
	unsigned char	magic[3];
	unsigned char	version[8];
	unsigned char	sectsize[2];
	unsigned char	clustsize;
	unsigned char	nresrv[2];
	unsigned char	nfats;
	unsigned char	rootsize[2];
	unsigned char	volsize[2];
	unsigned char	mediadesc;
	unsigned char	fatsize[2];
	unsigned char	trksize[2];
	unsigned char	nheads[2];
	unsigned char	nhidden[4];
	unsigned char	bigvolsize[4];		/* same as Dosboot32 up to here */
	unsigned char	driveno;
	unsigned char	reserved0;
	unsigned char	bootsig;
	unsigned char	volid[4];
	unsigned char	label[11];
	unsigned char	reserved1[8];
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
	unsigned char	magic[3];
	unsigned char	version[8];
	unsigned char	sectsize[2];
	unsigned char	clustsize;
	unsigned char	nresrv[2];
	unsigned char	nfats;
	unsigned char	rootsize[2];
	unsigned char	volsize[2];
	unsigned char	mediadesc;
	unsigned char	fatsize[2];
	unsigned char	trksize[2];
	unsigned char	nheads[2];
	unsigned char	nhidden[4];
	unsigned char	bigvolsize[4];		/* same as Dosboot up to here */
	unsigned char	fatsize32[4];		/* sectors per fat */
	unsigned char	extflags[2];		/* active fat flags */
	unsigned char	version1[2];		/* fat32 version; major & minor bytes */
	unsigned char	rootstart[4];		/* starting cluster of root dir */
	unsigned char	infospec[2];		/* fat allocation info sector */
	unsigned char	backupboot[2];		/* backup boot sector */
	unsigned char	reserved[12];
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
	unsigned char	sig1[4];
	unsigned char	pad[480];
	unsigned char	sig[4];
	unsigned char	freeclust[4];	/* num frre clusters; -1 is unknown */
	unsigned char	nextfree[4];	/* most recently allocated cluster */
	unsigned char	resrv[4*3];
};

/*
 * BIOS paramater block
 */
struct Dosbpb{
	MLock MLock;				/* access to fat */
	int	sectsize;		/* in bytes */
	int	clustsize;		/* in sectors */
	int	nresrv;			/* sectors */
	int	nfats;			/* usually 2; modified to 1 if fat mirroring disabled */
	int	rootsize;		/* number of entries, for fat12 and fat16 */
	int32_t	volsize;		/* in sectors */
	int	mediadesc;
	int32_t	fatsize;		/* in sectors */
	int	fatclusters;
	int	fatbits;		/* 12, 16, or 32 */
	int32_t	fataddr;		/* sector number of first valid fat entry */
	int32_t	rootaddr;		/* for fat16 or fat12, sector of root dir */
	int32_t	rootstart;		/* for fat32, cluster of root dir */
	int32_t	dataaddr;		/* initial sector of data clusters */
	int32_t	freeptr;		/* next free cluster candidate */
	int32_t	freeclusters;		/* count of free clusters, for fat32 */
	int	fatinfo;		/* fat info sector location; 0 => none */
};

enum
{
	DOSDIRSIZE	= 32,
	DOSEMPTY	= 0xe5,			/* first char in name if entry is unused */
	DOSRUNE		= 13,			/* runes per dosdir in a int32_t file name */
	DOSNAMELEN	= 261			/* max dos file name length */
};

struct Dosdir{
	unsigned char	name[8];
	unsigned char	ext[3];
	unsigned char	attr;
	unsigned char	reserved[1];
	unsigned char	ctime[3];		/* creation time */
	unsigned char	cdate[2];		/* creation date */
	unsigned char	adate[2];		/* last access date */
	unsigned char	hstart[2];		/* high bits of start for fat32 */
	unsigned char	time[2];		/* last modified time */
	unsigned char	date[2];		/* last modified date */
	unsigned char	start[2];
	unsigned char	length[4];
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
#define	GLONG(p)	(((int32_t)(p)[0])|(p)[1]<<8|(p)[2]<<16|(p)[3]<<24)
#define PSHORT(p,v)	((p)[0]=(v),(p)[1]=(v)>>8)
#define PLONG(p,v)	((p)[0]=(v),(p)[1]=(v)>>8,(p)[2]=(v)>>16,(p)[3]=(v)>>24)

struct Dosptr{
	uint32_t	addr;		/* sector & entry within of file's directory entry */
	uint32_t	offset;
	uint32_t	paddr;		/* of parent's directory entry */
	uint32_t	poffset;
	uint32_t	iclust;		/* ordinal within file */
	uint32_t	clust;
	uint32_t	naddr;		/* next block in directory (for writing multi entry elements) */
	uint32_t	prevaddr;
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
	int32_t	ref;		/* attach count */
	Qid	rootqid;	/* of plan9 constructed root directory */
	unsigned char	isfat32;	/* is a fat 32 file system? */
	short	dev;
	short	fmt;
	int32_t	offset;
	void	*ptr;
};

struct Xfile{
	Xfile	*next;		/* in hash bucket */
	int32_t	fid;
	uint32_t	flags;
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
