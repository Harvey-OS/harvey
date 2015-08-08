/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * sorted by 4,/^$/|sort -bd +1
 */
int addarena(Arena* name);
void addstat(int, int);
void addstat2(int, int, int, int);
ZBlock* alloczblock(u32int size, int zeroed, uint alignment);
Arena* amapitoa(Index* index, u64int a, u64int* aa);
Arena* amapitoag(Index* index, u64int a, u64int* gstart, u64int* glimit,
                 int* g);
u64int arenadirsize(Arena* arena, u32int clumps);
int arenatog(Arena* arena, u64int aa, u64int* gstart, u64int* glimit, int* g);
void arenaupdate(Arena* arena, u32int size, u8int* score);
int asumload(Arena* arena, int g, IEntry* entries, int maxentries);
void backsumarena(Arena* arena);
void binstats(long (*fn)(Stats* s0, Stats* s1, void*), void* arg, long t0,
              long t1, Statbin* bin, int nbin);
int bloominit(Bloom*, vlong, uchar*);
int bucklook(u8int*, int, u8int*, int);
u32int buildbucket(Index* ix, IEStream* ies, IBucket* ib, uint);
void checkdcache(void);
void checklumpcache(void);
int clumpinfoeq(ClumpInfo* c, ClumpInfo* d);
int clumpinfoeq(ClumpInfo* c, ClumpInfo* d);
u32int clumpmagic(Arena* arena, u64int aa);
uint countbits(uint n);
int delarena(Arena* arena);
void delaykickicache(void);
void delaykickround(Round*);
void delaykickroundproc(void*);
void dirtydblock(DBlock*, int);
void diskaccess(int);
void disksched(void);
void* emalloc(ulong);
void emptydcache(void);
void emptyicache(void);
void emptylumpcache(void);
void* erealloc(void*, uint32_t);
char* estrdup(char*);
void* ezmalloc(uint32_t);
Arena* findarena(char* name);
int flushciblocks(Arena* arena);
void flushdcache(void);
void flushicache(void);
int flushpart(Part*);
void flushqueue(void);
void fmtzbinit(Fmt* f, ZBlock* b);
void freearena(Arena* arena);
void freearenapart(ArenaPart* ap, int freearenas);
void freeiestream(IEStream* ies);
void freeifile(IFile* f);
void freeisect(ISect* is);
void freeindex(Index* index);
void freepart(Part* part);
void freezblock(ZBlock* b);
DBlock* _getdblock(Part* part, u64int addr, int mode, int load);
DBlock* getdblock(Part* part, u64int addr, int mode);
u32int hashbits(u8int* score, int nbits);
char* hargstr(HConnect*, char*, char*);
vlong hargint(HConnect*, char*, vlong);
int hdebug(HConnect*);
int hdisk(HConnect*);
int hnotfound(HConnect*);
int hproc(HConnect*);
int hsethtml(HConnect*);
int hsettext(HConnect*);
int httpdinit(char* address, char* webroot);
int iaddrcmp(IAddr* ia1, IAddr* ia2);
IEntry* icachedirty(u32int, u32int, u64int);
uint32_t icachedirtyfrac(void);
void icacheclean(IEntry*);
int icachelookup(u8int* score, int type, IAddr* ia);
AState icachestate(void);
int ientrycmp(const void* vie1, const void* vie2);
char* ifileline(IFile* f);
int ifilename(IFile* f, char* dst);
int ifileu32int(IFile* f, u32int* r);
int inbloomfilter(Bloom*, u8int*);
int indexsect(Index* ix, u8int* score);
int indexsect0(Index* ix, u32int buck);
Arena* initarena(Part* part, u64int base, u64int size, u32int blocksize);
ArenaPart* initarenapart(Part* part);
int initarenasum(void);
void initbloomfilter(Index*);
void initdcache(u32int mem);
void initicache(u32int mem);
void initicachewrite(void);
IEStream* initiestream(Part* part, u64int off, u64int clumps, u32int size);
ISect* initisect(Part* part);
Index* initindex(char* name, ISect** sects, int n);
void initlumpcache(u32int size, u32int nblocks);
int initlumpqueues(int nq);
Part* initpart(char* name, int mode);
void initround(Round*, char*, int);
int initventi(char* config, Config* conf);
void insertlump(Lump* lump, Packet* p);
int insertscore(u8int* score, IAddr* ia, int state, AState* as);
void kickdcache(void);
void kickicache(void);
void kickround(Round*, int wait);
int loadbloom(Bloom*);
ZBlock* loadclump(Arena* arena, u64int aa, int blocks, Clump* cl, u8int* score,
                  int verify);
DBlock* loadibucket(Index* index, u8int* score, ISect** is, u32int* buck,
                    IBucket* ib);
int loadientry(Index* index, u8int* score, int type, IEntry* ie);
void logerr(int severity, char* fmt, ...);
Lump* lookuplump(u8int* score, int type);
int lookupscore(u8int* score, int type, IAddr* ia);
int maparenas(AMap* am, Arena** arenas, int n, char* what);
void markbloomfilter(Bloom*, u8int*);
uint msec(void);
int namecmp(char* s, char* t);
void namecp(char* dst, char* src);
int nameok(char* name);
void needmainindex(void);
void needzeroscore(void);
Arena* newarena(Part* part, u32int, char* name, u64int base, u64int size,
                u32int blocksize);
ArenaPart* newarenapart(Part* part, u32int blocksize, u32int tabsize);
ISect* newisect(Part* part, u32int vers, char* name, u32int blocksize,
                u32int tabsize);
Index* newindex(char* name, ISect** sects, int n);
u32int now(void);
int okamap(AMap* am, int n, u64int start, u64int stop, char* what);
int okibucket(IBucket*, ISect*);
int outputamap(Fmt* f, AMap* am, int n);
int outputindex(Fmt* f, Index* ix);
int _packarena(Arena* arena, u8int* buf, int);
int packarena(Arena* arena, u8int* buf);
int packarenahead(ArenaHead* head, u8int* buf);
int packarenapart(ArenaPart* as, u8int* buf);
void packbloomhead(Bloom*, u8int*);
int packclump(Clump* c, u8int* buf, u32int);
void packclumpinfo(ClumpInfo* ci, u8int* buf);
void packibucket(IBucket* b, u8int* buf, u32int magic);
void packientry(IEntry* i, u8int* buf);
int packisect(ISect* is, u8int* buf);
void packmagic(u32int magic, u8int* buf);
ZBlock* packet2zblock(Packet* p, u32int size);
int parseamap(IFile* f, AMapN* amn);
int parseindex(IFile* f, Index* ix);
void partblocksize(Part* part, u32int blocksize);
int partifile(IFile* f, Part* part, u64int start, u32int size);
void printarenapart(int fd, ArenaPart* ap);
void printarena(int fd, Arena* arena);
void printindex(int fd, Index* ix);
void printstats(void);
void putdblock(DBlock* b);
void putlump(Lump* b);
int queuewrite(Lump* b, Packet* p, int creator, uint ms);
u32int readarena(Arena* arena, u64int aa, u8int* buf, long n);
int readarenamap(AMapN* amn, Part* part, u64int base, u32int size);
Bloom* readbloom(Part*);
int readclumpinfo(Arena* arena, int clump, ClumpInfo* ci);
int readclumpinfos(Arena* arena, int clump, ClumpInfo* cis, int n);
ZBlock* readfile(char* name);
int readifile(IFile* f, char* name);
Packet* readlump(u8int* score, int type, u32int size, int* cached);
int readpart(Part* part, u64int addr, u8int* buf, u32int n);
int resetbloom(Bloom*);
int runconfig(char* config, Config*);
int scorecmp(u8int*, u8int*);
void scoremem(u8int* score, u8int* buf, int size);
void setatailstate(AState*);
void seterr(int severity, char* fmt, ...);
void setstat(int, long);
void settrace(char* type);
u64int sortrawientries(Index* ix, Part* tmp, u64int* tmpoff, Bloom* bloom);
void startbloomproc(Bloom*);
Memimage* statgraph(Graph* g);
void statsinit(void);
int storeclump(Index* index, ZBlock* b, u8int* score, int type, u32int creator,
               IAddr* ia);
int storeientry(Index* index, IEntry* m);
int strscore(char* s, u8int* score);
int stru32int(char* s, u32int* r);
int stru64int(char* s, u64int* r);
void sumarena(Arena* arena);
int syncarena(Arena* arena, u32int n, int zok, int fix);
int syncindex(Index* ix);
void trace(char* type, char*, ...);
void traceinit(void);
int u64log2(u64int v);
u64int unittoull(char* s);
int unpackarena(Arena* arena, u8int* buf);
int unpackarenahead(ArenaHead* head, u8int* buf);
int unpackarenapart(ArenaPart* as, u8int* buf);
int unpackbloomhead(Bloom*, u8int*);
int unpackclump(Clump* c, u8int* buf, u32int);
void unpackclumpinfo(ClumpInfo* ci, u8int* buf);
void unpackibucket(IBucket* b, u8int* buf, u32int magic);
void unpackientry(IEntry* i, u8int* buf);
int unpackisect(ISect* is, u8int* buf);
u32int unpackmagic(u8int* buf);
void ventifmtinstall(void);
void vtloghdump(Hio*, VtLog*);
void vtloghlist(Hio*);
int vtproc(void (*)(void*), void*);
int vttypevalid(int type);
void waitforkick(Round*);
int wbarena(Arena* arena);
int wbarenahead(Arena* arena);
int wbarenamap(AMap* am, int n, Part* part, u64int base, u64int size);
int wbarenapart(ArenaPart* ap);
void wbbloomhead(Bloom*);
int wbisect(ISect* is);
int wbindex(Index* ix);
int whackblock(u8int* dst, u8int* src, int ssize);
u64int writeaclump(Arena* a, Clump* c, u8int* clbuf);
u32int writearena(Arena* arena, u64int aa, u8int* clbuf, u32int n);
int writebloom(Bloom*);
int writeclumpinfo(Arena* arean, int clump, ClumpInfo* ci);
int writepng(Hio*, Memimage*);
u64int writeiclump(Index* ix, Clump* c, u8int* clbuf);
int writelump(Packet* p, u8int* score, int type, u32int creator, uint ms);
int writepart(Part* part, u64int addr, u8int* buf, u32int n);
int writeqlump(Lump* u, Packet* p, int creator, uint ms);
Packet* zblock2packet(ZBlock* zb, u32int size);
void zeropart(Part* part, int blocksize);

/*
#pragma	varargck	argpos	sysfatal		1
#pragma	varargck	argpos	logerr		2
#pragma	varargck	argpos	SetErr		2
*/

#define scorecmp(h1, h2) memcmp((h1), (h2), VtScoreSize)
#define scorecp(h1, h2) memmove((h1), (h2), VtScoreSize)

#define MK(t) ((t*)emalloc(sizeof(t)))
#define MKZ(t) ((t*)ezmalloc(sizeof(t)))
#define MKN(t, n) ((t*)emalloc((n) * sizeof(t)))
#define MKNZ(t, n) ((t*)ezmalloc((n) * sizeof(t)))
#define MKNA(t, at, n) ((t*)emalloc(sizeof(t) + (n) * sizeof(at)))
