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
	u32	addr;
	u32	off;
	u32	size;
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
	u32	fnum;
	u32	mode;
	u32	mnum;
	u32	mtime;
	u32	size;

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
	u32	offset;
};

struct Jrec
{
	int	type;
	u32	mode;
	u32	fnum;
	u32	mnum;
	u32	tnum;
	u32	mtime;
	u32	parent;
	u32	size;
	u32	offset;
	u32	seq;
	char	name[MAXNSIZE+2];
};

struct Renum
{
	int	old;
	int	new;
};

extern char *prog;

extern	void	initdata(char*, int);
extern	void	clearsect(int);
extern	void	readdata(int, void*, u32, u32);
extern	int	writedata(int, int, void*, u32, u32);
extern	int	getc3(unsigned char*, u32*);
extern	int	putc3(unsigned char*, u32);
extern	u32	get4(unsigned char*);
extern	void	put4(unsigned char*, u32);
extern	int	convM2J(Jrec*, unsigned char*);
extern	int	convJ2M(Jrec*, unsigned char*);
extern	void	loadfs(int);
extern	char*	need(int bytes);
extern	void	put(Jrec*, int);
extern	void	putw(Jrec*, int, Extent *x, void*);
extern	int	Jconv(Fmt*);
extern	u32	now(void);
extern	void	serve(char*);


extern	void	einit(void);
extern	void	edump(void);
extern	Entry*	elookup(u32);
extern	Extent*	esum(Entry*, int, u32, int*);
extern	void	edestroy(Entry*);
extern	Entry*	ecreate(Entry*, char*, u32, u32,
				    u32, char**);
extern	char*	eremove(Entry*);
extern	Entry*	ewalk(Entry*, char*, char**);
extern	void	etrunc(Entry*, u32, u32);
extern	u32	echmod(Entry*, u32, u32);
extern	u32	eread(Entry*, int, void*, u32, u32);
extern	void	ewrite(Entry*, Extent *, int, u32);
extern	Qid	eqid(Entry*);
extern	void	estat(Entry*, Dir*, int);
extern	Dirr*	ediropen(Entry*);
extern	int	edirread(Dirr*, char*, i32);
extern	void	edirclose(Dirr*);
extern	void	erenum(Renum*);

extern	char	Edirnotempty[];
extern	char	Eexists[];
extern	char	Eisdir[];
extern	char	Enonexist[];
extern	char	Enotdir[];
extern	char	Eperm[];
extern	char	Erofs[];

extern	u32	sectsize;
extern	u32	nsects;
extern	unsigned char*	sectbuff;
extern	Entry	*root;
extern	int	readonly;
extern	u32	delta;
extern	int	eparity;
extern	unsigned char	magic[];
extern	u32	used;
extern	u32	limit;
extern	u32	maxwrite;
