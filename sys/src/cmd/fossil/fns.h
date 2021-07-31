Source* sourceRoot(Fs*, u32int, int);
Source* sourceOpen(Source*, ulong, int);
Source* sourceCreate(Source*, int, int, u32int);
Block* sourceBlock(Source*, ulong, int);
int sourceGetEntry(Source*, Entry*);
int sourceSetSize(Source*, uvlong);
uvlong sourceGetSize(Source*);
int sourceSetDirSize(Source*, ulong);
ulong sourceGetDirSize(Source*);
int sourceTruncate(Source*);
int sourceRemove(Source*);
void sourceClose(Source*);
int sourceLock(Source*, int);
void sourceUnlock(Source*);
int sourceLock2(Source*, Source*, int);

Cache* cacheAlloc(Disk*, VtSession*, ulong, int);
void cacheFree(Cache*);
Block* cacheLocal(Cache*, int, u32int, int);
Block* cacheLocalData(Cache*, u32int, int, u32int, int, u32int);
Block* cacheGlobal(Cache*, uchar[VtScoreSize], int, u32int, int);
Block* cacheAllocBlock(Cache*, int, u32int, u32int, u32int);
void cacheFlush(Cache*, int);
u32int cacheLocalSize(Cache*, int);

Block* blockCopy(Block*, u32int, u32int, u32int);
void blockDupLock(Block*);
void blockPut(Block*);
void blockDependency(Block*, Block*, int, uchar*, Entry*);
int blockDirty(Block*);
int blockRemoveLink(Block*, u32int, int, u32int);
int blockSetLabel(Block*, Label*);
Block* _blockSetLabel(Block*, Label*);
void blockSetIOState(Block*, int);
int blockWrite(Block*);
uchar* blockRollback(Block*, uchar*);

Disk* diskAlloc(int);
void diskFree(Disk*);
int diskReadRaw(Disk*, int, u32int, uchar*);
int diskWriteRaw(Disk*, int, u32int, uchar*);
void diskRead(Disk*, Block*);
void diskWrite(Disk*, Block*);
int diskFlush(Disk*);
u32int diskSize(Disk*, int);
int diskBlockSize(Disk*);

char* bsStr(int);
char* bioStr(int);
char* btStr(int);
u32int globalToLocal(uchar[VtScoreSize]);
void localToGlobal(u32int, uchar[VtScoreSize]);

int headerUnpack(Header*, uchar*);
void headerPack(Header*, uchar*);

int labelFmt(Fmt*);
int labelUnpack(Label*, uchar*, int);
void labelPack(Label*, uchar*, int);

int scoreFmt(Fmt*);

int superUnpack(Super*, uchar*);
void superPack(Super*, uchar*);

int entryUnpack(Entry*, uchar*, int);
void entryPack(Entry*, uchar*, int);
int entryType(Entry*);

Periodic* periodicAlloc(void (*)(void*), void*, int);
void periodicKill(Periodic*);

File* fileRoot(Source*);
int fileSnapshot(File*, File*, u32int, int);
int fileGetSources(File*, Entry*, Entry*, int);
int mkVac(VtSession*, uint, Entry*, Entry*, DirEntry*, uchar[VtScoreSize]);
int fsNextQid(Fs*, u64int*);
Block* superGet(Cache*, Super*);
void superPut(Block*, Super*, int);

Arch* archInit(Cache*, Disk*, Fs*, VtSession*);
void archFree(Arch*);
void archKick(Arch*);

void bwatchLock(Block*);
void bwatchUnlock(Block*);
void bwatchInit(void);
void bwatchSetBlockSize(uint);
void bwatchDependency(Block*);
void bwatchReset(uchar[VtScoreSize]);

void initWalk(WalkPtr*, Block*, uint);
int nextWalk(WalkPtr*, uchar[VtScoreSize], uchar*, u32int*, Entry**);

void snapGetTimes(Snap*, u32int*, u32int*);
void snapSetTimes(Snap*, u32int, u32int);

#pragma varargck type "L" Label*
