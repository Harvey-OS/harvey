/* magic to get SUSV2 standard, including pread, pwrite*/
#define _XOPEN_SOURCE 500
/* magic to get 64-bit pread/pwrite */
#define _LARGEFILE64_SOURCE
/* magic to get 64-bit stat on Linux, maybe others */
#define _FILE_OFFSET_BITS 64

#ifdef sgi
#define _BSD_TYPES	1	/* for struct timeval */
#include <sys/select.h>
#define _BSD_SOURCE	1	/* for ruserok */
/*
 * SGI IRIX 5.x doesn't allow inclusion of both inttypes.h and
 * sys/types.h.  These definitions are the ones we need from
 * inttypes.h that aren't in sys/types.h.
 *
 * Unlike most of our #ifdef's, IRIX5X must be set in the makefile.
 */
#ifdef IRIX5X
#define __inttypes_INCLUDED
typedef unsigned int            uint32_t;
typedef signed long long int    int64_t;
typedef unsigned long long int  uint64_t;
#endif /* IRIX5X */
#endif /* sgi */


#ifdef sun	/* sparc and __svr4__ are also defined on the offending machine */
#define __EXTENSIONS__	1	/* for struct timeval */
#endif

#include <inttypes.h>		/* for int64_t et al. */
#include <stdarg.h>		/* for va_list, vararg macros */
#ifndef va_copy
#ifdef __va_copy
#define va_copy	__va_copy
#else
#define va_copy(d, s)	memmove(&(d), &(s), sizeof(va_list))
#endif /* __va_copy */
#endif /* va_copy */
#include <sys/types.h>
#include <string.h>		/* for memmove */
#include <unistd.h>		/* for write */

#define ulong p9ulong		/* because sys/types.h has some of these sometimes */
#define ushort p9ushort
#define uchar p9uchar
#define uint p9uint
#define vlong p9vlong
#define uvlong p9uvlong
#define u32int p9u32int

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned long ulong;
typedef unsigned int uint;
typedef int64_t vlong;
typedef uint64_t uvlong;
typedef uint32_t u32int;
typedef uint64_t u64int;
typedef ushort Rune;

#define nil ((void*)0)
#define	nelem(x)	(sizeof(x)/sizeof((x)[0]))
#define	offsetof(s, m)	(ulong)(&(((s*)0)->m))
#define	assert(x)	if(x);else _assert("x")

extern char *argv0;
#define	ARGBEGIN	for((void)(argv0||(argv0=*argv)),argv++,argc--;\
			    argv[0] && argv[0][0]=='-' && argv[0][1];\
			    argc--, argv++) {\
				char *_args, *_argt;\
				Rune _argc;\
				_args = &argv[0][1];\
				if(_args[0]=='-' && _args[1]==0){\
					argc--; argv++; break;\
				}\
				_argc = 0;\
				while(*_args && (_args += chartorune(&_argc, _args)))\
				switch(_argc)
#define	ARGEND		SET(_argt);USED(_argt);USED(_argc);USED(_args);}\
					USED(argv);USED(argc);
#define	ARGF()		(_argt=_args, _args="",\
				(*_argt? _argt: argv[1]? (argc--, *++argv): 0))
#define	EARGF(x)		(_argt=_args, _args="",\
				(*_argt? _argt: argv[1]? (argc--, *++argv): ((x), abort(), (char*)0)))

#define	ARGC()		_argc

#define	SET(x)	(x) = 0
#define	USED(x)	(void)(x)

enum
{
	UTFmax		= 3,		/* maximum bytes per rune */
	Runesync	= 0x80,		/* cannot represent part of a UTF sequence (<) */
	Runeself	= 0x80,		/* rune and UTF sequences are the same (<) */
	Runeerror	= 0x80		/* decoding error in UTF */
};

extern	int	runetochar(char*, Rune*);
extern	int	chartorune(Rune*, char*);
extern	int	runelen(long);
extern	int	utflen(char*);
extern	char*	strecpy(char*, char*, char*);
extern	int	tokenize(char*, char**, int);
extern	int	getfields(char*, char**, int, int, char*);

/*
 * print routines
 */
typedef	struct	Fconv	Fconv;
struct	Fconv
{
	char*	out;		/* pointer to next output */
	char*	eout;		/* pointer to end */
	int	f1;
	int	f2;
	int	f3;
	int	chr;
};
extern	char*	doprint(char*, char*, char*, va_list *argp);
extern	int	print(char*, ...);
extern	char*	seprint(char*, char*, char*, ...);
extern	int	snprint(char*, int, char*, ...);
extern	int	sprint(char*, char*, ...);
extern	int	fprint(int, char*, ...);

extern	int	fmtinstall(int, int (*)(va_list*, Fconv*));
extern	int	numbconv(va_list*, Fconv*);
extern	void	strconv(char*, Fconv*);
extern	int	fltconv(va_list*, Fconv*);

#define	OREAD	0	/* open for read */
#define	OWRITE	1	/* write */
#define	ORDWR	2	/* read and write */
#define	OEXEC	3	/* execute, == read but check execute permission */
#define	OTRUNC	16	/* or'ed in (except for exec), truncate file first */
#define	OCEXEC	32	/* or'ed in, close on exec */
#define	ORCLOSE	64	/* or'ed in, remove on close */
#define	OEXCL	0x1000	/* or'ed in, exclusive use */

/* bits in Qid.type */
#define QTDIR		0x80		/* type bit for directories */
#define QTAPPEND	0x40		/* type bit for append only files */
#define QTEXCL		0x20		/* type bit for exclusive use files */
#define QTMOUNT		0x10		/* type bit for mounted channel */
#define QTAUTH		0x08
#define QTFILE		0x00		/* plain file */

/* bits in Dir.mode */
#define DMDIR		0x80000000	/* mode bit for directories */
#define DMAPPEND	0x40000000	/* mode bit for append only files */
#define DMEXCL		0x20000000	/* mode bit for exclusive use files */
#define DMMOUNT		0x10000000	/* mode bit for mounted channel */
#define DMREAD		0x4		/* mode bit for read permission */
#define DMWRITE		0x2		/* mode bit for write permission */
#define DMEXEC		0x1		/* mode bit for execute permission */

typedef
struct Qid
{
	vlong	path;
	ulong	vers;
	uchar	type;
} Qid;

typedef
struct Dir {
	/* system-modified data */
	ushort	type;	/* server type */
	uint	dev;	/* server subtype */
	/* file data */
	Qid	qid;	/* unique id from server */
	ulong	mode;	/* permissions */
	ulong	atime;	/* last read time */
	ulong	mtime;	/* last write time */
	vlong	length;	/* file length: see <u.h> */
	char	*name;	/* last element of path */
	char	*uid;	/* owner name */
	char	*gid;	/* group name */
	char	*muid;	/* last modifier name */
} Dir;

long readn(int, void*, long);
void remotehost(char*, int);

enum {
	NAMELEN = 28,
	ERRLEN = 64
};

/* DES */
#define DESKEYLEN 7
void	key_setup(char key[DESKEYLEN], char expandedkey[128]);
void	block_cipher(char expandedkey[128], char buf[8], int decrypting);
