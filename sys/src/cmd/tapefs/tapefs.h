#define	g2byte(x)	(((x)[1]<<8) + (x)[0])		/* little-endian */
#define	g3byte(x)	(((x)[2]<<16) + ((x)[1]<<8) + (x)[0])
#define	g4byte(x)	(((x)[3]<<24) + ((x)[2]<<16) + ((x)[1]<<8) + (x)[0])

enum
{
	OPERM	= 0x3,		/* mask of all permission types in open mode */
	Nram	= 512,
};

typedef struct Fid Fid;
typedef struct Ram Ram;

struct Fid
{
	short	busy;
	short	open;
	short	rclose;
	int	fid;
	Fid	*next;
	char	*user;
	Ram	*ram;
};

struct Ram
{
	char	busy;
	char	open;
	char	replete;
	Ram	*parent;	/* parent directory */
	Ram	*child;		/* first member of directory */
	Ram	*next;		/* next member of file's directory */
	Qid	qid;
	long	perm;
	char	name[NAMELEN];
	ulong	atime;
	ulong	mtime;
	char	*user;
	char	*group;
	void	*data;
	long	ndata;
};

enum
{
	Pexec =		1,
	Pwrite = 	2,
	Pread = 	4,
	Pother = 	1,
	Pgroup = 	8,
	Powner =	64,
};

typedef struct idmap {
	char	*name;
	int	id;
} Idmap;

typedef struct fileinf {
	char	*name;
	void	*addr;
	long	size;
	int	mode;
	int	uid;
	int	gid;
	long	mdate;
} Fileinf;

extern	ulong	path;		/* incremented for each new file */
extern	Ram	*ram;
extern	char	user[];
extern	Idmap	*uidmap;
extern	Idmap	*gidmap;
extern	int	replete;
void	error(char*);
void	*erealloc(void*, ulong);
void	*emalloc(ulong);
void	populate(char *);
void	dotrunc(Ram*);
void	docreate(Ram*);
char	*doread(Ram*, long, long);
void	dowrite(Ram*, char*, long, long);
int	dopermw(Ram*);
Idmap	*getpass(char*);
char	*mapid(Idmap*,int);
Ram	*poppath(Fileinf fi, int new);
Ram	*popfile(Ram *dir, Fileinf fi);
void	popdir(Ram*);
Ram	*lookup(Ram*, char*);
