#include <u.h>
#include <libc.h>
#include "errors.h"

/*
 * BLOCKSIZE is relatively small to keep memory consumption down.
 */

#define	BLOCKSIZE	2048
#define	RUNESIZE	sizeof(Rune)
#define	NDISC		5
#define	NBUFFILES	3+2*NDISC	/* plan 9+undo+snarf+NDISC*(transcript+buf) */
#define NSUBEXP	10

#define	TRUE		1
#define	FALSE		0

#define	INFINITY	0x7FFFFFFFL
#define	INCR		25
#define	STRSIZE		(2*BLOCKSIZE)

typedef long		Posn;		/* file position or address */
typedef	ushort		Mod;		/* modification number */

typedef struct Address	Address;
typedef struct Block	Block;
typedef struct Buffer	Buffer;
typedef struct Disc	Disc;
typedef struct Discdesc	Discdesc;
typedef struct File	File;
typedef struct List	List;
typedef struct Mark	Mark;
typedef struct Range	Range;
typedef struct Rangeset	Rangeset;
typedef struct String	String;

enum State
{
	Clean =		' ',
	Dirty =		'\'',
	Unread =	'-',
	Readerr =	'~',
};

struct Range
{
	Posn	p1, p2;
};

struct Rangeset
{
	Range	p[NSUBEXP];
};

struct Address
{
	Range	r;
	File	*f;
};

struct List	/* code depends on a long being able to hold a pointer */
{
	int	nalloc;
	int	nused;
	union{
		void	*listp;
		Block	*blkp;
		long	*longp;
		uchar*	*ucharp;
		String*	*stringp;
		File*	*filep;
		long	listv;
	}g;
};

#define	listptr		g.listp
#define	blkptr		g.blkp
#define	longptr		g.longp
#define	ucharpptr	g.ucharp
#define	stringpptr	g.stringp
#define	filepptr	g.filep
#define	listval		g.listv

/*
 * Block must fit in a long because the list routines manage arrays of
 * blocks.  Two problems: some machines (e.g. Cray) can't pull this off
 * -- on them, use bitfields -- and the ushort bnum limits temp file sizes
 * to about 200 megabytes.  Advantages: small, simple code and small
 * memory overhead.  If you really want to edit huge files, making BLOCKSIZE
 * bigger is the easiest way.
 */
struct Block
{
	ushort	bnum;		/* absolute number on disk */
	short	nrunes;		/* runes stored in this block */
};

struct Discdesc
{
	int	fd;		/* plan 9 file descriptor of temp file */
	ulong	nbk;		/* high water mark */
	List	free;		/* array of free block indices */
};

struct Disc
{
	Discdesc *desc;		/* descriptor of temp file */
	Posn	nrunes;		/* runes on disc file */
	List	block;		/* list of used block indices */
};

struct String
{
	short	n;
	short	size;
	Rune	*s;
};

struct Buffer
{
	Disc	*disc;		/* disc storage */
	Posn	nrunes;		/* total length of buffer */
	String	cache;		/* in-core storage for efficiency */
	Posn	c1, c2;		/* cache start and end positions in disc */
				/* note: if dirty, cache is really c1, c1+cache.n */
	int	dirty;		/* cache dirty */
};

#define	NGETC	128

struct File
{
	Buffer	*buf;		/* cached disc storage */
	Buffer	*transcript;	/* what's been done */
	Posn	markp;		/* file pointer to start of latest change */
	Mod	mod;		/* modification stamp */
	Posn	nrunes;		/* total length of file */
	Posn	hiposn;		/* highest address touched this Mod */
	Address	dot;		/* current position */
	Address	ndot;		/* new current position after update */
	Range	tdot;		/* what terminal thinks is current range */
	Range	mark;		/* tagged spot in text (don't confuse with Mark) */
	List	*rasp;		/* map of what terminal's got */
	String	name;		/* file name */
	short	tag;		/* for communicating with terminal */
	char	state;		/* Clean, Dirty, Unread, or Readerr*/
	char	closeok;	/* ok to close file? */
	char	deleted;	/* delete at completion of command */
	char	marked;		/* file has been Fmarked at least once; once
				 * set, this will never go off as undo doesn't
				 * revert to the dawn of time */
	long	dev;		/* file system from which it was read */
	long	qid;		/* file from which it was read */
	long	date;		/* time stamp of plan9 file */
	Posn	cp1, cp2;	/* Write-behind cache positions and */
	String	cache;		/* string */
	Rune	getcbuf[NGETC];
	int	ngetc;
	int	getci;
	Posn	getcp;
};

struct Mark
{
	Posn	p;
	Range	dot;
	Range	mark;
	Mod	m;
	short	s1;
};

/*
 * The precedent to any message in the transcript.
 * The component structures must be an integral number of Runes long.
 */
union Hdr
{
	struct _csl
	{
		short	c;
		short	s;
		long	l;
	}csl;
	struct _cs
	{
		short	c;
		short	s;
	}cs;
	struct _cll
	{
		short	c;
		long	l;
		long	l1;
	}cll;
	Mark	mark;
};

#define	Fgetc(f)  ((--(f)->ngetc<0)? Fgetcload(f, (f)->getcp) : (f)->getcbuf[(f)->getcp++, (f)->getci++])
#define	Fbgetc(f) (((f)->getci<=0)? Fbgetcload(f, (f)->getcp) : (f)->getcbuf[--(f)->getcp, --(f)->getci])

int	alnum(int);
void	Bclean(Buffer*);
void	Bterm(Buffer*);
void	Bdelete(Buffer*, Posn, Posn);
void	Bflush(Buffer*);
void	Binsert(Buffer*, String*, Posn);
Buffer	*Bopen(Discdesc*);
int	Bread(Buffer*, Rune*, int, Posn);
void	Dclose(Disc*);
void	Ddelete(Disc*, Posn, Posn);
void	Dinsert(Disc*, Rune*, int, Posn);
Disc	*Dopen(Discdesc*);
int	Dread(Disc*, Rune*, int, Posn);
void	Dreplace(Disc*, Posn, Posn, Rune*, int);
int	Fbgetcload(File*, Posn);
int	Fbgetcset(File*, Posn);
long	Fchars(File*, Rune*, Posn, Posn);
void	Fclose(File*);
void	Fdelete(File*, Posn, Posn);
int	Fgetcload(File*, Posn);
int	Fgetcset(File*, Posn);
void	Finsert(File*, String*, Posn);
File	*Fopen(void);
void	Fsetname(File*, String*);
void	Fstart(void);
int	Fupdate(File*, int, int);
int	Read(int, void*, int);
void	Seek(int, long, int);
int	plan9(File*, int, String*, int);
int	Write(int, void*, int);
int	bexecute(File*, Posn);
void	cd(String*);
void	closefiles(File*, String*);
void	closeio(Posn);
void	cmdloop(void);
void	cmdupdate(void);
void	compile(String*);
void	copy(File*, Address);
File	*current(File*);
void	delete(File*);
void	delfile(File*);
void	dellist(List*, int);
void	doubleclick(File*, Posn);
void	dprint(char*, ...);
void	edit(File*, int);
void	*emalloc(ulong);
void	*erealloc(void*, ulong);
void	error(Err);
void	error_c(Err, int);
void	error_s(Err, char*);
int	execute(File*, Posn, Posn);
int	filematch(File*, String*);
void	filename(File*);
File	*getfile(String*);
int	getname(File*, String*, int);
long	getnum(void);
void	hiccough(char*);
void	inslist(List*, int, long);
Address	lineaddr(Posn, Address, int);
void	listfree(List*);
void	load(File*);
File	*lookfile(String*);
void	lookorigin(File*, Posn, Posn);
int	lookup(int);
void	move(File*, Address);
void	moveto(File*, Range);
File	*newfile(void);
void	nextmatch(File*, String*, Posn, int);
int	newtmp(int);
void	notifyf(void*, char*);
void	panic(char*);
void	printposn(File*, int);
void	print_ss(char*, String*, String*);
void	print_s(char*, String*);
int	rcv(void);
Range	rdata(List*, Posn, Posn);
Posn	readio(File*, int*, int);
void	rescue(void);
void	resetcmd(void);
void	resetsys(void);
void	resetxec(void);
void	rgrow(List*, Posn, Posn);
void	samerr(char*);
void	settempfile(void);
int	skipbl(void);
void	snarf(File*, Posn, Posn, Buffer*, int);
void	sortname(File*);
void	startup(char*, int, char**, char**);
void	state(File*, int);
int	statfd(int, ulong*, ulong*, long*, long*, long*);
int	statfile(char*, ulong*, ulong*, long*, long*, long*);
void	Straddc(String*, int);
void	Strclose(String*);
int	Strcmp(String*, String*);
void	Strdelete(String*, Posn, Posn);
void	Strdupl(String*, Rune*);
void	Strduplstr(String*, String*);
void	Strinit(String*);
void	Strinit0(String*);
void	Strinsert(String*, String*, Posn);
void	Strinsure(String*, ulong);
void	Strzero(String*);
int	Strlen(Rune*);
char	*Strtoc(String*);
void	syserror(char*);
void	telldot(File*);
void	tellpat(void);
String	*tmpcstr(char*);
String	*tmprstr(Rune*, int);
void	freetmpstr(String*);
void	termcommand(void);
void	termwrite(char*);
File	*tofile(String*);
void	toterminal(File*, int);
void	trytoclose(File*);
void	trytoquit(void);
int	undo(void);
void	update(void);
int	waitfor(int);
void	warn(Warn);
void	warn_s(Warn, char*);
void	warn_SS(Warn, String*, String*);
void	warn_S(Warn, String*);
int	whichmenu(File*);
void	writef(File*);
Posn	writeio(File*);
Discdesc *Dstart(void);

extern Rune	samname[];	/* compiler dependent */
extern Rune	*left[];
extern Rune	*right[];

extern char	RSAM[];		/* system dependent */
extern char	SAMTERM[];
extern char	HOME[];
extern char	TMPDIR[];
extern char	SH[];
extern char	SHPATH[];
extern char	RX[];
extern char	RXPATH[];
extern char	SAMSAVECMD[];

extern char	*rsamname;	/* globals */
extern char	*samterm;
extern Rune	genbuf[];
extern char	*genc;
extern int	io;
extern int	patset;
extern int	quitok;
extern Address	addr;
extern Buffer	*undobuf;
extern Buffer	*snarfbuf;
extern Buffer	*plan9buf;
extern List	file;
extern List	tempfile;
extern File	*cmd;
extern File	*curfile;
extern File	*lastfile;
extern Mod	modnum;
extern Posn	cmdpt;
extern Posn	cmdptadv;
extern Rangeset	sel;
extern String	cmdstr;
extern String	genstr;
extern String	lastpat;
extern String	lastregexp;
extern String	plan9cmd;
extern int	downloaded;
extern int	eof;
extern int	bpipeok;
extern int	panicking;
extern Rune	empty[];
extern int	termlocked;
extern int	noflush;

#include "mesg.h"

void	outTs(Hmesg, int);
void	outT0(Hmesg);
void	outTl(Hmesg, long);
void	outTslS(Hmesg, int, long, String*);
void	outTS(Hmesg, String*);
void	outTsS(Hmesg, int, String*);
void	outTsllS(Hmesg, int, long, long, String*);
void	outTsll(Hmesg, int, long, long);
void	outTsl(Hmesg, int, long);
void	outTsv(Hmesg, int, long);
void	outstart(Hmesg);
void	outcopy(int, void*);
void	outshort(int);
void	outlong(long);
void	outvlong(void*);
void	outsend(void);
void	outflush(void);
