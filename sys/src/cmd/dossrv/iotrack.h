typedef struct Lock	Lock;
typedef	struct Iosect	Iosect;
typedef	struct Iotrack	Iotrack;
typedef struct Track	Track;
typedef struct Xfs	Xfs;

struct Lock
{
	char	key;
};

struct Iosect
{
	Iosect *next;
	short	flags;
	Lock	lock;
	Iotrack *t;
	uchar *	iobuf;
};

struct Iotrack
{
	short	flags;
	Xfs *	xf;
	long	addr;
	Iotrack	*next;		/* in lru list */
	Iotrack	*prev;
	Iotrack	*hnext;		/* in hash list */
	Iotrack	*hprev;
	Lock	lock;
	int	ref;
	Track	*tp;
};

enum{
	Sectorsize = 512,
	Sect2trk = 9,
	Trksize = Sectorsize*Sect2trk
};

struct Track
{
	Iosect *p[Sect2trk];
	uchar	buf[Sect2trk][Sectorsize];
};

#define	BMOD		(1<<0)
#define	BIMM		(1<<1)
#define	BSTALE		(1<<2)

Iosect*	getiosect(Xfs*, long, int);
Iosect*	getosect(Xfs*, long);
Iosect*	getsect(Xfs*, long);
Iosect*	newsect(void);
Iotrack*	getiotrack(Xfs*, long);
int	canlock(Lock*);
int	devread(Xfs*, long, void*, long);
int	devwrite(Xfs*, long, void*, long);
int	tread(Iotrack*);
int	twrite(Iotrack*);
void	freesect(Iosect*);
void	iotrack_init(void);
void	lock(Lock*);
void	purgebuf(Xfs*);
void	purgetrack(Iotrack*);
void	putsect(Iosect*);
void	sync(void);
void	unlock(Lock*);
