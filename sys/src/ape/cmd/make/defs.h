/* defs 4.2 85/10/28 */
#define _POSIX_SOURCE
#define _RESEARCH_SOURCE

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <dirent.h>
#include <limits.h>
#include <stdio.h>
#include <ctype.h>

#ifndef SHELLCOM
#define SHELLCOM "/bin/sh"
#endif

typedef char flag;	/* represent a few bit flag */

#define NO	0
#define YES	1

#define equal(a,b)	(! strcmp(a,b))
#define HASHSIZE	1021
#define NLEFTS	512
#define NCHARS	500
#define NINTS	250
#define INMAX	20000
#define OUTMAX	20000
#define QBUFMAX	20000
#define MAXDIR	10
#define MAXPROC	100
#define MAXINCLUDE	17
#define PROCLIMIT	3

#define ALLDEPS	1
#define SOMEDEPS	2

#define META	01
#define TERMINAL	02
extern char funny[128];


#define ALLOC(x) (struct x *) ckalloc(sizeof(struct x))
#define CHNULL	(char *) NULL

extern int sigivalue;
extern int sigqvalue;
extern int dbgflag;
extern int prtrflag;
extern int silflag;
extern int noexflag;
extern int keepgoing;
extern int noruleflag;
extern int touchflag;
extern int questflag;
extern int oldflag;
extern int ndocoms;
extern int ignerr;
extern int okdel;
extern int forceshell;
extern int inarglist;
extern char **envpp;	/* points to slot in environment vector */
extern char *prompt;
extern int nopdir;

typedef struct nameblock *nameblkp;
typedef struct depblock *depblkp;
typedef struct lineblock *lineblkp;
typedef struct chain *chainp;

struct nameblock
	{
	nameblkp nxtnameblock;
	char *namep;
	lineblkp linep;
	flag done;
	flag septype;
	flag isarch;
	flag isdir;
	time_t modtime;
	};

extern nameblkp mainname;
extern nameblkp firstname;
extern nameblkp *hashtab;
extern int nhashed;
extern int hashsize;
extern int hashthresh;

struct lineblock
	{
	lineblkp nxtlineblock;
	struct depblock *depp;
	struct shblock *shp;
	};
extern lineblkp sufflist;

struct depblock
	{
	depblkp nxtdepblock;
	nameblkp depname;
	char nowait;
	} ;

struct shblock
	{
	struct shblock *nxtshblock;
	char *shbp;
	};

struct varblock
	{
	struct varblock *nxtvarblock;
	char *varname;
	char *varval;
	char **export;
	flag noreset;
	flag used;
	};
extern struct varblock *firstvar;

struct pattern
	{
	struct pattern *nxtpattern;
	char *patval;
	};
extern struct pattern *firstpat;

struct dirhd
	{
	struct dirhd *nxtdirhd;
	time_t dirtime;
	int dirok;
	DIR * dirfc;
	char *dirn;
	};
extern struct dirhd *firstod;


struct chain
	{
	chainp nextp;
	char *datap;
	};

struct wild
	{
	struct wild *next;
	lineblkp linep;
	char *left;
	char *right;
	int llen;
	int rlen;
	int totlen;
	};

typedef struct wild *wildp;
extern wildp firstwild;
extern wildp lastwild;


/* date for processes */
extern int proclimit;	/* maximum spawned processes allowed alive at one time */
extern int proclive;	/* number of spawned processes awaited */
extern int nproc;	/* next slot in process stack to use */
extern struct process
	{
	int pid;
	flag nohalt;
	flag nowait;
	flag done;
	} procstack[ ];

extern void	intrupt(int);
extern void	enbint(void (*)(int));
extern int	doname(nameblkp, int, time_t *, int);
extern int	docom(struct shblock *, int, int);
extern int	dosys(char *, int, int, char *);
extern int	waitstack(int);
extern void	touch(int, char*);
extern time_t	exists(char *);
extern time_t	prestime(void);
extern depblkp	srchdir(char*, int, depblkp);
extern time_t	lookarch(char *);
extern void	dirsrch(char *);
extern void	baddirs(void);
extern nameblkp	srchname(char *);
extern nameblkp	makename(char *);
extern int	hasparen(char *);
extern void	newhash(int);
extern nameblkp	chkname(char *);
extern char	*copys(char *);
extern char	*concat(char *, char *, char *);
extern int	suffix(char *, char *, char *);
extern int	*ckalloc(int);
extern char	*subst(char *, char *);
extern void	setvar(char *, char *, int);
extern void	set3var(char *, char *);
extern int	eqsign(char *);
extern struct varblock *varptr(char *);
extern int	dynmacro(char *);
extern void	fatal1(char *, char *);
extern void	fatal(char *);
extern chainp	appendq(chainp, char *);
extern char	*mkqlist(chainp, char *);
extern wildp	iswild(char *);
extern char	*wildmatch(wildp, char *, int);
extern char	*wildsub(char *, char *);
extern int	parse(char *);
extern int	yylex(void);
