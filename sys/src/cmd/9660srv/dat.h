typedef	struct Iobuf	Iobuf;
typedef struct Xdata	Xdata;
typedef struct Xfile	Xfile;
typedef struct Xfs	Xfs;
typedef struct Xfsub	Xfsub;

enum
{
	Sectorsize = 2048
};

struct Iobuf
{
	Xdata*	dev;
	long	addr;
	Iobuf*	next;
	Iobuf*	prev;
	Iobuf*	hash;
	int	busy;
	uchar*	iobuf;
};

struct Xdata
{
	Xdata*	next;
	char*	name;		/* of underlying file */
	Qid	qid;
	short	type;
	short	fdev;
	int	ref;		/* attach count */
	int	dev;		/* for read/write */
};

struct Xfsub
{
	void	(*reset)(void);
	int	(*attach)(Xfile*);
	void	(*clone)(Xfile*, Xfile*);
	void	(*walkup)(Xfile*);
	void	(*walk)(Xfile*, char*);
	void	(*open)(Xfile*, int);
	void	(*create)(Xfile*, char*, long, int);
	long	(*readdir)(Xfile*, char*, long, long);
	long	(*read)(Xfile*, char*, long, long);
	long	(*write)(Xfile*, char*, long, long);
	void	(*clunk)(Xfile*);
	void	(*remove)(Xfile*);
	void	(*stat)(Xfile*, Dir*);
	void	(*wstat)(Xfile*, Dir*);
};

struct Xfs
{
	Xdata*	d;		/* how to get the bits */
	Xfsub*	s;		/* how to use them */
	int	ref;
	int	issusp;	/* follows system use sharing protocol */
	long	suspoff;	/* if so, offset at which SUSP area begins */
	int	isrock;	/* Rock Ridge format */
	int	isplan9;	/* has Plan 9-specific directory info */
	Qid	rootqid;
	void*	ptr;		/* private data */
};

struct Xfile
{
	Xfile*	next;		/* in fid hash bucket */
	Xfs*	xf;
	long	fid;
	ulong	flags;
	Qid	qid;
	int	len;		/* of private data */
	void*	ptr;
};

enum
{
	Asis,
	Clean,
	Clunk
};

enum
{
	Oread = 1,
	Owrite = 2,
	Orclose = 4,
	Omodes = 3,
};

extern char	Enonexist[];	/* file does not exist */
extern char	Eperm[];	/* permission denied */
extern char	Enofile[];	/* no file system specified */
extern char	Eauth[];	/* authentication failed */

extern char	*srvname;
extern char	*deffile;
extern int	chatty;
extern jmp_buf	err_lab[];
extern int	nerr_lab;
extern char	err_msg[];

extern int nojoliet;
extern int noplan9;
extern int norock;
