/*
 * sorted by 4,/^$/|sort -bd +1
 */
int		addArena(Arena *name);
ZBlock		*allocZBlock(u32int size, int zeroed);
Arena		*amapItoA(Index *index, u64int a, u64int *aa);
u64int		arenaDirSize(Arena *arena, u32int clumps);
void		arenaUpdate(Arena *arena, u32int size, u8int *score);
void		backSumArena(Arena *arena);
u32int		buildBucket(Index *ix, IEStream *ies, IBucket *ib);
void		checkDCache(void);
void		checkLumpCache(void);
int		clumpInfoEq(ClumpInfo *c, ClumpInfo *d);
int		clumpInfoEq(ClumpInfo *c, ClumpInfo *d);
u32int		clumpMagic(Arena *arena, u64int aa);
int		delArena(Arena *arena);
void		*emalloc(ulong);
void		*erealloc(void *, ulong);
char		*estrdup(char*);
void		*ezmalloc(ulong);
void		fatal(char *fmt, ...);
Arena		*findArena(char *name);
ISect		*findISect(Index *ix, u32int buck);
int		flushCIBlocks(Arena *arena);
void		fmtZBInit(Fmt *f, ZBlock *b);
void		freeArena(Arena *arena);
void		freeArenaPart(ArenaPart *ap, int freeArenas);
void		freeIEStream(IEStream *ies);
void		freeIFile(IFile *f);
void		freeISect(ISect *is);
void		freeIndex(Index *index);
void		freePart(Part *part);
void		freeZBlock(ZBlock *b);
DBlock		*getDBlock(Part *part, u64int addr, int read);
u32int		hashBits(u8int *score, int nbits);
int		httpdInit(char *address);
int		iAddrEq(IAddr *ia1, IAddr *ia2);
int		ientryCmp(void *vie1, void *vie2);
char		*ifileLine(IFile *f);
int		ifileName(IFile *f, char *dst);
int		ifileU32Int(IFile *f, u32int *r);
int		indexSect(Index *ix, u8int *score);
Arena		*initArena(Part *part, u64int base, u64int size, u32int blockSize);
ArenaPart	*initArenaPart(Part *part);
int		initArenaSum(void);
void		initDCache(u32int mem);
void		initICache(int bits, int depth);
IEStream	*initIEStream(Part *part, u64int off, u64int clumps, u32int size);
ISect		*initISect(Part *part);
Index		*initIndex(char *name, ISect **sects, int n);
void		initLumpCache(u32int size, u32int nblocks);
int		initLumpQueues(int nq);
Part*		initPart(char *name, int writable);
int		initVenti(char *config);
void		insertLump(Lump *lump, Packet *p);
int		insertScore(u8int *score, IAddr *ia);
ZBlock		*loadClump(Arena *arena, u64int aa, int blocks, Clump *cl, u8int *score, int verify);
int		loadIEntry(Index *index, u8int *score, int type, IEntry *ie);
void		logErr(int severity, char *fmt, ...);
Lump		*lookupLump(u8int *score, int type);
int		lookupScore(u8int *score, int type, IAddr *ia);
int		mapArenas(AMap *am, Arena **arenas, int n, char *what);
void		nameCp(char *dst, char *src);
int		nameEq(char *s, char *t);
int		nameOk(char *name);
Arena		*newArena(Part *part, char *name, u64int base, u64int size, u32int blockSize);
ArenaPart	*newArenaPart(Part *part, u32int blockSize, u32int tabSize);
ISect		*newISect(Part *part, char *name, u32int blockSize, u32int tabSize);
Index		*newIndex(char *name, ISect **sects, int n);
u32int		now(void);
int		okAMap(AMap *am, int n, u64int start, u64int stop, char *what);
int		outputAMap(Fmt *f, AMap *am, int n);
int		outputIndex(Fmt *f, Index *ix);
int		packArena(Arena *arena, u8int *buf);
int		packArenaHead(ArenaHead *head, u8int *buf);
int		packArenaPart(ArenaPart *as, u8int *buf);
int		packClump(Clump *c, u8int *buf);
void		packClumpInfo(ClumpInfo *ci, u8int *buf);
void		packIBucket(IBucket *b, u8int *buf);
void		packIEntry(IEntry *i, u8int *buf);
int		packISect(ISect *is, u8int *buf);
void		packMagic(u32int magic, u8int *buf);
ZBlock		*packet2ZBlock(Packet *p, u32int size);
int		parseAMap(IFile *f, AMapN *amn);
int		parseIndex(IFile *f, Index *ix);
void		partBlockSize(Part *part, u32int blockSize);
int		partIFile(IFile *f, Part *part, u64int start, u32int size);
void		printArenaPart(int fd, ArenaPart *ap);
void		printArena(int fd, Arena *arena);
void		printIndex(int fd, Index *ix);
void		printStats(void);
void		putDBlock(DBlock *b);
void		putLump(Lump *b);
int		queueWrite(Lump *b, Packet *p, int creator);
u32int		readArena(Arena *arena, u64int aa, u8int *buf, long n);
int		readArenaMap(AMapN *amn, Part *part, u64int base, u32int size);
int		readClumpInfo(Arena *arena, int clump, ClumpInfo *ci);
int		readClumpInfos(Arena *arena, int clump, ClumpInfo *cis, int n);
ZBlock		*readFile(char *name);
int		readIFile(IFile *f, char *name);
Packet		*readLump(u8int *score, int type, u32int size);
int		readPart(Part *part, u64int addr, u8int *buf, u32int n);
int		runConfig(char *config, Config*);
int		scoreEq(u8int *, u8int *);
void		scoreMem(u8int *score, u8int *buf, int size);
void		setErr(int severity, char *fmt, ...);
u64int		sortRawIEntries(Index *ix, Part *tmp, u64int *tmpOff);
void		statsInit(void);
int		storeClump(Index *index, ZBlock *b, u8int *score, int type, u32int creator, IAddr *ia);
int		storeIEntry(Index *index, IEntry *m);
int		strScore(char *s, u8int *score);
int		strU32Int(char *s, u32int *r);
int		strU64Int(char *s, u64int *r);
void		sumArena(Arena *arena);
int		syncArena(Arena *arena, u32int n, int zok, int fix);
int		syncArenaIndex(Index *ix, Arena *arena, u32int clump, u64int a, int fix);
int		syncIndex(Index *ix, int fix);
int		u64log2(u64int v);
u64int		unittoull(char *s);
int		unpackArena(Arena *arena, u8int *buf);
int		unpackArenaHead(ArenaHead *head, u8int *buf);
int		unpackArenaPart(ArenaPart *as, u8int *buf);
int		unpackClump(Clump *c, u8int *buf);
void		unpackClumpInfo(ClumpInfo *ci, u8int *buf);
void		unpackIBucket(IBucket *b, u8int *buf);
void		unpackIEntry(IEntry *i, u8int *buf);
int		unpackISect(ISect *is, u8int *buf);
u32int		unpackMagic(u8int *buf);
int		vtTypeValid(int type);
int		wbArena(Arena *arena);
int		wbArenaHead(Arena *arena);
int		wbArenaMap(AMap *am, int n, Part *part, u64int base, u64int size);
int		wbArenaPart(ArenaPart *ap);
int		wbISect(ISect *is);
int		wbIndex(Index *ix);
int		whackblock(u8int *dst, u8int *src, int ssize);
u64int		writeAClump(Arena *a, Clump *c, u8int *clbuf);
u32int		writeArena(Arena *arena, u64int aa, u8int *clbuf, u32int n);
int		writeClumpInfo(Arena *arean, int clump, ClumpInfo *ci);
u64int		writeIClump(Index *ix, Clump *c, u8int *clbuf);
int		writeLump(Packet *p, u8int *score, int type, u32int creator);
int		writePart(Part *part, u64int addr, u8int *buf, u32int n);
int		writeQLump(Lump *u, Packet *p, int creator);
Packet		*zblock2Packet(ZBlock *zb, u32int size);
void		zeroPart(Part *part, int blockSize);

#pragma	varargck	argpos	fatal		1
#pragma	varargck	argpos	logErr		2
#pragma	varargck	argpos	SetErr		2

#define scoreEq(h1,h2)		(memcmp((h1),(h2),VtScoreSize)==0)
#define scoreCp(h1,h2)		memmove((h1),(h2),VtScoreSize)

#define MK(t)			((t*)emalloc(sizeof(t)))
#define MKZ(t)			((t*)ezmalloc(sizeof(t)))
#define MKN(t,n)		((t*)emalloc((n)*sizeof(t)))
#define MKNZ(t,n)		((t*)ezmalloc((n)*sizeof(t)))
#define MKNA(t,at,n)		((t*)emalloc(sizeof(t) + (n)*sizeof(at)))
