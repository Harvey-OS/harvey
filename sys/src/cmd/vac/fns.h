Source	*sourceAlloc(Cache*, Lump *u, int elem, int readonly);
Source 	*sourceOpen(Source*, ulong entry, int readOnly);
Source 	*sourceCreate(Source*, int psize, int dsize, int isdir);
Lump	*sourceGetLump(Source*, ulong block, int readOnly, int lock);
Lump	*sourceWalk(Source *r, ulong block, int readOnly, int *);
int	sourceSetDepth(Source *r, uvlong size);
int	sourceSetSize(Source *r, uvlong size);
uvlong	sourceGetSize(Source *r);
int	sourceSetDirSize(Source *r, ulong size);
ulong	sourceGetDirSize(Source *r);
void	sourceRemove(Source*);
void	sourceFree(Source*);
int	sourceGetVtDirEntry(Source *r, VtDirEntry *dir);

Lump	*lumpWalk(Lump *u, int offset, int type, int size, int readOnly, int lock);
int	lumpGetScore(Lump *u, int offset, uchar score[VtScoreSize]);
void	lumpDecRef(Lump*, int unlock);
Lump	*lumpIncRef(Lump*);
void	lumpFreeEntry(Lump *u, int entry);

Cache 	*cacheAlloc(VtSession *z, int blockSize, long nblocks);
Lump 	*cacheAllocLump(Cache *c, int type, int size, int dir);
void	cacheFree(Cache *c);
long	cacheGetSize(Cache*);
int	cacheSetSize(Cache*, long);
int	cacheGetBlockSize(Cache *c);
Lump 	*cacheGetLump(Cache *c, uchar score[VtScoreSize], int type, int size);
void	cacheCheck(Cache*);

VacFile *vacFileRoot(VacFS *fs, uchar *score);

