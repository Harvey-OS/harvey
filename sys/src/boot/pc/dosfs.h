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
	int	dev;				/* device id */
	long	(*read)(int, void*, long);	/* read routine */
	long	(*seek)(int, long);		/* seek routine */

	int	start;		/* start of file system */
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
	long	dataaddr;
	long	freeptr;

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
#define	DDIR	0x10
#define	DARCH	0x20

extern int chatty;

extern int dosboot(Dos*, char*);
extern int dosinit(Dos*);
extern long dosread(Dosfile*, void*, long);
extern int dosstat(Dos*, char*, Dosfile*);
extern int doswalk(Dosfile*, char*);

extern int plan9ini(Dos*);
