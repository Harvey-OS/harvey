/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

Block*	sourceBlock(Source*, uint32_t, int);
Block*	_sourceBlock(Source*, uint32_t, int, int, uint32_t);
void	sourceClose(Source*);
Source*	sourceCreate(Source*, int, int, uint32_t);
uint32_t	sourceGetDirSize(Source*);
int	sourceGetEntry(Source*, Entry*);
uint64_t	sourceGetSize(Source*);
int	sourceLock2(Source*, Source*, int);
int	sourceLock(Source*, int);
char	*sourceName(Source *s);
Source*	sourceOpen(Source*, uint32_t, int, int);
int	sourceRemove(Source*);
Source*	sourceRoot(Fs*, uint32_t, int);
int	sourceSetDirSize(Source*, uint32_t);
int	sourceSetEntry(Source*, Entry*);
int	sourceSetSize(Source*, uint64_t);
int	sourceTruncate(Source*);
void	sourceUnlock(Source*);

Block*	cacheAllocBlock(Cache*, int, uint32_t, uint32_t, uint32_t);
Cache*	cacheAlloc(Disk*, VtSession*, uint32_t, int);
void	cacheCountUsed(Cache*, uint32_t, uint32_t*, uint32_t*, uint32_t*);
int	cacheDirty(Cache*);
void	cacheFlush(Cache*, int);
void	cacheFree(Cache*);
Block*	cacheGlobal(Cache*, unsigned char[VtScoreSize], int, uint32_t, int);
Block*	cacheLocal(Cache*, int, uint32_t, int);
Block*	cacheLocalData(Cache*, uint32_t, int, uint32_t, int, uint32_t);
uint32_t	cacheLocalSize(Cache*, int);
int	readLabel(Cache*, Label*, uint32_t addr);

Block*	blockCopy(Block*, uint32_t, uint32_t, uint32_t);
void	blockDependency(Block*, Block*, int, unsigned char*, Entry*);
int	blockDirty(Block*);
void	blockDupLock(Block*);
void	blockPut(Block*);
void	blockRemoveLink(Block*, uint32_t, int, uint32_t, int);
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
int	diskReadRaw(Disk*, int, uint32_t, unsigned char*);
uint32_t	diskSize(Disk*, int);
void	diskWriteAndWait(Disk*,	Block*);
void	diskWrite(Disk*, Block*);
int	diskWriteRaw(Disk*, int, uint32_t, unsigned char*);

char*	bioStr(int);
char*	bsStr(int);
char*	btStr(int);
uint32_t	globalToLocal(unsigned char[VtScoreSize]);
void	localToGlobal(uint32_t, unsigned char[VtScoreSize]);

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
int	fileSnapshot(File*, File*, uint32_t, int);
int	fsNextQid(Fs*, uint64_t*);
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
int	nextWalk(WalkPtr*, unsigned char[VtScoreSize], unsigned char*, uint32_t*, Entry**);

void	snapGetTimes(Snap*, uint32_t*, uint32_t*, uint32_t*);
void	snapSetTimes(Snap*, uint32_t, uint32_t, uint32_t);

void	fsCheck(Fsck*);

