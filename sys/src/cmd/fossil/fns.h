/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

Block*	sourceBlock(Source*, u32, int);
Block*	_sourceBlock(Source*, u32, int, int, u32);
void	sourceClose(Source*);
Source*	sourceCreate(Source*, int, int, u32);
u32	sourceGetDirSize(Source*);
int	sourceGetEntry(Source*, Entry*);
u64	sourceGetSize(Source*);
int	sourceLock2(Source*, Source*, int);
int	sourceLock(Source*, int);
char	*sourceName(Source *s);
Source*	sourceOpen(Source*, u32, int, int);
int	sourceRemove(Source*);
Source*	sourceRoot(Fs*, u32, int);
int	sourceSetDirSize(Source*, u32);
int	sourceSetEntry(Source*, Entry*);
int	sourceSetSize(Source*, u64);
int	sourceTruncate(Source*);
void	sourceUnlock(Source*);

Block*	cacheAllocBlock(Cache*, int, u32, u32, u32);
Cache*	cacheAlloc(Disk*, VtSession*, u32, int);
void	cacheCountUsed(Cache*, u32, u32*, u32*, u32*);
int	cacheDirty(Cache*);
void	cacheFlush(Cache*, int);
void	cacheFree(Cache*);
Block*	cacheGlobal(Cache*, unsigned char[VtScoreSize], int, u32, int);
Block*	cacheLocal(Cache*, int, u32, int);
Block*	cacheLocalData(Cache*, u32, int, u32, int, u32);
u32	cacheLocalSize(Cache*, int);
int	readLabel(Cache*, Label*, u32 addr);

Block*	blockCopy(Block*, u32, u32, u32);
void	blockDependency(Block*, Block*, int, unsigned char*, Entry*);
int	blockDirty(Block*);
void	blockDupLock(Block*);
void	blockPut(Block*);
void	blockRemoveLink(Block*, u32, int, u32, int);
unsigned char*	blockRollback(Block*, unsigned char*);
void	blockSetIOState(Block*, int);
Block*	_blockSetLabel(Block*, Label*);
int	blockSetLabel(Block*, Label*, int);
int	blockWrite(Block*, int);

Disk*	diskAlloc(int);
int	diskBlockSize(Disk*);
int	diskFlush(Disk*);
void	diskFree(Disk*);
void	diskRead(Disk*, Block*);
int	diskReadRaw(Disk*, int, u32, unsigned char*);
u32	diskSize(Disk*, int);
void	diskWriteAndWait(Disk*,	Block*);
void	diskWrite(Disk*, Block*);
int	diskWriteRaw(Disk*, int, u32, unsigned char*);

char*	bioStr(int);
char*	bsStr(int);
char*	btStr(int);
u32	globalToLocal(unsigned char[VtScoreSize]);
void	localToGlobal(u32, unsigned char[VtScoreSize]);

void	headerPack(Header*, unsigned char*);
int	headerUnpack(Header*, unsigned char*);

int	labelFmt(Fmt*);
void	labelPack(Label*, unsigned char*, int);
int	labelUnpack(Label*, unsigned char*, int);

int	scoreFmt(Fmt*);

void	superPack(Super*, unsigned char*);
int	superUnpack(Super*, unsigned char*);

void	entryPack(Entry*, unsigned char*, int);
int	entryType(Entry*);
int	entryUnpack(Entry*, unsigned char*, int);

Periodic* periodicAlloc(void (*)(void*), void*, int);
void	periodicKill(Periodic*);

int	fileGetSources(File*, Entry*, Entry*);
File*	fileRoot(Source*);
int	fileSnapshot(File*, File*, u32, int);
int	fsNextQid(Fs*, u64*);
int	mkVac(VtSession*, uint, Entry*, Entry*, DirEntry*, unsigned char[VtScoreSize]);
Block*	superGet(Cache*, Super*);

void	archFree(Arch*);
Arch*	archInit(Cache*, Disk*, Fs*, VtSession*);
void	archKick(Arch*);

void	bwatchDependency(Block*);
void	bwatchInit(void);
void	bwatchLock(Block*);
void	bwatchReset(unsigned char[VtScoreSize]);
void	bwatchSetBlockSize(uint);
void	bwatchUnlock(Block*);

void	initWalk(WalkPtr*, Block*, uint);
int	nextWalk(WalkPtr*, unsigned char[VtScoreSize], unsigned char*, u32*, Entry**);

void	snapGetTimes(Snap*, u32*, u32*, u32*);
void	snapSetTimes(Snap*, u32, u32, u32);

void	fsCheck(Fsck*);
