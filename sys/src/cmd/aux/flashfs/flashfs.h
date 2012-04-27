/*
 *	Journal Based Flash Entrysystem.
 */

enum
{
// sector header
	MAGSIZE		= 4,
	MAXHDR		= MAGSIZE+3+3,

	MAGIC0		= 'R',
	MAGIC1		= 'O',
	MAGIC2		= 'O',
	FFSVERS		= '0',

// transactions
	FT_create	= 'G',
	FT_FCREATE0	= 'C',
	FT_FCREATE1	= 'E',
	FT_DCREATE0	= 'D',
	FT_DCREATE1	= 'F',
	FT_chmod	= 'H',
	FT_CHMOD0	= 'M',
	FT_CHMOD1	= 'O',
	FT_REMOVE	= 'R',
	FT_WRITE	= 'W',
	FT_AWRITE	= 'A',
	FT_trunc	= 'I',
	FT_TRUNC0	= 'T',
	FT_TRUNC1	= 'V',
	FT_SUMMARY	= 'S',
	FT_SUMBEG	= 'B',
	FT_SUMEND	= 'Z',

	MAXFSIZE	= 1 << 21,
	MAXNSIZE	= 28,

	Ncreate		= 1+1+3*3+MAXNSIZE+1,
	Nchmod		= 1+1+2*3,
	Nremove		= 1+3,
	Nwrite		= 1+4*3,
	Ntrunc		= 1+1+4*3+MAXNSIZE+1,
	Nsumbeg		= 1+3,
	Nmax		= Ncreate,

	Nsum		= 2*(1+3),
	Nfree		= 3,

	HBITS		= 5,
	HSIZE		= 1 << HBITS,
	HMASK		= HSIZE - 1,
	NOTIME		= 0xFFFFFFFF,

	WRSIZE		= 4*1024,
};

typedef	struct	Extent	Extent;
typedef	struct	Exts	Exts;
typedef	struct	Entry	Entry;
typedef	struct	Dirr	Dirr;
typedef struct	Jrec	Jrec;
typedef struct	Renum	Renum;

struct Extent
{
	int	sect;
	ulong	addr;
	ulong	off;
	ulong	size;
	Extent*	next;
	Extent*	prev;
};

struct Exts
{
	Extent*	head;
	Extent*	tail;
};

struct Entry
{
	int	ref;
	char*	name;
	ulong	fnum;
	ulong	mode;
	ulong	mnum;
	ulong	mtime;
	ulong	size;

	union
	{
		struct
		{
			Entry**	htab;
			Entry*	files;
			Dirr*	readers;
		};
		struct
		{
			Exts	gen[2];
		};
	};

	Entry*	parent;
	Entry*	hnext;
	Entry*	hprev;
	Entry*	fnext;
	Entry*	fprev;
};

struct Dirr
{
	Entry*	dir;
	Entry*	cur;
	Dirr*	next;
	Dirr*	prev;
	ulong	offset;
};

struct Jrec
{
	int	type;
	ulong	mode;
	ulong	fnum;
	ulong	mnum;
	ulong	tnum;
	ulong	mtime;
	ulong	parent;
	ulong	size;
	ulong	offset;
	ulong	seq;
	char	name[MAXNSIZE+2];
};

struct Renum
{
	int	old;
	int	new;
};

extern	void	initdata(char*, int);
extern	void	clearsect(int);
extern	void	readdata(int, void*, ulong, ulong);
extern	int	writedata(int, int, void*, ulong, ulong);
extern	int	getc3(uchar*, ulong*);
extern	int	putc3(uchar*, ulong);
extern	ulong	get4(uchar*);
extern	void	put4(uchar*, ulong);
extern	int	convM2J(Jrec*, uchar*);
extern	int	convJ2M(Jrec*, uchar*);
extern	void	loadfs(int);
extern	char*	need(int bytes);
extern	void	put(Jrec*, int);
extern	void	putw(Jrec*, int, Extent *x, void*);
extern	int	Jconv(Fmt*);
extern	ulong	now(void);
extern	void	serve(char*);

#pragma	varargck	type	"J"	Jrec*

extern	void	einit(void);
extern	void	edump(void);
extern	Entry*	elookup(ulong);
extern	Extent*	esum(Entry*, int, ulong, int*);
extern	void	edestroy(Entry*);
extern	Entry*	ecreate(Entry*, char*, ulong, ulong, ulong, char**);
extern	char*	eremove(Entry*);
extern	Entry*	ewalk(Entry*, char*, char**);
extern	void	etrunc(Entry*, ulong, ulong);
extern	ulong	echmod(Entry*, ulong, ulong);
extern	ulong	eread(Entry*, int, void*, ulong, ulong);
extern	void	ewrite(Entry*, Extent *, int, ulong);
extern	Qid	eqid(Entry*);
extern	void	estat(Entry*, Dir*, int);
extern	Dirr*	ediropen(Entry*);
extern	int	edirread(Dirr*, char*, long);
extern	void	edirclose(Dirr*);
extern	void	erenum(Renum*);

extern	char	Edirnotempty[];
extern	char	Eexists[];
extern	char	Eisdir[];
extern	char	Enonexist[];
extern	char	Enotdir[];
extern	char	Eperm[];
extern	char	Erofs[];

extern	ulong	sectsize;
extern	ulong	nsects;
extern	uchar*	sectbuff;
extern	Entry	*root;
extern	int	readonly;
extern	ulong	delta;
extern	int	eparity;
extern	uchar	magic[];
extern	ulong	used;
extern	ulong	limit;
extern	ulong	maxwrite;
