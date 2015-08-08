/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * statfs.h - definitions for statistic gathering file server
 */

#define DEBUGFILE "iostats.out"
#define DONESTR "done"
#define DEBUG                                                                  \
	if(!dbg) {                                                             \
	} else                                                                 \
	fprint
#define MAXPROC 16
#define FHASHSIZE 64
#define fidhash(s) fhash[s % FHASHSIZE]

enum { Maxfdata = 8192, /* max size of data in 9P message */
       Maxrpc = 20000,  /* number of RPCs we'll log */
};

typedef struct Fsrpc Fsrpc;
typedef struct Fid Fid;
typedef struct File File;
typedef struct Proc Proc;
typedef struct Stats Stats;
typedef struct Rpc Rpc;
typedef struct Frec Frec;

struct Frec {
	Frec* next;
	char* op;
	uint32_t nread;
	uint32_t nwrite;
	uint32_t bread;
	uint32_t bwrite;
	uint32_t opens;
};

struct Rpc {
	char* name;
	uint32_t count;
	vlong time;
	vlong lo;
	vlong hi;
	uint32_t bin;
	uint32_t bout;
};

struct Stats {
	uint32_t totread;
	uint32_t totwrite;
	uint32_t nrpc;
	uint32_t nproto;
	Rpc rpc[Maxrpc];
};

struct Fsrpc {
	int busy;     /* Work buffer has pending rpc to service */
	uintptr pid;  /* Pid of slave process executing the rpc */
	int canint;   /* Interrupt gate */
	int flushtag; /* Tag on which to reply to flush */
	Fcall work;   /* Plan 9 incoming Fcall */
	uchar buf[IOHDRSZ + Maxfdata]; /* Data buffer */
};

struct Fid {
	int fid; /* system fd for i/o */
	File* f; /* File attached to this fid */
	int mode;
	int nr;    /* fid number */
	Fid* next; /* hash link */
	uint32_t nread;
	uint32_t nwrite;
	uint32_t bread;
	uint32_t bwrite;
	vlong offset; /* for directories */
};

struct File {
	char* name;
	Qid qid;
	int inval;
	File* parent;
	File* child;
	File* childlist;
};

struct Proc {
	uintptr pid;
	int busy;
	Proc* next;
};

enum { Nr_workbufs = 40,
       Dsegpad = 8192,
       Fidchunk = 1000,
};

Extern Fsrpc* Workq;
Extern int dbg;
Extern File* root;
Extern Fid** fhash;
Extern Fid* fidfree;
Extern int qid;
Extern Proc* Proclist;
Extern int done;
Extern Stats* stats;
Extern Frec* frhead;
Extern Frec* frtail;
Extern int myiounit;

/* File system protocol service procedures */
void Xcreate(Fsrpc*), Xclunk(Fsrpc*);
void Xversion(Fsrpc*), Xauth(Fsrpc*), Xflush(Fsrpc*);
void Xattach(Fsrpc*), Xwalk(Fsrpc*), Xauth(Fsrpc*);
void Xremove(Fsrpc*), Xstat(Fsrpc*), Xwstat(Fsrpc*);
void slave(Fsrpc*);

void reply(Fcall*, Fcall*, char*);
Fid* getfid(int);
int freefid(int);
Fid* newfid(int);
Fsrpc* getsbuf(void);
void initroot(void);
void fatal(char*);
void makepath(char*, File*, char*);
File* file(File*, char*);
void slaveopen(Fsrpc*);
void slaveread(Fsrpc*);
void slavewrite(Fsrpc*);
void blockingslave(void);
void reopen(Fid* f);
void noteproc(int, char*);
void flushaction(void*, char*);
void catcher(void*, char*);
uint32_t msec(void);
void fidreport(Fid*);
