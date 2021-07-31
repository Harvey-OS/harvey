typedef struct Dosboot	Dosboot;
typedef struct Dosbpb	Dosbpb;
typedef struct Dosdir	Dosdir;
typedef struct Dospart	Dospart;
typedef struct Dosptr	Dosptr;
typedef struct Xfs	Xfs;
typedef struct Xfile	Xfile;

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
	uchar	driveno;
	uchar	reserved0;
	uchar	bootsig;
	uchar	volid[4];
	uchar	label[11];
	uchar	reserved1[8];
};

struct Dosbpb{
	Lock;			/* access to fat */
	int	sectsize;	/* in bytes */
	int	clustsize;	/* in sectors */
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
	long	dataaddr;
	long	freeptr;	/* next free cluster candidate */
};

struct Dosdir{
	uchar	name[8];
	uchar	ext[3];
	uchar	attr;
	uchar	reserved[10];
	uchar	time[2];
	uchar	date[2];
	uchar	start[2];
	uchar	length[4];
};

#define	DRONLY	0x01
#define	DHIDDEN	0x02
#define	DSYSTEM	0x04
#define	DVLABEL	0x08
#define	DDIR	0x10
#define	DARCH	0x20

#define	GSHORT(p) (((p)[1]<<8)|(p)[0])
#define	GLONG(p) ((GSHORT(p+2)<<16)|GSHORT(p))

struct Dosptr{
	ulong	addr;	/* of file's directory entry */
	ulong	offset;
	ulong	paddr;	/* of parent's directory entry */
	ulong	poffset;
	ulong	iclust;	/* ordinal within file */
	ulong	clust;
	Iosect *p;
	Dosdir *d;
};

struct Xfs{
	Xfs *	next;
	char *	name;		/* of file containing external f.s. */
	Qid	qid;		/* of file containing external f.s. */
	long	ref;		/* attach count */
	Qid	rootqid;	/* of plan9 constructed root directory */
	short	dev;
	short	fmt;
	long	offset;
	void *	ptr;
};

struct Xfile{
	Xfile *	next;		/* in hash bucket */
	long	client;
	long	fid;
	ulong	flags;
	Qid	qid;
	Xfs *	xf;
	void *	ptr;
};

enum{
	Asis, Clean, Clunk
};

enum{
	Oread = 1, Owrite = 2, Orclose = 4,
	Omodes = 3,
};

enum{
	Enevermind,
	Eformat,
	Eio,
	Enomem,
	Enonexist,
	Eperm,
	Enofilsys,
	Eauth,
};

extern int	chatty;
extern int	errno;
extern char	*deffile;
