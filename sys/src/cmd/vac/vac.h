typedef struct VacFS VacFS;
typedef struct VacDir VacDir;
typedef struct VacFile VacFile;
typedef struct VacDirEnum VacDirEnum;

/*
 * Mode bits
 */
enum {
	ModeOtherExec = (1<<0),		
	ModeOtherWrite = (1<<1),
	ModeOtherRead = (1<<2),
	ModeGroupExec = (1<<3),
	ModeGroupWrite = (1<<4),
	ModeGroupRead = (1<<5),
	ModeOwnerExec = (1<<6),
	ModeOwnerWrite = (1<<7),
	ModeOwnerRead = (1<<8),
	ModeSticky = (1<<9),
	ModeSetUid = (1<<10),
	ModeSetGid = (1<<11),
	ModeAppend = (1<<12),		/* append only file */
	ModeExclusive = (1<<13),	/* lock file - plan 9 */
	ModeLink = (1<<14),		/* sym link */
	ModeDir	= (1<<15),		/* duplicate of DirEntry */
	ModeHidden = (1<<16),		/* MS-DOS */
	ModeSystem = (1<<17),		/* MS-DOS */
	ModeArchive = (1<<18),		/* MS-DOS */
	ModeTemporary = (1<<19),	/* MS-DOS */
	ModeSnapshot = (1<<20),		/* read only snapshot */
};

enum {
	MetaMagic = 0x5656fc79,
	MetaHeaderSize = 12,
	MetaIndexSize = 4,
	IndexEntrySize = 8,
	DirMagic = 0x1c4d9072,
};

enum {
	DirPlan9Entry = 1,
	DirNTEntry,
	DirQidSpaceEntry,
	DirGenEntry,
};

struct VacDir {
	int version;		/* vac version */
	char *elem;		/* path element */
	ulong entry;		/* entry in directory */
	ulong gen;		/* generation of dir entry */
	uvlong size;		/* size of file */
	uvlong qid;		/* unique file id */
	
	char *uid;		/* owner id */
	char *gid;		/* group id */
	char *mid;		/* last modified by */
	ulong mtime;		/* last modified time */
	ulong mcount;		/* number of modifications: change qid if it wraps */
	ulong ctime;		/* directory entry last changed */
	ulong atime;		/* last time accessed */
	ulong mode;		/* various mode bits */

	/* plan 9 */
	int plan9;
	uvlong p9path;
	ulong p9version;

	/* NTTime */
	int ntTime;
	uvlong createTime;
	uvlong modifyTime;
	uvlong accessTime;

	/* sub space of qid */
	int qidSpace;
	uvlong qidOffset;	/* qid offset */
	uvlong qidMax;		/* qid maximum */
};

VacFS *vacFSOpen(VtSession *z, char *file, int readOnly, long ncache);
VacFS *vacFSCreate(VtSession *z, int bsize, long ncache);
int vacFSGetBlockSize(VacFS*);
long vacFSGetCacheSize(VacFS*);
int vacFSSetCacheSize(VacFS*, long);
int vacFSSnapshot(VacFS*, char *src, char *dst);
int vacFSSync(VacFS*);
int vacFSClose(VacFS*);
int vacFSGetScore(VacFS*, uchar score[VtScoreSize]);

/* 
 * other ideas
 *
 * VacFS *vacFSSnapshot(VacFS*, char *src);
 * int vacFSGraft(VacFS*, char *name, VacFS*);
 */

VacFile *vacFileOpen(VacFS*, char *path);
VacFile *vacFileCreate(VacFS*, char *path, int perm, char *user);
VacFile *vacFileWalk(VacFile *vf, char *elem);
int vacFileRemove(VacFile*);
int vacFileRead(VacFile*, void *, int n, vlong offset);
int vacFileWrite(VacFile*, void *, int n, vlong offset, char *user);
int vacFileReadPacket(VacFile*, Packet**, vlong offset);
int vacFileWritePacket(VacFile*, Packet*, vlong offset, char *user);
uvlong vacFileGetId(VacFile*);
int vacFileIsDir(VacFile*);
int vacFileGetMode(VacFile*, ulong *mode);
int vacFileGetBlockScore(VacFile*, ulong bn, uchar score[VtScoreSize]);
int vacFileGetSize(VacFile*, uvlong *size);
int vacFileGetDir(VacFile*, VacDir*);
int vacFileSetDir(VacFile*, VacDir*);
int vacFileGetVtDirEntry(VacFile*, VtDirEntry*);
int vacFileSync(VacFile*);
VacFile *vacFileIncRef(VacFile*);
void vacFileDecRef(VacFile*);
VacDirEnum *vacFileDirEnum(VacFile*);

void	vacDirCleanup(VacDir *dir);
void	vacDirCopy(VacDir *dst, VacDir *src);

VacDirEnum *vacDirEnumOpen(VacFS*, char *path);
int vacDirEnumRead(VacDirEnum*, VacDir *, int n);
void vacDirEnumFree(VacDirEnum*);

