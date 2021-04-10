/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

typedef struct Fs Fs;
typedef struct File File;
typedef struct DirEntryEnum DirEntryEnum;

//#pragma incomplete Fs
//#pragma incomplete File
//#pragma incomplete DirEntryEnum

/* modes */

enum {
	OReadOnly,
	OReadWrite,
	OOverWrite,
};

extern char *currfsysname;
extern char *foptname;

void	fsClose(Fs*);
int	fsEpochLow(Fs*, u32);
File	*fsGetRoot(Fs*);
int	fsHalt(Fs*);
Fs	*fsOpen(char*, VtSession*, i32, int);
int	fsRedial(Fs*, char*);
void	fsSnapshotCleanup(Fs*, u32);
int	fsSnapshot(Fs*, char*, char*, int);
void	fsSnapshotRemove(Fs*);
int	fsSync(Fs*);
int	fsUnhalt(Fs*);
int	fsVac(Fs*, char*, unsigned char[VtScoreSize]);

void	deeClose(DirEntryEnum*);
DirEntryEnum *deeOpen(File*);
int	deeRead(DirEntryEnum*, DirEntry*);
int	fileClri(File*, char*, char*);
int	fileClriPath(Fs*, char*, char*);
File	*fileCreate(File*, char*, u32, char*);
int	fileDecRef(File*);
int	fileGetDir(File*, DirEntry*);
u64	fileGetId(File*);
u32	fileGetMcount(File*);
u32	fileGetMode(File*);
File	*fileGetParent(File*);
int	fileGetSize(File*, u64*);
File	*fileIncRef(File*);
int	fileIsDir(File*);
int	fileIsTemporary(File*);
int	fileIsAppend(File*);
int	fileIsExclusive(File*);
int	fileIsRoFs(File*);
int	fileIsRoot(File*);
int	fileMapBlock(File*, u32, unsigned char[VtScoreSize], u32);
int	fileMetaFlush(File*, int);
char	*fileName(File *f);
File	*fileOpen(Fs*, char*);
int	fileRead(File*, void *, int, i64);
int	fileRemove(File*, char*);
int	fileSetDir(File*, DirEntry*, char*);
int	fileSetQidSpace(File*, u64, u64);
int	fileSetSize(File*, u64);
int	fileSync(File*);
int	fileTruncate(File*, char*);
File	*fileWalk(File*, char*);
File	*_fileWalk(File*, char*, int);
int	fileWalkSources(File*);
int	fileWrite(File*, void *, int, i64, char*);
