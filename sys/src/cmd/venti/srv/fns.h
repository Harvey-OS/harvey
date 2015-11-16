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
int		addarena(Arena *name);
void		addstat(int, int);
void		addstat2(int, int, int, int);
ZBlock		*alloczblock(uint32_t size, int zeroed, uint alignment);
Arena		*amapitoa(Index *index, uint64_t a, uint64_t *aa);
Arena		*amapitoag(Index *index, uint64_t a, uint64_t *gstart, uint64_t *glimit, int *g);
uint64_t		arenadirsize(Arena *arena, uint32_t clumps);
int		arenatog(Arena *arena, uint64_t aa, uint64_t *gstart, uint64_t *glimit, int *g);
void		arenaupdate(Arena *arena, uint32_t size, uint8_t *score);
int		asumload(Arena *arena, int g, IEntry *entries, int maxentries);
void		backsumarena(Arena *arena);
void	binstats(int32_t (*fn)(Stats *s0, Stats *s1, void*), void *arg, int32_t t0, int32_t t1, Statbin *bin, int nbin);
int		bloominit(Bloom*, int64_t, unsigned char*);
int		bucklook(uint8_t*, int, uint8_t*, int);
uint32_t		buildbucket(Index *ix, IEStream *ies, IBucket *ib, uint);
void		checkdcache(void);
void		checklumpcache(void);
int		clumpinfoeq(ClumpInfo *c, ClumpInfo *d);
int		clumpinfoeq(ClumpInfo *c, ClumpInfo *d);
uint32_t		clumpmagic(Arena *arena, uint64_t aa);
uint		countbits(uint n);
int		delarena(Arena *arena);
void		delaykickicache(void);
void		delaykickround(Round*);
void		delaykickroundproc(void*);
void		dirtydblock(DBlock*, int);
void		diskaccess(int);
void		disksched(void);
void		*emalloc(uint32_t);
void		emptydcache(void);
void		emptyicache(void);
void		emptylumpcache(void);
void		*erealloc(void *, uint32_t);
char		*estrdup(char*);
void		*ezmalloc(uint32_t);
Arena		*findarena(char *name);
int		flushciblocks(Arena *arena);
void		flushdcache(void);
void		flushicache(void);
int		flushpart(Part*);
void		flushqueue(void);
void		fmtzbinit(Fmt *f, ZBlock *b);
void		freearena(Arena *arena);
void		freearenapart(ArenaPart *ap, int freearenas);
void		freeiestream(IEStream *ies);
void		freeifile(IFile *f);
void		freeisect(ISect *is);
void		freeindex(Index *index);
void		freepart(Part *part);
void		freezblock(ZBlock *b);
DBlock		*_getdblock(Part *part, uint64_t addr, int mode, int load);
DBlock		*getdblock(Part *part, uint64_t addr, int mode);
uint32_t		hashbits(uint8_t *score, int nbits);
char		*hargstr(HConnect*, char*, char*);
int64_t	hargint(HConnect*, char*, int64_t);
int		hdebug(HConnect*);
int		hdisk(HConnect*);
int		hnotfound(HConnect*);
int		hproc(HConnect*);
int		hsethtml(HConnect*);
int		hsettext(HConnect*);
int		httpdinit(char *address, char *webroot);
int		iaddrcmp(IAddr *ia1, IAddr *ia2);
IEntry*	icachedirty(uint32_t, uint32_t, uint64_t);
uint32_t	icachedirtyfrac(void);
void		icacheclean(IEntry*);
int		icachelookup(uint8_t *score, int type, IAddr *ia);
AState	icachestate(void);
int		ientrycmp(const void *vie1, const void *vie2);
char		*ifileline(IFile *f);
int		ifilename(IFile *f, char *dst);
int		ifileuint32_t(IFile *f, uint32_t *r);
int		inbloomfilter(Bloom*, uint8_t*);
int		indexsect(Index *ix, uint8_t *score);
int		indexsect0(Index *ix, uint32_t buck);
Arena		*initarena(Part *part, uint64_t base, uint64_t size, uint32_t blocksize);
ArenaPart	*initarenapart(Part *part);
int		initarenasum(void);
void		initbloomfilter(Index*);
void		initdcache(uint32_t mem);
void		initicache(uint32_t mem);
void		initicachewrite(void);
IEStream	*initiestream(Part *part, uint64_t off, uint64_t clumps, uint32_t size);
ISect		*initisect(Part *part);
Index		*initindex(char *name, ISect **sects, int n);
void		initlumpcache(uint32_t size, uint32_t nblocks);
int		initlumpqueues(int nq);
Part*		initpart(char *name, int mode);
void		initround(Round*, char*, int);
int		initventi(char *config, Config *conf);
void		insertlump(Lump *lump, Packet *p);
int		insertscore(uint8_t *score, IAddr *ia, int state, AState *as);
void		kickdcache(void);
void		kickicache(void);
void		kickround(Round*, int wait);
int		loadbloom(Bloom*);
ZBlock		*loadclump(Arena *arena, uint64_t aa, int blocks, Clump *cl, uint8_t *score, int verify);
DBlock	*loadibucket(Index *index, uint8_t *score, ISect **is, uint32_t *buck, IBucket *ib);
int		loadientry(Index *index, uint8_t *score, int type, IEntry *ie);
void		logerr(int severity, char *fmt, ...);
Lump		*lookuplump(uint8_t *score, int type);
int		lookupscore(uint8_t *score, int type, IAddr *ia);
int		maparenas(AMap *am, Arena **arenas, int n, char *what);
void		markbloomfilter(Bloom*, uint8_t*);
uint		msec(void);
int		namecmp(char *s, char *t);
void		namecp(char *dst, char *src);
int		nameok(char *name);
void		needmainindex(void);
void		needzeroscore(void);
Arena		*newarena(Part *part, uint32_t, char *name, uint64_t base, uint64_t size, uint32_t blocksize);
ArenaPart	*newarenapart(Part *part, uint32_t blocksize, uint32_t tabsize);
ISect		*newisect(Part *part, uint32_t vers, char *name, uint32_t blocksize, uint32_t tabsize);
Index		*newindex(char *name, ISect **sects, int n);
uint32_t		now(void);
int		okamap(AMap *am, int n, uint64_t start, uint64_t stop, char *what);
int		okibucket(IBucket*, ISect*);
int		outputamap(Fmt *f, AMap *am, int n);
int		outputindex(Fmt *f, Index *ix);
int		_packarena(Arena *arena, uint8_t *buf, int);
int		packarena(Arena *arena, uint8_t *buf);
int		packarenahead(ArenaHead *head, uint8_t *buf);
int		packarenapart(ArenaPart *as, uint8_t *buf);
void		packbloomhead(Bloom*, uint8_t*);
int		packclump(Clump *c, uint8_t *buf, uint32_t);
void		packclumpinfo(ClumpInfo *ci, uint8_t *buf);
void		packibucket(IBucket *b, uint8_t *buf, uint32_t magic);
void		packientry(IEntry *i, uint8_t *buf);
int		packisect(ISect *is, uint8_t *buf);
void		packmagic(uint32_t magic, uint8_t *buf);
ZBlock		*packet2zblock(Packet *p, uint32_t size);
int		parseamap(IFile *f, AMapN *amn);
int		parseindex(IFile *f, Index *ix);
void		partblocksize(Part *part, uint32_t blocksize);
int		partifile(IFile *f, Part *part, uint64_t start, uint32_t size);
void		printarenapart(int fd, ArenaPart *ap);
void		printarena(int fd, Arena *arena);
void		printindex(int fd, Index *ix);
void		printstats(void);
void		putdblock(DBlock *b);
void		putlump(Lump *b);
int		queuewrite(Lump *b, Packet *p, int creator, uint ms);
uint32_t		readarena(Arena *arena, uint64_t aa, uint8_t *buf, int32_t n);
int		readarenamap(AMapN *amn, Part *part, uint64_t base, uint32_t size);
Bloom	*readbloom(Part*);
int		readclumpinfo(Arena *arena, int clump, ClumpInfo *ci);
int		readclumpinfos(Arena *arena, int clump, ClumpInfo *cis, int n);
ZBlock		*readfile(char *name);
int		readifile(IFile *f, char *name);
Packet		*readlump(uint8_t *score, int type, uint32_t size, int *cached);
int		readpart(Part *part, uint64_t addr, uint8_t *buf, uint32_t n);
int		resetbloom(Bloom*);
int		runconfig(char *config, Config*);
int		scorecmp(uint8_t *, uint8_t *);
void		scoremem(uint8_t *score, uint8_t *buf, int size);
void		setatailstate(AState*);
void		seterr(int severity, char *fmt, ...);
void		setstat(int, int32_t);
void		settrace(char *type);
uint64_t		sortrawientries(Index *ix, Part *tmp, uint64_t *tmpoff, Bloom *bloom);
void		startbloomproc(Bloom*);
Memimage*	statgraph(Graph *g);
void		statsinit(void);
int		storeclump(Index *index, ZBlock *b, uint8_t *score, int type, uint32_t creator, IAddr *ia);
int		storeientry(Index *index, IEntry *m);
int		strscore(char *s, uint8_t *score);
int		struint32_t(char *s, uint32_t *r);
int		struint64_t(char *s, uint64_t *r);
void		sumarena(Arena *arena);
int		syncarena(Arena *arena, uint32_t n, int zok, int fix);
int		syncindex(Index *ix);
void		trace(char *type, char*, ...);
void		traceinit(void);
int		u64log2(uint64_t v);
uint64_t		unittoull(char *s);
int		unpackarena(Arena *arena, uint8_t *buf);
int		unpackarenahead(ArenaHead *head, uint8_t *buf);
int		unpackarenapart(ArenaPart *as, uint8_t *buf);
int		unpackbloomhead(Bloom*, uint8_t*);
int		unpackclump(Clump *c, uint8_t *buf, uint32_t);
void		unpackclumpinfo(ClumpInfo *ci, uint8_t *buf);
void		unpackibucket(IBucket *b, uint8_t *buf, uint32_t magic);
void		unpackientry(IEntry *i, uint8_t *buf);
int		unpackisect(ISect *is, uint8_t *buf);
uint32_t		unpackmagic(uint8_t *buf);
void		ventifmtinstall(void);
void		vtloghdump(Hio*, VtLog*);
void		vtloghlist(Hio*);
int		vtproc(void(*)(void*), void*);
int		vttypevalid(int type);
void		waitforkick(Round*);
int		wbarena(Arena *arena);
int		wbarenahead(Arena *arena);
int		wbarenamap(AMap *am, int n, Part *part, uint64_t base, uint64_t size);
int		wbarenapart(ArenaPart *ap);
void		wbbloomhead(Bloom*);
int		wbisect(ISect *is);
int		wbindex(Index *ix);
int		whackblock(uint8_t *dst, uint8_t *src, int ssize);
uint64_t		writeaclump(Arena *a, Clump *c, uint8_t *clbuf);
uint32_t		writearena(Arena *arena, uint64_t aa, uint8_t *clbuf, uint32_t n);
int		writebloom(Bloom*);
int		writeclumpinfo(Arena *arean, int clump, ClumpInfo *ci);
int		writepng(Hio*, Memimage*);
uint64_t		writeiclump(Index *ix, Clump *c, uint8_t *clbuf);
int		writelump(Packet *p, uint8_t *score, int type, uint32_t creator, uint ms);
int		writepart(Part *part, uint64_t addr, uint8_t *buf, uint32_t n);
int		writeqlump(Lump *u, Packet *p, int creator, uint ms);
Packet		*zblock2packet(ZBlock *zb, uint32_t size);
void		zeropart(Part *part, int blocksize);

/*
#pragma	varargck	argpos	sysfatal		1
#pragma	varargck	argpos	logerr		2
#pragma	varargck	argpos	SetErr		2
*/

#define scorecmp(h1,h2)		memcmp((h1),(h2),VtScoreSize)
#define scorecp(h1,h2)		memmove((h1),(h2),VtScoreSize)

#define MK(t)			((t*)emalloc(sizeof(t)))
#define MKZ(t)			((t*)ezmalloc(sizeof(t)))
#define MKN(t,n)		((t*)emalloc((n)*sizeof(t)))
#define MKNZ(t,n)		((t*)ezmalloc((n)*sizeof(t)))
#define MKNA(t,at,n)		((t*)emalloc(sizeof(t) + (n)*sizeof(at)))
