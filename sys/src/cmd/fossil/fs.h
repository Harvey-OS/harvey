typedef struct Fs Fs;
typedef struct File File;
typedef struct DirEntryEnum DirEntryEnum;

/* modes */

enum {
	OReadOnly,
	OReadWrite,
	OOverWrite,
};

Fs *fsOpen(char*, VtSession*, long, int);
void fsClose(Fs*);
File *fsGetRoot(Fs*);
int fsSnapshot(Fs*, int);
int fsSync(Fs*);
int fsVac(Fs*, char*, uchar[VtScoreSize]);
int fsRedial(Fs*, char*);
int fsEpochLow(Fs*, u32int);

File *fileOpen(Fs*, char*);
File *fileCreate(File*, char*, ulong, char*);
File *fileWalk(File*, char*);
int fileRemove(File*, char*);
int fileClri(Fs*, char*, char*);
int fileRead(File*, void *, int, vlong);
int fileWrite(File*, void *, int, vlong, char*);
uvlong fileGetId(File*);
ulong fileGetMcount(File*);
int fileIsDir(File*);
int fileGetSize(File*, uvlong*);
int fileGetDir(File*, DirEntry*);
int fileSetDir(File*, DirEntry*, char*);
File *fileGetParent(File*);
int fileSync(File*);
File *fileIncRef(File*);
int fileDecRef(File*);
int fileIsRoot(File*);
void fileMetaFlush(File*, int);
int fileSetQidSpace(File*, u64int, u64int);
int fileTruncate(File*, char*);
int fileIsRoFs(File*);
ulong fileGetMode(File*);
DirEntryEnum *deeOpen(File*);
int deeRead(DirEntryEnum*, DirEntry*);
void deeClose(DirEntryEnum*);
int fileWalkSources(File*);
