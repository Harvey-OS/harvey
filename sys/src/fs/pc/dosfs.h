typedef struct Dosboot	Dosboot;
typedef struct Dos	Dos;
typedef struct Dosdir	Dosdir;
typedef struct Dosfile	Dosfile;
typedef struct Dospart	Dospart;

struct Dospart
{
	uchar	flag;		/* active flag */
	uchar	shead;		/* starting head */
	uchar	scs[2];		/* starting cylinder/sector */
	uchar	type;		/* partition type */
	uchar	ehead;		/* ending head */
	uchar	ecs[2];		/* ending cylinder/sector */
	uchar	start[4];	/* starting sector */
	uchar	len[4];		/* length in sectors */
};

struct Dosboot{
	uchar	magic[3];
	uchar	version[8];
	uchar	sectbytes[2];
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

struct Dosfile{
	Dos *	dos;		/* owning dos file system */
	int	pdir;		/* sector containing directory entry */
	int	odir;		/* offset to same */
	char	name[8];
	char	ext[3];
	uchar	attr;
	Devsize	length;
	Devsize	pstart;		/* physical start cluster address */
	Devsize	pcurrent;	/* physical current cluster address */
	Devsize	lcurrent;	/* logical current cluster address */
	Devsize	offset;
};

struct Dos{
	int	dev;				/* device id */
	Off	(*read)(int, void*, long);	/* read routine */
	Devsize	(*seek)(int, Devsize);		/* seek routine */
	Off	(*write)(int, void*, long);	/* write routine */

	int	start;		/* start of file system (sector no.) */
	int	sectbytes;	/* size of a sector */
	int	clustsize;	/* size of a cluster (in sectors) */
	int	clustbytes;	/* size of a cluster (in bytes) */
	int	nresrv;		/* sectors */
	int	nfats;		/* usually 2 */
	int	rootsize;	/* number of entries */
	int	volsize;	/* in sectors */
	int	mediadesc;
	int	fatsize;	/* size of a fat (in sectors) */
	int	fatbytes;	/* size of a fat (in bytes) */
	int	fatclusters;	/* no. of clusters governed by fat */
	int	fatbits;	/* 12 or 16 */
	Devsize	fataddr;	/* sector address of first fat */
	Devsize	rootaddr;	/* sector address of root directory */
	Devsize	dataaddr;	/* sector address of first data block */
	Devsize	freeptr;	/* for cluster allocation */

	Dosfile	root;
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
#define	DOSDIR	0x10
#define	DARCH	0x20

extern int	dosinit(Dos*);
extern Dosfile*	dosopen(Dos*, char*, Dosfile*);
extern int	dostrunc(Dosfile*);
extern long	dosread(Dosfile*, void*, long);
extern long	doswrite(Dosfile*, void*, long);

extern Dos	dos;
