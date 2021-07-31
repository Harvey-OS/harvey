#include	<u.h>
#include	<libc.h>
#include	<bio.h>

typedef	struct	Dev	Dev;
typedef	struct	Chan	Chan;
typedef	struct	Table	Table;

enum
{
	Tunk		= 0,	/* types */
	Tcdda,
	Tcdrom,
	Tcdi,

	Dunk		= 0,	/* devices */
	Dtosh,
	Dnec,
	Dplex,
	Dphil,
	Dfile,
	Ddisk,

	Sread		= 0,
	Swrite,
	Snone		= Swrite,

	Bcdda		= 2352,
	Bcdrom		= 2048,

	Ntrack		= 201,		/* total number of tracks */
	Maxtrack	= 200,
	Tracktmp	= Maxtrack,
	Trackall	= Maxtrack+1,

	Ctlrtarg	= 7,		/* target of scsi controller */
};
struct	Chan
{
	int	open;
	int	fcmd;
	int	fdata;
};
struct	Table
{
	long	type;
	long	start;
	long	size;
	union
	{
		struct	/* min size of extra garbage */
		{
			long	pad;		/* bytes of good data */
			long	pad;
			long	pad;
			long	pad;
		};
		struct	/* disk */
		{
			long	fsize;		/* frame size of source */
			long	bsize;		/* block size of source */
			long	tsize;		/* total size in source block sizes */
			long	magic;		/* for error detection */
		};
		struct	/* philips */
		{
			long	xsize;		/* free blocks */
			long	tmode;		/* track modee */
			long	dmode;		/* data mode */
			long	firstw;		/* first writable address */
		};
	};
};
struct	Dev
{
	Chan	chan;
	int	firsttrack;
	int	lasttrack;
	Table	table[Ntrack];
};
char	line[1000];
char	word[100];
uchar	blkbuf[4096];	/* larger than any block size, ie > 2352 */
Biobuf	bin;

void	input(int);
int	getword(void);
int	getdev(char*);
int	gettype(char*);
int	gettrack(char*);
int	eol(void);
void	doprobe(int, Chan*);
int	scsi(Chan, uchar*, int, uchar*, int, int);
Chan	openscsi(int);
void	closescsi(Chan);
int	readscsi(Chan, void*, long, long, int, int, int);
int	writescsi(Chan, void*, long, long, int);
void	swab(uchar*, int);
long	bige(uchar*);
void	doremove(void);

int	dopen(void);
uchar*	dbufalloc(long, long);
void	dcommit(int, int);
int	dwrite(uchar*, long);
int	dclearentry(int, int);
int	dcleartoc(void);
void	dsum(void);
int	dverify(int, int);

void	doload(void);
void	tload(void);
void	xload(void);
void	pload(void);
void	fload(void);
void	nload(void);

void	doverif(void);
void	tverif(void);
void	xverif(void);

void	dstore(void);
void	dpublish(void);

void	dotoc(void);
void	dtoc(void);
void	ttoc(void);
void	xtoc(void);
void	ntoc(void);

void	ptoc(void);
long	pwrite(uchar*, long);
int	pinit(long, long);
void	pflush(void);
void	pfixate(void);
void	psession(void);

void	mk9660(void);
