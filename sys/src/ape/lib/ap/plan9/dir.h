#define DIRLEN 116

typedef	long		vlong;
typedef union
{
	char	clength[8];
	vlong	vlength;
	struct
	{
		long	hlength;
		long	length;
	};
} Length;

typedef
struct Qid
{
	unsigned long	path;
	unsigned long	vers;
} Qid;

typedef
struct Dir
{
	char	name[NAMELEN];
	char	uid[NAMELEN];
	char	gid[NAMELEN];
	Qid	qid;
	unsigned long	mode;
	long	atime;
	long	mtime;
	Length;
	short	type;
	short	dev;
} Dir;

void	_dirtostat(struct stat *, char *);
int	convM2D(char*, Dir*);
int	convD2M(Dir*, char*);
