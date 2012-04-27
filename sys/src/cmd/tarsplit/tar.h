/* tar archive format definitions and functions */

#define islink(lf)	(isreallink(lf) || issymlink(lf))
#define isreallink(lf)	((lf) == Lflink)
#define issymlink(lf)	((lf) == Lfsymlink1 || (lf) == Lfsymlink2)

#define HOWMANY(a, size)	(((a) + (size) - 1) / (size))
#define ROUNDUP(a, size)	(HOWMANY(a, size) * (size))

#define TAPEBLKS(bytes)		HOWMANY(bytes, Tblock)

enum {
	Tblock = 512u,
	Namesz = 100,

	/* link flags */
	Lfplain1 = '\0',
	Lfplain2 = '0',
	Lflink,
	Lfsymlink1,
	Lfchr,
	Lfblk,
	Lfdir,
	Lffifo,
	Lfcontig,
	Lfsymlink2 = 's',
};

typedef struct {
	char	name[Namesz];
	char	mode[8];
	char	uid[8];
	char	gid[8];
	char	size[12];
	char	mtime[12];
	char	chksum[8];
	char	linkflag;
	char	linkname[Namesz];
} Header;

typedef union {
	uchar	dummy[Tblock];
	Header;
} Hblock;

/* tarsub.c */
char *thisnm, *lastnm;

unsigned checksum(Hblock *hp);
int	closeout(int outf, char *, int prflag);
int	getdir(Hblock *, int in, vlong *);
ulong	otoi(char *s);
void	newarch(void);
uvlong	passtar(Hblock *hp, int in, int outf, vlong bytes);
void	putempty(int out);
void	readtar(int in, char *buffer, long size);
uvlong	writetar(int outf, char *buffer, ulong size);
