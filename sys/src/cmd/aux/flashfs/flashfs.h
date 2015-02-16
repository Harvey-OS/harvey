/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

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
	uint32_t	addr;
	uint32_t	off;
	uint32_t	size;
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
	uint32_t	fnum;
	uint32_t	mode;
	uint32_t	mnum;
	uint32_t	mtime;
	uint32_t	size;

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
	uint32_t	offset;
};

struct Jrec
{
	int	type;
	uint32_t	mode;
	uint32_t	fnum;
	uint32_t	mnum;
	uint32_t	tnum;
	uint32_t	mtime;
	uint32_t	parent;
	uint32_t	size;
	uint32_t	offset;
	uint32_t	seq;
	char	name[MAXNSIZE+2];
};

struct Renum
{
	int	old;
	int	new;
};

extern	void	initdata(char*, int);
extern	void	clearsect(int);
extern	void	readdata(int, void*, uint32_t, uint32_t);
extern	int	writedata(int, int, void*, uint32_t, uint32_t);
extern	int	getc3(uchar*, uint32_t*);
extern	int	putc3(uchar*, uint32_t);
extern	uint32_t	get4(uchar*);
extern	void	put4(uchar*, uint32_t);
extern	int	convM2J(Jrec*, uchar*);
extern	int	convJ2M(Jrec*, uchar*);
extern	void	loadfs(int);
extern	char*	need(int bytes);
extern	void	put(Jrec*, int);
extern	void	putw(Jrec*, int, Extent *x, void*);
extern	int	Jconv(Fmt*);
extern	uint32_t	now(void);
extern	void	serve(char*);

#pragma	varargck	type	"J"	Jrec*

extern	void	einit(void);
extern	void	edump(void);
extern	Entry*	elookup(uint32_t);
extern	Extent*	esum(Entry*, int, uint32_t, int*);
extern	void	edestroy(Entry*);
extern	Entry*	ecreate(Entry*, char*, uint32_t, uint32_t,
				    uint32_t, char**);
extern	char*	eremove(Entry*);
extern	Entry*	ewalk(Entry*, char*, char**);
extern	void	etrunc(Entry*, uint32_t, uint32_t);
extern	uint32_t	echmod(Entry*, uint32_t, uint32_t);
extern	uint32_t	eread(Entry*, int, void*, uint32_t, uint32_t);
extern	void	ewrite(Entry*, Extent *, int, uint32_t);
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

extern	uint32_t	sectsize;
extern	uint32_t	nsects;
extern	uchar*	sectbuff;
extern	Entry	*root;
extern	int	readonly;
extern	uint32_t	delta;
extern	int	eparity;
extern	uchar	magic[];
extern	uint32_t	used;
extern	uint32_t	limit;
extern	uint32_t	maxwrite;
