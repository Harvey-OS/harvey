/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#define _LOCK_EXTENSION
#define _QLOCK_EXTENSION
#define _BSD_EXTENSION
#include <sys/types.h>
#include <lock.h>
#include <qlock.h>
#include <lib9.h>
#include <stdlib.h>
#include <string.h>
#include <bsd.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <utf.h>
#include <fmt.h>
#include <signal.h>
// #include <time.h>

#define nelem(x) (sizeof(x) / sizeof((x)[0]))

extern int tokenize(char*, char**, int);

typedef struct Qid {
	uvlong path;
	uint32_t vers;
	uchar type;
} Qid;

typedef struct Dir {
	/* system-modified data */
	ushort type; /* server type */
	uint dev;    /* server subtype */
	/* file data */
	Qid qid;        /* unique id from server */
	uint32_t mode;  /* permissions */
	uint32_t atime; /* last read time */
	uint32_t mtime; /* last write time */
	vlong length;   /* file length: see <u.h> */
	char* name;     /* last element of path */
	char* uid;      /* owner name */
	char* gid;      /* group name */
	char* muid;     /* last modifier name */
} Dir;

uint _convM2D(uchar*, uint, Dir*, char*);
uint _convD2M(Dir*, uchar*, uint);
Dir* _dirstat(char*);
int _dirwstat(char*, Dir*);
Dir* _dirfstat(int);
int _dirfwstat(int, Dir*);
long _dirread(int, Dir**);
long _dirreadall(int, Dir**);
void _nulldir(Dir*);
uint _sizeD2M(Dir*);

typedef struct Waitmsg {
	int pid;               /* of loved one */
	unsigned long time[3]; /* of loved one & descendants */
	char* msg;
} Waitmsg;

/*
 * Time-of-day
 */

typedef struct Tm {
	int sec;
	int min;
	int hour;
	int mday;
	int mon;
	int year;
	int wday;
	int yday;
	char zone[4];
	int tzoff;
} Tm;

extern Tm* gmtime(long);
extern Tm* localtime(long);
extern char* asctime(Tm*);
extern char* ctime(long);
extern double cputime(void);
extern long times(long*);
extern long tm2sec(Tm*);
extern vlong nsec(void);

extern void cycles(uvlong*); /* 64-bit value of the cycle counter if there is
                                one, 0 if there isn't */

extern long time(long*);

extern int _AWAIT(char*, int);
extern int _ALARM(unsigned long);
extern int _BIND(const char*, const char*, int);
extern int _CHDIR(const char*);
extern int _CLOSE(int);
extern int _CREATE(char*, int, unsigned long);
extern int _DUP(int, int);
extern int _ERRSTR(char*, unsigned int);
extern int _EXEC(char*, char* []);
extern void _EXITS(char*);
extern int _FD2PATH(int, char*, int);
extern int _FAUTH(int, char*);
extern int _FSESSION(int, char*, int);
extern int _FSTAT(int, unsigned char*, int);
extern int _FWSTAT(int, unsigned char*, int);
extern int _MOUNT(int, int, const char*, int, const char*);
extern int _NOTED(int);
extern int _NOTIFY(int (*)(void*, char*));
extern int _OPEN(const char*, int);
extern int _PIPE(int*);
extern long _PREAD(int, void*, long, long long);
extern long _PWRITE(int, void*, long, long long);
extern long _READ(int, void*, long);
extern int _REMOVE(const char*);
extern int _RENDEZVOUS(unsigned long, unsigned long);
extern int _RFORK(int);
extern int _SEGATTACH(int, char*, void*, unsigned long);
extern int _SEGBRK(void*, void*);
extern int _SEGDETACH(void*);
extern int _SEGFLUSH(void*, unsigned long);
extern int _SEGFREE(void*, unsigned long);
extern long long _SEEK(int, long long, int);
extern int _SLEEP(long);
extern int _STAT(const char*, unsigned char*, int);
extern Waitmsg* _WAIT(void);
extern long _WRITE(int, const void*, long);
extern int _WSTAT(const char*, unsigned char*, int);
extern void* _MALLOCZ(int, int);
extern int _WERRSTR(char*, ...);
extern long _READN(int, void*, long);
extern int _IOUNIT(int);

#define dirstat _dirstat
#define dirfstat _dirfstat

#define OREAD 0
#define OWRITE 1
#define ORDWR 2
#define OCEXEC 32

#define AREAD 4
#define AWRITE 2
#define AEXEC 1
#define AEXIST 0

#define open _OPEN
#define close _CLOSE
#define read _READ
#define write _WRITE
#define _exits(s) _exit(s&&*(char*)s ? 1 : 0)
#define exits(s) exit(s&&*(char*)s ? 1 : 0)
#define create _CREATE
#define pread _PREAD
#define readn _READN
#define mallocz _MALLOCZ
#define iounit _IOUNIT

/* assume being called as in event.c */
#define postnote(x, pid, msg) kill(pid, SIGTERM)
#define atnotify(x, y) signal(SIGTERM, ekill)

#define ERRMAX 128

extern void setmalloctag(void*, uint32_t);
extern uint32_t getcallerpc(void*);

/* Used in libsec.h and not picked up in earlier type definitions */
typedef unsigned int u32int;
typedef unsigned long long u64int;

int dec16(uchar*, int, char*, int);
int enc16(char*, int, uchar*, int);
int dec32(uchar*, int, char*, int);
int enc32(char*, int, uchar*, int);
int dec64(uchar*, int, char*, int);
int enc64(char*, int, uchar*, int);

extern vlong nsec(void);

extern void sysfatal(char*, ...);

extern uint32_t truerand(void); /* uses /dev/random */
extern int getfields(char*, char**, int, int, char*);

#pragma varargck type "lld" vlong
#pragma varargck type "llo" vlong
#pragma varargck type "llx" vlong
#pragma varargck type "llb" vlong
#pragma varargck type "lld" uvlong
#pragma varargck type "llo" uvlong
#pragma varargck type "llx" uvlong
#pragma varargck type "llb" uvlong
#pragma varargck type "ld" long
#pragma varargck type "lo" long
#pragma varargck type "lx" long
#pragma varargck type "lb" long
#pragma varargck type "ld" ulong
#pragma varargck type "lo" ulong
#pragma varargck type "lx" ulong
#pragma varargck type "lb" ulong
#pragma varargck type "d" int
#pragma varargck type "o" int
#pragma varargck type "x" int
#pragma varargck type "c" int
#pragma varargck type "C" int
#pragma varargck type "b" int
#pragma varargck type "d" uint
#pragma varargck type "x" uint
#pragma varargck type "c" uint
#pragma varargck type "C" uint
#pragma varargck type "b" uint
#pragma varargck type "f" double
#pragma varargck type "e" double
#pragma varargck type "g" double
#pragma varargck type "s" char *
#pragma varargck type "q" char *
#pragma varargck type "S" Rune *
#pragma varargck type "Q" Rune *
#pragma varargck type "r" void
#pragma varargck type "%" void
#pragma varargck type "n" int *
#pragma varargck type "p" ulong /* uintptr */
#pragma varargck type "p" void *
#pragma varargck flag ','
#pragma varargck flag ' '
#pragma varargck flag 'h'
#pragma varargck type "<" void *
#pragma varargck type "[" void *
#pragma varargck type "H" void *
#pragma varargck type "lH" void *
