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

#pragma incomplete Fs
#pragma incomplete File
#pragma incomplete DirEntryEnum

/* modes */

enum { OReadOnly,
       OReadWrite,
       OOverWrite,
};

extern char* currfsysname;
extern char* foptname;

void fsClose(Fs*);
int fsEpochLow(Fs*, uint32_t);
File* fsGetRoot(Fs*);
int fsHalt(Fs*);
Fs* fsOpen(char*, VtSession*, int32_t, int);
int fsRedial(Fs*, char*);
void fsSnapshotCleanup(Fs*, uint32_t);
int fsSnapshot(Fs*, char*, char*, int);
void fsSnapshotRemove(Fs*);
int fsSync(Fs*);
int fsUnhalt(Fs*);
int fsVac(Fs*, char*, unsigned char[VtScoreSize]);

void deeClose(DirEntryEnum*);
DirEntryEnum* deeOpen(File*);
int deeRead(DirEntryEnum*, DirEntry*);
int fileClri(File*, char*, char*);
int fileClriPath(Fs*, char*, char*);
File* fileCreate(File*, char*, uint32_t, char*);
int fileDecRef(File*);
int fileGetDir(File*, DirEntry*);
uint64_t fileGetId(File*);
uint32_t fileGetMcount(File*);
uint32_t fileGetMode(File*);
File* fileGetParent(File*);
int fileGetSize(File*, uint64_t*);
File* fileIncRef(File*);
int fileIsDir(File*);
int fileIsTemporary(File*);
int fileIsAppend(File*);
int fileIsExclusive(File*);
int fileIsRoFs(File*);
int fileIsRoot(File*);
int fileMapBlock(File*, uint32_t, unsigned char[VtScoreSize], uint32_t);
int fileMetaFlush(File*, int);
char* fileName(File* f);
File* fileOpen(Fs*, char*);
int fileRead(File*, void*, int, int64_t);
int fileRemove(File*, char*);
int fileSetDir(File*, DirEntry*, char*);
int fileSetQidSpace(File*, uint64_t, uint64_t);
int fileSetSize(File*, uint64_t);
int fileSync(File*);
int fileTruncate(File*, char*);
File* fileWalk(File*, char*);
File* _fileWalk(File*, char*, int);
int fileWalkSources(File*);
int fileWrite(File*, void*, int, int64_t, char*);
