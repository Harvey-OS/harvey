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
ZBlock		*alloczblock(u32 size, int zeroed, u32 alignment);
Arena		*amapitoa(Index *index, u64 a, u64 *aa);
Arena		*amapitoag(Index *index, u64 a, u64 *gstart, u64 *glimit, int *g);
u64		arenadirsize(Arena *arena, u32 clumps);
int		arenatog(Arena *arena, u64 aa, u64 *gstart, u64 *glimit, int *g);
void		arenaupdate(Arena *arena, u32 size, u8 *score);
int		asumload(Arena *arena, int g, IEntry *entries, int maxentries);
void		backsumarena(Arena *arena);
void	binstats(i32 (*fn)(Stats *s0, Stats *s1, void*), void *arg, i32 t0, i32 t1, Statbin *bin, int nbin);
int		bloominit(Bloom*, i64, unsigned char*);
int		bucklook(u8*, int, u8*, int);
u32		buildbucket(Index *ix, IEStream *ies, IBucket *ib, u32);
void		checkdcache(void);
void		checklumpcache(void);
int		clumpinfoeq(ClumpInfo *c, ClumpInfo *d);
int		clumpinfoeq(ClumpInfo *c, ClumpInfo *d);
u32		clumpmagic(Arena *arena, u64 aa);
u32		countbits(u32 n);
int		delarena(Arena *arena);
void		delaykickicache(void);
void		delaykickround(Round*);
void		delaykickroundproc(void*);
void		dirtydblock(DBlock*, int);
void		diskaccess(int);
void		disksched(void);
void		*emalloc(u32);
void		emptydcache(void);
void		emptyicache(void);
void		emptylumpcache(void);
void		*erealloc(void *, u32);
char		*estrdup(char*);
void		*ezmalloc(u32);
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
DBlock		*_getdblock(Part *part, u64 addr, int mode, int load);
DBlock		*getdblock(Part *part, u64 addr, int mode);
u32		hashbits(u8 *score, int nbits);
char		*hargstr(HConnect*, char*, char*);
i64	hargint(HConnect*, char*, i64);
int		hdebug(HConnect*);
int		hdisk(HConnect*);
int		hnotfound(HConnect*);
int		hproc(HConnect*);
int		hsethtml(HConnect*);
int		hsettext(HConnect*);
int		httpdinit(char *address, char *webroot);
int		iaddrcmp(IAddr *ia1, IAddr *ia2);
IEntry*	icachedirty(u32, u32, u64);
u32	icachedirtyfrac(void);
void		icacheclean(IEntry*);
int		icachelookup(u8 *score, int type, IAddr *ia);
AState	icachestate(void);
int		ientrycmp(const void *vie1, const void *vie2);
char		*ifileline(IFile *f);
int		ifilename(IFile *f, char *dst);
int		ifileu32(IFile *f, u32 *r);
int		inbloomfilter(Bloom*, u8*);
int		indexsect(Index *ix, u8 *score);
int		indexsect0(Index *ix, u32 buck);
Arena		*initarena(Part *part, u64 base, u64 size, u32 blocksize);
ArenaPart	*initarenapart(Part *part);
int		initarenasum(void);
void		initbloomfilter(Index*);
void		initdcache(u32 mem);
void		initicache(u32 mem);
void		initicachewrite(void);
IEStream	*initiestream(Part *part, u64 off, u64 clumps, u32 size);
ISect		*initisect(Part *part);
Index		*initindex(char *name, ISect **sects, int n);
void		initlumpcache(u32 size, u32 nblocks);
int		initlumpqueues(int nq);
Part*		initpart(char *name, int mode);
void		initround(Round*, char*, int);
int		initventi(char *config, Config *conf);
void		insertlump(Lump *lump, Packet *p);
int		insertscore(u8 *score, IAddr *ia, int state, AState *as);
void		kickdcache(void);
void		kickicache(void);
void		kickround(Round*, int wait);
int		loadbloom(Bloom*);
ZBlock		*loadclump(Arena *arena, u64 aa, int blocks, Clump *cl,
				 u8 *score, int verify);
DBlock	*loadibucket(Index *index, u8 *score, ISect **is,
			   u32 *buck, IBucket *ib);
int		loadientry(Index *index, u8 *score, int type, IEntry *ie);
void		logerr(int severity, char *fmt, ...);
Lump		*lookuplump(u8 *score, int type);
int		lookupscore(u8 *score, int type, IAddr *ia);
int		maparenas(AMap *am, Arena **arenas, int n, char *what);
void		markbloomfilter(Bloom*, u8*);
u32		msec(void);
int		namecmp(char *s, char *t);
void		namecp(char *dst, char *src);
int		nameok(char *name);
void		needmainindex(void);
void		needzeroscore(void);
Arena		*newarena(Part *part, u32, char *name, u64 base, u64 size, u32 blocksize);
ArenaPart	*newarenapart(Part *part, u32 blocksize, u32 tabsize);
ISect		*newisect(Part *part, u32 vers, char *name, u32 blocksize, u32 tabsize);
Index		*newindex(char *name, ISect **sects, int n);
u32		now(void);
int		okamap(AMap *am, int n, u64 start, u64 stop, char *what);
int		okibucket(IBucket*, ISect*);
int		outputamap(Fmt *f, AMap *am, int n);
int		outputindex(Fmt *f, Index *ix);
int		_packarena(Arena *arena, u8 *buf, int);
int		packarena(Arena *arena, u8 *buf);
int		packarenahead(ArenaHead *head, u8 *buf);
int		packarenapart(ArenaPart *as, u8 *buf);
void		packbloomhead(Bloom*, u8*);
int		packclump(Clump *c, u8 *buf, u32);
void		packclumpinfo(ClumpInfo *ci, u8 *buf);
void		packibucket(IBucket *b, u8 *buf, u32 magic);
void		packientry(IEntry *i, u8 *buf);
int		packisect(ISect *is, u8 *buf);
void		packmagic(u32 magic, u8 *buf);
ZBlock		*packet2zblock(Packet *p, u32 size);
int		parseamap(IFile *f, AMapN *amn);
int		parseindex(IFile *f, Index *ix);
void		partblocksize(Part *part, u32 blocksize);
int		partifile(IFile *f, Part *part, u64 start, u32 size);
void		printarenapart(int fd, ArenaPart *ap);
void		printarena(int fd, Arena *arena);
void		printindex(int fd, Index *ix);
void		printstats(void);
void		putdblock(DBlock *b);
void		putlump(Lump *b);
int		queuewrite(Lump *b, Packet *p, int creator, u32 ms);
u32		readarena(Arena *arena, u64 aa, u8 *buf,
				  i32 n);
int		readarenamap(AMapN *amn, Part *part, u64 base, u32 size);
Bloom	*readbloom(Part*);
int		readclumpinfo(Arena *arena, int clump, ClumpInfo *ci);
int		readclumpinfos(Arena *arena, int clump, ClumpInfo *cis, int n);
ZBlock		*readfile(char *name);
int		readifile(IFile *f, char *name);
Packet		*readlump(u8 *score, int type, u32 size,
				int *cached);
int		readpart(Part *part, u64 addr, u8 *buf, u32 n);
int		resetbloom(Bloom*);
int		runconfig(char *config, Config*);
int		scorecmp(u8 *, u8 *);
void		scoremem(u8 *score, u8 *buf, int size);
void		setatailstate(AState*);
void		seterr(int severity, char *fmt, ...);
void		setstat(int, i32);
void		settrace(char *type);
u64		sortrawientries(Index *ix, Part *tmp, u64 *tmpoff, Bloom *bloom);
void		startbloomproc(Bloom*);
Memimage*	statgraph(Graph *g);
void		statsinit(void);
int		storeclump(Index *index, ZBlock *b, u8 *score, int type,
			      u32 creator, IAddr *ia);
int		storeientry(Index *index, IEntry *m);
int		strscore(char *s, u8 *score);
int		stru32(char *s, u32 *r);
int		stru64(char *s, u64 *r);
void		sumarena(Arena *arena);
int		syncarena(Arena *arena, u32 n, int zok, int fix);
int		syncindex(Index *ix);
void		trace(char *type, char*, ...);
void		traceinit(void);
int		u64log2(u64 v);
u64		unittoull(char *s);
int		unpackarena(Arena *arena, u8 *buf);
int		unpackarenahead(ArenaHead *head, u8 *buf);
int		unpackarenapart(ArenaPart *as, u8 *buf);
int		unpackbloomhead(Bloom*, u8*);
int		unpackclump(Clump *c, u8 *buf, u32);
void		unpackclumpinfo(ClumpInfo *ci, u8 *buf);
void		unpackibucket(IBucket *b, u8 *buf, u32 magic);
void		unpackientry(IEntry *i, u8 *buf);
int		unpackisect(ISect *is, u8 *buf);
u32		unpackmagic(u8 *buf);
void		ventifmtinstall(void);
void		vtloghdump(Hio*, VtLog*);
void		vtloghlist(Hio*);
int		vtproc(void(*)(void*), void*);
int		vttypevalid(int type);
void		waitforkick(Round*);
int		wbarena(Arena *arena);
int		wbarenahead(Arena *arena);
int		wbarenamap(AMap *am, int n, Part *part, u64 base, u64 size);
int		wbarenapart(ArenaPart *ap);
void		wbbloomhead(Bloom*);
int		wbisect(ISect *is);
int		wbindex(Index *ix);
int		whackblock(u8 *dst, u8 *src, int ssize);
u64		writeaclump(Arena *a, Clump *c, u8 *clbuf);
u32		writearena(Arena *arena, u64 aa, u8 *clbuf,
				   u32 n);
int		writebloom(Bloom*);
int		writeclumpinfo(Arena *arean, int clump, ClumpInfo *ci);
int		writepng(Hio*, Memimage*);
u64		writeiclump(Index *ix, Clump *c, u8 *clbuf);
int		writelump(Packet *p, u8 *score, int type, u32 creator,
			     u32 ms);
int		writepart(Part *part, u64 addr, u8 *buf, u32 n);
int		writeqlump(Lump *u, Packet *p, int creator, u32 ms);
Packet		*zblock2packet(ZBlock *zb, u32 size);
void		zeropart(Part *part, int blocksize);

/*
//#pragma	varargck	argpos	sysfatal		1
//#pragma	varargck	argpos	logerr		2
//#pragma	varargck	argpos	SetErr		2
*/

#define scorecmp(h1,h2)		memcmp((h1),(h2),VtScoreSize)
#define scorecp(h1,h2)		memmove((h1),(h2),VtScoreSize)

#define MK(t)			((t*)emalloc(sizeof(t)))
#define MKZ(t)			((t*)ezmalloc(sizeof(t)))
#define MKN(t,n)		((t*)emalloc((n)*sizeof(t)))
#define MKNZ(t,n)		((t*)ezmalloc((n)*sizeof(t)))
#define MKNA(t,at,n)		((t*)emalloc(sizeof(t) + (n)*sizeof(at)))
