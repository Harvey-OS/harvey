typedef struct Fs Fs;
typedef struct File File;
typedef struct DirEntryEnum DirEntryEnum;

#pragma incomplete Fs
#pragma incomplete File
#pragma incomplete DirEntryEnum

/* modes */

enum {
	OReadOnly,
	OReadWrite,
	OOverWrite,
};

Fs *fsOpen(char*, VtSession*, long, int);
void fsClose(Fs*);
File *fsGetRoot(Fs*);
int fsSnapshot(Fs*, char*, char*, int);
void fsSnapshotRemove(Fs*);
void fsSnapshotCleanup(Fs*, u32int);
int fsSync(Fs*);
int fsHalt(Fs*);
int fsUnhalt(Fs*);
int fsVac(Fs*, char*, uchar[VtScoreSize]);
int fsRedial(Fs*, char*);
int fsEpochLow(Fs*, u32int);

File *fileOpen(Fs*, char*);
File *fileCreate(File*, char*, ulong, char*);
File *fileWalk(File*, char*);
File *_fileWalk(File*, char*, int);
int fileRemove(File*, char*);
int fileClri(File*, char*, char*);
int fileClriPath(Fs*, char*, char*);
int fileRead(File*, void *, int, vlong);
int fileWrite(File*, void *, int, vlong, char*);
int fileMapBlock(File*, ulong, uchar[VtScoreSize], ulong);
uvlong fileGetId(File*);
ulong fileGetMcount(File*);
int fileIsDir(File*);
int fileGetSize(File*, uvlong*);
int fileGetDir(File*, DirEntry*);
int fileSetDir(File*, DirEntry*, char*);
int fileSetSize(File*, uvlong);
File *fileGetParent(File*);
int fileSync(File*);
File *fileIncRef(File*);
int fileDecRef(File*);
int fileIsRoot(File*);
int fileMetaFlush(File*, int);
int fileSetQidSpace(File*, u64int, u64int);
int fileTruncate(File*, char*);
int fileIsRoFs(File*);
ulong fileGetMode(File*);
DirEntryEnum *deeOpen(File*);
int deeRead(DirEntryEnum*, DirEntry*);
void deeClose(DirEntryEnum*);
int fileWalkSources(File*);
