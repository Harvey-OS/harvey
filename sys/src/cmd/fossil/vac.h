/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

typedef struct DirEntry DirEntry;
typedef struct MetaBlock MetaBlock;
typedef struct MetaEntry MetaEntry;

enum { MetaMagic = 0x5656fc7a,
       MetaHeaderSize = 12,
       MetaIndexSize = 4,
       IndexEntrySize = 8,
       DirMagic = 0x1c4d9072,
};

/*
 * Mode bits
 */
enum { ModeOtherExec = (1 << 0),
       ModeOtherWrite = (1 << 1),
       ModeOtherRead = (1 << 2),
       ModeGroupExec = (1 << 3),
       ModeGroupWrite = (1 << 4),
       ModeGroupRead = (1 << 5),
       ModeOwnerExec = (1 << 6),
       ModeOwnerWrite = (1 << 7),
       ModeOwnerRead = (1 << 8),
       ModeSticky = (1 << 9),
       ModeSetUid = (1 << 10),
       ModeSetGid = (1 << 11),
       ModeAppend = (1 << 12),    /* append only file */
       ModeExclusive = (1 << 13), /* lock file - plan 9 */
       ModeLink = (1 << 14),      /* sym link */
       ModeDir = (1 << 15),       /* duplicate of DirEntry */
       ModeHidden = (1 << 16),    /* MS-DOS */
       ModeSystem = (1 << 17),    /* MS-DOS */
       ModeArchive = (1 << 18),   /* MS-DOS */
       ModeTemporary = (1 << 19), /* MS-DOS */
       ModeSnapshot = (1 << 20),  /* read only snapshot */
};

/* optional directory entry fields */
enum { DePlan9 = 1, /* not valid in version >= 9 */
       DeNT,        /* not valid in version >= 9 */
       DeQidSpace,
       DeGen, /* not valid in version >= 9 */
};

struct DirEntry {
	char* elem;      /* path element */
	uint32_t entry;  /* entry in directory for data */
	uint32_t gen;    /* generation of data entry */
	uint32_t mentry; /* entry in directory for meta */
	uint32_t mgen;   /* generation of meta entry */
	uint64_t size;   /* size of file */
	uint64_t qid;    /* unique file id */

	char* uid;       /* owner id */
	char* gid;       /* group id */
	char* mid;       /* last modified by */
	uint32_t mtime;  /* last modified time */
	uint32_t mcount; /* number of modifications: can wrap! */
	uint32_t ctime;  /* directory entry last changed */
	uint32_t atime;  /* last time accessed */
	uint32_t mode;   /* various mode bits */

	/* plan 9 */
	int plan9;
	uint64_t p9path;
	uint32_t p9version;

	/* sub space of qid */
	int qidSpace;
	uint64_t qidOffset; /* qid offset */
	uint64_t qidMax;    /* qid maximum */
};

struct MetaEntry {
	unsigned char* p;
	uint8_t size;
};

struct MetaBlock {
	int maxsize;  /* size of block */
	int size;     /* size used */
	int free;     /* free space within used size */
	int maxindex; /* entries allocated for table */
	int nindex;   /* amount of table used */
	int botch;    /* compensate for my stupidity */
	unsigned char* buf;
};

void deCleanup(DirEntry*);
void deCopy(DirEntry*, DirEntry*);
int deSize(DirEntry*);
void dePack(DirEntry*, MetaEntry*);
int deUnpack(DirEntry*, MetaEntry*);

void mbInit(MetaBlock*, unsigned char*, int, int);
int mbUnpack(MetaBlock*, unsigned char*, int);
void mbInsert(MetaBlock*, int, MetaEntry*);
void mbDelete(MetaBlock*, int);
void mbPack(MetaBlock*);
unsigned char* mbAlloc(MetaBlock*, int);
int mbResize(MetaBlock*, MetaEntry*, int);
int mbSearch(MetaBlock*, char*, int*, MetaEntry*);

void meUnpack(MetaEntry*, MetaBlock*, int);
