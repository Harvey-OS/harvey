/* mostly plan 9 compatibility */

#undef _BSD_EXTENSION		/* avoid multiple def'n if predefined */
#undef _PLAN9_SOURCE
#undef _POSIX_SOURCE
#undef _RESEARCH_SOURCE
#undef _SUSV2_SOURCE

#define _BSD_EXTENSION
#define _PLAN9_SOURCE
#define _POSIX_SOURCE
#define _RESEARCH_SOURCE
#define _SUSV2_SOURCE

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <inttypes.h>

#ifndef NSIG
#define NSIG 32
#endif

#define RFPROC 1
#define RFFDG 1
#define RFNOTEG 1

#define OREAD	O_RDONLY
#define OWRITE	O_WRONLY
#define ORDWR	O_RDWR
#define OCEXEC	0

#define ERRMAX 128

#define uintptr uintptr_t
#define nil ((void*)0)

#define assert(cond)
#define seek lseek
#define print printf
#define fprint fprintf
#define snprint snprintf

char *strdup(const char *);
void unixclsexec(int);

/* in case uchar, etc. are built-in types */
#define uchar	_fmtuchar
#define ushort	_fmtushort
#define uint	_fmtuint
#define ulong	_fmtulong
#define vlong	_fmtvlong
#define uvlong	_fmtuvlong

typedef unsigned char		uchar;
typedef unsigned short		ushort;
typedef unsigned int		uint;
typedef unsigned long		ulong;
typedef unsigned long long	uvlong;

typedef ulong Rune;

enum
{
	UTFmax		= 4,		/* maximum bytes per rune */
	Runesync	= 0x80,		/* cannot represent part of a UTF sequence (<) */
	Runeself	= 0x80,		/* rune and UTF sequences are the same (<) */
	Runeerror	= 0xFFFD,	/* decoding error in UTF */
	Runemax		= 0x10FFFF,	/* 21-bit rune */
	Runemask	= 0x1FFFFF,	/* bits used by runes (see grep) */
};

/*
 * rune routines
 */
extern	int	runetochar(char*, Rune*);
extern	int	chartorune(Rune*, char*);
extern	int	runelen(long);
extern	int	runenlen(Rune*, int);
extern	int	fullrune(char*, int);
extern	int	utflen(char*);
extern	int	utfnlen(char*, long);
extern	char*	utfrune(char*, long);
extern	char*	utfrrune(char*, long);
extern	char*	utfutf(char*, char*);
extern	char*	utfecpy(char*, char*, char*);

extern char *argv0;
#define	ARGBEGIN	for((argv0||(argv0=*argv)),argv++,argc--;\
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
#define	ARGEND		SET(_argt);USED(_argt,_argc,_args);}USED(argv, argc);
#define	ARGF()		(_argt=_args, _args="",\
				(*_argt? _argt: argv[1]? (argc--, *++argv): 0))
#define	EARGF(x)	(_argt=_args, _args="",\
				(*_argt? _argt: argv[1]? (argc--, *++argv): ((x), abort(), (char*)0)))

#define	ARGC()		_argc

#define	nelem(x)	(sizeof(x)/sizeof((x)[0]))
